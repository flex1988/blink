#ifndef __REDISDB_H__
#define __REDISDB_H__

#include <string>

#include "mutex.h"
#include "port.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

class RedisDB {
public:
    RedisDB(const std::string &path);
    ~RedisDB();

    // Status Del(const std::string &key, int64_t *count);
    // Status Set(const std::string &key, const std::string &val,
    // const int32_t ttl = 0);
    rocksdb::Status Get(const std::string &key, std::string &val);
    rocksdb::Status Set(const std::string &key, const std::string &val);

    rocksdb::Status LPush(const std::string &key, const std::string &val, int64_t *llen);
    rocksdb::Status LPop(const std::string &key, std::string *val);
    rocksdb::Status LIndex(const std::string &key, const int64_t index, std::string *val);

private:
    std::string _path;
    rocksdb::DB *_kv;
    rocksdb::DB *_list;
    rocksdb::Options options_;

    port::RecordMutex _mutex_list_record;
};
#endif
