#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "bitvector.hpp"
#include "config.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace bitvectortest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class BitvectorUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	builder_ = new SuRFBuilder(kReal);
	bv_ = NULL;
	num_items_ = 0;
    }
    virtual void TearDown () {
	delete builder_;
	if (bv_ != NULL)
	    delete bv_;
    }

    void setupWordsTest();

    SuRFBuilder* builder_;
    Bitvector* bv_;
    Bitvector* bv2_;
    std::vector<position_t> num_items_per_level_;
    position_t num_items_;
};

void BitvectorUnitTest::setupWordsTest() {
    builder_->build(words);
    for (level_t level = 0; level < builder_->getTreeHeight(); level++)
	num_items_per_level_.push_back(builder_->getLabels()[level].size());
    for (level_t level = 0; level < num_items_per_level_.size(); level++)
	num_items_ += num_items_per_level_[level];
    bv_ = new Bitvector(builder_->getChildIndicatorBits(), num_items_per_level_);
    bv2_ = new Bitvector(builder_->getLoudsBits(), num_items_per_level_);
}

TEST_F (BitvectorUnitTest, numWordsTest) {
    setupWordsTest();
    ASSERT_EQ((num_items_ / kWordSize), bv_->numWords());
}

TEST_F (BitvectorUnitTest, readBitTest) {
    setupWordsTest();
    position_t bv_pos = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < num_items_per_level_[level]; pos++) {
	    bool expected_bit = SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos);
	    bool bv_bit = bv_->readBit(bv_pos);
	    ASSERT_EQ(expected_bit, bv_bit);
	    bv_pos++;
	}
    }
}

TEST_F (BitvectorUnitTest, distanceToNextOneTest) {
    setupWordsTest();
    std::vector<position_t> distanceVector;
    position_t distance = 1;
    for (position_t pos = 1; pos < num_items_; pos++) {
	bool is_bit_set = bv2_->readBit(pos);
	if (is_bit_set) {
	    while (distance > 0) {
		distanceVector.push_back(distance);
		distance--;
	    }
	    distance = 1;
	}
	else {
	    distance++;
	}
    }

    for (position_t pos = 0; pos < num_items_; pos++) {
	distance = bv2_->distanceToNextOne(pos);
	ASSERT_EQ(distanceVector[pos], distance);
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

} // namespace bitvectortest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::bitvectortest::loadWordList();
    return RUN_ALL_TESTS();
}
