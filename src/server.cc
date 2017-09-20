#include "server.h"
#include "connection.h"

#include <muduo/base/Atomic.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

Server::Server(muduo::net::EventLoop* loop) : _loop(loop), _server(loop, muduo::net::InetAddress(1234), "pika")
{
    _server.setConnectionCallback(boost::bind(&Server::onConnection, this, _1));
    _server.setThreadNum(2);
    
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
