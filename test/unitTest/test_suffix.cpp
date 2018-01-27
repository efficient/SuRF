#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "suffix.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace suffixtest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class SuffixUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	computeWordsBySuffixStartLevel();
	data_ = nullptr;
    }
    virtual void TearDown () {
	if (data_)
	    delete[] data_;
    }

    void computeWordsBySuffixStartLevel();
    void testSerialize();
    void testCheckEquality();

    SuRFBuilder* builder_;
    BitvectorSuffix* suffixes_;
    std::vector<std::vector<std::string> > words_by_suffix_start_level_;
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

void SuffixUnitTest::computeWordsBySuffixStartLevel() {
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

	while (words_by_suffix_start_level_.size() < (unsigned)(commonPrefixLen + 1))
	    words_by_suffix_start_level_.push_back(std::vector<std::string>());

	words_by_suffix_start_level_[commonPrefixLen].push_back(words[i]);
    }
}

void SuffixUnitTest::testSerialize() {
    uint64_t size = suffixes_->serializedSize();
    data_ = new char[size];
    BitvectorSuffix* ori_suffixes = suffixes_;
    char* data = data_;
    ori_suffixes->serialize(data);
    data = data_;
    suffixes_ = BitvectorSuffix::deSerialize(data);

    ASSERT_EQ(ori_suffixes->bitsSize(), suffixes_->bitsSize());

    ori_suffixes->destroy();
    delete ori_suffixes;
}

void SuffixUnitTest::testCheckEquality() {
    position_t suffix_idx = 0;
    for (level_t level = 0; level < words_by_suffix_start_level_.size(); level++) {
        for (unsigned k = 0; k < words_by_suffix_start_level_[level].size(); k++) {
            if (level == 1 && k == 32) {
                bool is_equal = suffixes_->checkEquality(suffix_idx,
                                                         words_by_suffix_start_level_[level][k],
                                                         (level + 1));
                ASSERT_TRUE(is_equal);
            }
	    suffix_idx++;
	}
    }
}

TEST_F (SuffixUnitTest, constructRealSuffixTest) {
    const level_t level = 2;
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int i = 0; i < 5; i++) {
	level_t suffix_len = suffix_len_array[i];
	for (unsigned j = 0; j < words.size(); j++) {
	    word_t suffix = BitvectorSuffix::constructSuffix(kReal, words[j], 0, level, suffix_len);
	    if (words[j].length() < level || ((words[j].length() - level) * 8) < suffix_len) {
		ASSERT_EQ(0, suffix);
		continue;
	    }
	    for (position_t bitpos = 0; bitpos < suffix_len; bitpos++) {
		position_t byte_id = bitpos / 8;
		position_t byte_offset = bitpos % 8;
		uint8_t byte_mask = 0x80;
		byte_mask >>= byte_offset;
		bool expected_suffix_bit = false;
		if (level + byte_id < words[j].size())
		    expected_suffix_bit = (bool)(words[j][level + byte_id] & byte_mask);

		word_t word_mask = kMsbMask;
		word_mask >>= (kWordSize - suffix_len + bitpos);
		bool suffix_bit = (bool)(suffix & word_mask);

		ASSERT_EQ(expected_suffix_bit, suffix_bit);
	    }
	}
    }
}

TEST_F (SuffixUnitTest, constructMixedSuffixTest) {
    const level_t level = 2;
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int i = 0; i < 5; i++) {
	level_t suffix_len = suffix_len_array[i];
	for (unsigned j = 0; j < words.size(); j++) {
	    word_t suffix = BitvectorSuffix::constructSuffix(kMixed, words[j], suffix_len,
                                                             level, suffix_len);
            word_t hash_suffix = BitvectorSuffix::extractHashSuffix(suffix, suffix_len);
            word_t expected_hash_suffix = BitvectorSuffix::constructHashSuffix(words[j], suffix_len);
            ASSERT_EQ(expected_hash_suffix, hash_suffix);

            word_t real_suffix = BitvectorSuffix::extractRealSuffix(suffix, suffix_len);
	    if (words[j].length() < level || ((words[j].length() - level) * 8) < suffix_len) {
		ASSERT_EQ(0, real_suffix);
		continue;
	    }
	    for (position_t bitpos = 0; bitpos < suffix_len; bitpos++) {
		position_t byte_id = bitpos / 8;
		position_t byte_offset = bitpos % 8;
		uint8_t byte_mask = 0x80;
		byte_mask >>= byte_offset;
		bool expected_suffix_bit = false;
		if (level + byte_id < words[j].size())
		    expected_suffix_bit = (bool)(words[j][level + byte_id] & byte_mask);

		word_t word_mask = kMsbMask;
		word_mask >>= (kWordSize - suffix_len + bitpos);
		bool suffix_bit = (bool)(real_suffix & word_mask);
		ASSERT_EQ(expected_suffix_bit, suffix_bit);
	    }
	}
    }
}

TEST_F (SuffixUnitTest, checkEqualityTest) {
    bool include_dense = false;
    uint32_t sparse_dense_ratio = 0;
    SuffixType suffix_type_array[3] = {kHash, kReal, kMixed};
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 5; j++) {
	    // build test
	    SuffixType suffix_type = suffix_type_array[i];
	    level_t suffix_len = suffix_len_array[j];

            if (i == 0)
                builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, suffix_len, 0);
            else if (i == 1)
                builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, 0, suffix_len);
            else
                builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio,
                                           suffix_type, suffix_len, suffix_len);
	    builder_->build(words);

	    level_t height = builder_->getLabels().size();
	    std::vector<position_t> num_suffix_bits_per_level;
	    for (level_t level = 0; level < height; level++) {
                if (suffix_type == kMixed)
                    num_suffix_bits_per_level.push_back(builder_->getSuffixCounts()[level] * suffix_len * 2);
                else
                    num_suffix_bits_per_level.push_back(builder_->getSuffixCounts()[level] * suffix_len);
            }

            if (i == 0)
                suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), suffix_len, 0, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);
            else if (i == 1)
                suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), 0, suffix_len, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);
            else
                suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), suffix_len, suffix_len, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);

	    testCheckEquality();
	    delete builder_;
	    suffixes_->destroy();
	    delete suffixes_;
	}
    }
}

TEST_F (SuffixUnitTest, serializeTest) {
    bool include_dense = false;
    uint32_t sparse_dense_ratio = 0;
    SuffixType suffix_type_array[3] = {kHash, kReal, kMixed};
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 5; j++) {
	    // build test
	    SuffixType suffix_type = suffix_type_array[i];
	    level_t suffix_len = suffix_len_array[j];
            if (i == 0)
                builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, suffix_len, 0);
            else if (i == 1)
                builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, 0, suffix_len);
            else
                builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio,
                                           suffix_type, suffix_len, suffix_len);
	    builder_->build(words);

	    level_t height = builder_->getLabels().size();
	    std::vector<position_t> num_suffix_bits_per_level;
	    for (level_t level = 0; level < height; level++) {
                if (suffix_type == kMixed)
                    num_suffix_bits_per_level.push_back(builder_->getSuffixCounts()[level] * suffix_len * 2);
                else
                    num_suffix_bits_per_level.push_back(builder_->getSuffixCounts()[level] * suffix_len);
            }

            if (i == 0)
                suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), suffix_len, 0, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);
            else if (i == 1)
                suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), 0, suffix_len, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);
            else
                suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), suffix_len, suffix_len, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);

	    testSerialize();
	    testCheckEquality();
	    delete builder_;
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

} // namespace suffixtest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::suffixtest::loadWordList();
    return RUN_ALL_TESTS();
}
