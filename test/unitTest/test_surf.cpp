#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "surf.hpp"

namespace surf {

namespace surftest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kWordTestSize = 234369;
static const uint64_t kIntTestStart = 10;
static const int kIntTestBound = 1000001;
static const uint64_t kIntTestSkip = 10;
static const SuffixType kSuffixType = kReal;
static std::vector<std::string> words;

class SuRFUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	truncateWordSuffixes();
	fillinInts();
	data_ = nullptr;
    }
    virtual void TearDown () {
	if (data_)
	    delete[] data_;
    }

    void truncateWordSuffixes();
    void fillinInts();
    void testSerialize();
    void testLookupWord();

    SuRF* surf_;
    std::vector<std::string> words_trunc_;
    std::vector<std::string> ints_;
    char* data_;
};

static int getCommonPrefixLen(const std::string &a, const std::string &b) {
    int len = 0;
    while ((len < (int)a.length()) && (len < (int)b.length()) && (a[len] == b[len]))
	len++;
    return len;
}

static int getMax(int a, int b) {
    if (a < b)
	return b;
    return a;
}

void SuRFUnitTest::truncateWordSuffixes() {
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

	if (commonPrefixLen < (int)words[i].length()) {
	    words_trunc_.push_back(words[i].substr(0, commonPrefixLen + 1));
	} else {
	    words_trunc_.push_back(words[i]);
	    words_trunc_[i] += (char)kTerminator;
	}
    }
}

void SuRFUnitTest::fillinInts() {
    for (uint64_t i = 0; i < kIntTestBound; i += kIntTestSkip) {
	ints_.push_back(surf::uint64ToString(i));
    }
}

void SuRFUnitTest::testSerialize() {
    data_ = surf_->serialize();
    surf_->destroy();
    delete surf_;
    char* data = data_;
    surf_ = SuRF::deSerialize(data);
}

void SuRFUnitTest::testLookupWord() {
    for (unsigned i = 0; i < words.size(); i++) {
	bool key_exist = surf_->lookupKey(words[i]);
	ASSERT_TRUE(key_exist);
    }

    if (kSuffixType == kNone) return;

    for (unsigned i = 0; i < words.size(); i++) {
	for (unsigned j = 0; j < words_trunc_[i].size() && j < words[i].size(); j++) {
	    std::string key = words[i];
	    key[j] = 'A';
	    bool key_exist = surf_->lookupKey(key);
	    ASSERT_FALSE(key_exist);
	}
    }
}

TEST_F (SuRFUnitTest, IntStringConvertTest) {
    for (uint64_t i = 0; i < kIntTestBound; i++) {
	ASSERT_EQ(i, stringToUint64(uint64ToString(i)));
    }
}
    
TEST_F (SuRFUnitTest, lookupWordTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	testLookupWord();
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, serializeTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	testSerialize();
	testLookupWord();
    }
}

