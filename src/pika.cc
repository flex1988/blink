#include "redis_command.h"
#include "redis_db.h"
#include "server.h"

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

extern boost::shared_ptr<RedisDB> redisdb_;

int main(int argc, char** argv)
{
    const char* path = "/tmp/db";

    if (argc >= 2) path = argv[1];

    initRedisCommand(path);

    muduo::net::EventLoop loop;

    loop.runEvery(60, boost::bind(&RedisDB::DumpMeta, redisdb_));

    Server server(&loop);

    server.start();

    loop.loop();

    return 0;
}
