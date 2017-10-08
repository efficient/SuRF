#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "rank.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace ranktest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class RankUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = false;
	uint32_t sparse_dense_ratio = 0;
	level_t suffix_len = 8;
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, suffix_len);
	bv_ = NULL;
	bv2_ = NULL;
	num_items_ = 0;
    }
    virtual void TearDown () {
	delete builder_;
	if (bv_ != NULL) delete bv_;
	if (bv2_ != NULL) delete bv2_;
    }

    void setupWordsTest();

    static const position_t kRankBasicBlockSize = 512;

    SuRFBuilder* builder_;
    BitvectorRank* bv_;
    BitvectorRank* bv2_;
    std::vector<position_t> num_items_per_level_;
    position_t num_items_;
};

void RankUnitTest::setupWordsTest() {
    builder_->build(words);
    for (level_t level = 0; level < builder_->getTreeHeight(); level++)
	num_items_per_level_.push_back(builder_->getLabels()[level].size());
    for (level_t level = 0; level < num_items_per_level_.size(); level++)
	num_items_ += num_items_per_level_[level];
    bv_ = new BitvectorRank(kRankBasicBlockSize, builder_->getChildIndicatorBits(), num_items_per_level_);
    bv2_ = new BitvectorRank(kRankBasicBlockSize, builder_->getLoudsBits(), num_items_per_level_);
}

TEST_F (RankUnitTest, readBitTest) {
    setupWordsTest();
    position_t bv_pos = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < num_items_per_level_[level]; pos++) {
	    bool expected_bit = SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos);
	    bool bv_bit = bv_->readBit(bv_pos);
	    ASSERT_EQ(expected_bit, bv_bit);

	    expected_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    bv_bit = bv2_->readBit(bv_pos);
	    ASSERT_EQ(expected_bit, bv_bit);

	    bv_pos++;
	}
    }
}

TEST_F (RankUnitTest, rankTest) {
    setupWordsTest();
    position_t expected_rank = 0;
    position_t expected_rank2 = 0;
    for (position_t pos = 0; pos < num_items_; pos++) {
	if (bv_->readBit(pos)) expected_rank++;
	position_t rank = bv_->rank(pos);
	ASSERT_EQ(expected_rank, rank);

	if (bv2_->readBit(pos)) expected_rank2++;
	position_t rank2 = bv2_->rank(pos);
	ASSERT_EQ(expected_rank2, rank2);
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

} // namespace ranktest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::ranktest::loadWordList();
    return RUN_ALL_TESTS();
}
