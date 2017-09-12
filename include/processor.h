#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include "command.h"
#include "redisdb.h"

class Processor {
public:
    Processor();

private:
    boost::unordered_map<std::string, RedisCommand> _cmd_map;
    RedisDB _redisdb;
};
#endif
