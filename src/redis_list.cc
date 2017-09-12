#include "common.h"
#include "meta.h"
#include "mutex.h"
#include "redisdb.h"

rocksdb::Status RedisDB::LPush(const std::string& key, const std::string& val, int64_t* llen)
{
    if (key.size() >= KEY_MAX_LENGTH || key.size() <= 0) {
        return rocksdb::Status::InvalidArgument("Invalid key length");
    }

    rocksdb::Status s;
    rocksdb::WriteBatch batch;

    std::string meta_key = "L1:" + key;
    std::string meta_val;

    RecordLock l(&_mutex_list_record, key);

    s = _list->Get(rocksdb::ReadOptions(), meta_key, &meta_val);

    if (s.ok()) {
        ListMeta* meta = (ListMeta*)meta_val.c_str();
        meta->size++;
        if (meta->size == meta->limit) {
            return rocksdb::Status::InvalidArgument("Maximum element size limited");
        }

        ListMetaItem* mitem = &meta->items[0];
        if (mitem->size == LIST_BLOCK_SIZE) {
            if (meta->msize == LIST_META_SIZE) {
                return rocksdb::Status::InvalidArgument("Maximum block size limited");
            }

            meta_val = meta_val.substr(0, 40) + std::string(8, 0) + meta_val.substr(40);
            meta = (ListMeta*)meta_val.c_str();
            mitem = meta->items[0];
        }

        mitem->size++;
    }
    else if (s.IsNotFound()) {
        meta_val.reserve(LIST_META_SIZE * sizeof(struct ListMetaItem) + sizeof(ListMeta));
        ListMeta* meta = (ListMeta*)meta_val.c_str();
        meta->size = 1;
        meta->addr_seq = 0;
        meta->limit = LIST_ELEMENT_SIZE;
        meta->msize = 1;
        meta->mlimit = LIST_META_SIZE;

        ListMetaItem* mitem = &meta->items[0];
        mitem->addr = 0;
        mitem->size = 1;

        batch.Put(meta_key, meta_val);

        std::string meta_idx = "L2:0:" + key;
        std::string meta_idx_val;

        meta_idx_val.reserve(4096 * sizeof(int64_t));
        ListMetaIndex* idx = (ListMetaIndex*)meta_idx_val.c_str();
        idx->addr[0] = meta->addr_seq++;
        batch.Put(meta_idx, meta_idx_val);

        std::string leaf = "l:0:" + key;
        batch.Put(leaf, val);
        s = _list->Write(rocksdb::WriteOptions(), &batch);
        return s;
    }
    else {
        return rocksdb::Status::Corruption("get listmeta error");
    }
    return s;
}

rocksdb::Status RedisDB::LPop(const std::string& key, std::string* val)
{
    rocksdb::Status s;

    return s;
}
