#include "command.h"

#include <muduo/base/Logging.h>

static boost::unordered_map<std::string, RedisCommand> _cmdMap;

void getCommand(Connection *conn) { LOG_INFO << "get command"; }
void setCommand(Connection *conn) { LOG_INFO << "set command"; }
void initRedisCommand()
{
    _cmdMap["get"] = {"get", getCommand, 0};
    _cmdMap["set"] = {"set", setCommand, 0};
}

RedisCommand *lookupCommand(std::string cmd)
{
    if (_cmdMap.find(cmd) == _cmdMap.end()) return NULL;

    return &_cmdMap[cmd];
}
