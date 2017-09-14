#ifndef __META_H__
#define __META_H__

#include "common.h"

struct ListMetaBlock {
    int32_t addr;  // meta item address
    int32_t size;  // meta item contain keys
};

class ListMeta {
public:
    ListMeta();
    ListMeta(std::string);
    std::string toString();
    ListMetaBlock *getBlock(int);
    int fetchSeq();
    int currentSeq();

    int64_t _size;
    int64_t _limit;
    int64_t _addr_seq;
    int64_t _msize;
    int64_t _mlimit;
    ListMetaBlock _blocks[LIST_META_BLOCKS];
};

struct ListMetaBlockKeys {
    int64_t addr[LIST_BLOCK_KEYS];

    ListMetaBlockKeys() {}
    ListMetaBlockKeys(const std::string &str) { str.copy((char *)addr, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    std::string toString()
    {
        std::string str;
        str.append((char *)addr, sizeof(int64_t) * LIST_BLOCK_KEYS);
        return str;
    }

    void insert(int index, int size, int val)
    {
        int cursor = size;
        while (cursor > index) {
            addr[cursor] = addr[--cursor];
        }
        addr[index] = val;
    }
};

#endif
