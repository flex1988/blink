#include "meta.h"
#include "gtest/gtest.h"

class MetaTest : public testing::Test {
    virtual void SetUp(){};
};

TEST_F(MetaTest, LISTMETA)
{
    ListMeta *meta = new ListMeta("mykey", INIT);

    meta->IncrSize();
    meta->IncrBSize();
    meta->InsertNewMetaBlockPtr(10);
    meta->InsertNewMetaBlockPtr(100);

    std::string str = meta->ToString();

    ListMeta *meta1 = new ListMeta(str, REINIT);

    ASSERT_TRUE(*meta == *meta1);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
