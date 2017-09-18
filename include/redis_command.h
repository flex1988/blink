#ifndef __PIKA_COMMAND__
#define __PIKA_COMMAND__

#include "connection.h"
#include "redis_db.h"

typedef void redisCommandProc(Connection* conn);

struct RedisCommand {
    std::string name;
    redisCommandProc* proc;
    int flags;
    int argc;
};

void initRedisCommand(const char *);

RedisCommand* lookupCommand(std::string cmd);
#endif
