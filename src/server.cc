#include "server.h"
#include "connection.h"
#include "db.h"

#include <muduo/base/Atomic.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

namespace blink {

Server::Server(muduo::net::EventLoop* loop, std::string path) : _server(loop, muduo::net::InetAddress(1234), "blink"), _loop(loop)
{
    _server.setConnectionCallback(boost::bind(&Server::onConnection, this, _1));
    _server.setThreadNum(2);

    db = std::shared_ptr<RedisDB>(new RedisDB(std::string(path)));
}

Server::~Server() {}

void Server::start() { _server.start(); }

void Server::stop() { _loop->quit(); }

void Server::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (conn->connected()) {
        boost::shared_ptr<Connection> s_conn(new Connection(this, conn));
        muduo::MutexLockGuard lock(_mutex);
        _conn_map[conn->name()] = s_conn;
    }
    else {
        muduo::MutexLockGuard lock(_mutex);
        _conn_map.erase(conn->name());
    }
}
}
