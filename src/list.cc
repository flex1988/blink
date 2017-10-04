#include "list.h"
#include "common.h"
#include "db.h"
#include "meta.h"
#include "mutex.h"

#include <memory>

rocksdb::Status RedisDB::LPush(const std::string& key, const std::string& val, int64_t* llen)
{
    rocksdb::Status s;

    RecordLock l(&mutex_list_record_, key);

    int64_t addr;

    s = InsertListMetaAt(key, 0, &addr, llen);

    if (!s.ok()) {
        return s;
    }

    if (IsReloadingAof()) return s;

    std::string leaf = EncodeListValueKey(key, addr);

    LOG_DEBUG << "set leaf: " << leaf;

    s = list_->Put(rocksdb::WriteOptions(), leaf, val);

    assert(s.ok());
    //写入失败就麻烦大了，内存索引已经修改无法回滚

    return s;
}

rocksdb::Status RedisDB::LPushX(const std::string& key, const std::string& val, int64_t* llen)
{
    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (meta == nullptr || meta->Size() == 0) {
        *llen = 0;
        return rocksdb::Status::OK();
    }

    // 只能上一个锁，而且这里不能释放锁，所以目前还不能复用LPush函数，whatever
    // return LPush(key, val, llen);

    rocksdb::Status s;
    int64_t addr;

    s = InsertListMetaAt(key, 0, &addr, llen);

    if (!s.ok()) {
        return s;
    }

    if (IsReloadingAof()) return s;

    std::string leaf = EncodeListValueKey(key, addr);

    LOG_DEBUG << "set leaf: " << leaf;

    s = list_->Put(rocksdb::WriteOptions(), leaf, val);

    assert(s.ok());
    //写入失败就麻烦大了，内存索引已经修改无法回滚

    return s;
}

rocksdb::Status RedisDB::LLen(const std::string& key, int64_t* llen)
{
    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (meta == nullptr) {
        *llen = 0;
    }
    else {
        *llen = meta->Size();
    }

    return rocksdb::Status::OK();
}

rocksdb::Status RedisDB::LPop(const std::string& key, std::string& val)
{
    rocksdb::Status s;
    int64_t addr;

    s = RemoveListMetaAt(key, 0, &addr);

    if (!s.ok()) return s;

    std::string leaf = EncodeListValueKey(key, addr);

    LOG_DEBUG << "get leaf: " << leaf;

    s = list_->Get(rocksdb::ReadOptions(), leaf, &val);

    if (!s.ok()) return s;

    s = list_->Delete(rocksdb::WriteOptions(), leaf);

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

rocksdb::Status RedisDB::LRange(const std::string& key, int start, int end, std::vector<std::string>& range)
{
    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (start > meta->Size() - 1) {
        return rocksdb::Status::OK();
    }

    if (end > meta->Size() - 1) {
        end = meta->Size() - 1;
    }

    LOG_DEBUG << "LRANGE start: " << start << " end: " << end;

    std::vector<std::string> keys;

    std::vector<rocksdb::Slice> kslices;

    GetMetaRangeKeys(meta, start, end - start + 1, keys);

    for (auto k : keys) {
        kslices.push_back(k);
    }

    auto s = list_->MultiGet(rocksdb::ReadOptions(), kslices, &range);

    return rocksdb::Status::OK();
}
