#ifndef __COMMON__
#define __COMMON__
#include <assert.h>
#include <muduo/base/Logging.h>

#include "rocksdb/db.h"
#include "rocksdb/utilities/db_ttl.h"

#define KEY_MAX_LENGTH 255

#define LIST_META_BLOCKS 1 << 12

#define LIST_BLOCK_KEYS 1 << 12

#define LIST_ELEMENT_SIZE 1 << 23

#define MAX_LIST_NUMBER 1 << 20

#define SET_MAX_ELEMENT 1 << 26

#endif
