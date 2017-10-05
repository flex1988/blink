#include "command.h"

#include <muduo/base/Logging.h>
#include <string>
#include <thread>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

namespace blink {
static boost::unordered_map<std::string, RedisCommand> commands_;

std::shared_ptr<RedisDB> redisdb_;

static void GetCommand(Response* rsp, std::vector<std::string> argv)
{
    std::string value;
    rocksdb::Status s = redisdb_->Get(argv[1], value);

    if (s.code() == rocksdb::Status::kNotFound) {
        rsp->SendReply("$-1\r\n");
        return;
    }

    if (!s.ok()) {
        rsp->SendReplyError("interval error");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReplyBulk(value);
}

static void SetCommand(Response* rsp, std::vector<std::string> argv)
{
    rocksdb::Status s = redisdb_->Set(argv[1], argv[2]);

    if (!s.ok()) {
        rsp->SendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReply("+OK\r\n");
}

static void LPushCommand(Response* rsp, std::vector<std::string> argv)
{
    int64_t size;
    rocksdb::Status s = redisdb_->LPush(argv[1], argv[2], &size);

    if (!s.ok()) {
        rsp->SendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReplyLongLong(size);
}

static void LPushXCommand(Response* rsp, std::vector<std::string> argv)
{
    int64_t size;
    rocksdb::Status s = redisdb_->LPushX(argv[1], argv[2], &size);

    if (!s.ok()) {
        rsp->SendReplyError("internal error");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReplyLongLong(size);
}

static void LRangeCommand(Response* rsp, std::vector<std::string> argv)
{
    std::vector<std::string> range;

    rocksdb::Status s = redisdb_->LRange(argv[1], std::atoi(argv[2].c_str()), std::atoi(argv[3].c_str()), range);

    if (!s.ok()) {
        rsp->SendReply("$-1\r\n");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReplyMultiBulk(range);
}

static void LIndexCommand(Response* rsp, std::vector<std::string> argv)
{
    std::string val;
    rocksdb::Status s = redisdb_->LIndex(argv[1], std::stoi(argv[2], NULL, 0), &val);

    if (!s.ok()) {
        rsp->SendReply("$-1\r\n");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReplyBulk(val);
}

static void LLenCommmand(Response* rsp, std::vector<std::string> argv)
{
    int64_t llen;
    rocksdb::Status s = redisdb_->LLen(argv[1], &llen);
    rsp->SendReplyLongLong(llen);
}

static void LPopCommand(Response* rsp, std::vector<std::string> argv)
{
    std::string val;
    rocksdb::Status s = redisdb_->LPop(argv[1], val);

    if (!s.ok()) {
        rsp->SendReply("$-1\r\n");
        LOG_ERROR << s.getState();
        return;
    }

    rsp->SendReplyBulk(val);
}

static void SAddCommand(Response* rsp, std::vector<std::string> argv)
{
    int64_t ret = 0;
    int64_t size;

    int members = argv.size() - 2;
    rocksdb::Status s;

    while (members--) {
        s = redisdb_->SAdd(argv[1], argv[members + 2], &size);
        ret += size;
        if (!s.ok()) {
            rsp->SendReplyError("internal error");
            LOG_ERROR << s.getState();
            return;
        }
    }

    rsp->SendReplyLongLong(ret);
}

static void SCardCommand(Response* rsp, std::vector<std::string> argv)
{
    int64_t ret;
    rocksdb::Status s;

    s = redisdb_->SCard(argv[1], &ret);

    rsp->SendReplyLongLong(ret);
}

void InitRedisCommand(std::shared_ptr<RedisDB> db)
{
    redisdb_ = db;

    std::thread appender(&RedisDB::AppendMeta, redisdb_);
    appender.detach();

    commands_["get"] = {"get", GetCommand, 0, 2};
    commands_["set"] = {"set", SetCommand, PROPAGATE_AOF, 3};

    commands_["lpush"] = {"lpush", LPushCommand, PROPAGATE_AOF, 3};
    commands_["lpushx"] = {"lpushx", LPushXCommand, PROPAGATE_AOF, 3};
    commands_["lindex"] = {"lindex", LIndexCommand, 0, 3};
    commands_["llen"] = {"llen", LLenCommmand, 0, 2};
    commands_["lpop"] = {"lpop", LPopCommand, PROPAGATE_AOF, 2};
    commands_["lrange"] = {"lrange", LRangeCommand, 0, 4};

    commands_["sadd"] = {"sadd", SAddCommand, PROPAGATE_AOF, 3};
    commands_["scard"] = {"scard", SCardCommand, 0, 2};
}

RedisCommand* LookupCommand(std::string cmd)
{
    if (commands_.find(cmd) == commands_.end())
        return NULL;
    return &commands_[cmd];
}
}
