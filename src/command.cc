#include "command.h"

#include <muduo/base/Logging.h>
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

static boost::unordered_map<std::string, RedisCommand> _cmdMap;

static boost::shared_ptr<RedisDB> _redisdb(new RedisDB("/tmp/db"));

void CommandDict::getCommand(Connection* conn)
{
    std::string value;
    rocksdb::Status s = _redisdb->Get(conn->_argv[1], value);

    if (s.code() == rocksdb::Status::kNotFound) {
        conn->sendReply("$-1\r\n");
        return;
    }

    if (!s.ok()) {
        conn->sendReplyError("interval error");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReplyBulk(value);
}

void CommandDict::setCommand(Connection* conn)
{
    rocksdb::Status s = _redisdb->Set(conn->_argv[1], conn->_argv[2]);

    if (!s.ok()) {
        conn->sendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReply("+OK\r\n");
}

void CommandDict::lpushCommand(Connection* conn)
{
    int64_t size;
    rocksdb::Status s = _redisdb->LPush(conn->_argv[1], conn->_argv[2], &size);

    if (!s.ok()) {
        conn->sendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReplyLongLong(size);
}

void CommandDict::initRedisCommand()
{
    _cmdMap["get"] = {"get", CommandDict::getCommand, 0, 2};
    _cmdMap["set"] = {"set", CommandDict::setCommand, 0, 3};
    _cmdMap["lpush"] = {"lpush", CommandDict::lpushCommand, 0, 3};
}

RedisCommand* CommandDict::lookupCommand(std::string cmd)
{
    if (_cmdMap.find(cmd) == _cmdMap.end()) return NULL;
    return &_cmdMap[cmd];
}
