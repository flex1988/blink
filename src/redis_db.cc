#include "redis_db.h"
#include "common.h"
#include "meta.h"
#include "redis_list.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <fstream>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

static size_t readSync(int fd, char* buffer, size_t nread)
{
    size_t n;
    size_t last = nread;
    for (;;) {
        if (last == 0) return nread;
        n = read(fd, buffer, last);
        if (n < 0) {
            return n;
        }
        else if (n == 0) {
            return nread - last;
        }
        else {
            last -= n;
            buffer += n;
        }
    }
}

static size_t readNextChar(int fd, char* buffer, char target)
{
    size_t nread = 0;
    size_t n;
    for (;;) {
        n = read(fd, buffer, 1);

        if (n == -1 || n == 0) return n;

        nread++;

        if (buffer[0] == target) {
            return nread;
        }
    }
}

RedisDB::RedisDB(const std::string& path) : metaqueue_(), path_(path), meta_log_size_(0)
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

    rocksdb::Status s = rocksdb::DBWithTTL::Open(options_, path_ + "/kv", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    kv_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    s = rocksdb::DBWithTTL::Open(options_, path_ + "/list", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    list_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    s = rocksdb::DBWithTTL::Open(options_, path_ + "/set", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    set_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    msnap_ = path_ + "/meta.snapshot";
    mnewsnap_ = path_ + "/meta.snapshot.new";
    mlog_ = path_ + "/meta.log";

    LOG_INFO << "Server now is ready to accept connection";
    LOG_INFO << "DB path: " + path_ + "/meta";

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
    LOG_INFO << "Load meta info form disk file path: " + path_ + "/meta";
    loadMetaSnapshot();
    loadMetaLog();
}

void RedisDB::loadMetaSnapshot()
{
    std::ifstream snap(msnap_);

    for (std::string line; std::getline(snap, line);) {
        switch (line.at(0)) {
            case 'L': {
                std::shared_ptr<ListMeta> meta = std::shared_ptr<ListMeta>(new ListMeta(line, REINIT));
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

    LOG_INFO << "load meta snapshot success";
}

void RedisDB::loadMetaLog()
{
    int fd = ::open(mlog_.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_INFO << "open meta log not exists!";
        return;
    }

    char buffer[2048];
    uint8_t msize;
    size_t ret;
    MetaType type;

    int load = 0;

    for (;;) {
    header:
        ret = readNextChar(fd, buffer, ACTION_BUFFER_MAGIC);
        if (ret == 0 || ret == -1) {
            LOG_INFO << "load meta log from disk success: " << load;
            return;
        }

        ret = readSync(fd, buffer, 2);

        if (ret == 0) {
            LOG_INFO << "load meta log from disk success";
            return;
        }

        if (ret == -1) {
            LOG_INFO << "read meta log header failed " << std::string(strerror(errno));
            return;
        }

        if (ret < 2) {
            LOG_INFO << "meta log may lose data";
            return;
        }

        type = static_cast<MetaType>(*buffer);
        msize = *(uint8_t*)(buffer + 1);

        if (msize == 0) goto header;

        ret = readSync(fd, buffer, msize + 2);

        if (ret == -1) {
            LOG_INFO << "read meta log body failed " << std::string(strerror(errno));
            return;
        }

        if (ret < msize + 2) {
            LOG_INFO << "meta log may lose data";
            return;
        }

        load++;

        switch (type) {
            case LIST:
                reloadListActionBuffer(buffer, ret - 2);
        }
    }
}

void RedisDB::CompactMeta()
{
    LOG_INFO << "Dump meta to disk!";

    // uint64_t truncate_size = meta_log_size_;
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

void RedisDB::reloadListActionBuffer(char* buf, size_t blen)
{
    size_t index = 0;
    std::shared_ptr<ListMeta> meta;

    while (index < blen) {
        int16_t action = *(int16_t*)(buf + index);
        int16_t op = *(int16_t*)(buf + index + 2);

        index += 4;

        switch (action) {
            case INIT: {
                std::string key = std::string(buf + index, op);
                std::string metakey = EncodeMetaKey(key);
                meta = std::shared_ptr<ListMeta>(new ListMeta(key, INIT));
                memmeta_[metakey] = std::dynamic_pointer_cast<MetaBase>(meta);
                index += op;
                break;
            }
            case REINIT: {
                std::string key = std::string(buf + index, op);
                std::string metakey = EncodeMetaKey(key);
                assert(memmeta_.find(metakey) != memmeta_.end());
                meta = std::dynamic_pointer_cast<ListMeta>(memmeta_[metakey]);
                index += op;
                break;
            }
            case SIZE:
                meta->SetSize(op);
                break;
            case ALLOC:
                meta->AllocArea();
                break;
            case BSIZE:
                break;
            case INSERT:
                break;

            default:
                LOG_INFO << "unknown action: " << action;
                index += blen;
                break;
        }
    }
}
