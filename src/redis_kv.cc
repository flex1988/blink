#include "redisdb.h"

rocksdb::Status RedisDB::Set(const std::string& key, const std::string& val)
{
    return _db->Put(rocksdb::WriteOptions(), key, val);
}

rocksdb::Status RedisDB::Get(const std::string& key, std::string& val)
{
    return _db->Get(rocksdb::ReadOptions(), key, &val);
}
