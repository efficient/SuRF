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
	uint32_t sparse_dense_ratio = 0;
	level_t suffix_len = 8;
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal, 0, suffix_len);
	data_ = nullptr;
    }
    virtual void TearDown () {
	delete builder_;
	if (data_)
	    delete[] data_;
    }

    void setupWordsTest();
    void testSerialize();
    void testSearch();

    SuRFBuilder* builder_;
    LabelVector* labels_;
    char* data_;
};

void LabelVectorUnitTest::setupWordsTest() {
    builder_->build(words);
    labels_ = new LabelVector(builder_->getLabels());
}

void LabelVectorUnitTest::testSerialize() {
    uint64_t size = labels_->serializedSize();
    ASSERT_TRUE((labels_->size() - size) >= 0);
    data_ = new char[size];
    LabelVector* ori_labels = labels_;
    char* data = data_;
    ori_labels->serialize(data);
    data = data_;
    labels_ = LabelVector::deSerialize(data);

    ASSERT_EQ(ori_labels->getNumBytes(), labels_->getNumBytes());

    for (position_t i = 0; i < ori_labels->getNumBytes(); i++) {
	label_t ori_label = ori_labels->read(i);
	label_t label = labels_->read(i);
	ASSERT_EQ(ori_label, label);
    }
    
    ori_labels->destroy();
    delete ori_labels;
}

void LabelVectorUnitTest::testSearch() {
    position_t start_pos = 0;
    position_t search_len = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < builder_->getLabels()[level].size(); pos++) {
	    bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    if (louds_bit) {
		position_t search_pos;
		bool search_success;
		for (position_t i = start_pos; i < start_pos + search_len; i++) {
		    label_t test_label = labels_->read(i);
		    if (i == start_pos && test_label == kTerminator && search_len > 1)
			continue;
		    // search success
		    search_pos = start_pos;
		    search_success = labels_->search(test_label, search_pos, search_len);
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
    labels_->destroy();
    delete labels_;
}

TEST_F (LabelVectorUnitTest, searchAlgTest) {
    setupWordsTest();
    position_t start_pos = 0;
    position_t search_len = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < builder_->getLabels()[level].size(); pos++) {
	    bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    if (louds_bit) {
		position_t binary_search_pos, simd_search_pos, linear_search_pos;
		bool binary_search_success, simd_search_success, linear_search_success;
		for (position_t i = start_pos; i < start_pos + search_len; i++) {
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
    labels_->destroy();
    delete labels_;
}

TEST_F (LabelVectorUnitTest, searchTest) {
    setupWordsTest();
    testSearch();
    labels_->destroy();
    delete labels_;
}

TEST_F (LabelVectorUnitTest, serializeTest) {
    setupWordsTest();
    testSerialize();
    testSearch();
}

TEST_F (LabelVectorUnitTest, searchGreaterThanTest) {
    setupWordsTest();
    position_t start_pos = 0;
    position_t search_len = 0;
    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	for (position_t pos = 0; pos < builder_->getLabels()[level].size(); pos++) {
	    bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    if (louds_bit) {
		position_t search_pos;
		position_t terminator_offset = 0;
		bool search_success;
		for (position_t i = start_pos; i < start_pos + search_len; i++) {
		    label_t cur_label = labels_->read(i);
		    if (i == start_pos && cur_label == kTerminator && search_len > 1) {
			terminator_offset = 1;
			continue;
		    }

		    if (i < start_pos + search_len - 1) {
			label_t next_label = labels_->read(i+1);
			// search existing label
			search_pos = start_pos;
			search_success = labels_->searchGreaterThan(cur_label, search_pos, search_len);
			ASSERT_TRUE(search_success);
			ASSERT_EQ(i+1, search_pos);

			// search midpoint (could be non-existing label)
			label_t test_label = cur_label + ((next_label - cur_label) / 2);
			search_pos = start_pos;
			search_success = labels_->searchGreaterThan(test_label, search_pos, search_len);
			ASSERT_TRUE(search_success);
			ASSERT_EQ(i+1, search_pos);
		    } else {
			// search out-of-bound label
			search_pos = start_pos;
			search_success = labels_->searchGreaterThan(labels_->read(start_pos + search_len - 1), search_pos, search_len);
			ASSERT_FALSE(search_success);
			ASSERT_EQ(start_pos + terminator_offset, search_pos);
		    }
		}
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
