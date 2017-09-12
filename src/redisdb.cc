#include "redisdb.h"
#include "common.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

RedisDB::RedisDB(const std::string& path)
{
    _kvoptions.IncreaseParallelism();
    _kvoptions.create_if_missing = true;
    _kvoptions.compression = rocksdb::kNoCompression;

    _path = path;
    rocksdb::Status s = rocksdb::DB::Open(_kvoptions, _path + "/kv", &_kv);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());

    _listoptions.IncreaseParallelism();
    _listoptions.create_if_missing = true;
    _listoptions.compression = rocksdb::kNoCompression;

    s = rocksdb::DB::Open(_listoptions, _path + "/list", &_list);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());
}

RedisDB::~RedisDB() {}
