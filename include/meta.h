#ifndef __META_H__
#define __META_H__

#include "common.h"
#include "redis_list.h"

enum MetaType { LIST, LISTBLOCK, SET };

enum Action { DEFAULT, INIT, REINIT, UNIQUE, SIZE, BSIZE, INSERT, ALLOC };

struct ListMetaBlockPtr {
    int32_t addr;  // meta item address
    int32_t size;  // meta item contain keys
};

class MetaBase {
   public:
    MetaBase() = default;
    std::string ActionBuffer();
    void SaveAction(Action action, int16_t op, const std::string& str);
    void InitActionHeader();
    void ResetBuffer();

    virtual int64_t Size() = 0;
    virtual MetaType Type() = 0;
    virtual std::string Key() = 0;
    virtual std::string Serialize() = 0;

   private:
    std::string action_buffer_;
};

class ListMeta : public MetaBase {
   public:
    ListMeta(const std::string&, Action);
    ~ListMeta() = default;

    virtual MetaType Type() { return LIST; };
    virtual std::string Key() { return key_; };
    virtual std::string Serialize();

    ListMetaBlockPtr* BlockAt(int index) { return &blocks_[index]; };
    int AllocArea();
    int CurrentArea() { return area_index_ - 1; };
    bool IsElementsFull() { return size_ == limit_; };
    bool IsBlocksFull() { return bsize_ == blimit_; };
    std::string key() { return key_; };
    int64_t Size() { return size_; };
    int64_t BSize() { return bsize_; };
    void SetArea(uint64_t area) { area_index_ = area; };
    int64_t IncrSize() { return size_++; };
    void DecrSize() { size_--; };
    int64_t IncrBSize() { return bsize_++; };
    void DecrBSize() { bsize_--; }
    ListMetaBlockPtr* InsertNewMetaBlockPtr(int index);
    rocksdb::Status Insert(const std::string& key, uint64_t index, uint64_t* addr);
    bool operator==(const ListMeta& meta);
    ListMetaBlockPtr* IfNeedCreateBlock(int64_t index, int* bidx);
    ListMetaBlockPtr* BlockAtIndex(int64_t index, int* bidx);

   private:
    std::string key_;

    int64_t size_;
    int64_t limit_;

    int64_t bsize_;
    int64_t blimit_;

    int64_t area_index_;

    ListMetaBlockPtr blocks_[LIST_META_BLOCKS];
};

class ListMetaBlock : public MetaBase {
   public:
    ListMetaBlock(const std::string& key, int64_t addr) : self_addr_(addr), key_(key), size_(0)
    {
        std::memset(addr_, 0, sizeof(int64_t) * LIST_BLOCK_KEYS);
        unique_ = EncodeListBlockKey(key, addr);
    }

    ListMetaBlock(const std::string& str) : self_addr_(0), size_(0) { str.copy((char*)addr_, sizeof(int64_t) * LIST_BLOCK_KEYS); }
    virtual int64_t Size() { return size_; };
    virtual MetaType Type() { return LISTBLOCK; };
    virtual std::string Key() { return key_; };
    virtual std::string Serialize();

    int64_t FetchAddr(int64_t index) { return addr_[index]; };
    int Remove(int index);
    void Insert(int index, int val);

   private:
    int64_t addr_[LIST_BLOCK_KEYS];
    int64_t self_addr_;
    std::string key_;
    std::string unique_;
    int64_t size_;
};

class SetMeta : public MetaBase {
   public:
    SetMeta(const std::string& key);
    ~SetMeta();

    virtual MetaType Type() { return SET; };
    virtual std::string Key() { return key_; };
    virtual std::string Serialize();

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
