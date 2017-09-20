#include "meta_db.h"
#include "common.h"

MetaDB::MetaDB(const std::string& path) : path_(path)
{
    fdb_status status;

    config_ = fdb_get_default_config();
    config_.durability_opt = FDB_DRB_ASYNC;
    config_.buffercache_size = 1073741824;

    kvs_config_ = fdb_get_default_kvs_config();

    std::string p = path_ + "/meta.db";

    status = fdb_open(&fhandle_, p.data(), &config_);
    assert(status == FDB_RESULT_SUCCESS);

    status = fdb_kvs_open(fhandle_, &kvhandle_, NULL, &kvs_config_);
    assert(status == FDB_RESULT_SUCCESS);
}

fdb_status MetaDB::Set(const std::string& key, const std::string& val)
{
    fdb_status status;
    fdb_doc* doc;

    status = fdb_doc_create(&doc, key.data(), key.size(), NULL, 0, val.data(), val.size());
    if (status != FDB_RESULT_SUCCESS) return status;

    status = fdb_set(kvhandle_, doc);
    if (status != FDB_RESULT_SUCCESS) return status;

    status = fdb_commit(fhandle_, FDB_COMMIT_NORMAL);
    if (status != FDB_RESULT_SUCCESS) return status;

    status = fdb_doc_free(doc);

    LOG_DEBUG << "Set success, key: " << key << val;

    return status;
}

fdb_status MetaDB::Get(const std::string& key, std::string& val)
{
    fdb_doc* doc;
    fdb_status status;

    status = fdb_doc_create(&doc, key.data(), key.size(), NULL, 0, NULL, 0);
    if (status != FDB_RESULT_SUCCESS) return status;

    status = fdb_get(kvhandle_, doc);
    if (status != FDB_RESULT_SUCCESS) return status;

    val.append((char*)doc->body, doc->bodylen);

    status = fdb_doc_free(doc);

    return status;
}
