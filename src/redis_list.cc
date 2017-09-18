#include "redis_list.h"
#include "common.h"
#include "meta.h"
#include "mutex.h"
#include "redis_db.h"

rocksdb::Status RedisDB::LPush(const std::string& key, const std::string& val, int64_t* llen)
{
    rocksdb::Status s;

    std::string metakey = EncodeMetaKey(key);
    std::string metaval;

    RecordLock l(&mutex_list_record_, key);

    if (listmeta_.find(metakey) != listmeta_.end()) {
        ListMeta meta(listmeta_[metakey]);

        if (meta.IsElementsFull()) {
            return rocksdb::Status::InvalidArgument("Maximum element size limited: " + std::to_string(meta.Size()));
        }
        meta.IncrSize();

        ListMetaBlockPtr* blockptr = meta.BlockAt(0);
        if (blockptr->size == LIST_BLOCK_KEYS) {
            blockptr = meta.InsertNewMetaBlockPtr(0);

            if (blockptr == NULL) {
                return rocksdb::Status::InvalidArgument("Maximum block size limited: " + std::to_string(meta.BSize()));
            }
        }

        // mitem->size == 0 means block key not exists
        if (blockptr->size == 0) {
            blockptr->size++;
            blockptr->addr = meta.AllocArea();

            std::string blockkey = EncodeBlockKey(key, blockptr->addr);
            std::string blockval;

            ListMetaBlock block;
            block.addr[0] = meta.AllocArea();

            std::string leaf = EncodeValueKey(key, block.addr[0]);

            LOG_DEBUG << "set leaf key: " + leaf;

            listmeta_[metakey] = meta.ToString();
            listmeta_[blockkey] = block.ToString();

            s = list_->Put(rocksdb::WriteOptions(), leaf, val);

            if (s.ok()) *llen = meta.Size();

            return s;
        }
        else {
            std::string blockkey = EncodeBlockKey(key, blockptr->addr);

            if (listmeta_.find(blockkey) == listmeta_.end()) {
                return rocksdb::Status::Corruption("miss block key");
            }

            std::string blockval = listmeta_[blockkey];

            ListMetaBlock block(blockval);
            block.Insert(0, blockptr->size, meta.AllocArea());
            blockptr->size++;

            std::string leaf = EncodeValueKey(key, block.addr[0]);

            LOG_DEBUG << "set leaf key: " + leaf;

            listmeta_[metakey] = meta.ToString();
            listmeta_[blockkey] = block.ToString();

            s = list_->Put(rocksdb::WriteOptions(), leaf, val);

            if (s.ok()) *llen = meta.Size();

            return s;
        }
    }
    else {
        ListMeta meta;
        ListMetaBlockPtr* blockptr = meta.BlockAt(0);
        meta.IncrSize();

        blockptr->addr = meta.AllocArea();
        blockptr->size = 1;

        std::string blockkey = EncodeBlockKey(key, blockptr->addr);

        std::string blockval;

        ListMetaBlock block;
        block.addr[0] = meta.AllocArea();

        std::string leaf = EncodeValueKey(key, block.addr[0]);

        LOG_DEBUG << "set leaf key: " + leaf;

        listmeta_[metakey] = meta.ToString();
        listmeta_[blockkey] = block.ToString();

        s = list_->Put(rocksdb::WriteOptions(), leaf, val);
        if (s.ok()) *llen = meta.Size();
        return s;
    }
}

rocksdb::Status RedisDB::LPop(const std::string& key, std::string* val)
{
    rocksdb::Status s;

    return s;
}

rocksdb::Status RedisDB::LIndex(const std::string& key, const int64_t index, std::string* val)
{
    rocksdb::Status s;

    std::string metakey = EncodeMetaKey(key);
    std::string metaval;

    int64_t cursor = index;

    RecordLock l(&mutex_list_record_, key);

    s = list_->Get(rocksdb::ReadOptions(), metakey, &metaval);

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
                std::string blockkey = EncodeBlockKey(key, blockptr->addr);
                std::string blockval;

                s = list_->Get(rocksdb::ReadOptions(), blockkey, &blockval);
                if (!s.ok()) return s;

                ListMetaBlock block(blockval);

                std::string valuekey = EncodeValueKey(key, block.addr[cursor]);

                LOG_DEBUG << "get leaf key: " + valuekey;
                s = list_->Get(rocksdb::ReadOptions(), valuekey, val);
                return s;
            }
        }

        return rocksdb::Status::Corruption("get list element error");
    }
    else {
        return s;
    }
}
