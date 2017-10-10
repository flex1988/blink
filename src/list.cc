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

    if (IsReloadingAof())
        return s;

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

    if (IsReloadingAof())
        return s;

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

    if (!s.ok())
        return s;

    std::string leaf = EncodeListValueKey(key, addr);

    LOG_DEBUG << "get leaf: " << leaf;

    s = list_->Get(rocksdb::ReadOptions(), leaf, &val);

    if (!s.ok())
        return s;

    s = list_->Delete(rocksdb::WriteOptions(), leaf);

    return s;
}

rocksdb::Status RedisDB::LIndex(const std::string& key, const int64_t index, std::string* val)
{
    rocksdb::Status s;
    int64_t cursor = index;

    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (meta == nullptr) {
        return rocksdb::Status::InvalidArgument("list meta not exists");
    }

    if (cursor < 0)
        cursor = meta->Size() + cursor;
    if (cursor >= meta->Size())
        return rocksdb::Status::InvalidArgument("outof list index");

    for (int i = 0; i < meta->BSize(); i++) {
        ListMetaBlockPtr* blockptr = meta->BlockAt(i);
        if (cursor >= blockptr->size) {
            cursor -= blockptr->size;
        }
        else {
            std::shared_ptr<ListMetaBlock> block = GetListMetaBlock(key, blockptr->addr);

            if (block == nullptr) {
                return rocksdb::Status::InvalidArgument("list meta block not exists");
            }

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

    if (meta == nullptr) {
        return rocksdb::Status::InvalidArgument("list meta not exists");
    }

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

    for (auto i : s) {
        assert(i.ok());
    }

    return rocksdb::Status::OK();
}

rocksdb::Status RedisDB::LSet(const std::string& key, int index, const std::string& val)
{
    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (meta == nullptr) {
        return rocksdb::Status::InvalidArgument("no such key");
    }

    int64_t addr = GetIndexAddr(meta, key, index);

    if (addr == -1) {
        return rocksdb::Status::Corruption("list meta error");
    }

    std::string leaf = EncodeListValueKey(key, addr);

    auto s = list_->Put(rocksdb::WriteOptions(), leaf, val);

    return s;
}

rocksdb::Status RedisDB::LRem(const std::string& key, int count, const std::string& val, int64_t* remove)
{
    RecordLock l(&mutex_list_record_, key);

    std::shared_ptr<ListMeta> meta = GetListMeta(key);

    if (meta == nullptr) {
        return rocksdb::Status::InvalidArgument("no such key");
    }

    int64_t addr;
    int index;
    std::string v;
    rocksdb::Status s;

    if (count > 0) {
        index = 0;
        while (count) {
            addr = GetIndexAddr(meta, key, index++);
            if (addr == -1)
                break;

            std::string leaf = EncodeListValueKey(key, addr);

            s = list_->Get(rocksdb::ReadOptions(), leaf, &v);

            assert(s.ok());

            if (v == val) {
                s = list_->Delete(rocksdb::WriteOptions(), leaf);
                assert(s.ok());

                RemoveListMetaAt(key, index - 1, &addr);
                (*remove)++;

                index--;
                count--;
            }
        }
    }
    else if (count < 0) {
        index = meta->Size() - 1;
        count = -count;

        while (count) {
            if (index < 0)
                break;

            addr = GetIndexAddr(meta, key, index--);

            if (addr == -1)
                break;

            std::string leaf = EncodeListValueKey(key, addr);

            s = list_->Get(rocksdb::ReadOptions(), leaf, &v);

            assert(s.ok());

            if (v == val) {
                s = list_->Delete(rocksdb::WriteOptions(), leaf);
                assert(s.ok());

                RemoveListMetaAt(key, index + 1, &addr);
                (*remove)++;

                index++;
                count--;
            }
        }
    }
    else {
        index = 0;
        while (1) {
            addr = GetIndexAddr(meta, key, index++);
            if (addr == -1)
                break;

            std::string leaf = EncodeListValueKey(key, addr);

            s = list_->Get(rocksdb::ReadOptions(), leaf, &v);

            assert(s.ok());

            if (v == val) {
                s = list_->Delete(rocksdb::WriteOptions(), leaf);
                assert(s.ok());

                RemoveListMetaAt(key, index - 1, &addr);
                index--;
                (*remove)++;
            }
        }
    }

    return rocksdb::Status::OK();
}
