#ifndef __PIKA_SERVER__
#define __PIKA_SERVER__

#include "connection.h"
#include "db.h"

#include <muduo/base/Mutex.h>
#include <muduo/net/TcpServer.h>

namespace boost {
std::size_t hash_value(const muduo::string& x);
}

#include <boost/unordered_map.hpp>

namespace boost {
inline std::size_t hash_value(const muduo::string& x) { return hash_range(x.begin(), x.end()); }
}

namespace blink {
class Connection;

class Server {
   public:
    Server(muduo::net::EventLoop* loop, std::string path);
    ~Server();

    void start();
    void stop();

    std::shared_ptr<RedisDB> db;

   private:
    muduo::net::TcpServer _server;
    muduo::net::EventLoop* _loop;

    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    boost::unordered_map<muduo::string, boost::shared_ptr<Connection>> _conn_map;
    mutable muduo::MutexLock _mutex;
};
}
#endif
