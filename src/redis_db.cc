#include "redis_db.h"
#include "common.h"
#include "meta_db.h"

#include <fcntl.h>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

RedisDB::RedisDB(const std::string& path) : _path(path), metaqueue_()
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

    LOG_INFO << path + "/meta";

    metafd_ = ::open(std::string(path + "/meta").data(), O_CREAT | O_WRONLY | O_APPEND);

    // metadb_ = std::shared_ptr<MetaDB>(new MetaDB(_path + "/meta"));
}

RedisDB::~RedisDB() {}
void RedisDB::AppendMeta()
{
    while (1) {
        std::string meta = metaqueue_.pop();
        size_t w = ::write(metafd_, meta.data(), meta.size());
        assert(w = meta.size());
    }
}

void RedisDB::DumpMeta()
{
    
    for (auto i = listmeta_.begin(); i != listmeta_.end(); ++i) {
        ;//LOG_INFO << i->first << " : " << i->second;
    }
}
