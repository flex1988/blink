#ifndef __REDIS_SET_H__
#define __REDIS_SET_H__

#include <string>

inline std::string EncodeSetKey(const std::string& key, const std::string& member)
{
    std::string buf;
    buf.append(1, 's');
    buf.append(key.data(), key.size());
    buf.append(member.data(), member.size());
    return buf;
}
#endif
