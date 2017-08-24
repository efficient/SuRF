#include "gtest/gtest.h"

#include <fstream>
#include <string>
#include <vector>

#include "config.hpp"
#include "surf_builder.hpp"

namespace NS_SuRFBuilderUnitTest {

    static const std::string file_path = "../../../test/words.txt";
    static std::vector<std::string> words;

    class SuRFBuilderUnitTest : public ::testing::Test {
    public:
	virtual void SetUp () {
	    builder = new surf::SuRFBuilder(surf::kReal);
	}
	virtual void TearDown () {
	    delete builder;
	}

	surf::SuRFBuilder *builder;
    };

    TEST_F (SuRFBuilderUnitTest, EmptyTest) {
	ASSERT_TRUE(true);
    }

    TEST_F (SuRFBuilderUnitTest, buildWordListTest) {
	builder->buildVectors(words);
    }

    void loadWordList() {
	std::ifstream infile(file_path);
	std::string key;
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


