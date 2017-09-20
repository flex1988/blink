#include "redis_command.h"

#include <muduo/base/Logging.h>
#include <string>
#include <thread>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

static boost::unordered_map<std::string, RedisCommand> commands_;

boost::shared_ptr<RedisDB> redisdb_;

static void getCommand(Connection* conn)
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

static void setCommand(Connection* conn)
{
    rocksdb::Status s = redisdb_->Set(conn->_argv[1], conn->_argv[2]);

    if (!s.ok()) {
        conn->sendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    conn->sendReply("+OK\r\n");
}

static void lPushCommand(Connection* conn)
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

static void lIndexCommand(Connection* conn)
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

static void sAddCommand(Connection* conn)
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

static void sCardCommand(Connection* conn)
{
    int64_t ret;
    rocksdb::Status s;

    s = redisdb_->SCard(conn->_argv[1], &ret);

    conn->sendReplyLongLong(ret);
}

void initRedisCommand(const char* path)
{
    redisdb_ = boost::shared_ptr<RedisDB>(new RedisDB(std::string(path)));

    std::thread appender(&RedisDB::AppendMeta, redisdb_);
    appender.detach();

    commands_["get"] = {"get", getCommand, 0, 2};
    commands_["set"] = {"set", setCommand, 0, 3};
    commands_["lpush"] = {"lpush", lPushCommand, 0, 3};
    commands_["lindex"] = {"lindex", lIndexCommand, 0, 3};
    commands_["sadd"] = {"sadd", sAddCommand, 0, 3};
    commands_["scard"] = {"scard", sCardCommand, 0, 2};
}

RedisCommand* lookupCommand(std::string cmd)
{
    if (commands_.find(cmd) == commands_.end()) return NULL;
    return &commands_[cmd];
}
