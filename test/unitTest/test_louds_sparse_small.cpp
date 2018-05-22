#include "gtest/gtest.h"

#include <assert.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "surf.hpp"

namespace surf {

namespace surftest {

static const bool kIncludeDense = false;
static const uint32_t kSparseDenseRatio = 0;
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

    SuRFBuilder* builder = new SuRFBuilder(kIncludeDense, kSparseDenseRatio, kSuffixType, 0, kSuffixLen);
    builder->build(keys);
    LoudsSparse* louds_sparse = new LoudsSparse(builder);
    LoudsSparse::Iter iter(louds_sparse);
    
    louds_sparse->moveToKeyGreaterThan(std::string("to"), true, iter);
    ASSERT_TRUE(iter.isValid());
    ASSERT_EQ(0, iter.getKey().compare("top"));
    iter++;
    ASSERT_EQ(0, iter.getKey().compare("toy"));
}

} // namespace surftest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
