#include "redis_list.h"
#include "common.h"
#include "meta.h"
#include "mutex.h"
#include "redis_db.h"

#include <memory>

rocksdb::Status RedisDB::LPush(const std::string& key, const std::string& val, int64_t* llen)
{
    rocksdb::Status s;

    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetOrCreateListMeta(key);

    int64_t addr;
    s = InsertListMeta(key, meta, 0, &addr);

    if (!s.ok()) {
        meta->ResetBuffer();
        return s;
    }

    std::string leaf = EncodeListValueKey(key, addr);

    LOG_DEBUG << "set leaf: " << leaf;

    s = list_->Put(rocksdb::WriteOptions(), leaf, val);

    if (s.ok()) {
        *llen = meta->Size();
        metaqueue_.push(meta->ActionBuffer());
    }
    else {
        meta->ResetBuffer();
    }

    return s;
}

std::shared_ptr<ListMeta> RedisDB::GetOrCreateListMeta(const std::string& key)
{
    std::string metakey = EncodeListMetaKey(key);
    std::string metaval;

    if (memmeta_.find(metakey) == memmeta_.end()) {
        memmeta_[metakey] = std::shared_ptr<MetaBase>(new ListMeta(key, INIT));
    }

    std::shared_ptr<ListMeta> meta = std::dynamic_pointer_cast<ListMeta>(memmeta_[metakey]);

    return meta;
}

std::shared_ptr<ListMeta> RedisDB::GetListMeta(const std::string& key)
{
    std::string metakey = EncodeListMetaKey(key);
    std::string metaval;

    if (memmeta_.find(metakey) == memmeta_.end()) {
        return NULL;
    }

    std::shared_ptr<ListMeta> meta = std::dynamic_pointer_cast<ListMeta>(memmeta_[metakey]);

    return meta;
}

std::shared_ptr<ListMetaBlock> RedisDB::GetOrCreateListMetaBlock(const std::string& key, int64_t addr)
{
    std::string blockkey = EncodeListBlockKey(key, addr);
    std::string blockval;

    if (memmeta_.find(blockkey) == memmeta_.end()) {
        memmeta_[blockkey] = std::shared_ptr<MetaBase>(new ListMetaBlock(key, addr));
    }

    std::shared_ptr<ListMetaBlock> block = std::dynamic_pointer_cast<ListMetaBlock>(memmeta_[blockkey]);

    return block;
}

rocksdb::Status RedisDB::InsertListMeta(const std::string& key, std::shared_ptr<ListMeta> meta, int64_t index, int64_t* addr)
{
    if (meta->IsElementsFull()) {
        return rocksdb::Status::InvalidArgument("Maximum element size limited: " + std::to_string(meta->Size()));
    }

    if (index > meta->Size() || index < 0) {
        return rocksdb::Status::InvalidArgument("index size beyond max index: " + std::to_string(index));
    }

    meta->IncrSize();

    int blockidx = 0;
    ListMetaBlockPtr* blockptr = meta->GetIndexBlockPtr(index, &blockidx);

    if (blockptr == nullptr) {
        if (index == 0) {
            blockidx = 0;
            blockptr = meta->InsertNewMetaBlockPtr(0);
        }
        else {
            blockidx = meta->BSize();
            blockptr = meta->InsertNewMetaBlockPtr(meta->BSize());
        }
    }

    if (blockptr->size == LIST_BLOCK_KEYS) {
        blockptr = meta->InsertNewMetaBlockPtr(blockidx);

        if (blockptr == nullptr) {
            return rocksdb::Status::InvalidArgument("Maximum block size limited: " + std::to_string(meta->BSize()));
        }
    }

    if (blockptr->size == 0) {
        blockptr->addr = meta->AllocArea();
    }

    std::shared_ptr<ListMetaBlock> block = GetOrCreateListMetaBlock(key, blockptr->addr);

    block->Insert(index, blockptr->size, meta->AllocArea());
    blockptr->size++;

    *addr = meta->CurrentArea();

    return rocksdb::Status::OK();
}

rocksdb::Status RedisDB::LPop(const std::string& key, std::string* val)
{
    rocksdb::Status s;

    return s;
}

rocksdb::Status RedisDB::LIndex(const std::string& key, const int64_t index, std::string* val)
{
    rocksdb::Status s;

    int64_t cursor = index;

    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (meta == NULL) {
        return rocksdb::Status::InvalidArgument("list meta not exists");
    }

    if (cursor < 0) cursor = meta->Size() + cursor;
    if (cursor >= meta->Size()) return rocksdb::Status::InvalidArgument("outof list index");

    for (int i = 0; i < LIST_META_BLOCKS; i++) {
        ListMetaBlockPtr* blockptr = meta->BlockAt(i);
        if (cursor > blockptr->size)
            cursor -= blockptr->size;
        else {
            std::string blockkey = EncodeListBlockKey(key, blockptr->addr);
            std::string blockval;

            if (memmeta_.find(blockkey) == memmeta_.end()) {
                return rocksdb::Status::InvalidArgument("list meta block not exists");
            }

            std::shared_ptr<ListMetaBlock> block = std::dynamic_pointer_cast<ListMetaBlock>(memmeta_[blockkey]);

            std::string valuekey = EncodeListValueKey(key, block->FetchAddr(cursor));

            LOG_DEBUG << "get leaf key: " + valuekey;
            s = list_->Get(rocksdb::ReadOptions(), valuekey, val);
            return s;
        }
    }

    return rocksdb::Status::Corruption("get list element error");
}
