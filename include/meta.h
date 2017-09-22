#ifndef __META_H__
#define __META_H__

#include "common.h"

struct ListMetaBlockPtr {
    int32_t addr;  // meta item address
    int32_t size;  // meta item contain keys
};

enum Action { DEFAULT, NEWLIST, UNIQUE, SIZE, BSIZE, INSERT, ALLOC };

class MetaBase {
public:
    MetaBase() = default;
    std::string ActionBuffer();
    void PushAction(Action action, int16_t op, const std::string& str);

    virtual int64_t Size() { return 0; };
    virtual int64_t IncrSize() { return 0; };
    virtual std::string ToString() { return NULL; };
    virtual void SetUnique(std::string unique);
    
    virtual std::string GetUnique() { return unique_; };
private:
    std::vector<int32_t> action_buffer_;
    std::string unique_;
};

class ListMetaBlock : public MetaBase {
public:
    int64_t addr[LIST_BLOCK_KEYS];

    ListMetaBlock() { std::memset(addr, 0, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    ListMetaBlock(const std::string& str) { str.copy((char*)addr, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    std::string ToString();

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
    ListMeta(const std::string&);
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
        PushAction(SIZE, ++size_, "");
        return size_;
    };
    int64_t IncrBSize()
    {
        PushAction(BSIZE, ++bsize_, "");
        return bsize_;
    };
    ListMetaBlockPtr* InsertNewMetaBlockPtr(int index);

private:
    int64_t size_;
    int64_t limit_;

    int64_t bsize_;
    int64_t blimit_;

    int64_t area_index_;

    ListMetaBlockPtr blocks_[LIST_META_BLOCKS];

    int64_t current_block_index_;
    ListMetaBlockPtr* current_block_;
};

class SetMeta : public MetaBase {
public:
    SetMeta(const std::string& key);
    ~SetMeta();
    std::string ToString();

    int64_t IncrSize() { return size_++; };
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
