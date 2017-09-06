#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace buildertest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class SuRFBuilderUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = true;
	uint32_t sparse_dense_ratio = 0;
	builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio, kReal);
	truncateWordSuffixes();
    }
    virtual void TearDown () {
	delete builder_;
    }

    void truncateWordSuffixes();
    bool DoesPrefixMatchInWordsTrunc(int i, int j, int len);

    //debug
    void printDenseNode(level_t level, position_t node_num);
    void printSparseNode(level_t level, position_t pos);

    SuRFBuilder *builder_;
    std::vector<std::string> words_trunc_;
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

void SuRFBuilderUnitTest::truncateWordSuffixes() {
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

	if (commonPrefixLen < words[i].length()) {
	    words_trunc_.push_back(words[i].substr(0, commonPrefixLen + 1));
	} else {
	    words_trunc_.push_back(words[i]);
	    words_trunc_[i] += (char)kTerminator;
	}
    }
}

//debug
void printIndent(level_t level) {
    for (level_t l = 0; l < level; l++)
	std::cout << "\t";
}

void SuRFBuilderUnitTest::printDenseNode(level_t level, position_t node_num) {
    printIndent(level);
    std::cout << "level = " << level << "\tnode_num = " << node_num << "\n";

    // print labels
    printIndent(level);
    for (position_t i = 0; i < kFanout; i++) {
	if (SuRFBuilder::readBit(builder_->getBitmapLabels()[level], node_num * kFanout + i)) {
	    if ((i >= 65 && i <= 90) || (i >= 97 && i <= 122))
		std::cout << (char)i << " ";
	    else
		std::cout << (int16_t)i << " ";
	}
    }
    std::cout << "\n";

    // print child indicator bitmap
    printIndent(level);
    for (position_t i = 0; i < kFanout; i++) {
	if (SuRFBuilder::readBit(builder_->getBitmapLabels()[level], node_num * kFanout + i)) {
	    if (SuRFBuilder::readBit(builder_->getBitmapChildIndicatorBits()[level], node_num * kFanout + i))
		std::cout << "1 ";
	    else
		std::cout << "0 ";
	}
    }
    std::cout << "\n";

    // print prefixkey indicator
    printIndent(level);
    if (SuRFBuilder::readBit(builder_->getPrefixkeyIndicatorBits()[level], node_num))
	std::cout << "1 ";
    else
	std::cout << "0 ";
    std::cout << "\n";
}

void SuRFBuilderUnitTest::printSparseNode(level_t level, position_t pos) {
    printIndent(level);
    std::cout << "level = " << level << "\tpos = " << pos << "\n";

    position_t start_pos = pos;

    // print labels
    printIndent(level);
    bool is_end_of_node = false;
    while (!is_end_of_node && pos < builder_->getLabels()[level].size()) {
	label_t label = builder_->getLabels()[level][pos];
	if ((label >= 65 && label <= 90) || (label >= 97 && label <= 122))
	    std::cout << (char)label << " ";
	else
	    std::cout << (int16_t)label << " ";
	pos++;
	is_end_of_node = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
    }
    std::cout << "\n";

    // print child indicators
    printIndent(level);
    is_end_of_node = false;
    pos = start_pos;
    while (!is_end_of_node && pos < builder_->getLabels()[level].size()) {
	bool has_child = SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos);
	if (has_child)
	    std::cout << "1 ";
	else
	    std::cout << "0 ";
	pos++;
	is_end_of_node = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
    }
    std::cout << "\n";

    // print louds bits
    printIndent(level);
    is_end_of_node = false;
    pos = start_pos;
    while (!is_end_of_node && pos < builder_->getLabels()[level].size()) {
	bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	if (louds_bit)
	    std::cout << "1 ";
	else
	    std::cout << "0 ";
	pos++;
	is_end_of_node = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
    }
    std::cout << "\n";
}

bool SuRFBuilderUnitTest::DoesPrefixMatchInWordsTrunc(int i, int j, int len) {
    if (i < 0 || i >= words_trunc_.size()) return false;
    if (j < 0 || j >= words_trunc_.size()) return false;
    if (len <= 0) return true;
    if (words_trunc_[i].length() < len) return false;
    if (words_trunc_[j].length() < len) return false;
    if (words_trunc_[i].substr(0, len).compare(words_trunc_[j].substr(0, len)) == 0)
	return true;
    return false;
}

