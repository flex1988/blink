#ifndef __PIKA_COMMAND__
#define __PIKA_COMMAND__

#include "connection.h"

typedef void redisCommandProc(Connection* conn);

struct RedisCommand {
    std::string name;
    redisCommandProc* proc;
    int flags;
};

void initRedisCommand();

RedisCommand* lookupCommand(std::string cmd);
#endif
