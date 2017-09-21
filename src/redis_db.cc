#include "redis_db.h"
#include "common.h"
#include "meta_db.h"

#include <fcntl.h>
#include <stdio.h>

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

    LOG_INFO << "Server now is ready to accept connection";
    LOG_INFO << "DB path: " + path + "/meta";

    metafd_ = ::open(std::string(path + "/meta.log").data(), O_CREAT | O_WRONLY | O_APPEND);
}

RedisDB::~RedisDB() {}
void RedisDB::AppendMeta()
{
    while (1) {
        snap_mutex_.lock();
        std::string meta = metaqueue_.pop();
        
        size_t w = ::write(metafd_, meta.data(), meta.size());
        assert(w = meta.size());
        meta_log_size_ += w;
        snap_mutex_.unlock();
    }
}

void RedisDB::DumpMeta()
{
    snap_mutex_.lock();

    LOG_INFO << "Dump meta to disk!";

    uint64_t truncate_size = meta_log_size_;
    meta_log_size_ = 0;

    int mfd = ::open(std::string(_path + "/meta.snapshot.new").data(), O_CREAT | O_WRONLY | O_APPEND);

    for (auto i = listmeta_.begin(); i != listmeta_.end(); ++i) {
        std::string str = i->second->ToString();
        write(mfd, str.data(), str.size());
    }

    fdatasync(mfd);
    fdatasync(metafd_);

    const char* snap = std::string(_path + "/meta.snapshot").data();
    const char* newsnap = std::string(_path + "/meta.snapshot.new").data();
    const char* log = std::string(_path + "/meta.log").data();

    unlink(snap);
    rename(newsnap, snap);
    unlink(newsnap);

    metafd_ = open(log, O_TRUNC);

    snap_mutex_.unlock();
}
