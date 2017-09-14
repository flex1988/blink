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

    std::string metakey = "M:" + key;
    std::string metaval;

    RecordLock l(&_mutex_list_record, key);

    s = _list->Get(rocksdb::ReadOptions(), metakey, &metaval);

    if (s.ok()) {
        ListMeta meta(metaval);

        if (meta.IsElementsFull()) {
            return rocksdb::Status::InvalidArgument("Maximum element size limited: " + std::to_string(meta.Size()));
        }

        meta.IncrSize();

        ListMetaBlockPtr* blockptr = meta.BlockAt(0);
        if (blockptr->size == LIST_BLOCK_KEYS) {
            if (meta.IsBlocksFull()) {
                return rocksdb::Status::InvalidArgument("Maximum block size limited: " + std::to_string(meta.BSize()));
            }

            int cursor = meta.BSize();
            while (cursor > 0) {
                *meta.BlockAt(cursor) = *meta.BlockAt(cursor - 1);
                cursor--;
            }

            meta.IncrBSize();

            blockptr->size = 0;
            blockptr->addr = 0;
        }

        // mitem->size == 0 means block key not exists
        if (blockptr->size == 0) {
            meta.IncrBSize();

            blockptr->size++;
            blockptr->addr = meta.AllocArea();

            std::string blockkey = "B:" + std::to_string(blockptr->addr) + ":" + key;
            std::string blockval;

            ListMetaBlock block;
            block.addr[0] = meta.AllocArea();

            std::string leaf = "V:" + std::to_string(block.addr[0]) + ":" + key;

            LOG_DEBUG << "set leaf key: " + leaf;

            batch.Put(metakey, meta.ToString());
            batch.Put(blockkey, block.ToString());
            batch.Put(leaf, val);

            s = _list->Write(rocksdb::WriteOptions(), &batch);

            if (s.ok()) *llen = meta.Size();

            return s;
        }
        else {
            std::string blockkey = "B:" + std::to_string(blockptr->addr) + ":" + key;
            std::string blockval;

            s = _list->Get(rocksdb::ReadOptions(), blockkey, &blockval);

            if (!s.ok()) return s;

            ListMetaBlock block(blockval);
            block.Insert(0, blockptr->size, meta.AllocArea());
            blockptr->size++;

            std::string leaf = "V:" + std::to_string(block.addr[0]) + ":" + key;

            LOG_DEBUG << "set leaf key: " + leaf;

            batch.Put(metakey, meta.ToString());
            batch.Put(blockkey, block.ToString());
            batch.Put(leaf, val);

            s = _list->Write(rocksdb::WriteOptions(), &batch);

            if (s.ok()) *llen = meta.Size();

            return s;
        }
    }
    else if (s.IsNotFound()) {
        ListMeta meta;
        ListMetaBlockPtr* blockptr = meta.BlockAt(0);
        meta.IncrSize();

        blockptr->addr = meta.AllocArea();
        blockptr->size = 1;

        std::string blockkey = "B:" + std::to_string(blockptr->addr) + ":" + key;
        std::string blockval;

        ListMetaBlock block;
        block.addr[0] = meta.AllocArea();

        std::string leaf = "V:" + std::to_string(block.addr[0]) + ":" + key;

        LOG_DEBUG << "set leaf key: " + leaf;

        batch.Put(metakey, meta.ToString());
        batch.Put(blockkey, block.ToString());
        batch.Put(leaf, val);

        s = _list->Write(rocksdb::WriteOptions(), &batch);
        if (s.ok()) *llen = meta.Size();
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

rocksdb::Status RedisDB::LIndex(const std::string& key, const int64_t index, std::string* val)
{
    rocksdb::Status s;

    std::string metakey = "M:" + key;
    std::string metaval;

    int64_t cursor = index;

    RecordLock l(&_mutex_list_record, key);

    s = _list->Get(rocksdb::ReadOptions(), metakey, &metaval);

    if (s.ok()) {
        ListMeta meta(metaval);
        if (cursor < 0) cursor = meta.Size() + cursor;
        if (cursor >= meta.Size()) return rocksdb::Status::InvalidArgument("outof list index");

        int i;
        for (i = 0; i < LIST_META_BLOCKS; i++) {
            ListMetaBlockPtr* blockptr = meta.BlockAt(i);
            if (cursor > blockptr->size)
                cursor -= blockptr->size;
            else {
                std::string blockkey = "B:" + std::to_string(blockptr->addr) + ":" + key;
                std::string blockval;

                s = _list->Get(rocksdb::ReadOptions(), blockkey, &blockval);
                if (!s.ok()) return s;

                ListMetaBlock block(blockval);

                std::string valuekey = "V:" + std::to_string(block.addr[cursor]) + ":" + key;

                LOG_DEBUG << "get leaf key: " + valuekey;
                s = _list->Get(rocksdb::ReadOptions(), valuekey, val);
                return s;
            }
        }

        return rocksdb::Status::Corruption("get list element error");
    }
    else {
        return s;
    }
}
