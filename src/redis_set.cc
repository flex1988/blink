#include "redis_set.h"
#include "meta.h"
#include "redis_db.h"

rocksdb::Status RedisDB::SAdd(const std::string& key, const std::string& member, int64_t* res)
{
    rocksdb::Status s;
    RecordLock l(&mutex_set_record_, key);

    std::string setkey = EncodeSetKey(key);

    if (memmeta_.find(setkey) == memmeta_.end()) {
        memmeta_[setkey] = std::shared_ptr<MetaBase>(new SetMeta(setkey));
    }
    std::shared_ptr<SetMeta> meta = std::dynamic_pointer_cast<SetMeta>(memmeta_[setkey]);

    std::string memberkey = EncodeMemberKey(key, member);
    std::string val;

    if (meta->IsSetFull()) {
        return rocksdb::Status::InvalidArgument("set if full");
    }

    if (meta->BFNotExists(member)) {
        *res = 1;

        meta->IncrSize();

        s = set_->Put(rocksdb::WriteOptions(), memberkey, rocksdb::Slice());

        if (s.ok()) {
            meta->BFAdd(member);
            metaqueue_.push(meta->ToString());
        }
    }
    else {
        s = set_->Get(rocksdb::ReadOptions(), memberkey, &val);

        if (s.IsNotFound()) {
            *res = 1;

            meta->IncrSize();

            s = set_->Put(rocksdb::WriteOptions(), memberkey, rocksdb::Slice());

            if (s.ok()) {
                meta->BFAdd(member);
                metaqueue_.push(meta->ToString());
            }
        }
        else if (s.ok()) {
            *res = 0;
        }
        else {
            return rocksdb::Status::Corruption("sadd check member error");
        }
    }

    return s;
}

rocksdb::Status RedisDB::SCard(const std::string& key, int64_t* res)
{
    std::string setkey = EncodeSetKey(key);

    if (memmeta_.find(setkey) == memmeta_.end()) {
        *res = 0;
    }
    else {
        *res = memmeta_[setkey]->Size();
    }

    return rocksdb::Status::OK();
}
