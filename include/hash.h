#ifndef __HASH_H__
#define __HASH_H__

#include <stdint.h>

uint64_t MurmurHash64A(const void *key, int len);
uint16_t crc16(const char *buf, int len);

#endif


