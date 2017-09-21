#include "common.h"
#include "hash.h"
#include "meta.h"

SetMeta::SetMeta(const std::string& key) : size_(0), limit_(SET_MAX_ELEMENT), key_(key), bf_(256, 0) {}
SetMeta::~SetMeta() {}
std::string SetMeta::ToString()
{
    std::string buf;
    buf.append(1, 'S');
    buf.append(1, (uint8_t)key_.size());
    buf.append(key_.data(), key_.size());
    buf.append((char*)&size_, sizeof(int64_t));
    buf.append((char*)&limit_, sizeof(int64_t));
    buf.append("\r\n");
    return buf;
}

void SetMeta::BFAdd(const std::string& member)
{
    uint32_t v = MurmurHash64A(member.data(), member.size());
    uint32_t index = v % (256 * 8);

    bf_.at(index / 8) |= v % 8;

    v = crc16(member.data(), member.size());
    index = v % (256 * 8);
    bf_.at(index / 8) |= v % 8;
}

bool SetMeta::BFNotExists(const std::string& member)
{
    uint32_t v = MurmurHash64A(member.data(), member.size());
    uint32_t index = v % (256 * 8);
    if (bf_.at(index / 8) & v % 8) return false;

    v = crc16(member.data(), member.size());
    index = v % (256 * 8);
    if (bf_.at(index / 8) & v % 8) return false;
    return true;
}
