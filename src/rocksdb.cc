#include "rocksdb.h"
#include "common.h"

RocksDB::RocksDB(std::string path, rocksdb::Options* options)
    : _path(path), _options(options)
{
}

RocksDB::~RocksDB() {}
rocksdb::Status RocksDB::Open()
{
    rocksdb::Status s = rocksdb::DB(_options, "./db/pika.db", _db);
    assert(s.ok());
}
