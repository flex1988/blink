#include "common.h"
#include "meta.h"

std::string MetaBase::ActionBuffer()
{
    std::string buf;
    buf.append((char*)&action_buffer_.at(0), sizeof(int32_t) * action_buffer_.size());
    buf.append("\r\n", 2);
    action_buffer_.clear();
    return buf;
}

void MetaBase::PushAction(Action action, int16_t op, const std::string& str)
{
    action_buffer_.push_back(action << 16 | op);

    if (str.empty()) return;

    for (size_t i = 0; i < str.size(); i += 4) {
        action_buffer_.push_back(*(int32_t*)&str.at(i));
    }
}

void MetaBase::SetUnique(std::string unique)
{
    assert(unique.size() > 0);
    PushAction(UNIQUE, unique.size(), unique);
    unique_ = unique;
}

ListMeta::ListMeta() : size_(0), limit_(LIST_ELEMENT_SIZE), bsize_(0), blimit_(LIST_META_BLOCKS), area_index_(0)
{
    std::memset(blocks_, 0, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    PushAction(NEWLIST, 0, "");
}

ListMeta::ListMeta(const std::string& str)
{
    assert(str.at(0) == 'L');
    uint8_t klen = (int8_t)str.at(1);
    assert(klen < str.size() - 2);
    SetUnique(str.substr(2, klen));

    size_ = *(int64_t*)&str.at(klen + 2);
    limit_ = *(int64_t*)&str.at(sizeof(int64_t) + klen + 2);
    bsize_ = *(int64_t*)&str.at(sizeof(int64_t) * 2 + klen + 2);
    blimit_ = *(int64_t*)&str.at(sizeof(int64_t) * 3 + klen + 2);
    area_index_ = *(int64_t*)&str.at(sizeof(int64_t) * 4 + klen + 2);
    str.copy((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS, 42 + klen);
}

std::string ListMeta::ToString()
{
    std::string str;
    std::string unique = GetUnique();
    str.append(1, 'L');
    str.append(1, unique.size());
    str.append(unique.c_str(), unique.size());
    str.append((char*)&size_, sizeof(int64_t));
    str.append((char*)&limit_, sizeof(int64_t));
    str.append((char*)&bsize_, sizeof(int64_t));
    str.append((char*)&blimit_, sizeof(int64_t));
    str.append((char*)&area_index_, sizeof(int64_t));
    str.append((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    str.append("\r\n");
    return str;
}

ListMetaBlockPtr* ListMeta::BlockAt(int index) { return &blocks_[index]; }
int ListMeta::AllocArea()
{
    PushAction(ALLOC, area_index_ + 1, "");
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

    PushAction(INSERT, static_cast<int16_t>(index), "");

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

std::string ListMetaBlock::ToString()
{
    std::string str;
    str.append(1, 'B');
    str.append((char*)addr, sizeof(int64_t) * LIST_BLOCK_KEYS);
    return str;
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
