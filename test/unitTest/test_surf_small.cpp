#include "gtest/gtest.h"

#include <assert.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "surf.hpp"

namespace surf {

namespace surftest {

static const SuffixType kSuffixType = kReal;
static const level_t kSuffixLen = 8;

class SuRFSmallTest : public ::testing::Test {
public:
    virtual void SetUp () {}
    virtual void TearDown () {}
};

TEST_F (SuRFSmallTest, ExampleInPaperTest) {
    std::vector<std::string> keys;

    keys.push_back(std::string("f"));
    keys.push_back(std::string("far"));
    keys.push_back(std::string("fas"));
    keys.push_back(std::string("fast"));
    keys.push_back(std::string("fat"));
    keys.push_back(std::string("s"));
    keys.push_back(std::string("top"));
    keys.push_back(std::string("toy"));
    keys.push_back(std::string("trie"));
    keys.push_back(std::string("trip"));
    keys.push_back(std::string("try"));

    SuRF* surf = new SuRF(keys, kIncludeDense, kSparseDenseRatio, kSuffixType, 0, kSuffixLen);
    bool exist = surf->lookupRange(std::string("top"), false, std::string("toyy"), false);
    ASSERT_TRUE(exist);
    exist = surf->lookupRange(std::string("toq"), false, std::string("toyy"), false);
    ASSERT_TRUE(exist);
    exist = surf->lookupRange(std::string("trie"), false, std::string("tripp"), false);
    ASSERT_TRUE(exist);
}

} // namespace surftest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
