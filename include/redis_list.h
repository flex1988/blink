#ifndef __REDIS_LIST_H__
#define __REDIS_LIST_H__

#include <string>

inline std::string EncodeMetaKey(const std::string& key) { return "M:" + key; }
inline std::string EncodeBlockKey(const std::string& key, int64_t addr) { return "B:" + std::to_string(addr) + ":" + key; }
inline std::string EncodeValueKey(const std::string& key, int64_t addr) { return "V:" + std::to_string(addr) + ":" + key; }
#endif
