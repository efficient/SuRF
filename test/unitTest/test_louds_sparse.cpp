#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "louds_sparse.hpp"
#include "surf_builder.hpp"

namespace surf {

namespace sparsetest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class SparseUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = false;
	builder_ = new SuRFBuilder(include_dense, kReal);
	truncateWordSuffixes();
    }
    virtual void TearDown () {
	delete builder_;
    }

    void truncateWordSuffixes();

    SuRFBuilder* builder_;
    LoudsSparse* louds_sparse_;
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

void SparseUnitTest::truncateWordSuffixes() {
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

TEST_F (SparseUnitTest, lookupTest) {
    builder_->build(words);
    louds_sparse_ = new LoudsSparse(builder_);
    position_t in_node_num = 0;

    for (unsigned i = 0; i < words.size(); i++) {
	bool key_exist = louds_sparse_->lookupKey(words[i], in_node_num);
	ASSERT_TRUE(key_exist);
    }

    for (unsigned i = 0; i < words.size(); i++) {
	for (unsigned j = 0; j < words_trunc_[i].size() && j < words[i].size(); j++) {
	    std::string key = words[i];
	    key[j] = 'A';
	    bool key_exist = louds_sparse_->lookupKey(key, in_node_num);
	    ASSERT_FALSE(key_exist);
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

} // namespace sparsetest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::sparsetest::loadWordList();
    return RUN_ALL_TESTS();
}
