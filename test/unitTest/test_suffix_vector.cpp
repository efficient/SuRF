#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "suffix_vector.hpp"
#include "surf_builder.hpp"

namespace surf {

// DEPRECATED
namespace suffixvectortest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class SuffixVectorUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	;
    }
    virtual void TearDown () {
	delete builder_;
	delete suffixes_;
    }

    SuRFBuilder* builder_;
    SuffixVector* suffixes_;
};

TEST_F (SuffixVectorUnitTest, buildNoneTest) {
    bool include_dense = false;
    uint32_t sparse_dense_ratio = 0;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kNone);
    builder_->build(words);
    suffixes_ = new SuffixVector(kNone, builder_->getSuffixes());
}

TEST_F (SuffixVectorUnitTest, buildHashTest) {
    bool include_dense = false;
    uint32_t sparse_dense_ratio = 0;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kHash);
    builder_->build(words);
    suffixes_ = new SuffixVector(kHash, builder_->getSuffixes());
}

TEST_F (SuffixVectorUnitTest, buildRealTest) {
    bool include_dense = false;
    uint32_t sparse_dense_ratio = 0;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal);
    builder_->build(words);
    suffixes_ = new SuffixVector(kReal, builder_->getSuffixes());
}

//TODO checkEqualityTest
//TODO compareTest

void loadWordList() {
    std::ifstream infile(kFilePath);
    std::string key;
    int count = 0;
    while (infile.good() && count < kTestSize) {
	infile >> key;
	words.push_back(key);
	count++;
    }
}

} // namespace suffixvectortest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::suffixvectortest::loadWordList();
    return RUN_ALL_TESTS();
}
