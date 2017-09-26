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
    std::string val;

    for (int i = 0; i < 100000; i++) {
        s = db_->LPush("mylist", std::to_string(i), &llen);
        EXPECT_EQ(true, s.ok());
        EXPECT_EQ(llen, i + 1);
    }

    //for (int i = 0; i < 100000; i++) {
        //s = db_->LIndex("mylist", i, &val);
        //ASSERT_TRUE(s.ok());
        //EXPECT_EQ(std::to_string(99999 - i), val);
    //}
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
