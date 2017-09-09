#ifndef __DB_H__
#define __DB_H__

#include "rocksdb/db.h"
#include "rocksdb/options.h"

class RocksDB {
public:
    RocksDB(std::string path, rocksdb::Options* options);
    ~RocksDB();
    rocksdb::Status Open();

private:
    rocksdb::DB* _db;
    std::string _path;
    rocksdb::Options* _options;
};
#endif
