#include "meta.h"

ListMeta::ListMeta() : _size(0), _addr_seq(0), _limit(LIST_ELEMENT_SIZE), _msize(0), _mlimit(LIST_META_BLOCKS) {}
ListMeta::ListMeta(std::string str)
{
    const char *p = str.data();
    _size = *(int64_t *)p;
    _addr_seq = *(int64_t *)(p + sizeof(int64_t));
    _limit = *(int64_t *)(p + sizeof(int64_t) * 2);
    _msize = *(int64_t *)(p + sizeof(int64_t) * 3);
    _mlimit = *(int64_t *)(p + sizeof(int64_t) * 4);
    str.copy((char *)_blocks, sizeof(ListMetaBlock) * LIST_META_BLOCKS, 40);
}

std::string ListMeta::toString()
{
    std::string str;
    str.append((char *)&_size, sizeof(int64_t));
    str.append((char *)&_addr_seq, sizeof(int64_t));
    str.append((char *)&_limit, sizeof(int64_t));
    str.append((char *)&_msize, sizeof(int64_t));
    str.append((char *)&_mlimit, sizeof(int64_t));
    str.append((char *)_blocks, sizeof(ListMetaBlock) * LIST_META_BLOCKS);

    return str;
}

ListMetaBlock *ListMeta::getBlock(int index) { return &_blocks[index]; }
int ListMeta::fetchSeq() { return _addr_seq++; }
int ListMeta::currentSeq() { return _addr_seq; }
