#include "meta_db.h"

MetaDB::MetaDB(const std::string &path) : path_(path)
{
    fdb_status status;

    config_ = fdb_get_default_config();
    kvs_config_ = fdb_get_default_kvs_config();

    std::string p = path_ + "/meta.db";

    status = fdb_open(&fhandle_, p.data(), &config_);
    assert(status == FDB_RESULT_SUCCESS);

    status = fdb_kvs_open(fhandle_, &kvhandle_, NULL, &kvs_config_);
    assert(status == FDB_RESULT_SUCCESS);
}
