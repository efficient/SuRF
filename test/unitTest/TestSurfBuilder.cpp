#include "gtest/gtest.h"

#include <string>
#include <fstream>
#include <vector>

#include "surfBuilder.hpp"

using namespace std;

namespace NS_SuRFBuilderUnitTest {

    static const string filePath = "../../../test/words.txt";
    static vector<string> words;

    class SuRFBuilderUnitTest : public ::testing::Test {
    public:
	virtual void SetUp () {
	    builder = new SuRFBuilder(SuffixType_Real);
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
	builder->buildVectors(words);
    }

    void loadWordList() {
	ifstream infile(filePath);
	string key;
	while (infile.good()) {
	    infile >> key;
	    words.push_back(key);
	}
    }

} //namespace

int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    NS_SuRFBuilderUnitTest::loadWordList();

    return RUN_ALL_TESTS();
}


