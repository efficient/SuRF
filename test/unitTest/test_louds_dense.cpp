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
static const int kIntTestBound = 1000001;
static const uint64_t kIntTestSkip = 10;
static std::vector<std::string> words;

class DenseUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	truncateWordSuffixes();
	fillinInts();
    }

    void truncateWordSuffixes();
    void fillinInts();
    void testSerialize();
    void testLookupWord();

    SuRFBuilder* builder_;
    LoudsDense* louds_dense_;
    std::vector<std::string> words_trunc_;
    std::vector<std::string> ints_;
    std::string dst_;
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
    for (uint64_t i = 0; i < kIntTestBound; i += kIntTestSkip) {
	ints_.push_back(surf::uint64ToString(i));
    }
}

void DenseUnitTest::testSerialize() {
    louds_dense_->serialize(&dst_);    
    uint64_t size = extractBlockSize(dst_, 0);
    louds_dense_->destroy();
    delete louds_dense_;
    louds_dense_ = new LoudsDense();
    uint64_t offset = 0;
    LoudsDense::deSerialize(dst_, offset, louds_dense_);
}

void DenseUnitTest::testLookupWord() {
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

TEST_F (DenseUnitTest, lookupWordTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
	builder_->build(words);
	louds_dense_ = new LoudsDense(builder_);
	testLookupWord();
	delete builder_;
	louds_dense_->destroy();
	delete louds_dense_;
    }
}

TEST_F (DenseUnitTest, serializeTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
	builder_->build(words);
	louds_dense_ = new LoudsDense(builder_);

	testSerialize();
	testLookupWord();
	delete builder_;
    }
}

TEST_F (DenseUnitTest, lookupIntTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len = 8;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
    builder_->build(ints_);
    louds_dense_ = new LoudsDense(builder_);
    position_t out_node_num = 0;

    for (uint64_t i = 0; i < kIntTestBound; i += kIntTestSkip) {
	bool key_exist = louds_dense_->lookupKey(surf::uint64ToString(i), out_node_num);
	if (i % kIntTestSkip == 0) {
	    ASSERT_TRUE(key_exist);
	    ASSERT_EQ(0, out_node_num);
	} else {
	    ASSERT_FALSE(key_exist);
	    ASSERT_EQ(0, out_node_num);
	}
    }
    delete builder_;
    louds_dense_->destroy();
    delete louds_dense_;
}

