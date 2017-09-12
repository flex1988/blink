#ifndef __COMMON__
#define __COMMON__
#include <assert.h>
#include <muduo/base/Logging.h>

const uint32_t KEY_MAX_LENGTH = 255;

const uint32_t LIST_META_SIZE = 1 >> 10;

const uint32_t LIST_BLOCK_SIZE = 1 >> 10;

const uint32_t LIST_ELEMENT_SIZE = 1 >> 20;
#endif
