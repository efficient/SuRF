#include "gtest/gtest.h"

#include <assert.h>

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "surf.hpp"

namespace surf {

namespace surftest {

static const std::string kFilePath = "../../../test/words.txt";
static const int kTestSize = 234369;
static std::vector<std::string> words;

class SuRFUnitTest : public ::testing::Test {
public:
    virtual void SetUp () {
	bool include_dense = true;
	uint32_t sparse_dense_ratio = 0;
	truncateWordSuffixes();
    }
    virtual void TearDown () {
	delete surf_;
    }

    void truncateWordSuffixes();

    SuRF* surf_;
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

void SuRFUnitTest::truncateWordSuffixes() {
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

TEST_F (SuRFUnitTest, lookupTest) {
    surf_ = new SuRF(words, kIncludeDense, kSparseDenseRatio, kReal);

    for (unsigned i = 0; i < words.size(); i++) {
	bool key_exist = surf_->lookupKey(words[i]);

	if (!key_exist) {
	    std::cout << i << ": " << words[i] << "\n";
	}

	ASSERT_TRUE(key_exist);
    }

    for (unsigned i = 0; i < words.size(); i++) {
	for (unsigned j = 0; j < words_trunc_[i].size() && j < words[i].size(); j++) {
	    std::string key = words[i];
	    key[j] = 'A';
	    bool key_exist = surf_->lookupKey(key);
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

} // namespace surftest

} // namespace surf

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    surf::surftest::loadWordList();
    return RUN_ALL_TESTS();
}
