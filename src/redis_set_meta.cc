#include "common.h"
#include "meta.h"

SetMeta::SetMeta(const std::string &key) : size_(0), limit_(SET_MAX_ELEMENT), key_(key) {}
SetMeta::~SetMeta() {}
std::string SetMeta::ToString()
{
    std::string buf;
    buf.append(1, 'S');
    buf.append(1, (uint8_t)key_.size());
    buf.append(key_.data(), key_.size());
    buf.append((char *)&size_, sizeof(int64_t));
    buf.append((char *)&limit_, sizeof(int64_t));
    buf.append("\r\n");
    return buf;
}
