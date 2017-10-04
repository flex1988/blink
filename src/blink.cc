#include "command.h"
#include "db.h"
#include "server.h"

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

std::unique_ptr<blink::Server> server_;

int main(int argc, char** argv)
{
    std::string dbpath = "/tmp/db";

    if (argc >= 2) dbpath = std::string(argv[1]);

    muduo::net::EventLoop loop;

    server_ = std::unique_ptr<blink::Server>(new blink::Server(&loop, dbpath));

    server_->start();

    // loop.runEvery(10, boost::bind(&RedisDB::CompactMeta, server_->db));

    blink::InitRedisCommand(server_->db);

    loop.loop();

    return 0;
}
