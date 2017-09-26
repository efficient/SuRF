#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "louds_sparse.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace sparsetest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kWordTestSize = 234369;
static const uint64_t kIntTestStart = 10;
static const int kIntTestBound = 10000001;
static const uint64_t kIntTestSkip = 10; // should be less than 128
static std::vector<std::string> words;

class SparseUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = false;
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
    LoudsSparse* louds_sparse_;
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

void SparseUnitTest::truncateWordSuffixes() {
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

void SparseUnitTest::fillinInts() {
    for (uint64_t i = kIntTestStart; i < kIntTestBound; i += kIntTestSkip) {
	ints_.push_back(surf::uint64ToString(i));
    }
}

TEST_F (SparseUnitTest, lookupWordTest) {
    builder_->build(words);
    louds_sparse_ = new LoudsSparse(builder_);
    position_t in_node_num = 0;

    for (unsigned i = 0; i < words.size(); i++) {
	bool key_exist = louds_sparse_->lookupKey(words[i], in_node_num);
	ASSERT_TRUE(key_exist);
    }

    for (unsigned i = 0; i < words.size(); i++) {
	for (unsigned j = 0; j < words_trunc_[i].size() && j < words[i].size(); j++) {
	    std::string key = words[i];
	    key[j] = 'A';
	    bool key_exist = louds_sparse_->lookupKey(key, in_node_num);
	    ASSERT_FALSE(key_exist);
	}
    }
}

TEST_F (SparseUnitTest, lookupIntTest) {
    builder_->build(ints_);
    louds_sparse_ = new LoudsSparse(builder_);
    position_t in_node_num = 0;

    for (uint64_t i = kIntTestStart; i < kIntTestBound; i += kIntTestSkip) {
	bool key_exist = louds_sparse_->lookupKey(surf::uint64ToString(i), in_node_num);
	if (i % kIntTestSkip == 0)
	    ASSERT_TRUE(key_exist);
	else
	    ASSERT_FALSE(key_exist);
    }
}

TEST_F (SparseUnitTest, moveToKeyGreaterThanTest) {
    builder_->build(words);
    louds_sparse_ = new LoudsSparse(builder_);

    for (unsigned i = 0; i < words.size(); i++) {
	bool inclusive = true;
	LoudsSparse::Iter iter(louds_sparse_);
	louds_sparse_->moveToKeyGreaterThan(words[i], inclusive, iter);

	ASSERT_TRUE(iter.isValid());
	std::string iter_key = iter.getKey();
	std::string word_prefix = words[i].substr(0, iter_key.length());
	bool is_prefix = (word_prefix.compare(iter_key) == 0);
	ASSERT_TRUE(is_prefix);
    }

    for (unsigned i = 0; i < words.size() - 1; i++) {
	bool inclusive = false;
	LoudsSparse::Iter iter(louds_sparse_);
	louds_sparse_->moveToKeyGreaterThan(words[i], inclusive, iter);

	ASSERT_TRUE(iter.isValid());
	std::string iter_key = iter.getKey();
	std::string word_prefix = words[i+1].substr(0, iter_key.length());
	bool is_prefix = (word_prefix.compare(iter_key) == 0);
	ASSERT_TRUE(is_prefix);
    }

    bool inclusive = false;
    LoudsSparse::Iter iter(louds_sparse_);
    louds_sparse_->moveToKeyGreaterThan(words[words.size() - 1], inclusive, iter);
    ASSERT_FALSE(iter.isValid());
}

    /*
TEST_F (SparseUnitTest, lowerBoundIntTest) {
    builder_->build(ints_);
    louds_sparse_ = new LoudsSparse(builder_);
    position_t in_node_num = 0;
    std::string output_key;

    bool key_exist;
    for (uint64_t i = 0; i < kIntTestBound; i++) {
	key_exist = louds_sparse_->getLowerBoundKey(surf::uint64ToString(i), output_key, in_node_num);
	ASSERT_TRUE(key_exist);
	uint64_t output_int = surf::stringToUint64(output_key);
	uint64_t expected_int = (i / kIntTestSkip) * kIntTestSkip;
	if (i % kIntTestSkip)
	    expected_int += kIntTestSkip;
	ASSERT_EQ(expected_int, output_int);
    }

    key_exist = louds_sparse_->getLowerBoundKey(surf::uint64ToString(kIntTestBound), output_key, in_node_num);
    ASSERT_FALSE(key_exist);
}
    */
void loadWordList() {
    std::ifstream infile(kFilePath);
    std::string key;
    int count = 0;
    while (infile.good() && count < kWordTestSize) {
	infile >> key;
	words.push_back(key);
	count++;
    }
}

} // namespace sparsetest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::sparsetest::loadWordList();
    return RUN_ALL_TESTS();
}
