#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "common.h"
#include "connection.h"

#include <muduo/net/TcpConnection.h>

namespace blink {
class Connection;
class Request {
  public:
    Request(Connection* conn);
    ~Request();

    enum REQUEST_TYPE { REQUEST_NULL, REQUEST_INLINE, REQUEST_MULTIBULK };

    void ProcessBuffer(muduo::net::Buffer* buf);
    bool ProcessInlineBuffer(muduo::net::Buffer* buf);
    bool ProcessMultibulkBuffer(muduo::net::Buffer* buf);
    bool SplitQueryArgs(std::string param);
    void SetProtocolError(muduo::net::Buffer* buf, muduo::string msg, int len);
    void SetFlag(int flag) { flags_ |= flag; };

    void Reset();

  private:
    int flags_;
    int multibulklen_;
    int bulklen_;
    int request_type_;
    std::vector<std::string> argv_;
    muduo::net::Buffer* buf_;
    Connection* conn_;
};
}
#endif
