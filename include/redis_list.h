#ifndef __REDIS_LIST_H__
#define __REDIS_LIST_H__

#include <string>

inline std::string EncodeMetaKey(const std::string& key) { return "L:M:" + key; }
inline std::string EncodeBlockKey(const std::string& key, int64_t addr) { return "L:B:" + std::to_string(addr) + ":" + key; }
inline std::string EncodeValueKey(const std::string& key, int64_t addr) { return "L:V:" + std::to_string(addr) + ":" + key; }
#endif
