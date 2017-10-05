#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "common.h"

#include <muduo/net/TcpConnection.h>

namespace blink {
class BodyParser {
   public:
    BodyParser();
    ~BodyParser();

    bool ProcessInlineBuffer();
    bool ProcessMultibulkBuffer(muduo::net::Buffer* buf);
    bool SplitQueryArgs(std::string param);

    void Reset();

   private:
    int flags_;
    int multibulklen_;
    int bulklen_;
    std::vector<std::string> argv_;
    muduo::net::Buffer* buf_;
};
}
#endif
