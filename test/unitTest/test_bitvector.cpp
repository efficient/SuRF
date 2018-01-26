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
	bool include_dense = true;
	uint32_t sparse_dense_ratio = 0;
	level_t suffix_len = 8;
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, 0, suffix_len);
	num_items_ = 0;
    }
    virtual void TearDown () {
	delete builder_;
	delete bv_;
	delete bv2_;
	delete bv3_;
	delete bv4_;
	delete bv5_;
    }

    void setupWordsTest();

    SuRFBuilder* builder_;
    Bitvector* bv_; // sparse: child indicator bits
    Bitvector* bv2_; // sparse: louds bits
    Bitvector* bv3_; // dense: label bitmap
    Bitvector* bv4_; // dense: child indicator bitmap
    Bitvector* bv5_; // dense: prefixkey indicator bits
    std::vector<position_t> num_items_per_level_; // sparse
    position_t num_items_; // sparse
    std::vector<position_t> num_bits_per_level_; // dense
};

void BitvectorUnitTest::setupWordsTest() {
    builder_->build(words);
    for (level_t level = 0; level < builder_->getTreeHeight(); level++)
	num_items_per_level_.push_back(builder_->getLabels()[level].size());
    for (level_t level = 0; level < num_items_per_level_.size(); level++)
	num_items_ += num_items_per_level_[level];
    bv_ = new Bitvector(builder_->getChildIndicatorBits(), num_items_per_level_);
    bv2_ = new Bitvector(builder_->getLoudsBits(), num_items_per_level_);

    for (level_t level = 0; level < builder_->getTreeHeight(); level++)
	num_bits_per_level_.push_back(builder_->getBitmapLabels()[level].size() * kWordSize);
    bv3_ = new Bitvector(builder_->getBitmapLabels(), num_bits_per_level_);
    bv4_ = new Bitvector(builder_->getBitmapChildIndicatorBits(), num_bits_per_level_);
    bv5_ = new Bitvector(builder_->getPrefixkeyIndicatorBits(), builder_->getNodeCounts());
}

TEST_F (BitvectorUnitTest, readBitTest) {
    setupWordsTest();

    position_t bv_pos = 0;
    int node_num = -1;
    label_t prev_label = 0;
    position_t bv5_pos = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < num_items_per_level_[level]; pos++) {
	    // bv test
	    bool has_child = SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos);
	    bool bv_bit = bv_->readBit(bv_pos);
	    ASSERT_EQ(has_child, bv_bit);

	    // bv2 test
	    bool is_node_start = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    bv_bit = bv2_->readBit(bv_pos);
	    ASSERT_EQ(is_node_start, bv_bit);

	    bv_pos++;

	    if (is_node_start)
		node_num++;

	    // bv5 test
	    bool is_terminator = false;
	    if (is_node_start) {
	        is_terminator = (builder_->getLabels()[level][pos] == kTerminator)
		    && !SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos);
		bv_bit = bv5_->readBit(bv5_pos);
		ASSERT_EQ(is_terminator, bv_bit);
		bv5_pos++;
	    }

	    if (is_terminator) {
		for (unsigned c = prev_label + 1; c < kFanout; c++) {
		    bool bv3_bit = bv3_->readBit((node_num - 1) * kFanout + c);
		    ASSERT_FALSE(bv3_bit);
		    bool bv4_bit = bv4_->readBit((node_num - 1) * kFanout + c);
		    ASSERT_FALSE(bv4_bit);
		}
		prev_label = '\255';
		continue;
	    }

	    // bv3 test
	    label_t label = builder_->getLabels()[level][pos];
	    bool bv3_bit = bv3_->readBit(node_num * kFanout + label);
	    ASSERT_TRUE(bv3_bit);

	    // bv4 test
	    bool bv4_bit = bv4_->readBit(node_num * kFanout + label);
	    ASSERT_EQ(has_child, bv4_bit);

	    // bv3 bv4 zero bit test
	    if (is_node_start) {
		if (node_num > 0) {
		    for (unsigned c = prev_label + 1; c < kFanout; c++) {
			bv3_bit = bv3_->readBit((node_num - 1) * kFanout + c);
			ASSERT_FALSE(bv3_bit);
			bv4_bit = bv4_->readBit((node_num - 1) * kFanout + c);
			ASSERT_FALSE(bv4_bit);
		    }
		}
		for (unsigned c = 0; c < (unsigned)label; c++) {
		    bv3_bit = bv3_->readBit(node_num * kFanout + c);
		    ASSERT_FALSE(bv3_bit);
		    bv4_bit = bv4_->readBit(node_num * kFanout + c);
		    ASSERT_FALSE(bv4_bit);
		}
	    } else {
		for (unsigned c = prev_label + 1; c < (unsigned)label; c++) {
		    bv3_bit = bv3_->readBit(node_num * kFanout + c);
		    ASSERT_FALSE(bv3_bit);
		    bv4_bit = bv4_->readBit(node_num * kFanout + c);
		    ASSERT_FALSE(bv4_bit);
		}
	    }
	    prev_label = label;
	}
    }

}

TEST_F (BitvectorUnitTest, distanceToNextSetBitTest) {
    setupWordsTest();
    std::vector<position_t> distanceVector;
    position_t distance = 1;
    for (position_t pos = 1; pos < num_items_; pos++) {
	if (bv2_->readBit(pos)) {
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
    while (distance > 0) {
	distanceVector.push_back(distance);
	distance--;
    }

    for (position_t pos = 0; pos < num_items_; pos++) {
	distance = bv2_->distanceToNextSetBit(pos);
	ASSERT_EQ(distanceVector[pos], distance);
    }
}

TEST_F (BitvectorUnitTest, distanceToPrevSetBitTest) {
    setupWordsTest();
    std::vector<position_t> distanceVector;
    for (position_t pos = 0; pos < num_items_; pos++)
	distanceVector.push_back(0);
    
    position_t distance = 1;
    for (position_t pos = num_items_ - 2; pos > 0; pos--) {
	if (bv2_->readBit(pos)) {
	    for (position_t i = 1; i <= distance; i++)
		distanceVector[pos + i] = i;
	    distance = 1;
	}
	else {
	    distance++;
	}
    }
    if (bv2_->readBit(0)) {
	for (position_t i = 1; i <= distance; i++)
	    distanceVector[i] = i;
    } else {
	distance++;
	for (position_t i = 1; i <= distance; i++)
	    distanceVector[i - 1] = i;
    }

    for (position_t pos = 0; pos < num_items_; pos++) {
	distance = bv2_->distanceToPrevSetBit(pos);
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
