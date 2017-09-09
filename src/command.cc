#include "command.h"
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

#include <muduo/base/Logging.h>

static boost::unordered_map<std::string, RedisCommand> _cmdMap;

static rocksdb::DB* db;
static rocksdb::Options options;

void getCommand(Connection* conn)
{
    std::string value;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), conn->_argv[1], &value);
    
    conn->sendReplyValue(value);
}

void setCommand(Connection* conn)
{
    rocksdb::Status s =
        db->Put(rocksdb::WriteOptions(), conn->_argv[1], conn->_argv[2]);
    
    assert(s.ok());
    conn->sendReply("+OK\r\n");
}
void initRedisCommand()
{
    _cmdMap["get"] = {"get", getCommand, 0};
    _cmdMap["set"] = {"set", setCommand, 0};
}

RedisCommand* lookupCommand(std::string cmd)
{
    if (_cmdMap.find(cmd) == _cmdMap.end()) return NULL;
    return &_cmdMap[cmd];
}

void initRocksDB()
{
    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
    options.IncreaseParallelism();
    //options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    options.compression = rocksdb::kNoCompression;
    // open DB
    rocksdb::Status s = rocksdb::DB::Open(options, "/tmp/db", &db);
    if (!s.ok()) LOG_INFO << s.getState();
    assert(s.ok());
}
