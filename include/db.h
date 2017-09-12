#ifndef __DB_H__
#define __DB_H__

#include <string>

class Status;
struct Options;

class DB {
public:
    DB() {}
    virtual ~DB();
    virtual Status Put() = 0;
    virtual Status Write() = 0;
    virtual Status Get() = 0;
    virtual Status Delete() = 0;
    virtual Status Merge() = 0;
    virtual Status NewIterator() = 0;
    virtual Status CompactRange() = 0;

private:
    std::string _path;
    Options* _options;
};
#endif
