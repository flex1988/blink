#include "redis_db.h"
#include "gtest/gtest.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"

class RedisDBTest : public testing::Test {
   protected:
    virtual void SetUp() { db_ = std::shared_ptr<RedisDB>(new RedisDB("/tmp/db")); }
    std::shared_ptr<RedisDB> db_;
};

TEST_F(RedisDBTest, KV)
{
    rocksdb::Status s;
    std::string val;
    s = db_->Get("1234", val);
    ASSERT_FALSE(s.ok());

    s = db_->Set("1234", "5678");
    ASSERT_TRUE(s.ok());

    s = db_->Get("1234", val);
    EXPECT_EQ(val, "5678");
}

TEST_F(RedisDBTest, LIST)
{
    rocksdb::Status s;
    int64_t llen;
    int64_t rlen;
    std::string val;

    for (int i = 0; i < 200000; i++) {
        s = db_->LPush("mylist", std::to_string(i), &llen);
        EXPECT_EQ(true, s.ok());
        EXPECT_EQ(llen, i + 1);

        s = db_->LLen("mylist", &rlen);
        EXPECT_EQ(true, s.ok());
        EXPECT_EQ(llen, rlen);

        s = db_->LIndex("mylist", 0, &val);
        ASSERT_TRUE(s.ok());
        EXPECT_EQ(std::to_string(i), val);
    }

    for (int i = 199999; i >= 0; i--) {
        s = db_->LPop("mylist", val);
        ASSERT_TRUE(s.ok());
        EXPECT_EQ(std::to_string(i), val);
    }
}

TEST_F(RedisDBTest, COMPACT)
{
    int64_t llen;
    rocksdb::Status s;
    std::string val;

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 10; j++) {
            s = db_->LPush(std::to_string(i), std::to_string(j), &llen);
            EXPECT_EQ(true, s.ok());
        }
    }

    db_->CompactMeta();

    delete db_.get();

    db_ = std::shared_ptr<RedisDB>(new RedisDB("/tmp/db"));

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j >= 0; j--) {
            s = db_->LIndex(std::to_string(i), j, &val);
            EXPECT_EQ(true, s.ok());
            EXPECT_EQ(std::to_string(j), val);
        }
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
