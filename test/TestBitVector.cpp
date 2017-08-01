//************************************************
// BitVector unit tests
//************************************************
#include "gtest/gtest.h"

#include "bitVector.hpp"

using namespace std;

class BitVectorUnitTest : public ::testing::Test {
public:
    virtual void SetUp () { }
    virtual void TearDown () { }
};

TEST_F(BitVectorUnitTest, EmptyTest) {
    ASSERT_TRUE(true);
}

TEST_F(BitVectorUnitTest, InitContentTest) {
    ASSERT_TRUE(true);
}

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
