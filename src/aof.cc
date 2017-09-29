#include "aof.h"
#include "common.h"

extern std::unique_ptr<Server> server_;

static void FeedAppendOnlyFile(std::vector<std::string>& argv)
{
    std::string buf;
    buf.append(1, '*');
    buf.append(std::to_string(argv.size()));
    buf.append("\r\n");

    for (auto const& v : argv) {
        buf.append(1,'$');
        buf.append(std::to_string(v.size()));
        buf.append("\r\n");
        buf.append(v);
        buf.append("\r\n");
    }
    
    server_->db->metaqueue_.push(buf);

    LOG_DEBUG << buf;
}
void Propagate(RedisCommand* cmd, std::vector<std::string>& argv)
{
    if (cmd->flags & PROPAGATE_AOF) {
        FeedAppendOnlyFile(argv);
    }
}
