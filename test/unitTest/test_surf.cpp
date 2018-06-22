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
static const int kNumSuffixType = 4;
static const SuffixType kSuffixTypeList[kNumSuffixType] = {kNone, kHash, kReal, kMixed};
static const int kNumSuffixLen = 6;
static const level_t kSuffixLenList[kNumSuffixLen] = {1, 3, 7, 8, 13, 26};
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

    void newSuRFWords(SuffixType suffix_type, level_t suffix_len);
    void newSuRFInts(SuffixType suffix_type, level_t suffix_len);
    void truncateWordSuffixes();
    void fillinInts();
    void testSerialize();
    void testLookupWord(SuffixType suffix_type);

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

static bool isEqual(const std::string& a, const std::string& b, const unsigned bitlen) {
    if (bitlen == 0) {
	return (a.compare(b) == 0);
    } else {
	std::string a_prefix = a.substr(0, a.length() - 1);
	std::string b_prefix = b.substr(0, b.length() - 1);
	if (a_prefix.compare(b_prefix) != 0) return false;
	char mask = 0xFF << (8 - bitlen);
	char a_suf = a[a.length() - 1] & mask;
	char b_suf = b[b.length() - 1] & mask;
	return (a_suf == b_suf);
    }
}

void SuRFUnitTest::newSuRFWords(SuffixType suffix_type, level_t suffix_len) {
    if (suffix_type == kNone)
        surf_ = new SuRF(words);
    else if (suffix_type == kHash)
        surf_ = new SuRF(words, kHash, suffix_len, 0);
    else if (suffix_type == kReal)
        surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kReal, 0, suffix_len);
    else if (suffix_type == kMixed)
        surf_ = new SuRF(words, kMixed, suffix_len, suffix_len);
    else
	surf_ = new SuRF(words);
}

void SuRFUnitTest::newSuRFInts(SuffixType suffix_type, level_t suffix_len) {
    if (suffix_type == kNone)
        surf_ = new SuRF(ints_);
    else if (suffix_type == kHash)
        surf_ = new SuRF(ints_, kHash, suffix_len, 0);
    else if (suffix_type == kReal)
        surf_ = new SuRF(ints_, kIncludeDense, kSparseDenseRatio, kReal, 0, suffix_len);
    else if (suffix_type == kMixed)
        surf_ = new SuRF(ints_, kMixed, suffix_len, suffix_len);
    else
	surf_ = new SuRF(ints_);
}

void SuRFUnitTest::truncateWordSuffixes() {
    assert(words.size() > 1);
    int commonPrefixLen = 0;
    for (unsigned i = 0; i < words.size(); i++) {
	if (i == 0)
	    commonPrefixLen = getCommonPrefixLen(words[i], words[i+1]);
	else if (i == words.size() - 1)
	    commonPrefixLen = getCommonPrefixLen(words[i-1], words[i]);
	else
	    commonPrefixLen = getMax(getCommonPrefixLen(words[i-1], words[i]),
				     getCommonPrefixLen(words[i], words[i+1]));

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
	ints_.push_back(uint64ToString(i));
    }
}

void SuRFUnitTest::testSerialize() {
    data_ = surf_->serialize();
    surf_->destroy();
    delete surf_;
    char* data = data_;
    surf_ = SuRF::deSerialize(data);
}

void SuRFUnitTest::testLookupWord(SuffixType suffix_type) {
    for (unsigned i = 0; i < words.size(); i++) {
	bool key_exist = surf_->lookupKey(words[i]);
	ASSERT_TRUE(key_exist);
    }

    if (suffix_type == kNone) return;

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
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFWords(kSuffixTypeList[t], kSuffixLenList[k]);
	    testLookupWord(kSuffixTypeList[t]);
	    surf_->destroy();
	    delete surf_;
	}
    }
}

TEST_F (SuRFUnitTest, serializeTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFWords(kSuffixTypeList[t], kSuffixLenList[k]);
	    testSerialize();
	    testLookupWord(kSuffixTypeList[t]);
	}
    }
}

TEST_F (SuRFUnitTest, lookupIntTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFInts(kSuffixTypeList[t], kSuffixLenList[k]);
	    for (uint64_t i = 0; i < kIntTestBound; i += kIntTestSkip) {
		bool key_exist = surf_->lookupKey(uint64ToString(i));
		if (i % kIntTestSkip == 0)
		    ASSERT_TRUE(key_exist);
		else
		    ASSERT_FALSE(key_exist);
	    }
	    surf_->destroy();
	    delete surf_;
	}
    }
}

