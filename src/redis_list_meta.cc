#include "common.h"
#include "meta.h"

void MetaBase::PushAction(Action action, int16_t op) { action_buffer_.push_back(action << 16 | op); }

std::string MetaBase::ActionBuffer()
{
    std::string buf;
    buf.append((char*)&action_buffer_.at(0), sizeof(int32_t) * action_buffer_.size());
    buf.append("\r\n", 2);
    action_buffer_.clear();
    return buf;
}

ListMeta::ListMeta() : size_(0), area_index_(0), limit_(LIST_ELEMENT_SIZE), bsize_(0), blimit_(LIST_META_BLOCKS) {}
ListMeta::ListMeta(std::string str)
{
    const char* p = str.data();
    size_ = *(int64_t*)p;
    limit_ = *(int64_t*)(p + sizeof(int64_t));
    assert(limit_ == LIST_ELEMENT_SIZE);
    bsize_ = *(int64_t*)(p + sizeof(int64_t) * 2);
    blimit_ = *(int64_t*)(p + sizeof(int64_t) * 3);
    assert(blimit_ == LIST_META_BLOCKS);
    area_index_ = *(int64_t*)(p + sizeof(int64_t) * 4);
    str.copy((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS, 40);
}

std::string ListMeta::ToString()
{
    std::string str;
    str.append("LM ", 3);
    str.append((char*)&size_, sizeof(int64_t));
    str.append((char*)&limit_, sizeof(int64_t));
    str.append((char*)&bsize_, sizeof(int64_t));
    str.append((char*)&blimit_, sizeof(int64_t));
    str.append((char*)&area_index_, sizeof(int64_t));
    str.append((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    return str;
}

ListMetaBlockPtr* ListMeta::BlockAt(int index) { return &blocks_[index]; }
int ListMeta::AllocArea()
{
    PushAction(ALLOC, area_index_ + 1);
    return area_index_++;
}
int ListMeta::CurrentArea() { return area_index_; }
bool ListMeta::IsElementsFull() { return size_ == limit_; }
bool ListMeta::IsBlocksFull() { return bsize_ == blimit_; }
ListMetaBlockPtr* ListMeta::InsertNewMetaBlockPtr(int index)
{
    if (IsBlocksFull()) {
        return NULL;
    }

    if (index > bsize_ || index < 0) {
        return NULL;
    }

    PushAction(INSERT, static_cast<int16_t>(index));

    int cursor = bsize_;
    while (cursor > index) {
        *BlockAt(cursor) = *BlockAt(cursor - 1);
        cursor--;
    }

    IncrBSize();

    ListMetaBlockPtr* ptr = BlockAt(index);
    ptr->size = 0;
    ptr->addr = 0;

    return ptr;
}

// void ListMeta::pushAction(Action action, int16_t op) { action_buffer_.push_back(action << 16 | op); }
// std::string ListMeta::ActionBuffer()
//{
// std::string buf;
// buf.append((char*)&action_buffer_.at(0), sizeof(int32_t) * action_buffer_.size());
// buf.append("\r\n", 2);
// action_buffer_.clear();
// return buf;
//}
