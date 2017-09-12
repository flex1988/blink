#include "meta.h"

static std::string Meta::encodeListMeta() {}
static std::string Meta::buildNewListMeta()
{
    std::string meta;
    meta.append(8, 0);
    meta.append(8, 0);
    return meta;
}
