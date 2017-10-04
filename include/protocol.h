#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

namespace blink {
class BufferParser {
   public:
    BufferParser();
    ~BufferParser();

    bool ProcessInlineBuffer(muduo::net::Buffer* buf);
    bool ProcessMultibulkBuffer(muduo::net::Buffer* buf);
    bool ExecuteCommand();

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
