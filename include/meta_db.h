#ifndef __METADB_H__
#define __METADB_H__

#include <string>

#include "common.h"
#include "libforestdb/forestdb.h"

class MetaDB {
public:
    MetaDB(const std::string &path);
    ~MetaDB() = default;

    fdb_status Set(const std::string &key, const std::string &val);
    fdb_status Get(const std::string& key, std::string& val);

private:
    std::string path_;
    fdb_file_handle *fhandle_;
    fdb_kvs_handle *kvhandle_;
    fdb_config config_;
    fdb_kvs_config kvs_config_;
};
#endif
