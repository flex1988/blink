#ifndef __REDISDB_H__
#define __REDISDB_H__

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "mutex.h"
#include "port.h"

#include "meta.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/db_ttl.h"

#include <muduo/base/Thread.h>

class SetMeta;

class RedisDB {
public:
    RedisDB(const std::string &path);
    ~RedisDB();

    // Status Del(const std::string &key, int64_t *count);
    // Status Set(const std::string &key, const std::string &val,
    // const int32_t ttl = 0);
    rocksdb::Status Get(const std::string &key, std::string &val);
    rocksdb::Status Set(const std::string &key, const std::string &val);

    // LIST
    rocksdb::Status LPush(const std::string &key, const std::string &val, int64_t *llen);
    rocksdb::Status LPop(const std::string &key, std::string *val);
    rocksdb::Status LIndex(const std::string &key, const int64_t index, std::string *val);

    // SET
    rocksdb::Status SAdd(const std::string &key, const std::string &member, int64_t *res);
    rocksdb::Status SCard(const std::string &key, int64_t *res);

    void AppendMetaLog(const std::string &meta);

private:
    std::string _path;
    std::unique_ptr<rocksdb::DBWithTTL> kv_;
    std::unique_ptr<rocksdb::DBWithTTL> list_;
    std::unique_ptr<rocksdb::DBWithTTL> set_;

    rocksdb::Options options_;

    std::unordered_map<std::string, std::string> listmeta_;
    std::unordered_map<std::string, std::shared_ptr<SetMeta>> setmeta_;

    port::RecordMutex mutex_list_record_;
    port::RecordMutex mutex_set_record_;

    int meta_;
};
#endif
