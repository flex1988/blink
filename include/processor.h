#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include "command.h"
#include "redisdb.h"

class RedisCommand;

class Processor {
public:
    Processor();

    RedisCommand* LookupCommand(std::string cmd);

    void GetCommand(Connection* conn);
    void SetCommand(Connection* conn);

private:
    boost::unordered_map<std::string, RedisCommand> _cmd_map;
    RedisDB _redisdb;
};
#endif