TEST_F (DenseUnitTest, moveToKeyGreaterThanWordTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
	builder_->build(words);
	louds_dense_ = new LoudsDense(builder_);
	for (unsigned i = 0; i < words.size(); i++) {
	    bool inclusive = true;
	    LoudsDense::Iter iter(louds_dense_);
	    louds_dense_->moveToKeyGreaterThan(words[i], inclusive, iter);

	    ASSERT_TRUE(iter.isValid());
	    ASSERT_TRUE(iter.isComplete());
	    std::string iter_key = iter.getKey();
	    std::string word_prefix = words[i].substr(0, iter_key.length());
	    bool is_prefix = (word_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}

	for (unsigned i = 0; i < words.size() - 1; i++) {
	    bool inclusive = false;
	    LoudsDense::Iter iter(louds_dense_);
	    louds_dense_->moveToKeyGreaterThan(words[i], inclusive, iter);

	    ASSERT_TRUE(iter.isValid());
	    ASSERT_TRUE(iter.isComplete());
	    std::string iter_key = iter.getKey();
	    std::string word_prefix = words[i+1].substr(0, iter_key.length());
	    bool is_prefix = (word_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}

	bool inclusive = false;
	LoudsDense::Iter iter(louds_dense_);
	louds_dense_->moveToKeyGreaterThan(words[words.size() - 1], inclusive, iter);
	ASSERT_FALSE(iter.isValid());
	delete builder_;
	louds_dense_->destroy();
	delete louds_dense_;
    }
}

TEST_F (DenseUnitTest, moveToKeyGreaterThanIntTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len = 8;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
    builder_->build(ints_);
    louds_dense_ = new LoudsDense(builder_);
    for (uint64_t i = 0; i < kIntTestBound; i++) {
	bool inclusive = true;
	LoudsDense::Iter iter(louds_dense_);
	louds_dense_->moveToKeyGreaterThan(surf::uint64ToString(i), inclusive, iter);

	ASSERT_TRUE(iter.isValid());
	ASSERT_TRUE(iter.isComplete());
	std::string iter_key = iter.getKey();
	std::string int_key;
	if (i % kIntTestSkip == 0)
	    int_key = surf::uint64ToString(i);
	else
	    int_key = surf::uint64ToString(i - (i % kIntTestSkip) + kIntTestSkip);
	std::string int_prefix = int_key.substr(0, iter_key.length());
	bool is_prefix = (int_prefix.compare(iter_key) == 0);
	ASSERT_TRUE(is_prefix);
    }

    for (uint64_t i = 0; i < kIntTestBound - 1; i++) {
	bool inclusive = false;
	LoudsDense::Iter iter(louds_dense_);
	louds_dense_->moveToKeyGreaterThan(surf::uint64ToString(i), inclusive, iter);

	ASSERT_TRUE(iter.isValid());
	ASSERT_TRUE(iter.isComplete());
	std::string iter_key = iter.getKey();
	std::string int_key = surf::uint64ToString(i - (i % kIntTestSkip) + kIntTestSkip);
	std::string int_prefix = int_key.substr(0, iter_key.length());
	bool is_prefix = (int_prefix.compare(iter_key) == 0);
	ASSERT_TRUE(is_prefix);
    }

    bool inclusive = false;
    LoudsDense::Iter iter(louds_dense_);
    louds_dense_->moveToKeyGreaterThan(surf::uint64ToString(kIntTestBound - 1), inclusive, iter);
    ASSERT_FALSE(iter.isValid());
    delete builder_;
    louds_dense_->destroy();
    delete louds_dense_;
}

TEST_F (DenseUnitTest, IteratorIncrementWordTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len = 8;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
    builder_->build(words);
    louds_dense_ = new LoudsDense(builder_);
    bool inclusive = true;
    LoudsDense::Iter iter(louds_dense_);
    louds_dense_->moveToKeyGreaterThan(words[0], inclusive, iter);    
    for (unsigned i = 1; i < words.size(); i++) {
	iter++;
	ASSERT_TRUE(iter.isValid());
	ASSERT_TRUE(iter.isComplete());
	std::string iter_key = iter.getKey();
	std::string word_prefix = words[i].substr(0, iter_key.length());
	bool is_prefix = (word_prefix.compare(iter_key) == 0);
	ASSERT_TRUE(is_prefix);
    }
    iter++;
    ASSERT_FALSE(iter.isValid());
    delete builder_;
    louds_dense_->destroy();
    delete louds_dense_;
}

TEST_F (DenseUnitTest, IteratorIncrementIntTest) {
    bool include_dense = true;
    uint32_t sparse_dense_ratio = 0;
    level_t suffix_len = 8;
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
    builder_->build(ints_);
    louds_dense_ = new LoudsDense(builder_);
    bool inclusive = true;
    LoudsDense::Iter iter(louds_dense_);
    louds_dense_->moveToKeyGreaterThan(surf::uint64ToString(0), inclusive, iter);    
    for (uint64_t i = kIntTestSkip; i < kIntTestBound; i += kIntTestSkip) {
	iter++;
	ASSERT_TRUE(iter.isValid());
	ASSERT_TRUE(iter.isComplete());
	std::string iter_key = iter.getKey();
	std::string int_prefix = surf::uint64ToString(i).substr(0, iter_key.length());
	bool is_prefix = (int_prefix.compare(iter_key) == 0);
	ASSERT_TRUE(is_prefix);
    }
    iter++;
    ASSERT_FALSE(iter.isValid());
    delete builder_;
    louds_dense_->destroy();
    delete louds_dense_;
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