TEST_F (SuRFBuilderUnitTest, buildSparseTest) {
    builder_->build(words);

    for (level_t level = 0; level < builder_->getTreeHeight(); level++) {
	position_t pos = 0; pos--;
	position_t suffix_pos = 0;
	for (int i = 0; i < (int)words_trunc_.size(); i++) {
	    if (level >= words_trunc_[i].length())
		continue;
	    if (DoesPrefixMatchInWordsTrunc(i-1, i, level+1))
		continue;
	    pos++;

	    // label test
	    label_t label = words_trunc_[i][level];
	    bool exist_in_node = (builder_->getLabels()[level][pos] == label);
	    ASSERT_TRUE(exist_in_node);

	    // child indicator test
	    bool has_child = SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], pos);
	    bool same_prefix_in_prev_key = DoesPrefixMatchInWordsTrunc(i-1, i, level+1);
	    bool same_prefix_in_next_key = DoesPrefixMatchInWordsTrunc(i, i+1, level+1);
	    bool expected_has_child = same_prefix_in_prev_key || same_prefix_in_next_key;
	    ASSERT_EQ(expected_has_child, has_child);

	    // LOUDS bit test
	    bool louds_bit = SuRFBuilder::readBit(builder_->getLoudsBits()[level], pos);
	    bool expected_louds_bit = !DoesPrefixMatchInWordsTrunc(i-1, i, level);
	    if (pos == 0)
		ASSERT_TRUE(louds_bit);
	    else
		ASSERT_EQ(expected_louds_bit, louds_bit);

	    // suffix test
	    if (!has_child) {
		suffix_t suffix = builder_->getSuffixes()[level+1][suffix_pos];
		suffix_t expected_suffix = 0;
		if ((level+1) < words[i].length())
		    expected_suffix = words[i][level+1];
		ASSERT_EQ(expected_suffix, suffix);
		suffix_pos++;
	    }
	}
    }
}

TEST_F (SuRFBuilderUnitTest, buildDenseTest) {
    builder_->build(words);

    for (level_t level = 0; level < builder_->getSparseStartLevel(); level++) {
	int node_num = -1;

	label_t prev_label = 0;
	for (unsigned i = 0; i < builder_->getLabels()[level].size(); i++) {
	    bool is_node_start = SuRFBuilder::readBit(builder_->getLoudsBits()[level], i);
	    if (is_node_start) 
		node_num++;

	    label_t label = builder_->getLabels()[level][i];
	    bool exist_in_node = SuRFBuilder::readBit(builder_->getBitmapLabels()[level], node_num * kFanout + label);
	    bool has_child_sparse = SuRFBuilder::readBit(builder_->getChildIndicatorBits()[level], i);
	    bool has_child_dense = SuRFBuilder::readBit(builder_->getBitmapChildIndicatorBits()[level], node_num * kFanout + label);

	    // prefixkey indicator test
	    if (is_node_start) {
		bool prefixkey_indicator = SuRFBuilder::readBit(builder_->getPrefixkeyIndicatorBits()[level], node_num);
		if ((label == kTerminator) && !has_child_sparse)
		    ASSERT_TRUE(prefixkey_indicator);
		else
		    ASSERT_FALSE(prefixkey_indicator);
		prev_label = label;
		continue;
	    }

	    // label bitmap test
	    ASSERT_TRUE(exist_in_node);

	    // child indicator bitmap test
	    ASSERT_EQ(has_child_sparse, has_child_dense);

	    // label, child indicator bitmap zero bit test
	    if (is_node_start) {
		if (node_num > 0) {
		    for (unsigned c = prev_label + 1; c < kFanout; c++) {
			exist_in_node = SuRFBuilder::readBit(builder_->getBitmapLabels()[level], (node_num - 1) * kFanout + c);
			ASSERT_FALSE(exist_in_node);
			has_child_dense = SuRFBuilder::readBit(builder_->getBitmapChildIndicatorBits()[level], (node_num - 1) * kFanout + c);
			ASSERT_FALSE(has_child_dense);
		    }
		}
		for (unsigned c = 0; c < (unsigned)label; c++) {
		    exist_in_node = SuRFBuilder::readBit(builder_->getBitmapLabels()[level], node_num * kFanout + c);
		    ASSERT_FALSE(exist_in_node);
		    has_child_dense = SuRFBuilder::readBit(builder_->getBitmapChildIndicatorBits()[level], node_num * kFanout + c);
		    ASSERT_FALSE(has_child_dense);
		}
	    } else {
		for (unsigned c = prev_label + 1; c < (unsigned)label; c++) {
		    exist_in_node = SuRFBuilder::readBit(builder_->getBitmapLabels()[level], node_num * kFanout + c);
		    ASSERT_FALSE(exist_in_node);
		    has_child_dense = SuRFBuilder::readBit(builder_->getBitmapChildIndicatorBits()[level], node_num * kFanout + c);
		    ASSERT_FALSE(has_child_dense);
		}
	    }
	    prev_label = label;
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

} // namespace buildertest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::buildertest::loadWordList();
    return RUN_ALL_TESTS();
}
