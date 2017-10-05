#include "connection.h"
#include "aof.h"
#include "command.h"
#include "common.h"
#include "server.h"

#include <muduo/base/Logging.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace blink {
Connection::Connection(Server* owner, const muduo::net::TcpConnectionPtr& conn)
    : request_(new Request(this)), response_(new Response(conn)), _owner(owner), _conn(conn)
{
    _conn->setMessageCallback(boost::bind(&Connection::onMessage, this, _1, _2, _3));
}

Connection::~Connection() {}

void Connection::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time)
{
    request_->ProcessBuffer(buf);
}

bool Connection::ExecuteCommand()
{
    if (_argv[0] == "quit") {
        response_->SendReply("+OK\r\n");
        request_->SetFlag(CONN_CLOSE_AFTER_REPLY);
        return false;
    }

    boost::algorithm::to_lower(_argv[0]);

    RedisCommand* cmd = LookupCommand(_argv[0]);

    if (!cmd) {
        response_->SendReplyError("unknown command: " + _argv[0]);
        return true;
    }

    if (cmd->argc > (int)_argv.size()) {
        response_->SendReplyError("wrong number of arguments");
        return true;
    }

    if (_argv.size() > 1 && (_argv[1].size() >= KEY_MAX_LENGTH || _argv[1].size() <= 0)) {
        response_->SendReplyError("invalid key length");
        return true;
    }

    cmd->proc(response_, _argv);

    Propagate(cmd, _argv);

    return true;
}
}
