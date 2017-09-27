#ifndef __REDISDB_H__
#define __REDISDB_H__

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "concurrent_queue.h"
#include "meta.h"
#include "mutex.h"
#include "port.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/db_ttl.h"

class SetMeta;

class RedisDB {
public:
    RedisDB(const std::string &path);
    ~RedisDB();

    enum CommitMode { SYNC, ASYNC };
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

    void AppendMeta();
    void CompactMeta();
    void LoadMeta();

    Queue<std::string> metaqueue_;

private:
    std::string path_;
    std::unique_ptr<rocksdb::DBWithTTL> kv_;
    std::unique_ptr<rocksdb::DBWithTTL> list_;
    std::unique_ptr<rocksdb::DBWithTTL> set_;

    rocksdb::Options options_;

    std::unordered_map<std::string, std::shared_ptr<MetaBase>> memmeta_;

    port::RecordMutex mutex_list_record_;
    port::RecordMutex mutex_set_record_;

    void LoadMetaSnapshot();
    void LoadMetaLog();
    void ReloadListActionBuffer(char *buf, size_t blen);

    rocksdb::Status InsertListMeta(const std::string &key, std::shared_ptr<ListMeta> meta, int64_t index,int64_t* addr);

    std::shared_ptr<ListMeta> GetOrCreateListMeta(const std::string &key);
    std::shared_ptr<ListMetaBlock> GetOrCreateListMetaBlock(const std::string &key, int64_t addr);
    std::shared_ptr<ListMeta> GetListMeta(const std::string& key);

    int metafd_;
    uint64_t meta_log_size_;

    std::mutex snap_mutex_;
    std::string msnap_;
    std::string mnewsnap_;
    std::string mlog_;
};
#endif
