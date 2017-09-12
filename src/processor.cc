#include "processor.h"
#include "command.h"
#include "connection.h"

#include <muduo/base/Logging.h>

class Connection;

void GetCommand(Connection* conn)
{
    std::string value;
    // rocksdb::Status s = db->Get(rocksdb::ReadOptions(), conn->_argv[1],
    // &value);
    // if (!s.ok() || s.code() == rocksdb::Status::kNotFound) {
    // conn->sendReply("$-1\r\n");
    // return;
    //}
    // else if (!s.ok()) {
    // LOG_INFO << s.getState();
    // conn->sendError("internal rocksdb error");
    // return;
    //}
    // conn->sendReplyValue(value);
}

Processor::Processor() : _redisdb("./db")
{
    _cmd_map["get"] = {"get", GetCommand, 0, 2};
    //_cmd_map["set"] = RedisCommand("set", SetCommand, 0, 3);
}

RedisCommand* Processor::LookupCommand(std::string cmd)
{
    if (_cmd_map.find(cmd) == _cmd_map.end()) return NULL;
    return &_cmd_map[cmd];
}
