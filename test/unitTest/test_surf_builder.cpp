#include "gtest/gtest.h"

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace buildertest {

static const std::string file_path = "../../../test/words.txt";
static std::vector<std::string> words;

class SuRFBuilderUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	builder = new SuRFBuilder(kReal);
    }
    virtual void TearDown () {
	delete builder;
    }

    SuRFBuilder *builder;
};

TEST_F (SuRFBuilderUnitTest, EmptyTest) {
    ASSERT_TRUE(true);
}

TEST_F (SuRFBuilderUnitTest, buildWordListTest) {
    builder->build(words);
}

TEST_F (SuRFBuilderUnitTest, labelsTest) {
    builder->build(words);

    label_t prev_label, cur_label;
    for (level_t level = 0; level < builder->getSparseStartLevel(); level++) {
	position_t node_num = 0;
	prev_label = kTerminator;
	for (unsigned i = 0; i < words.size(); i++) {
	    cur_label = (label_t)words[i][level];
	    if (cur_label != prev_label) {
		bool exist_in_this_node = SuRFBuilder::readBit(builder->getBitmapLabels()[level], node_num * kFanout + cur_label);
		bool exist_in_next_node = SuRFBuilder::readBit(builder->getBitmapLabels()[level], (node_num + 1) * kFanout + cur_label);
		ASSERT_TRUE(exist_in_this_node || exist_in_next_node);

		if (exist_in_next_node)
		    node_num++;
	    }
	    prev_label = cur_label;
	}
    }

    for (level_t level = builder->getSparseStartLevel(); level < builder->getTreeHeight(); level++) {
	position_t pos = 0;
	prev_label =kTerminator;
	for (unsigned i = 0; i < words.size(); i++) {
	    cur_label = (label_t)words[i][level];
	    if (cur_label != prev_label) {
		ASSERT_TRUE(builder->getLabels()[level][pos] == cur_label);
		pos++;
	    }
	    prev_label = cur_label;
	}
    }
}

void loadWordList() {
    std::ifstream infile(file_path);
    std::string key;
    while (infile.good()) {
	infile >> key;
	words.push_back(key);
    }
}

} // namespace buildertest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    surf::buildertest::loadWordList();

    return RUN_ALL_TESTS();
}


