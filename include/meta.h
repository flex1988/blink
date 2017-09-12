#ifndef __META_H__
#define __META_H__

#include "common.h"

struct ListMetaItem {
    int32_t addr;  // meta item address
    int32_t size;   // meta item contain keys
};

struct ListMeta {
    int64_t size;           // total element number
    int64_t addr_seq;            // current max element sequence+1
    int64_t limit;          // maxmum element number
    int64_t msize;          // total meta item size
    int64_t mlimit;         // maxmum meta item number
    ListMetaItem items[0];  // meta items array
};

struct ListMetaIndex {
    int64_t addr[4096];
};

class Meta {
public:
    static std::string EncodeListMeta();

private:
    Meta();
};
#endif
