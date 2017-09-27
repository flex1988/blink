#include "common.h"
#include "meta.h"

ListMeta::ListMeta(const std::string& str, Action action)
{
    InitActionHeader();

    if (action == REINIT) {
        assert(str.at(0) == 'L');
        uint8_t klen = (int8_t)str.at(1);
        assert(klen < str.size() - 2);

        key_ = str.substr(2, klen);
        size_ = *(int64_t*)&str.at(klen + 2);
        limit_ = *(int64_t*)&str.at(sizeof(int64_t) + klen + 2);
        bsize_ = *(int64_t*)&str.at(sizeof(int64_t) * 2 + klen + 2);
        blimit_ = *(int64_t*)&str.at(sizeof(int64_t) * 3 + klen + 2);
        area_index_ = *(int64_t*)&str.at(sizeof(int64_t) * 4 + klen + 2);
        str.copy((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS, 42 + klen);
    }
    else if (action == INIT) {
        key_ = str;
        size_ = 0;
        limit_ = LIST_ELEMENT_SIZE;
        bsize_ = 0;
        blimit_ = LIST_META_BLOCKS;
        area_index_ = 0;

        std::memset(blocks_, 0, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    }

    SaveAction(action, str.size(), str);
}

std::string ListMeta::Serialize()
{
    std::string str;
    std::string key = Key();

    uint16_t len = 2 + key.size() + sizeof(int64_t) * 5 + sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS + 2;

    str.append(1, META_SNAP_MAGIC);
    str.append((char*)&len, 2);
    str.append(1, 'L');
    str.append(1, key.size());
    str.append(key.data(), key.size());
    str.append((char*)&size_, sizeof(int64_t));
    str.append((char*)&limit_, sizeof(int64_t));
    str.append((char*)&bsize_, sizeof(int64_t));
    str.append((char*)&blimit_, sizeof(int64_t));
    str.append((char*)&area_index_, sizeof(int64_t));
    str.append((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    str.append("\r\n");
    return str;
}

int ListMeta::AllocArea()
{
    SaveAction(ALLOC, area_index_ + 1, "");
    return area_index_++;
}

ListMetaBlockPtr* ListMeta::InsertNewMetaBlockPtr(int index)
{
    if (IsBlocksFull()) {
        return NULL;
    }

    if (index > bsize_ || index < 0) {
        return NULL;
    }

    SaveAction(INSERT, static_cast<int16_t>(index), "");

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

bool ListMeta::operator==(const ListMeta& meta)
{
    if (key_ != meta.key_) return false;
    if (size_ != meta.size_) return false;
    if (limit_ != meta.limit_) return false;
    if (bsize_ != meta.bsize_) return false;
    if (blimit_ != meta.blimit_) return false;
    if (area_index_ != meta.area_index_) return false;
    if (std::memcmp(blocks_, meta.blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS) != 0) return false;

    return true;
}

void ListMetaBlock::Insert(int index, int size, int val)
{
    int cursor = size;
    while (cursor > index) {
        addr_[cursor] = addr_[cursor - 1];
        cursor--;
    }
    addr_[index] = val;
}

std::string ListMetaBlock::Serialize()
{
    std::string unique = GetUnique();
    assert(unique.size() > 0);

    uint16_t len = 2 + unique.size() + sizeof(int64_t) * LIST_BLOCK_KEYS + 2;

    std::string str;
    str.append(1, META_SNAP_MAGIC);
    str.append((char*)&len, 2);
    str.append(1, 'B');
    str.append(1, unique.size());
    str.append(unique.data(), unique.size());
    str.append((char*)addr_, sizeof(int64_t) * LIST_BLOCK_KEYS);
    str.append("\r\n");
    return str;
}

ListMetaBlockPtr* ListMeta::GetIndexBlockPtr(int64_t index, int* blockidx)
{
    ListMetaBlockPtr* blockptr;

    int i = 0;
    for (; i < BSize(); i++) {
        blockptr = BlockAt(i);

        if (index < blockptr->size) {
            *blockidx = i;
            return blockptr;
        }
        else
            index -= blockptr->size;
    }

    return nullptr;
}