TEST_F (SuRFUnitTest, moveToKeyGreaterThanWordTest) {
    for (int t = 2; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFWords(kSuffixTypeList[t], kSuffixLenList[k]);
	    bool inclusive = true;
	    for (int i = 0; i < 2; i++) {
		if (i == 1)
		    inclusive = false;
		for (int j = -1; j <= (int)words.size(); j++) {
		    SuRF::Iter iter;
		    if (j < 0)
			iter = surf_->moveToFirst();
		    else if (j >= (int)words.size())
			iter = surf_->moveToLast();
		    else
			iter = surf_->moveToKeyGreaterThan(words[j], inclusive);

		    unsigned bitlen;
		    bool is_prefix = false;
		    if (j < 0) {
			ASSERT_TRUE(iter.isValid());
			std::string iter_key = iter.getKeyWithSuffix(&bitlen);
			std::string word_prefix = words[0].substr(0, iter_key.length());
			is_prefix = isEqual(word_prefix, iter_key, bitlen);
			ASSERT_TRUE(is_prefix);
		    } else if (j >= (int)words.size()) {
			ASSERT_TRUE(iter.isValid());
			std::string iter_key = iter.getKeyWithSuffix(&bitlen);
			std::string word_prefix = words[words.size() - 1].substr(0, iter_key.length());
			is_prefix = isEqual(word_prefix, iter_key, bitlen);
			ASSERT_TRUE(is_prefix);
		    } else if (j == (int)words.size() - 1) {
			if (iter.getFpFlag()) {
			    ASSERT_TRUE(iter.isValid());
			    std::string iter_key = iter.getKeyWithSuffix(&bitlen);
			    std::string word_prefix = words[words.size() - 1].substr(0, iter_key.length());
			    is_prefix = isEqual(word_prefix, iter_key, bitlen);
			    ASSERT_TRUE(is_prefix);
			} else {
			    ASSERT_FALSE(iter.isValid());
			}
		    } else {
			ASSERT_TRUE(iter.isValid());
			std::string iter_key = iter.getKeyWithSuffix(&bitlen);
			std::string word_prefix_fp = words[j].substr(0, iter_key.length());
			std::string word_prefix_true = words[j+1].substr(0, iter_key.length());
			if (iter.getFpFlag())
			    is_prefix = isEqual(word_prefix_fp, iter_key, bitlen);
			else
			    is_prefix = isEqual(word_prefix_true, iter_key, bitlen);
			ASSERT_TRUE(is_prefix);

			// test getKey()
			std::string iter_get_key = iter.getKey();
			std::string iter_key_prefix = iter_key.substr(0, iter_get_key.length());
			is_prefix = (iter_key_prefix.compare(iter_get_key) == 0);
			ASSERT_TRUE(is_prefix);

			// test getSuffix()
			if (kSuffixTypeList[t] == kReal || kSuffixTypeList[t] == kMixed) {
			    word_t iter_suffix = 0;
			    int iter_suffix_len = iter.getSuffix(&iter_suffix);
			    ASSERT_EQ(kSuffixLenList[k], iter_suffix_len);
			    std::string iter_key_suffix_str
				= iter_key.substr(iter_get_key.length(), iter_key.length());
			    word_t iter_key_suffix = 0;
			    int suffix_len = (int)kSuffixLenList[k];
			    int suffix_str_len = (int)(iter_key.length() - iter_get_key.length());
			    level_t pos = 0;
			    while (suffix_len > 0 && suffix_str_len > 0) {
				iter_key_suffix += (word_t)iter_key_suffix_str[pos];
				iter_key_suffix <<= 8;
				suffix_len -= 8;
				suffix_str_len--;
				pos++;
			    }
			    if (pos > 0) {
				iter_key_suffix >>= 8;
				if (kSuffixLenList[k] % 8 != 0)
				    iter_key_suffix >>= (8 - (kSuffixLenList[k] % 8));
			    }
			    ASSERT_EQ(iter_key_suffix, iter_suffix);
			}
		    }
		}
	    }
	    surf_->destroy();
	    delete surf_;
	}
    }
}


