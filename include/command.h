#ifndef __PIKA_COMMAND__
#define __PIKA_COMMAND__

#include "connection.h"
#include "db.h"

typedef void RedisCommandProc(Connection* conn);

struct RedisCommand {
    std::string name;
    RedisCommandProc* proc;
    int flags;
    int argc;
};

void initRedisCommand(std::shared_ptr<RedisDB> db);

RedisCommand* LookupCommand(std::string cmd);
#endif
