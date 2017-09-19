#include "redis_set.h"
#include "meta.h"
#include "redis_db.h"

rocksdb::Status RedisDB::SAdd(const std::string& key, const std::string& member, int64_t* res)
{
    rocksdb::Status s;
    RecordLock l(&mutex_set_record_, key);

    std::string setkey = EncodeSetKey(key, member);
    std::string val;

    s = set_->Get(rocksdb::ReadOptions(), setkey, &val);

    if (s.IsNotFound()) {
        *res = 1;

        if (setmeta_.find(key) == setmeta_.end()) {
            setmeta_[key] = std::shared_ptr<SetMeta>(new SetMeta(key));
        }

        std::shared_ptr<SetMeta> meta = setmeta_[key];

        if (meta->IsSetFull()) {
            return rocksdb::Status::InvalidArgument("set if full");
        }

        meta->IncrSize();

        s = set_->Put(rocksdb::WriteOptions(), setkey, rocksdb::Slice());

        if (s.ok()) {
            metaqueue_.push(meta->ToString());
        }
    }
    else if (s.ok()) {
        *res = 0;
    }
    else {
        return rocksdb::Status::Corruption("sadd check member error");
    }

    return s;
}

rocksdb::Status RedisDB::SCard(const std::string& key, int64_t* res)
{
    if (setmeta_.find(key) == setmeta_.end()) {
        *res = 0;
    }
    else {
        *res = setmeta_[key]->Size();
    }

    return rocksdb::Status::OK();
}