TEST_F (SuRFUnitTest, lookupIntTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	for (uint64_t i = 0; i < kIntTestBound; i += kIntTestSkip) {
	    bool key_exist = surf_->lookupKey(surf::uint64ToString(i));
	    if (i % kIntTestSkip == 0)
		ASSERT_TRUE(key_exist);
	    else
		ASSERT_FALSE(key_exist);
	}
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, moveToKeyGreaterThanWordTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	for (unsigned i = 0; i < words.size(); i++) {
	    bool inclusive = true;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(words[i], inclusive);

	    ASSERT_TRUE(iter.isValid());
	    std::string iter_key;
	    if (suffix_len % 8 == 0)
		iter_key = iter.getKeyWithSuffix();
	    else
		iter_key = iter.getKey();
	    std::string word_prefix = words[i].substr(0, iter_key.length());
	    bool is_prefix = (word_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}

	for (unsigned i = 0; i < words.size() - 1; i++) {
	    bool inclusive = false;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(words[i], inclusive);

	    ASSERT_TRUE(iter.isValid());
	    std::string iter_key;
	    if (suffix_len % 8 == 0)
		iter_key = iter.getKeyWithSuffix();
	    else
		iter_key = iter.getKey();
	    std::string word_prefix = words[i+1].substr(0, iter_key.length());
	    bool is_prefix = (word_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}

	bool inclusive = false;
	SuRF::Iter iter = surf_->moveToKeyGreaterThan(words[words.size() - 1], inclusive);
	ASSERT_FALSE(iter.isValid());
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, moveToKeyGreaterThanIntTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	for (uint64_t i = 0; i < kIntTestBound; i++) {
	    bool inclusive = true;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(surf::uint64ToString(i), inclusive);

	    ASSERT_TRUE(iter.isValid());
	    std::string iter_key;
	    if (suffix_len % 8 == 0)
		iter_key = iter.getKeyWithSuffix();
	    else
		iter_key = iter.getKey();
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
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(surf::uint64ToString(i), inclusive);

	    ASSERT_TRUE(iter.isValid());
	    std::string iter_key;
	    if (suffix_len % 8 == 0)
		iter_key = iter.getKeyWithSuffix();
	    else
		iter_key = iter.getKey();
	    std::string int_key = surf::uint64ToString(i - (i % kIntTestSkip) + kIntTestSkip);
	    std::string int_prefix = int_key.substr(0, iter_key.length());
	    bool is_prefix = (int_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}

	bool inclusive = false;
	SuRF::Iter iter = surf_->moveToKeyGreaterThan(surf::uint64ToString(kIntTestBound - 1), inclusive);
	ASSERT_FALSE(iter.isValid());
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, IteratorIncrementWordTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	bool inclusive = true;
	SuRF::Iter iter = surf_->moveToKeyGreaterThan(words[0], inclusive);
	for (unsigned i = 1; i < words.size(); i++) {
	    iter++;
	    ASSERT_TRUE(iter.isValid());
	    std::string iter_key;
	    if (suffix_len % 8 == 0)
		iter_key = iter.getKeyWithSuffix();
	    else
		iter_key = iter.getKey();
	    std::string word_prefix = words[i].substr(0, iter_key.length());
	    bool is_prefix = (word_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}
	iter++;
	ASSERT_FALSE(iter.isValid());
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, IteratorIncrementIntTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	bool inclusive = true;
	SuRF::Iter iter = surf_->moveToKeyGreaterThan(surf::uint64ToString(0), inclusive);
	for (uint64_t i = kIntTestSkip; i < kIntTestBound; i += kIntTestSkip) {
	    iter++;
	    ASSERT_TRUE(iter.isValid());
	    std::string iter_key;
	    if (suffix_len % 8 == 0)
		iter_key = iter.getKeyWithSuffix();
	    else
		iter_key = iter.getKey();
	    std::string int_prefix = surf::uint64ToString(i).substr(0, iter_key.length());
	    bool is_prefix = (int_prefix.compare(iter_key) == 0);
	    ASSERT_TRUE(is_prefix);
	}
	iter++;
	ASSERT_FALSE(iter.isValid());
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, lookupRangeWordTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(words, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);

	bool exist = surf_->lookupRange(std::string("\1"), true, words[0], true);
	ASSERT_TRUE(exist);
	exist = surf_->lookupRange(std::string("\1"), true, words[0], false);
	ASSERT_FALSE(exist);

	for (unsigned i = 0; i < words.size() - 1; i++) {
	    exist = surf_->lookupRange(words[i], true, words[i+1], true);
	    ASSERT_TRUE(exist);
	    exist = surf_->lookupRange(words[i], true, words[i+1], false);
	    ASSERT_TRUE(exist);
	    exist = surf_->lookupRange(words[i], false, words[i+1], true);
	    ASSERT_TRUE(exist);
	    exist = surf_->lookupRange(words[i], false, words[i+1], false);
	    ASSERT_FALSE(exist);
	}

	exist = surf_->lookupRange(words[words.size() - 1], true, std::string("zzzzzzzz"), false);
	ASSERT_TRUE(exist);
	exist = surf_->lookupRange(words[words.size() - 1], false, std::string("zzzzzzzz"), false);
	ASSERT_FALSE(exist);

    }    
}

TEST_F (SuRFUnitTest, lookupRangeIntTest) {
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int k = 0; k < 5; k++) {
	level_t suffix_len = suffix_len_array[k];
	surf_ = new SuRF(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	//surf_ = new SuRF();
	//surf_->create(ints_, kIncludeDense, kSparseDenseRatio, kSuffixType, suffix_len);
	for (uint64_t i = 0; i < kIntTestBound; i++) {
	    bool exist = surf_->lookupRange(surf::uint64ToString(i), true, 
					    surf::uint64ToString(i), true);
	    if (i % kIntTestSkip == 0)
		ASSERT_TRUE(exist);
	    else
		ASSERT_FALSE(exist);

	    for (unsigned j = 1; j < kIntTestSkip + 2; j++) {
		exist = surf_->lookupRange(surf::uint64ToString(i), false, 
					   surf::uint64ToString(i + j), true);
		uint64_t left_bound_interval_id = i / kIntTestSkip;
		uint64_t right_bound_interval_id = (i + j) / kIntTestSkip;
		if ((i < kIntTestBound - 1) && ((left_bound_interval_id < right_bound_interval_id)
						|| ((i + j) % kIntTestSkip == 0)))
		    ASSERT_TRUE(exist);
		else
		    ASSERT_FALSE(exist);
	    }
	}	
    }
}

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

} // namespace surftest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::surftest::loadWordList();
    return RUN_ALL_TESTS();
}
