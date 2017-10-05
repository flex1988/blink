#include "request.h"

namespace blink {

Request::Request(Connection* conn) : flags_(0), multibulklen_(0), bulklen_(-1), request_type_(REQUEST_NULL), conn_(conn) {}

void Request::ProcessBuffer(muduo::net::Buffer* buf)
{
    while (buf->readableBytes() > 0) {
        if (flags_ & CONN_CLOSE_AFTER_REPLY)
            return;

        if (request_type_ == REQUEST_NULL) {
            if (*buf->peek() == '*') {
                request_type_ = REQUEST_MULTIBULK;
            }
            else {
                request_type_ = REQUEST_INLINE;
            }
        }

        if (request_type_ == REQUEST_INLINE) {
            if (!ProcessInlineBuffer(buf))
                return;
        }
        else if (request_type_ == REQUEST_MULTIBULK) {
            if (!ProcessMultibulkBuffer(buf))
                return;
        }
        else {
            assert(0);
        }

        if (conn_->_argv.size() == 0) {
            Reset();
        }
        else {
            if (conn_->ExecuteCommand()) {
                Reset();
            }
        }
    }
}

bool Request::ProcessInlineBuffer(muduo::net::Buffer* buf)
{
    const char* newline;
    size_t querylen;

    newline = buf->findCRLF();

    if (newline == NULL) {
        if (buf->readableBytes() > PROTO_INLINE_MAX_SIZE) {
            conn_->response_->SendReplyError("Protocol error: too big inline request");
            SetProtocolError(buf_, "too big mbulk count string", 0);
        }
        return false;
    }

    querylen = newline - buf->peek();

    std::string req(buf->peek(), querylen);

    SplitQueryArgs(req);

    if (conn_->_argv.size() == 0) {
        conn_->response_->SendReplyError("Protocol error: unbalanced quotes in request");
        SetProtocolError(buf_, "unbalanced quotes in inline request", 0);
        return false;
    }

    buf->retrieve(querylen + 2);

    return true;
}

bool Request::ProcessMultibulkBuffer(muduo::net::Buffer* buf)
{
    const char* newline = NULL;
    int pos = 0;
    long long ll;

    if (multibulklen_ == 0) {
        newline = buf->findCRLF();
        if (newline == NULL) {
            if (buf->readableBytes() > PROTO_INLINE_MAX_SIZE) {
                conn_->response_->SendReplyError("Protocol error: too big mbulk count string");
                SetProtocolError(buf, "too big mbulk count string", 0);
            }
            return false;
        }

        assert(*buf->peek() == '*');
        ll = std::stoll(std::string(buf->peek() + 1, newline - (buf->peek() + 1)), NULL, 0);
        if (ll > 1024 * 1024) {
            conn_->response_->SendReplyError("Protocol error: invalid multibulk length");
            SetProtocolError(buf, "invalid mbulk count", pos);
            return false;
        }

        pos = (newline - buf->peek()) + 2;
        if (ll <= 0) {
            buf->retrieve(pos);
            return true;
        }

        multibulklen_ = ll;
        buf->retrieve(pos);
    }

    while (multibulklen_) {
        if (bulklen_ == -1) {
            newline = buf->findCRLF();
            if (newline == NULL) {
                if (buf->readableBytes() > PROTO_INLINE_MAX_SIZE) {
                    conn_->response_->SendReplyError("Protocol error: too big mbulk count string");
                    SetProtocolError(buf, "too big bulk count string", 0);
                    return false;
                }
                break;
            }

            if (*buf->peek() != '$') {
                conn_->response_->SendReplyError("Protocol error: expected '$', got %c" + *buf->peek());
                SetProtocolError(buf, "expected $ but got something else", 0);
                return false;
            }

            ll = std::stoll(std::string(buf->peek() + 1, newline - (buf->peek() + 1)), NULL, 0);

            if (ll < 0 || ll > 512 * 1024 * 1024) {
                conn_->response_->SendReplyError("Protocol error: invalid bulk length");
                SetProtocolError(buf, "invalid bulk length", 0);
                return false;
            }

            buf->retrieve(newline - buf->peek() + 2);

            bulklen_ = ll;
        }

        if ((int)buf->readableBytes() < bulklen_ + 2) {
            break;
        }
        else {
            std::string bulk = std::string(buf->peek(), bulklen_);
            conn_->_argv.push_back(bulk);
            buf->retrieve(bulklen_ + 2);
            bulklen_ = -1;
            multibulklen_--;
        }
    }

    if (multibulklen_ == 0)
        return true;

    return false;
}

void Request::Reset()
{
    conn_->_argv.clear();
    request_type_ = REQUEST_NULL;
    multibulklen_ = 0;
    bulklen_ = -1;
}

bool Request::SplitQueryArgs(std::string req)
{
    std::istringstream iss(req);
    for (std::string s; iss >> s;) {
        conn_->_argv.push_back(s);
    }

    return true;
}

void Request::SetProtocolError(muduo::net::Buffer* buf, muduo::string msg, int len)
{
    flags_ |= CONN_CLOSE_AFTER_REPLY;
    buf->retrieve(len);
}

}
