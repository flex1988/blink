#include "redisdb.h"
#include "common.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

RedisDB::RedisDB(const std::string& path)
{
    _options.IncreaseParallelism();
    _options.create_if_missing = true;
    _options.compression = rocksdb::kNoCompression;

    _path = path;
    rocksdb::Status s = rocksdb::DB::Open(_options, _path, &_db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());
}

RedisDB::~RedisDB() {}
rocksdb::Status RedisDB::Put(const std::string& key, const std::string& val)
{
    return _db->Put(rocksdb::WriteOptions(), key, val);
}
