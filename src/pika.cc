#include "redis_command.h"
#include "server.h"

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

int main(int argc, char** argv)
{
    const char* path = "/tmp/db";

    if (argc >= 2) path = argv[1];

    initRedisCommand(path);

    muduo::net::EventLoop loop;

    Server server(&loop);

    server.start();

    loop.loop();

    return 0;
}
