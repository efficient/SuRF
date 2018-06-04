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
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, 0, suffix_len);
	data_ = nullptr;
	data2_ = nullptr;
	num_items_ = 0;
    }
    virtual void TearDown () {
	delete builder_;
	if (data_)
	    delete[] data_;
	if (data2_)
	    delete[] data2_;
    }

    void setupWordsTest();
    void testSerialize();
    void testRank();

    static const position_t kRankBasicBlockSize = 512;

    SuRFBuilder* builder_;
    BitvectorRank* bv_;
    BitvectorRank* bv2_;
    std::vector<position_t> num_items_per_level_;
    position_t num_items_;
    char* data_;
    char* data2_;
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

void RankUnitTest::testSerialize() {
    uint64_t size = bv_->serializedSize();
    ASSERT_TRUE((bv_->size() - size) >= 0);
    data_ = new char[size];
    BitvectorRank* ori_bv = bv_;
    char* data = data_;
    ori_bv->serialize(data);
    data = data_;
    bv_ = BitvectorRank::deSerialize(data);

    ASSERT_EQ(ori_bv->bitsSize(), bv_->bitsSize());
    ASSERT_EQ(ori_bv->rankLutSize(), bv_->rankLutSize());
    
    ori_bv->destroy();
    delete ori_bv;

    size = bv2_->serializedSize();
    data2_ = new char[size];
    BitvectorRank* ori_bv2 = bv2_;
    char* data2 = data2_;
    ori_bv2->serialize(data2);
    data2 = data2_;
    bv2_ = BitvectorRank::deSerialize(data2);

    ASSERT_EQ(ori_bv2->bitsSize(), bv2_->bitsSize());
    ASSERT_EQ(ori_bv2->rankLutSize(), bv2_->rankLutSize());
    
    ori_bv2->destroy();
    delete ori_bv2;
}

void RankUnitTest::testRank() {
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
    bv_->destroy();
    delete bv_;
    bv2_->destroy();
    delete bv2_;
}

TEST_F (RankUnitTest, rankTest) {
    setupWordsTest();
    testRank();
    bv_->destroy();
    delete bv_;
    bv2_->destroy();
    delete bv2_;
}

TEST_F (RankUnitTest, serializeTest) {
    setupWordsTest();
    testSerialize();
    testRank();
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
