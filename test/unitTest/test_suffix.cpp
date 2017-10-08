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
    }

    void computeWordsBySuffixStartLevel();

    SuRFBuilder* builder_;
    BitvectorSuffix* suffixes_;
    std::vector<std::vector<std::string> > words_by_suffix_start_level_;
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

	while (words_by_suffix_start_level_.size() < (unsigned)(commonPrefixLen + 2))
	    words_by_suffix_start_level_.push_back(std::vector<std::string>());

	words_by_suffix_start_level_[commonPrefixLen + 1].push_back(words[i]);
    }
}

TEST_F (SuffixUnitTest, constructSuffixTest) {
    // Real suffix test
    level_t level = 2;
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int i = 0; i < 5; i++) {
	level_t suffix_len = suffix_len_array[i];
	for (unsigned j = 0; j < words.size(); j++) {
	    word_t suffix = BitvectorSuffix::constructSuffix(words[j], level, kReal, suffix_len);
	    for (position_t bitpos = 0; bitpos < suffix_len; bitpos++) {
		position_t byte_id = bitpos / 8;
		position_t byte_offset = bitpos % 8;
		uint8_t byte_mask = 0x80;
		byte_mask >>= byte_offset;
		bool expected_suffix_bit = (bool)(words[i][level + byte_id] & byte_mask);

		word_t word_mask = kMsbMask;
		word_mask >>= (kWordSize - suffix_len + bitpos);
		bool suffix_bit = (bool)(suffix & word_mask);

		ASSERT_EQ(expected_suffix_bit, suffix_bit);
	    }
	}
    }
}

TEST_F (SuffixUnitTest, CheckEqualityTest) {
    bool include_dense = false;
    uint32_t sparse_dense_ratio = 0;
    SuffixType suffix_type_array[2] = {kHash, kReal};
    level_t suffix_len_array[5] = {1, 3, 7, 8, 13};
    for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 5; i++) {
	    // build test
	    SuffixType suffix_type = suffix_type_array[i];
	    level_t suffix_len = suffix_len_array[j];
	    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, suffix_len);
	    builder_->build(words);

	    level_t height = builder_->getLabels().size();
	    suffix_len = builder_->getSuffixLen();
	    std::vector<position_t> num_suffix_bits_per_level;
	    for (level_t level = 0; level < height; level++)
		num_suffix_bits_per_level.push_back(builder_->getSuffixCounts()[level] * suffix_len);
	    suffixes_ = new BitvectorSuffix(builder_->getSuffixType(), suffix_len, builder_->getSuffixes(), num_suffix_bits_per_level, 0, height);

	    // checkEquality test
	    position_t suffix_idx = 0;
	    for (level_t level = 0; level < words_by_suffix_start_level_.size(); level++) {
		for (unsigned i = 0; i < words_by_suffix_start_level_[level].size(); i++) {
		    bool is_equal = suffixes_->checkEquality(suffix_idx,
							   words_by_suffix_start_level_[level][i],
							   level);
		    ASSERT_TRUE(is_equal);
		    suffix_idx++;
		}
	    }
	    delete builder_;
	    delete suffixes_;
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
