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

    std::string metakey = "L1:" + key;
    std::string metaval;

    RecordLock l(&_mutex_list_record, key);

    s = _list->Get(rocksdb::ReadOptions(), metakey, &metaval);

    if (s.ok()) {
        ListMeta meta(metaval);

        if (meta._size == meta._limit) {
            return rocksdb::Status::InvalidArgument("Maximum element size limited");
        }

        meta._size++;

        ListMetaBlock* mblock = meta.getBlock(0);
        if (mblock->size == LIST_BLOCK_KEYS) {
            if (meta._msize == LIST_META_BLOCKS) {
                return rocksdb::Status::InvalidArgument("Maximum block size limited");
            }
        }

        // mitem->size == 0 means index key not exists
        if (mblock->size == 0) {
            mblock->size++;
            mblock->addr = meta.fetchSeq();

            std::string blockkey = "L2:" + std::to_string(mblock->addr) + ":" + key;

            ListMetaBlockKeys keys;
            keys.addr[0] = meta.fetchSeq();

            std::string leaf = "l:" + std::to_string(keys.addr[0]) + ":" + key;

            batch.Put(metakey, meta.toString());
            batch.Put(blockkey, keys.toString());
            batch.Put(leaf, val);

            s = _list->Write(rocksdb::WriteOptions(), &batch);
            return s;
        }
        else {
            mblock->size++;

            std::string blockkey = "L2:0:" + key;
            std::string blockval;

            s = _list->Get(rocksdb::ReadOptions(), blockkey, &blockval);

            if (!s.ok()) return s;

            ListMetaBlockKeys keys(blockval);
            keys.insert(0, mblock->size, meta.fetchSeq());

            std::string leaf = "l:" + std::to_string(meta.currentSeq()) + ":" + key;

            batch.Put(metakey, meta.toString());
            batch.Put(blockkey, keys.toString());
            batch.Put(leaf, val);

            s = _list->Write(rocksdb::WriteOptions(), &batch);
            return s;
        }
    }
    else if (s.IsNotFound()) {
        ListMeta meta;
        ListMetaBlock* mblock = meta.getBlock(0);

        mblock->addr = meta.fetchSeq();
        mblock->size = 1;

        metaval = meta.toString();

        std::string block = "L2:" + std::to_string(mblock->addr) + ":" + key;
        std::string blockval;

        ListMetaBlockKeys keys;
        keys.addr[0] = meta.fetchSeq();
        blockval = keys.toString();

        std::string leaf = "l:" + std::to_string(keys.addr[0]) + ":" + key;

        batch.Put(metakey, metaval);
        batch.Put(block, blockval);
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
