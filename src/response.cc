#include "response.h"

namespace blink {
Response::Response(muduo::net::TcpConnectionPtr conn) : conn_(conn) {}

void Response::SendReply(const std::string& msg) { conn_->send(muduo::string(msg.c_str())); }

void Response::SendReplyBulk(const std::string& bulk)
{
    std::string buf;
    buf.append(1, '$');
    buf.append(std::to_string(bulk.size()));
    buf.append("\r\n");
    buf.append(bulk);
    buf.append("\r\n");

    conn_->send(muduo::string(buf.c_str()));
}

void Response::SendReplyError(const std::string& err)
{
    std::string msg = "-ERR " + err + "\r\n";
    conn_->send(muduo::string(msg.c_str()));
}

void Response::SendReplyMultiBulk(const std::vector<std::string> multi)
{
    std::string buf;
    buf.append(1, '*');
    buf.append(std::to_string(multi.size()));
    buf.append("\r\n");

    for (auto const& v : multi) {
        buf.append(1, '$');
        buf.append(std::to_string(v.size()));
        buf.append("\r\n");
        buf.append(v);
        buf.append("\r\n");
    }

    conn_->send(muduo::string(buf.c_str()));
}

void Response::SendReplyLongLong(int64_t val)
{
    std::string msg = ":" + std::to_string(val) + "\r\n";
    conn_->send(muduo::string(msg.c_str()));
}
}
