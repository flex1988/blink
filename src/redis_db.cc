#include "redis_db.h"
#include "common.h"
#include "meta.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <fstream>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

RedisDB::RedisDB(const std::string& path) : _path(path), metaqueue_(), meta_log_size_(0)
{
    options_.create_if_missing = true;
    options_.compression = rocksdb::CompressionType::kNoCompression;
    options_.disable_auto_compactions = true;
    options_.max_background_compactions = 1;
    options_.max_background_flushes = 1;
    options_.max_open_files = 5000;
    options_.max_manifest_file_size = 64 * 1024 * 1024;
    options_.max_log_file_size = 512 * 1024 * 1024;
    options_.keep_log_file_num = 10;
    options_.target_file_size_base = 2 * 1024 * 1024;
    options_.write_buffer_size = 256 * 1024 * 1024;
    options_.target_file_size_multiplier = 1;

    options_.max_write_buffer_number = 5;
    options_.min_write_buffer_number_to_merge = 2;

    rocksdb::DBWithTTL* db;

    rocksdb::Status s = rocksdb::DBWithTTL::Open(options_, _path + "/kv", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    kv_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    s = rocksdb::DBWithTTL::Open(options_, _path + "/list", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    list_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    s = rocksdb::DBWithTTL::Open(options_, _path + "/set", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    set_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    msnap_ = _path + "/meta.snapshot";
    mnewsnap_ = _path + "/meta.snapshot.new";
    mlog_ = _path + "/meta.log";

    LOG_INFO << "Server now is ready to accept connection";
    LOG_INFO << "DB path: " + _path + "/meta";

    LoadMeta();

    metafd_ = ::open(std::string(path + "/meta.log").data(), O_CREAT | O_WRONLY | O_APPEND);
}

RedisDB::~RedisDB() {}
void RedisDB::AppendMeta()
{
    while (1) {
        std::string meta = metaqueue_.pop();
        size_t w = ::write(metafd_, meta.data(), meta.size());
        assert(w = meta.size());
        meta_log_size_ += w;
    }
}

void RedisDB::LoadMeta()
{
    LOG_INFO << "Load meta info form disk file path: " + _path + "/meta";
    loadMetaSnapshot();
    loadMetaLog();
}

void RedisDB::loadMetaSnapshot()
{
    LOG_INFO << "load meta snapshot from disk";
    std::ifstream snap(msnap_);

    for (std::string line; std::getline(snap, line);) {
        switch (line.at(0)) {
            case 'L': {
                std::shared_ptr<ListMeta> meta = std::shared_ptr<ListMeta>(new ListMeta(line));
                LOG_INFO << "Key: " << meta->GetUnique();
                LOG_INFO << "Size: " << meta->Size();
                memmeta_[meta->GetUnique()] = std::dynamic_pointer_cast<MetaBase>(meta);
                break;
            }
            case 'S':
                LOG_INFO << "set";
                break;
            default:
                LOG_INFO << "unknown meta info!";
        }
    }
}

void RedisDB::loadMetaLog()
{
    LOG_INFO << "load meta log from disk";
    std::ifstream mlog(mlog_);
    if (!mlog.is_open()) {
        LOG_INFO << "open meta log failed!";
        return;
    }

    for (std::string line; std::getline(mlog, line);) {
        LOG_INFO << line;
        int ops = line.size() / sizeof(uint32_t);
        int n = 0;
        while (n < ops) {
            uint32_t p = static_cast<uint32_t>(line.at(n * sizeof(uint32_t)));
            uint16_t action = p >> 16;
            uint16_t op = p << 16;

            switch (action) {
                case NEWLIST:
                    LOG_INFO << "new list";
                case SIZE:
                    LOG_INFO << "size";
                default:
                    LOG_INFO << "unknown";
                    LOG_INFO << p;
            }

            n++;
        }
    }
}

void RedisDB::DumpMeta()
{
    LOG_INFO << "Dump meta to disk!";

    uint64_t truncate_size = meta_log_size_;
    meta_log_size_ = 0;

    int ret;
    size_t nwrite;
    int snapfd = ::open(mnewsnap_.c_str(), O_CREAT | O_WRONLY | O_TRUNC);

    for (auto i = memmeta_.begin(); i != memmeta_.end(); ++i) {
        std::string str = i->second->ToString();
        nwrite = write(snapfd, str.c_str(), str.size());
        assert(nwrite == str.size());
    }

    fdatasync(snapfd);

    unlink(msnap_.c_str());
    rename(mnewsnap_.c_str(), msnap_.c_str());
    unlink(mnewsnap_.c_str());

    fsync(metafd_);
    ret = ftruncate(metafd_, 0);

    if (ret == -1) {
        LOG_INFO << std::string(strerror(errno));
        assert(0);
    }
}
