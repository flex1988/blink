#include "redis_command.h"
#include "redis_db.h"
#include "server.h"

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

int main(int argc, char** argv)
{
    std::string dbpath = "/tmp/db";

    if (argc >= 2) dbpath = std::string(argv[1]);

    muduo::net::EventLoop loop;

    Server server(&loop, dbpath);

    server.start();

    //loop.runEvery(10, boost::bind(&RedisDB::CompactMeta, server.db));

    initRedisCommand(server.db);

    loop.loop();

    return 0;
}
