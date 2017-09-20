#ifndef __META_H__
#define __META_H__

#include "common.h"

struct ListMetaBlockPtr {
    int32_t addr;  // meta item address
    int32_t size;  // meta item contain keys
};

class MetaBase {
public:
    enum Action { SIZE, BSIZE, INSERT, ALLOC };

    MetaBase() = default;
    std::string ActionBuffer();
    void PushAction(Action action, int16_t op);

private:
    std::vector<int32_t> action_buffer_;
};

class ListMetaBlock : public MetaBase {
public:
    int64_t addr[LIST_BLOCK_KEYS];

    ListMetaBlock() {}
    ListMetaBlock(const std::string& str) { str.copy((char*)addr, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    std::string ToString()
    {
        std::string str;
        str.append("LB ", 3);
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

class ListMeta : public MetaBase {
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
    int64_t IncrSize()
    {
        PushAction(SIZE, ++size_);
        return size_;
    };
    int64_t IncrBSize()
    {
        PushAction(BSIZE, ++bsize_);
        return bsize_;
    };
    ListMetaBlockPtr* InsertNewMetaBlockPtr(int index);

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

class SetMeta {
public:
    SetMeta(const std::string& key);
    ~SetMeta();
    std::string ToString();

    void IncrSize() { size_++; };
    int64_t Size() { return size_; };
    bool IsSetFull() { return size_ == limit_; }
    void BFAdd(const std::string& member);
    bool BFNotExists(const std::string& member);

private:
    int64_t size_;
    int64_t limit_;
    std::string key_;
    std::vector<char> bf_;
};
#endif
