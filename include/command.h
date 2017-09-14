#ifndef __PIKA_COMMAND__
#define __PIKA_COMMAND__

#include "connection.h"
#include "redisdb.h"

typedef void redisCommandProc(Connection* conn);

struct RedisCommand {
    std::string name;
    redisCommandProc* proc;
    int flags;
    int argc;
};

class CommandDict {
public:
    static void getCommand(Connection* conn);
    static void setCommand(Connection* conn);
    static void lpushCommand(Connection* conn);
    static void initRedisCommand();
    static RedisCommand* lookupCommand(std::string cmd);
private:
    CommandDict() {}
};
#endif
