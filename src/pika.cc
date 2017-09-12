#include "server.h"
#include "command.h"

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

int main(int argc,char **argv){
    CommandDict::initRedisCommand();

    muduo::net::EventLoop loop;
    
    Server server(&loop);
    
    server.start();
    loop.loop();

    return 0;
}