TEST_F (SuRFUnitTest, moveToKeyGreaterThanIntTest) {
    for (int k = 0; k < kNumSuffixLen; k++) {
	newSuRFInts(kMixed, kSuffixLenList[k]);
	bool inclusive = true;
	for (int i = 0; i < 2; i++) {
	    if (i == 1)
		inclusive = false;
	    for (uint64_t j = 0; j < kIntTestBound - 1; j++) {
		SuRF::Iter iter = surf_->moveToKeyGreaterThan(uint64ToString(j), inclusive);

		ASSERT_TRUE(iter.isValid());
		unsigned bitlen;
		std::string iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string int_key_fp = uint64ToString(j - (j % kIntTestSkip));
		std::string int_key_true = uint64ToString(j - (j % kIntTestSkip) + kIntTestSkip);
		std::string int_prefix_fp = int_key_fp.substr(0, iter_key.length());
		std::string int_prefix_true = int_key_true.substr(0, iter_key.length());
		bool is_prefix = false;
		if (iter.getFpFlag())
		    is_prefix = isEqual(int_prefix_fp, iter_key, bitlen);
		else
		    is_prefix = isEqual(int_prefix_true, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }

	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(uint64ToString(kIntTestBound - 1), inclusive);
	    if (iter.getFpFlag()) {
		ASSERT_TRUE(iter.isValid());
		unsigned bitlen;
		std::string iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string int_key_fp = uint64ToString(kIntTestBound - 1);
		std::string int_prefix_fp = int_key_fp.substr(0, iter_key.length());
		bool is_prefix = isEqual(int_prefix_fp, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    } else {
		ASSERT_FALSE(iter.isValid());
	    }
	}
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, moveToKeyLessThanWordTest) {
    for (int k = 0; k < kNumSuffixLen; k++) {
	newSuRFWords(kMixed, kSuffixLenList[k]);
	bool inclusive = true;
	for (int i = 0; i < 2; i++) {
	    if (i == 1)
		inclusive = false;
	    for (unsigned j = 1; j < words.size(); j++) {
		SuRF::Iter iter = surf_->moveToKeyLessThan(words[j], inclusive);

		ASSERT_TRUE(iter.isValid());
		unsigned bitlen;
		std::string iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string word_prefix_fp = words[j].substr(0, iter_key.length());
		std::string word_prefix_true = words[j-1].substr(0, iter_key.length());
		bool is_prefix = false;
		if (iter.getFpFlag())
		    is_prefix = isEqual(word_prefix_fp, iter_key, bitlen);
		else
		    is_prefix = isEqual(word_prefix_true, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }

	    SuRF::Iter iter = surf_->moveToKeyLessThan(words[0], inclusive);
	    if (iter.getFpFlag()) {
		ASSERT_TRUE(iter.isValid());
		unsigned bitlen;
		std::string iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string word_prefix_fp = words[0].substr(0, iter_key.length());
		bool is_prefix = isEqual(word_prefix_fp, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    } else {
		ASSERT_FALSE(iter.isValid());
	    }
	}
	surf_->destroy();
	delete surf_;
    }
}

TEST_F (SuRFUnitTest, moveToKeyLessThanIntTest) {
    for (int k = 0; k < kNumSuffixLen; k++) {
	newSuRFInts(kMixed, kSuffixLenList[k]);
	bool inclusive = true;
	for (int i = 0; i < 2; i++) {
	    if (i == 1)
		inclusive = false;
	    for (uint64_t j = kIntTestSkip; j < kIntTestBound; j++) {
		SuRF::Iter iter = surf_->moveToKeyLessThan(uint64ToString(j), inclusive);

		ASSERT_TRUE(iter.isValid());
		unsigned bitlen;
		std::string iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string int_key = uint64ToString(j - (j % kIntTestSkip));
		std::string int_prefix = int_key.substr(0, iter_key.length());
		bool is_prefix = isEqual(int_prefix, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }
	    SuRF::Iter iter = surf_->moveToKeyLessThan(uint64ToString(0), inclusive);
	    if (iter.getFpFlag()) {
		ASSERT_TRUE(iter.isValid());
		unsigned bitlen;
		std::string iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string int_key = uint64ToString(0);
		std::string int_prefix = int_key.substr(0, iter_key.length());
		bool is_prefix = isEqual(int_prefix, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    } else {
		ASSERT_FALSE(iter.isValid());
	    }
	}
	surf_->destroy();
	delete surf_;
    }
}


TEST_F (SuRFUnitTest, IteratorIncrementWordTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFWords(kSuffixTypeList[t], kSuffixLenList[k]);
	    bool inclusive = true;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(words[0], inclusive);
	    for (unsigned i = 1; i < words.size(); i++) {
		iter++;
		ASSERT_TRUE(iter.isValid());
		std::string iter_key;
		unsigned bitlen;
		iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string word_prefix = words[i].substr(0, iter_key.length());
		bool is_prefix = isEqual(word_prefix, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }
	    iter++;
	    ASSERT_FALSE(iter.isValid());
	    surf_->destroy();
	    delete surf_;
	}
    }
}

TEST_F (SuRFUnitTest, IteratorIncrementIntTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFInts(kSuffixTypeList[t], kSuffixLenList[k]);
	    bool inclusive = true;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(uint64ToString(0), inclusive);
	    for (uint64_t i = kIntTestSkip; i < kIntTestBound; i += kIntTestSkip) {
		iter++;
		ASSERT_TRUE(iter.isValid());
		std::string iter_key;
		unsigned bitlen;
		iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string int_prefix = uint64ToString(i).substr(0, iter_key.length());
		bool is_prefix = isEqual(int_prefix, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }
	    iter++;
	    ASSERT_FALSE(iter.isValid());
	    surf_->destroy();
	    delete surf_;
	}
    }
}

TEST_F (SuRFUnitTest, IteratorDecrementWordTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFWords(kSuffixTypeList[t], kSuffixLenList[k]);
	    bool inclusive = true;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(words[words.size() - 1], inclusive);
	    for (int i = words.size() - 2; i >= 0; i--) {
		iter--;
		ASSERT_TRUE(iter.isValid());
		std::string iter_key;
		unsigned bitlen;
		iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string word_prefix = words[i].substr(0, iter_key.length());
		bool is_prefix = isEqual(word_prefix, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }
	    iter--;
	    ASSERT_FALSE(iter.isValid());
	    surf_->destroy();
	    delete surf_;
	}
    }
}

TEST_F (SuRFUnitTest, IteratorDecrementIntTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFInts(kSuffixTypeList[t], kSuffixLenList[k]);
	    bool inclusive = true;
	    SuRF::Iter iter = surf_->moveToKeyGreaterThan(uint64ToString(kIntTestBound - kIntTestSkip), inclusive);
	    for (uint64_t i = kIntTestBound - 1 - kIntTestSkip; i > 0; i -= kIntTestSkip) {
		iter--;
		ASSERT_TRUE(iter.isValid());
		std::string iter_key;
		unsigned bitlen;
		iter_key = iter.getKeyWithSuffix(&bitlen);
		std::string int_prefix = uint64ToString(i).substr(0, iter_key.length());
		bool is_prefix = isEqual(int_prefix, iter_key, bitlen);
		ASSERT_TRUE(is_prefix);
	    }
	    iter--;
	    iter--;
	    ASSERT_FALSE(iter.isValid());
	    surf_->destroy();
	    delete surf_;
	}
    }
}

TEST_F (SuRFUnitTest, lookupRangeWordTest) {
    for (int t = 0; t < kNumSuffixType; t++) {
	for (int k = 0; k < kNumSuffixLen; k++) {
	    newSuRFWords(kSuffixTypeList[t], kSuffixLenList[k]);
	    bool exist = surf_->lookupRange(std::string("\1"), true, words[0], true);
	    ASSERT_TRUE(exist);
	    exist = surf_->lookupRange(std::string("\1"), true, words[0], false);
	    ASSERT_TRUE(exist);

	    for (unsigned i = 0; i < words.size() - 1; i++) {
		exist = surf_->lookupRange(words[i], true, words[i+1], true);
		ASSERT_TRUE(exist);
		exist = surf_->lookupRange(words[i], true, words[i+1], false);
		ASSERT_TRUE(exist);
		exist = surf_->lookupRange(words[i], false, words[i+1], true);
		ASSERT_TRUE(exist);
		exist = surf_->lookupRange(words[i], false, words[i+1], false);
		ASSERT_TRUE(exist);
	    }

	    exist = surf_->lookupRange(words[words.size() - 1], true, std::string("zzzzzzzz"), false);
	    ASSERT_TRUE(exist);
	    exist = surf_->lookupRange(words[words.size() - 1], false, std::string("zzzzzzzz"), false);
	    ASSERT_TRUE(exist);

	}
    }
}

TEST_F (SuRFUnitTest, lookupRangeIntTest) {
    for (int k = 0; k < kNumSuffixLen; k++) {
	newSuRFInts(kMixed, kSuffixLenList[k]);
	for (uint64_t i = 0; i < kIntTestBound; i++) {
	    bool exist = surf_->lookupRange(uint64ToString(i), true, 
					    uint64ToString(i), true);
	    if (i % kIntTestSkip == 0)
		ASSERT_TRUE(exist);
	    else
		ASSERT_FALSE(exist);

	    for (unsigned j = 1; j < kIntTestSkip + 2; j++) {
		exist = surf_->lookupRange(uint64ToString(i), false, 
					   uint64ToString(i + j), true);
		uint64_t left_bound_interval_id = i / kIntTestSkip;
		uint64_t right_bound_interval_id = (i + j) / kIntTestSkip;
		if ((i % kIntTestSkip == 0) 
		    || ((i < kIntTestBound - 1) 
			&& ((left_bound_interval_id < right_bound_interval_id)
			    || ((i + j) % kIntTestSkip == 0))))
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
