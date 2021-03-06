#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "connection.h"
#include "db.h"

namespace blink {

typedef void RedisCommandProc(Response* rsp, std::vector<std::string> argv);

struct RedisCommand {
    std::string name;
    RedisCommandProc* proc;
    int flags;
    int argc;
};

void InitRedisCommand(std::shared_ptr<RedisDB> db);

RedisCommand* LookupCommand(std::string cmd);
}
#endif
