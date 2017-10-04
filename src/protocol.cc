#include "protocol.h"

BufferParser::ProcessInlineBuffer()
{
    const char* newline;
    size_t querylen;

    newline = buf_->findCRLF();

    if (newline == NULL) {
        if (buf_->readableBytes() > PROTO_INLINE_MAX_SIZE) {
            sendReplyError("Protocol error: too big inline request");
            setProtocolError(buf_, "too big mbulk count string", 0);
        }
        return false;
    }

    querylen = newline - buf_->peek();

    std::string req(buf_->peek(), querylen);

    splitQueryArgs(req);

    if (_argv.size() == 0) {
        sendReplyError("Protocol error: unbalanced quotes in request");
        setProtocolError(buf_, "unbalanced quotes in inline request", 0);
        return false;
    }

    buf_->retrieve(querylen + 2);

    return true;
}
