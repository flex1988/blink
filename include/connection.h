#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "request.h"
#include "response.h"
#include "server.h"

#include <muduo/net/TcpConnection.h>

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

//#define CONN_CLOSE_AFTER_REPLY (1 << 6)

//#define PROTO_INLINE_MAX_SIZE (1024 * 64)
//#define PROTO_MBULK_BIG_ARG (1024 * 32)

namespace blink {
class Server;
class Request;
class Connection {
  public:
    friend class Request;

    enum PROTO_REQ_TYPE { PROTO_NULL, PROTO_INLINE, PROTO_MULTIBULK };

    Connection(Server* owner, const muduo::net::TcpConnectionPtr& conn);

    ~Connection();

    void setProtocolError(muduo::net::Buffer* buf, muduo::string msg, int pos);

  private:
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp);

    bool ExecuteCommand();

    bool splitQueryArgs(std::string);

    Request* request_;

    Response* response_;

    Server* _owner;

    muduo::net::TcpConnectionPtr _conn;

    std::vector<std::string> _argv;
};
}
#endif
