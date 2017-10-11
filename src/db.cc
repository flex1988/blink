#include "db.h"
#include "common.h"
#include "list.h"
#include "meta.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <fstream>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

namespace blink {

static ssize_t ReadSync(int fd, char* buffer, ssize_t nread)
{
    ssize_t n;
    ssize_t last = nread;
    for (;;) {
        if (last == 0)
            return nread;
        n = read(fd, buffer, last);
        if (n < 0) {
            return n;
        }
        else if (n == 0) {
            return nread - last;
        }
        else {
            last -= n;
            buffer += n;
        }
    }
}

static ssize_t ReadUntilChar(int fd, char* buffer, char target)
{
    ssize_t nread = 0;
    ssize_t n;
    for (;;) {
        n = read(fd, buffer, 1);

        if (n == -1 || n == 0)
            return n;

        nread++;

        if (buffer[0] == target) {
            return nread;
        }
    }
}

static ssize_t ReadLineSync(int fd, char* buffer, ssize_t size)
{
    ssize_t nread = 0;
    int ret;

    size--;
    while (size) {
        char c;

        ret = ReadSync(fd, &c, 1);
        if (ret == -1 || ret == 0)
            return ret;

        if (c == '\n') {
            *buffer = '\0';
            if (nread && *(buffer - 1) == '\r')
                *(buffer - 1) = '\0';
            return nread;
        }
        else {
            *buffer++ = c;
            *buffer = '\0';
            nread++;
        }
    }

    return nread;
}

RedisDB::RedisDB(const std::string& path) : metaqueue_(), path_(path), meta_log_size_(0)
{
    options_.create_if_missing = true;
    options_.compression = rocksdb::CompressionType::kNoCompression;
    options_.disable_auto_compactions = true;
    options_.max_background_compactions = 1;
    options_.max_background_flushes = 1;
    options_.max_open_files = 5000;
    options_.max_manifest_file_size = 64 * 1024 * 1024;
    options_.max_log_file_size = 512 * 1024 * 1024;
    options_.keep_log_file_num = 10;
    options_.target_file_size_base = 2 * 1024 * 1024;
    options_.write_buffer_size = 256 * 1024 * 1024;
    options_.target_file_size_multiplier = 1;

    options_.max_write_buffer_number = 5;
    options_.min_write_buffer_number_to_merge = 2;

    lop_ = std::shared_ptr<ListMetaOperator>(new ListMetaOperator(&memmeta_));

    rocksdb::DBWithTTL* db;

    rocksdb::Status s = rocksdb::DBWithTTL::Open(options_, path_ + "/kv", &db);
    if (!s.ok())
        LOG_INFO << s.getState();
    assert(s.ok());

    kv_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    s = rocksdb::DBWithTTL::Open(options_, path_ + "/list", &db);
    if (!s.ok())
        LOG_INFO << s.getState();
    assert(s.ok());

    list_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    s = rocksdb::DBWithTTL::Open(options_, path_ + "/set", &db);
    if (!s.ok())
        LOG_INFO << s.getState();
    assert(s.ok());

    set_ = std::unique_ptr<rocksdb::DBWithTTL>(db);

    msnap_ = path_ + "/meta.snapshot";
    mnewsnap_ = path_ + "/meta.snapshot.new";
    aof_ = path_ + "/meta.appendonly";

    LOG_INFO << "Server now is ready to accept connection";
    LOG_INFO << "DB path: " + path_ + "/meta";

    LoadMeta();

    aof_fd_ = ::open(std::string(path + "/meta.appendonly").data(), O_CREAT | O_WRONLY | O_APPEND);
}

RedisDB::~RedisDB()
{ // Close();
}

void RedisDB::AppendMeta()
{
    while (1) {
        std::string meta = metaqueue_.pop();
        size_t w = ::write(aof_fd_, meta.data(), meta.size());
        assert(w = meta.size());
        meta_log_size_ += w;
    }
}
void RedisDB::Close()
{
    delete kv_.get();
    delete list_.get();
    delete set_.get();
}

void RedisDB::LoadMeta()
{
    LOG_INFO << "Load meta info form disk file path: " + path_ + "/meta";

    ReloadAof();

    LoadMetaSnapshot();

    LoadMetaAppendonly();

    ReloadAofDone();
}

void RedisDB::LoadMetaSnapshot()
{
    int fd = ::open(msnap_.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_INFO << "open meta log not exists!";
        return;
    }

    char buffer[65536];
    uint16_t len;

    ssize_t ret;

    for (;;) {
        ret = ReadUntilChar(fd, buffer, META_SNAP_MAGIC);
        if (ret == 0 || ret == -1)
            goto end;

        ret = ReadSync(fd, buffer, 2);
        if (ret == 0 || ret == -1 || ret < 2)
            goto end;

        len = *(uint16_t*)(buffer);

        ret = ReadSync(fd, buffer, len);
        if (ret == 0 || ret == -1 || ret < len)
            goto end;

        if (buffer[len - 2] != '\r' || buffer[len - 1] != '\n')
            goto end;

        switch (buffer[0]) {
        case 'L': {
            uint8_t klen = buffer[1];
            std::string key = std::string(buffer + 2, klen);
            LOG_DEBUG << "load list meta: " << key;
            std::string metakey = EncodeListMetaKey(key);
            memmeta_[metakey] = std::shared_ptr<MetaBase>(new ListMeta(std::string(buffer, len - 2), REINIT));
            break;
        }
        case 'B': {
            uint8_t klen = buffer[1];
            std::string key = std::string(buffer + 2, klen);
            int64_t addr = *(int64_t*)(buffer + 2 + klen);
            LOG_DEBUG << "load list meta block: " << key;
            std::string blockkey = EncodeListBlockKey(key, addr);
            memmeta_[blockkey] = std::shared_ptr<MetaBase>(new ListMetaBlock(std::string(buffer, len - 2)));
            break;
        }
        default:
            LOG_INFO << "meta snapshot corruption is not allowed";
            assert(0);
            break;
        }
    }

    LOG_INFO << "load meta snapshot success";
    return;

end:
    if (ret == 0) {
        LOG_INFO << "load meta snap from disk success";
        return;
    }

    if (ret == -1) {
        LOG_INFO << "read meta snap failed " << std::string(strerror(errno));
        return;
    }

    LOG_INFO << "meta snap corruption";
    return;
}

void RedisDB::LoadMetaAppendonly()
{
    int fd = ::open(aof_.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_INFO << "open meta appendonly not exists!";
        return;
    }

    size_t buflen = 1024;
    size_t len;
    int64_t bulks;
    int64_t bulk_length;
    std::vector<std::string> argv;

    char* buf = (char*)std::malloc(buflen);

    while (1) {
        len = ReadLineSync(fd, buf, buflen);
        if (len == -1 || len == 0 || buf[0] != '*')
            goto error;

        bulks = std::atoi(buf + 1);

        LOG_DEBUG << "bulks number: " << bulks;

        while (bulks-- > 0) {
            len = ReadLineSync(fd, buf, buflen);
            if (len == -1 || len == 0 || buf[0] != '$')
                goto error;

            bulk_length = std::atoi(buf + 1);

            if (bulk_length + 2 > buflen) {
                buflen = bulk_length + 2;
                buf = (char*)std::realloc(buf, buflen);
            }

            len = ReadSync(fd, buf, bulk_length + 2);
            if (len == -1 || len == 0 || len < bulk_length + 2)
                goto error;

            argv.push_back(std::string(buf, bulk_length));
        }

        ReloadAppendOnlyCommand(argv);

        argv.clear();
    }

error:
    if (len == 0) {
        LOG_INFO << "load meta log from disk success";
        return;
    }

    if (len == -1) {
        LOG_INFO << "read meta log failed " << std::string(strerror(errno));
        return;
    }

    LOG_INFO << "may lose data";
    return;
}

void RedisDB::CompactMeta()
{
    if (forbid_compact_)
        return;

    LOG_INFO << "Dump meta to disk!";

    // uint64_t truncate_size = meta_log_size_;
    meta_log_size_ = 0;

    int ret;
    size_t nwrite;
    int snapfd = ::open(mnewsnap_.c_str(), O_CREAT | O_WRONLY | O_TRUNC);

    for (auto i = memmeta_.begin(); i != memmeta_.end(); ++i) {
        std::string str = i->second->Serialize();
        LOG_DEBUG << "compact: " << str;
        nwrite = ::write(snapfd, str.c_str(), str.size());
        assert(nwrite == str.size());
    }

    fdatasync(snapfd);

    unlink(msnap_.c_str());
    rename(mnewsnap_.c_str(), msnap_.c_str());
    unlink(mnewsnap_.c_str());

    fsync(aof_fd_);
    ret = ftruncate(aof_fd_, 0);

    if (ret == -1) {
        LOG_INFO << std::string(strerror(errno));
        assert(0);
    }
}

void RedisDB::ReloadListActionBuffer(char* buf, size_t blen)
{
    size_t index = 0;
    std::shared_ptr<ListMeta> meta;

    while (index < blen) {
        int16_t action = *(int16_t*)(buf + index);
        int16_t op = *(int16_t*)(buf + index + 2);

        index += 4;

        switch (action) {
        case INIT: {
            if (op < 0 || op > (blen - index)) {
                LOG_INFO << "meta log corruption";
                return;
            }
            std::string key = std::string(buf + index, op);
            std::string metakey = EncodeListMetaKey(key);
            meta = std::shared_ptr<ListMeta>(new ListMeta(key, INIT));
            memmeta_[metakey] = std::dynamic_pointer_cast<MetaBase>(meta);
            index += op;
            break;
        }
        case REINIT: {
            if (op < 0 || op > (blen - index)) {
                LOG_INFO << "meta log corruption";
                return;
            }
            std::string key = std::string(buf + index, op);
            std::string metakey = EncodeListMetaKey(key);
            assert(memmeta_.find(metakey) != memmeta_.end());
            meta = std::dynamic_pointer_cast<ListMeta>(memmeta_[metakey]);
            index += op;
            break;
        }
        case SIZE:
            // meta->SetSize(op);
            break;
        case ALLOC:
            // meta->SetArea(op);
            break;
        case BSIZE:
            // meta->SetBSize(op);
            break;
        case INSERT:
            meta->InsertMetaBlock(op);
            break;

        default:
            LOG_INFO << "unknown action: " << action;
            return;
        }
    }
}

void RedisDB::ReloadAppendOnlyCommand(const std::vector<std::string>& argv)
{
    if (argv[0] == "lpush") {
        LOG_DEBUG << "reload lpush: " << argv[1];

        int64_t size;
        LPush(argv[1], argv[2], &size);
    }
    else if (argv[0] == "lpop") {
        std::string val;
        LPop(argv[1], val);
    }
    else {
        LOG_INFO << "reload appendonly corruption!";
    }
}
}
