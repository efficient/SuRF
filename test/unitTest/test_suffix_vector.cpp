#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "suffix_vector.hpp"
#include "surf_builder.hpp"

namespace surf {

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

TEST_F (SuffixVectorUnitTest, checkEqualityHashTest) {
    bool include_dense = false;
    builder_ = new SuRFBuilder(include_dense, kHash);
    builder_->build(words);
    suffixes_ = new SuffixVector(kHash, builder_->getSuffixes());

    position_t pos = 0;
    for (unsigned i = 0; i < words.size(); i++) {
	bool suffix_match = suffixes_->checkEquality(pos, words[i]);
	ASSERT_TRUE(suffix_match);
    }
}

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
