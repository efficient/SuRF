#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "label_vector.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace labelvectortest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class LabelVectorUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = false;
	builder_ = new SuRFBuilder(include_dense, kReal);
	labels_ = NULL;
    }
    virtual void TearDown () {
	delete builder_;
	if (labels_ != NULL) delete labels_;
    }

    void setupWordsTest();

    SuRFBuilder* builder_;
    LabelVector* labels_;
};

void LabelVectorUnitTest::setupWordsTest() {
    builder_->build(words);
    labels_ = new LabelVector(builder_->getLabels());
}

TEST_F (LabelVectorUnitTest, readTest) {
    setupWordsTest();
    position_t lv_pos = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < builder_->getLabels()[level].size(); pos++) {
	    label_t expected_label = builder_->getLabels()[level][pos];
	    label_t label = labels_->read(lv_pos);
	    ASSERT_EQ(expected_label, label);
	    lv_pos++;
	}
    }
}

TEST_F (LabelVectorUnitTest, searchTest) {
    setupWordsTest();
    position_t start_pos = 0;
    position_t search_len = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < builder_->getLabels()[level].size(); pos++) {
	    bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    if (louds_bit) {
		position_t binary_search_pos, simd_search_pos, linear_search_pos;
		bool binary_search_success, simd_search_success, linear_search_success;
		for (position_t i = start_pos; i < search_len; i++) {
		    // binary search success
		    binary_search_pos = start_pos;
		    binary_search_success = labels_->binarySearch(labels_->read(i), binary_search_pos, search_len);
		    ASSERT_TRUE(binary_search_success);
		    ASSERT_EQ(i, binary_search_pos);

		    // simd search success
		    simd_search_pos = start_pos;
		    simd_search_success = labels_->simdSearch(labels_->read(i), simd_search_pos, search_len);
		    ASSERT_TRUE(simd_search_success);
		    ASSERT_EQ(i, simd_search_pos);

		    // linear search success
		    linear_search_pos = start_pos;
		    linear_search_success = labels_->linearSearch(labels_->read(i), linear_search_pos, search_len);
		    ASSERT_TRUE(linear_search_success);
		    ASSERT_EQ(i, linear_search_pos);
		}
		// binary search fail
		binary_search_pos = start_pos;
		binary_search_success = labels_->binarySearch('\0', binary_search_pos, search_len);
		ASSERT_FALSE(binary_search_success);
		binary_search_pos = start_pos;
		binary_search_success = labels_->binarySearch('\255', binary_search_pos, search_len);
		ASSERT_FALSE(binary_search_success);

		// simd search fail
		simd_search_pos = start_pos;
		simd_search_success = labels_->simdSearch('\0', simd_search_pos, search_len);
		ASSERT_FALSE(simd_search_success);
		simd_search_pos = start_pos;
		simd_search_success = labels_->simdSearch('\255', simd_search_pos, search_len);
		ASSERT_FALSE(simd_search_success);

		// linear search fail
		linear_search_pos = start_pos;
		linear_search_success = labels_->linearSearch('\0', linear_search_pos, search_len);
		ASSERT_FALSE(linear_search_success);
		linear_search_pos = start_pos;
		linear_search_success = labels_->linearSearch('\255', linear_search_pos, search_len);
		ASSERT_FALSE(linear_search_success);

		start_pos += search_len;
		search_len = 0;
	    }
	    

	    if (builder_->getLabels()[level][pos] == kTerminator
		&& !SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos))
		start_pos++;
	    else
		search_len++;
	}
    }
}

TEST_F (LabelVectorUnitTest, searchTest2) {
    setupWordsTest();
    position_t start_pos = 0;
    position_t search_len = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < builder_->getLabels()[level].size(); pos++) {
	    bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    if (louds_bit) {
		position_t search_pos;
		bool search_success;
		for (position_t i = start_pos; i < search_len; i++) {
		    if ((i == start_pos) && (labels_->read(i) == kTerminator)
			&& !SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos))
			continue;
		    // search success
		    search_pos = start_pos;
		    search_success = labels_->search(labels_->read(i), search_pos, search_len);

		    ASSERT_TRUE(search_success);
		    ASSERT_EQ(i, search_pos);
		}
		// search fail
		search_pos = start_pos;
		search_success = labels_->search('\0', search_pos, search_len);
		ASSERT_FALSE(search_success);
		search_pos = start_pos;
		search_success = labels_->search('\255', search_pos, search_len);
		ASSERT_FALSE(search_success);

		start_pos += search_len;
		search_len = 0;
	    }
	    search_len++;
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

} // namespace labelvectortest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::labelvectortest::loadWordList();
    return RUN_ALL_TESTS();
}
