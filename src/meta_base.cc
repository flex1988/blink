#include "meta.h"

namespace blink {
MetaBase::~MetaBase() {}

std::string MetaBase::ActionBuffer()
{
    std::string buf;
    action_buffer_.at(2) = action_buffer_.size() - 3;
    buf.append(action_buffer_);
    buf.append("\r\n", 2);

    ResetBuffer();

    return buf;
}

void MetaBase::ResetBuffer()
{
    action_buffer_.clear();

    InitActionHeader();
    SaveAction(REINIT, Key().size(), Key());
}

void MetaBase::InitActionHeader()
{
    // action_buffer_.append(1, ACTION_BUFFER_MAGIC);
    // action_buffer_.append(1, Type());
    // action_buffer_.append(1, 0);
}

void MetaBase::SaveAction(Action action, int16_t op, const std::string& str)
{
    ; // action_buffer_.append((char*)&action, 2);
    // action_buffer_.append((char*)&op, 2);

    // if (str.empty()) return;

    // action_buffer_.append(str);
}
}
