#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "request.h"
#include "response.h"
#include "server.h"

#include <muduo/net/TcpConnection.h>

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

namespace blink {
class Server;
class Request;
class Connection {
  public:
    friend class Request;

    Connection(Server* owner, const muduo::net::TcpConnectionPtr& conn);

    ~Connection();

  private:
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp);

    bool ExecuteCommand();

    Request* request_;

    Response* response_;

    Server* _owner;

    muduo::net::TcpConnectionPtr _conn;

    std::vector<std::string> _argv;
};
}
#endif
