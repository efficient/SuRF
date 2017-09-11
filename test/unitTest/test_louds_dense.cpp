#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "louds_dense.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace densetest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static const uint64_t kIntTestStart = 10;
static const int kIntTestBound = 10000000;
static const uint64_t kIntTestSkip = 10;
static std::vector<std::string> words;

class DenseUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = true;
	uint32_t sparse_dense_ratio = 0;
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal);
	truncateWordSuffixes();
	fillinInts();
    }
    virtual void TearDown () {
	delete builder_;
    }

    void truncateWordSuffixes();
    void fillinInts();

    SuRFBuilder* builder_;
    LoudsDense* louds_dense_;
    std::vector<std::string> words_trunc_;
    std::vector<std::string> ints_;
};

static int getCommonPrefixLen(const std::string &a, const std::string &b) {
    int len = 0;
    while ((len < a.length()) && (len < b.length()) && (a[len] == b[len]))
	len++;
    return len;
}

static int getMax(int a, int b) {
    if (a < b)
	return b;
    return a;
}

void DenseUnitTest::truncateWordSuffixes() {
    assert(words.size() > 1);
    
    int commonPrefixLen = 0;
    for (unsigned i = 0; i < words.size(); i++) {
	if (i == 0) {
	    commonPrefixLen = getCommonPrefixLen(words[i], words[i+1]);
	} else if (i == words.size() - 1) {
	    commonPrefixLen = getCommonPrefixLen(words[i-1], words[i]);
	} else {
	    commonPrefixLen = getMax(getCommonPrefixLen(words[i-1], words[i]),
				     getCommonPrefixLen(words[i], words[i+1]));
	}

	if (commonPrefixLen < words[i].length()) {
	    words_trunc_.push_back(words[i].substr(0, commonPrefixLen + 1));
	} else {
	    words_trunc_.push_back(words[i]);
	    words_trunc_[i] += (char)kTerminator;
	}
    }
}

void DenseUnitTest::fillinInts() {
    for (uint64_t i = kIntTestStart; i < kIntTestBound; i += kIntTestSkip) {
	ints_.push_back(surf::uint64ToString(i));
    }
}

TEST_F (DenseUnitTest, lookupTest) {
    builder_->build(words);
    louds_dense_ = new LoudsDense(builder_);
    position_t out_node_num = 0;

    for (unsigned i = 0; i < words.size(); i++) {
	bool key_exist = louds_dense_->lookupKey(words[i], out_node_num);
	ASSERT_TRUE(key_exist);
    }

    for (unsigned i = 0; i < words.size(); i++) {
	for (unsigned j = 0; j < words_trunc_[i].size() && j < words[i].size(); j++) {
	    std::string key = words[i];
	    key[j] = 'A';
	    bool key_exist = louds_dense_->lookupKey(key, out_node_num);
	    ASSERT_FALSE(key_exist);
	}
    }
}

TEST_F (DenseUnitTest, lookupIntTest) {
    builder_->build(ints_);
    louds_dense_ = new LoudsDense(builder_);
    position_t out_node_num = 0;

    for (uint64_t i = kIntTestStart; i < kIntTestBound; i += kIntTestSkip) {
	bool key_exist = louds_dense_->lookupKey(surf::uint64ToString(i), out_node_num);
	if (i % kIntTestSkip == 0) {
	    ASSERT_TRUE(key_exist);
	    ASSERT_EQ(0, out_node_num);
	} else {
	    ASSERT_FALSE(key_exist);
	    ASSERT_EQ(0, out_node_num);
	}
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

} // namespace densetest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::densetest::loadWordList();
    return RUN_ALL_TESTS();
}
