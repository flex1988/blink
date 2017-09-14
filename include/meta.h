#ifndef __META_H__
#define __META_H__

#include "common.h"

struct ListMetaBlockPtr {
    int32_t addr;  // meta item address
    int32_t size;  // meta item contain keys
};

struct ListMetaBlock {
    int64_t addr[LIST_BLOCK_KEYS];

    ListMetaBlock() {}
    ListMetaBlock(const std::string& str) { str.copy((char*)addr, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    std::string ToString()
    {
        std::string str;
        str.append((char*)addr, sizeof(int64_t) * LIST_BLOCK_KEYS);
        return str;
    }

    void Insert(int index, int size, int val)
    {
        int cursor = size;
        while (cursor > index) {
            addr[cursor] = addr[cursor - 1];
            cursor--;
        }
        addr[index] = val;
    }
};

class ListMeta {
public:
    ListMeta();
    ListMeta(std::string);
    std::string ToString();
    ListMetaBlockPtr* BlockAt(int);
    int AllocArea();
    int CurrentArea();

    bool IsElementsFull();
    bool IsBlocksFull();

    int64_t Size() { return size_; };
    int64_t BSize() { return bsize_; };
    int64_t IncrSize() { return size_++; };
    int64_t IncrBSize() { return bsize_++; };
private:
    int64_t size_;
    int64_t limit_;
    int64_t area_index_;
    int64_t bsize_;
    int64_t blimit_;
    ListMetaBlockPtr blocks_[LIST_META_BLOCKS];

    int64_t current_block_index_;
    ListMetaBlockPtr* current_block_;
};

#endif
