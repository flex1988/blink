#ifndef __REDIS_LIST_H__
#define __REDIS_LIST_H__

#include <string>

inline std::string EncodeListMetaKey(const std::string& key) { return "L:M:" + key; }
inline std::string EncodeListBlockKey(const std::string& key, int64_t addr) { return "L:B:" + std::to_string(addr) + ":" + key; }
inline std::string EncodeListValueKey(const std::string& key, int64_t addr)
{
    std::string buf;
    buf.append("L:V:");
    buf.append(std::to_string(addr));
    buf.append(":");
    buf.append(key);
    return buf;
    // return "L:V:" + std::to_string(addr) + ":" + key;
}
class ListMetaInter {
};
#endif
