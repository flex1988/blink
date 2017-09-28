#include "command.h"

#include <muduo/base/Logging.h>
#include <string>
#include <thread>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

static boost::unordered_map<std::string, RedisCommand> commands_;

std::shared_ptr<RedisDB> redisdb_;

static void GetCommand(Connection* conn)
{
    std::string value;
    rocksdb::Status s = redisdb_->Get(conn->_argv[1], value);

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

static void SetCommand(Connection* conn)
{
    rocksdb::Status s = redisdb_->Set(conn->_argv[1], conn->_argv[2]);

    if (!s.ok()) {
        conn->sendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReply("+OK\r\n");
}

static void LPushCommand(Connection* conn)
{
    int64_t size;
    rocksdb::Status s = redisdb_->LPush(conn->_argv[1], conn->_argv[2], &size);

    if (!s.ok()) {
        conn->sendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReplyLongLong(size);
}

static void LIndexCommand(Connection* conn)
{
    std::string val;
    rocksdb::Status s = redisdb_->LIndex(conn->_argv[1], std::stoi(conn->_argv[2], NULL, 0), &val);

    if (!s.ok()) {
        conn->sendReply("$-1\r\n");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReplyBulk(val);
}

static void LLenCommmand(Connection* conn)
{
    int64_t llen;
    rocksdb::Status s = redisdb_->LLen(conn->_argv[1], &llen);
    conn->sendReplyLongLong(llen);
}

static void LPopCommand(Connection* conn)
{
    std::string val;
    rocksdb::Status s = redisdb_->LPop(conn->_argv[1], val);

    if (!s.ok()) {
        conn->sendReply("$-1\r\n");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReplyBulk(val);
}

static void SAddCommand(Connection* conn)
{
    int64_t ret = 0;
    int64_t size;

    int members = conn->_argv.size() - 2;
    rocksdb::Status s;

    while (members--) {
        s = redisdb_->SAdd(conn->_argv[1], conn->_argv[members + 2], &size);
        ret += size;
        if (!s.ok()) {
            conn->sendReplyError("internal error");
            LOG_ERROR << s.getState();
            return;
        }
    }

    conn->sendReplyLongLong(ret);
}

static void SCardCommand(Connection* conn)
{
    int64_t ret;
    rocksdb::Status s;

    s = redisdb_->SCard(conn->_argv[1], &ret);

    conn->sendReplyLongLong(ret);
}

void initRedisCommand(std::shared_ptr<RedisDB> db)
{
    redisdb_ = db;

    std::thread appender(&RedisDB::AppendMeta, redisdb_);
    appender.detach();

    commands_["get"] = {"get", GetCommand, 0, 2};
    commands_["set"] = {"set", SetCommand, 0, 3};
    commands_["lpush"] = {"lpush", LPushCommand, 0, 3};
    commands_["lindex"] = {"lindex", LIndexCommand, 0, 3};
    commands_["sadd"] = {"sadd", SAddCommand, 0, 3};
    commands_["scard"] = {"scard", SCardCommand, 0, 2};
    commands_["llen"] = {"llen", LLenCommmand, 0, 2};
    commands_["lpop"] = {"lpop", LPopCommand, 0, 2};
}

RedisCommand* LookupCommand(std::string cmd)
{
    if (commands_.find(cmd) == commands_.end()) return NULL;
    return &commands_[cmd];
}
