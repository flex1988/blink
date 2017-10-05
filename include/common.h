#ifndef __COMMON__
#define __COMMON__
#include <assert.h>
#include <muduo/base/Logging.h>

#include <cstring>

#include "rocksdb/db.h"
#include "rocksdb/utilities/db_ttl.h"

#define KEY_MAX_LENGTH 255

const uint16_t LIST_META_BLOCKS = 1 << 12;

const uint16_t LIST_BLOCK_KEYS = 1 << 12;

#define LIST_ELEMENT_SIZE 1 << 23

#define MAX_LIST_NUMBER 1 << 20

#define SET_MAX_ELEMENT 1 << 26

#define ACTION_BUFFER_MAGIC 0x78

const uint16_t META_SNAP_MAGIC = 0x56;

const int16_t PROPAGATE_AOF = 1 << 0;

const int32_t CONN_CLOSE_AFTER_REPLY = 1 << 6;

const int32_t PROTO_INLINE_MAX_SIZE = 1024 * 64;
const int32_t PROTO_MBULK_BIG_ARG = 1024 * 32;

#endif
