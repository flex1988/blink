#include "meta.h"

std::string MetaBase::ActionBuffer()
{
    std::string buf;
    action_buffer_.at(2) = action_buffer_.size() - 3;
    buf.append(action_buffer_);
    buf.append("\r\n", 2);
    action_buffer_.clear();

    pushActionHeader();
    PushAction(REINIT, unique_.size(), unique_);
    return buf;
}

void MetaBase::pushActionHeader()
{
    action_buffer_.append(1, ACTION_BUFFER_MAGIC);
    action_buffer_.append(1, type_);
    action_buffer_.append(1, 0);
}

void MetaBase::PushAction(Action action, int16_t op, const std::string& str)
{
    action_buffer_.append((char*)&action, 2);
    action_buffer_.append((char*)&op, 2);

    if (str.empty()) return;

    action_buffer_.append(str);
}

void MetaBase::SetUnique(std::string unique)
{
    assert(unique.size() > 0);
    unique_ = unique;
}

void MetaBase::SetType(MetaType type) { type_ = type; }
MetaType MetaBase::GetType() { return type_; }
