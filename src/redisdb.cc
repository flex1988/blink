#include "redisdb.h"
#include "common.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

RedisDB::RedisDB(const std::string& path)
{
    options_.IncreaseParallelism();
    options_.create_if_missing = true;
    options_.compression = rocksdb::kNoCompression;

    _path = path;
    rocksdb::Status s = rocksdb::DB::Open(options_, _path + "/kv", &_kv);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    s = rocksdb::DB::Open(options_, _path + "/list", &_list);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());
}

RedisDB::~RedisDB() {}
