#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <muduo/net/TcpConnection.h>

namespace blink {
class Response {
   public:
    Response(muduo::net::TcpConnectionPtr conn);
    void SendReply(const std::string& msg);
    void SendReplyBulk(const std::string& bulk);
    void SendReplyError(const std::string& error);
    void SendReplyLongLong(int64_t val);
    void SendReplyMultiBulk(const std::vector<std::string> multi);

   private:
    muduo::net::TcpConnectionPtr conn_;
};
}
#endif
