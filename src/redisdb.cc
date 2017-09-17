#include "redisdb.h"
#include "common.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

RedisDB::RedisDB(const std::string& path) : _path(path)
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

    rocksdb::Status s = rocksdb::DB::Open(options_, _path + "/kv", &_kv);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    s = rocksdb::DBWithTTL::Open(options_, _path + "/list", &_list);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());
}

RedisDB::~RedisDB() {}
