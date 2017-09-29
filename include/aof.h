#ifndef __AOF_H__
#define __AOF_H__

#include <string>
#include "command.h"

void Propagate(RedisCommand* cmd, std::vector<std::string>& argv);
#endif
