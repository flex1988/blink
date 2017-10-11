#include "meta.h"
#include "gtest/gtest.h"

using namespace blink;

class MetaTest : public testing::Test {
    virtual void SetUp(){};
};

TEST_F(MetaTest, LISTMETA)
{
    ListMeta *meta = new ListMeta("mykey", INIT);

    EXPECT_EQ(meta->Key(), "mykey");
    EXPECT_EQ(meta->Type(), LIST);

    for (int i = 0; i < 1000; i++) {
        meta->IncrSize();
        meta->InsertNewMetaBlockPtr(i);
    }

    std::string str = meta->Serialize();
    ListMeta *meta1 = new ListMeta(str.substr(3), REINIT);

    ASSERT_TRUE(*meta == *meta1);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
