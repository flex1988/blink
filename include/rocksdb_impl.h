#ifndef __ROCKSDB_H__
#define __ROCKSDB_H__

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"

using namespace rocksdb;

class RocksDbImpl {
public:
    RocksDbImpl();
    ~RocksDbImpl();
    Status Put();
    Status Write();
    Status Get();
    Status Delete();
    Status Merge();
    Status NewIterator();
    Status CompactRange();
};
#endif
