#include "common.h"
#include "db.h"
#include "meta.h"
#include "server.h"

namespace blink {

ListMeta::ListMeta(const std::string& str, Action action)
{
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
        area_index_ = 1;

        std::memset(blocks_, 0, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    }
}

std::string ListMeta::Serialize()
{
    std::string str;

    uint16_t len = 2 + key_.size() + sizeof(int64_t) * 5 + sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS + 2;

    LOG_DEBUG << "serialize key: " << key_ << " len: " << len;

    str.append((char*)&META_SNAP_MAGIC, 1);
    str.append((char*)&len, 2);
    str.append(1, 'L');
    str.append(1, key_.size());
    str.append(key_.data(), key_.size());
    str.append((char*)&size_, sizeof(int64_t));
    str.append((char*)&limit_, sizeof(int64_t));
    str.append((char*)&bsize_, sizeof(int64_t));
    str.append((char*)&blimit_, sizeof(int64_t));
    str.append((char*)&area_index_, sizeof(int64_t));
    str.append((char*)blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS);
    str.append("\r\n");
    return str;
}

int ListMeta::AllocArea() { return area_index_++; }

ListMetaOperator::ListMetaOperator(std::unordered_map<std::string, std::shared_ptr<MetaBase>>* memmeta) { memmeta_ = memmeta; }

std::shared_ptr<ListMeta> ListMetaOperator::GetMeta(const std::string& key, bool create)
{
    std::string metakey = EncodeListMetaKey(key);
    std::string metaval;

    if (memmeta_->find(metakey) == memmeta_->end()) {
        if (!create)
            return nullptr;

        (*memmeta_)[metakey] = std::shared_ptr<MetaBase>(new ListMeta(key, INIT));
    }

    std::shared_ptr<ListMeta> meta = std::dynamic_pointer_cast<ListMeta>((*memmeta_)[metakey]);

    return meta;
}

std::shared_ptr<ListMetaBlock> ListMetaOperator::GetMetaBlock(const std::string& key, int64_t addr, bool create)
{
    std::string blockkey = EncodeListBlockKey(key, addr);
    std::string blockval;

    if (memmeta_->find(blockkey) == memmeta_->end()) {
        if (!create)
            return nullptr;

        (*memmeta_)[blockkey] = std::shared_ptr<MetaBase>(new ListMetaBlock(key, addr));
    }

    std::shared_ptr<ListMetaBlock> block = std::dynamic_pointer_cast<ListMetaBlock>((*memmeta_)[blockkey]);

    return block;
}

ListMetaBlockPtr* ListMeta::InsertMetaBlock(int index)
{
    if (IsBlocksFull()) {
        return nullptr;
    }

    if (index > bsize_ || index < 0) {
        return nullptr;
    }

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
    if (key_ != meta.key_)
        return false;
    if (size_ != meta.size_)
        return false;
    if (limit_ != meta.limit_)
        return false;
    if (bsize_ != meta.bsize_)
        return false;
    if (blimit_ != meta.blimit_)
        return false;
    if (area_index_ != meta.area_index_)
        return false;
    if (std::memcmp(blocks_, meta.blocks_, sizeof(ListMetaBlockPtr) * LIST_META_BLOCKS) != 0)
        return false;

    return true;
}

void ListMeta::RemoveBlockPtr(int bidx)
{
    for (int cursor = bidx; cursor < bsize_; cursor++) {
        blocks_[cursor] = blocks_[cursor + 1];
    }

    bsize_--;
}

void ListMetaBlock::Insert(int index, int val)
{
    int cursor = size_;
    while (cursor > index) {
        addr_[cursor] = addr_[cursor - 1];
        cursor--;
    }
    addr_[index] = val;

    size_++;
}

int ListMetaBlock::Remove(int index)
{
    int cursor = index;
    int val = addr_[index];

    while (cursor < size_) {
        addr_[cursor] = addr_[cursor + 1];
        cursor++;
    }

    size_--;

    return val;
}

ListMetaBlock::ListMetaBlock(const std::string& str)
{
    assert(str.at(0) == 'B');
    uint8_t klen = (int8_t)str.at(1);

    key_ = std::string(str.substr(2, klen));
    self_addr_ = *(int64_t*)&str.at(2 + klen);
    size_ = *(int64_t*)&str.at(2 + klen + sizeof(int64_t));
    std::memcpy(addr_, &str.at(2 + klen + sizeof(int64_t) * 2), sizeof(int64_t) * LIST_BLOCK_KEYS);
}

std::string ListMetaBlock::Serialize()
{
    assert(key_.size() > 0);

    uint16_t len = 2 + key_.size() + 2 * sizeof(int64_t) + sizeof(int64_t) * LIST_BLOCK_KEYS + 2;

    std::string str;
    str.append((char*)&META_SNAP_MAGIC, 1);
    str.append((char*)&len, 2);
    str.append(1, 'B');
    str.append(1, key_.size());
    str.append(key_.data(), key_.size());
    str.append((char*)&self_addr_, sizeof(int64_t));
    str.append((char*)&size_, sizeof(int64_t));

    str.append((char*)addr_, sizeof(int64_t) * LIST_BLOCK_KEYS);
    str.append("\r\n");
    return str;
}

ListMetaBlockPtr* ListMeta::BlockAtIndex(int64_t index, int* idx, int* bidx)
{
    ListMetaBlockPtr* blockptr = nullptr;

    int i = 0;
    for (; i < BSize(); i++) {
        blockptr = BlockAt(i);

        if (index < blockptr->size) {
            *idx = index;
            *bidx = i;
            return blockptr;
        }
        else {
            index -= blockptr->size;
        }
    }

    return blockptr;
}

ListMetaBlockPtr* ListMeta::GetMetaBlockPtr(int64_t index, int* bidx)
{
    ListMetaBlockPtr* blockptr = nullptr;
    int blockidx;
    int cursor = index;

    int i = 0;
    for (; i < BSize(); i++) {
        blockptr = BlockAt(i);

        if (cursor < blockptr->size) {
            blockidx = i;
            break;
        }
        else {
            cursor -= blockptr->size;
        }
    }

    *bidx = cursor;

    // if block is not find, insert a new block
    if (blockptr == nullptr) {
        if (index == 0) {
            blockidx = 0;
            blockptr = InsertMetaBlock(0);
        }
        else {
            blockidx = BSize();
            blockptr = InsertMetaBlock(BSize());
        }
    }

    // if block is full ,insert a new block
    if (blockptr->size == LIST_BLOCK_KEYS) {
        blockptr = InsertMetaBlock(blockidx);

        if (blockptr == nullptr) {
            return nullptr;
        }
    }

    // if block is new create, alloc the block
    if (blockptr->addr == 0) {
        blockptr->addr = AllocArea();
    }

    return blockptr;
}

rocksdb::Status ListMetaOperator::InsertListMeta(const std::string& key, int64_t index, int64_t* addr, int64_t* size)
{
    std::shared_ptr<ListMeta> meta = GetMeta(key, true);

    if (meta->IsElementsFull()) {
        return rocksdb::Status::InvalidArgument("Maximum element size limited: " + std::to_string(meta->Size()));
    }

    if (index > meta->Size() || index < 0) {
        return rocksdb::Status::InvalidArgument("index size beyond max index: " + std::to_string(index));
    }

    int bidx;
    ListMetaBlockPtr* blockptr = meta->GetMetaBlockPtr(index, &bidx);

    if (blockptr == nullptr) {
        return rocksdb::Status::InvalidArgument("Maximum block size limited: " + std::to_string(meta->BSize()));
    }

    std::shared_ptr<ListMetaBlock> block = GetMetaBlock(key, blockptr->addr, true);

    block->Insert(bidx, meta->AllocArea());

    blockptr->size++;

    meta->IncrSize();

    *addr = meta->CurrentArea();
    *size = meta->Size();

    return rocksdb::Status::OK();
}

rocksdb::Status ListMetaOperator::RemoveListMeta(const std::string& key, int64_t index, int64_t* addr)
{
    std::shared_ptr<ListMeta> meta = GetMeta(key, false);

    if (meta == nullptr)
        return rocksdb::Status::OK();

    if (index > meta->Size() || index < 0) {
        return rocksdb::Status::InvalidArgument("index size beyond max index: " + std::to_string(index));
    }

    int idx = 0;
    int bidx = 0;
    ListMetaBlockPtr* blockptr = meta->BlockAtIndex(index, &idx, &bidx);

    if (blockptr == nullptr) {
        return rocksdb::Status::InvalidArgument("invalid index: " + std::to_string(index));
    }

    std::shared_ptr<ListMetaBlock> block = GetMetaBlock(key, blockptr->addr, false);
    assert(block != nullptr);

    *addr = block->Remove(idx);

    blockptr->size--;

    if (blockptr->size == 0) {
        meta->RemoveBlockPtr(bidx);
        std::string blockkey = EncodeListBlockKey(key, blockptr->addr);
        memmeta_->erase(blockkey);
    }

    meta->DecrSize();

    if (meta->Size() == 0) {
        std::string metakey = EncodeListMetaKey(key);
        memmeta_->erase(metakey);
    }

    return rocksdb::Status::OK();
}

void ListMetaOperator::GetMetaRangeKeys(std::shared_ptr<ListMeta> meta, int start, int nums, std::vector<std::string>& keys)
{
    int index = 0;
    for (int i = 0; i < meta->BSize(); i++) {
        if (nums == 0)
            break;

        ListMetaBlockPtr* blockptr = meta->BlockAt(i);
        index += blockptr->size;
        if (start < index) {
            int begin = start > (index - blockptr->size) ? start : 0;
            int len = nums > blockptr->size ? blockptr->size : nums;

            std::shared_ptr<ListMetaBlock> block = GetMetaBlock(meta->Key(), blockptr->addr, false);

            GetMetaBlockRangeKeys(block, begin, len, keys);

            nums -= len;
        }
    }
}

void ListMetaOperator::GetMetaBlockRangeKeys(std::shared_ptr<ListMetaBlock> block, int start, int nums, std::vector<std::string>& keys)
{
    assert(start >= 0);
    assert(nums <= block->Size());

    if (start >= block->Size())
        return;

    while (nums--) {
        std::string key = EncodeListValueKey(block->Key(), block->FetchAddr(static_cast<int64_t>(start++)));
        keys.push_back(key);
    }
}

int64_t ListMetaOperator::GetIndexAddr(std::shared_ptr<ListMeta> meta, const std::string& key, int index)
{
    for (int i = 0; i < meta->BSize(); i++) {
        ListMetaBlockPtr* blockptr = meta->BlockAt(i);
        if (index >= blockptr->size) {
            index -= blockptr->size;
        }
        else {
            std::shared_ptr<ListMetaBlock> block = GetMetaBlock(key, blockptr->addr, false);

            if (block->Size() == 0) {
                std::string blockkey = EncodeListBlockKey(key, blockptr->addr);
                memmeta_->erase(blockkey);
                return -1;
            }

            if (block == nullptr)
                return -1;

            return block->FetchAddr(index);
        }
    }

    return -1;
}
}
