#ifndef __META_H__
#define __META_H__

#include "common.h"
#include "redis_list.h"

enum MetaType { LIST, SET };

enum Action { DEFAULT, INIT, REINIT, UNIQUE, SIZE, BSIZE, INSERT, ALLOC };

class MetaBase {
public:
    MetaBase() = default;
    std::string ActionBuffer();
    void SaveAction(Action action, int16_t op, const std::string& str);
    void InitActionHeader();
    void SetType(MetaType type);
    MetaType GetType();
    void ResetBuffer();

    virtual void SetUnique(std::string unique);
    virtual int64_t Size() { return 0; };
    virtual int64_t IncrSize() { return 0; };
    virtual std::string ToString() { return NULL; };
    virtual std::string GetUnique() { return unique_; };
private:
    std::string action_buffer_;
    std::string unique_;
    MetaType type_;
};

class ListMetaBlock : public MetaBase {
public:
    ListMetaBlock(const std::string& key, int64_t addr) : self_(addr), key_(key)
    {
        std::memset(addr_, 0, sizeof(int64_t) * LIST_BLOCK_KEYS);
        unique_ = EncodeListBlockKey(key, addr);
    }

    ListMetaBlock(const std::string& str) : self_(0) { str.copy((char*)addr_, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    std::string ToString();

    int64_t FetchAddr(int64_t index) { return addr_[index]; };
    void Insert(int index, int size, int val)
    {
        int cursor = size;
        while (cursor > index) {
            addr_[cursor] = addr_[cursor - 1];
            cursor--;
        }
        addr_[index] = val;
    }

    std::string GetUnique() { return unique_; };
private:
    int64_t addr_[LIST_BLOCK_KEYS];
    int64_t self_;
    std::string key_;
    std::string unique_;
};

struct ListMetaBlockPtr {
    int32_t addr;  // meta item address
    int32_t size;  // meta item contain keys
};

class ListMeta : public MetaBase {
public:
    ListMeta(const std::string&, Action);
    ~ListMeta() = default;
    std::string ToString();
    ListMetaBlockPtr* BlockAt(int);
    int AllocArea();
    int CurrentArea();

    bool IsElementsFull();
    bool IsBlocksFull();

    int64_t Size() { return size_; };
    void SetSize(int64_t size) { size_ = size; };
    int64_t BSize() { return bsize_; };
    void SetBSize(int64_t size) { bsize_ = size; };
    void SetArea(uint64_t area) { area_index_ = area; };
    int64_t IncrSize()
    {
        SaveAction(SIZE, ++size_, "");
        return size_;
    };
    int64_t IncrBSize()
    {
        SaveAction(BSIZE, ++bsize_, "");
        return bsize_;
    };
    ListMetaBlockPtr* InsertNewMetaBlockPtr(int index);
    rocksdb::Status Insert(const std::string& key, uint64_t index, uint64_t* addr);

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
