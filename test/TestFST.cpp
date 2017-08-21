//************************************************
// SuRF unit tests
//************************************************
#include "gtest/gtest.h"
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <fstream>
#include <algorithm>

#include "surf.hpp"

#define TEST_SIZE 234369
#define TEST_SIZE_INT 1000000
#define RANGE_SIZE 10

#define INCLUDE_SUFFIX_LEN 2
#define HUF_CONFIG true

using namespace std;

const string testFilePath = "../../test/testStringKeys.txt";

class SuRFUnitTest : public ::testing::Test {
public:
    virtual void SetUp () { }
    virtual void TearDown () { }
};

inline double get_now() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

inline void printStatSuRF(SuRF* index) {
    cout << "mem = " << index->mem() << "\n";

    cout << "cMemU = " << index->cMemU() << "\n";
    cout << "tMemU = " << index->tMemU() << "\n";
    cout << "oMemU = " << index->oMemU() << "\n";
    cout << "suffixMemU = " << index->suffixMemU() << "\n";

    cout << "cMem = " << index->cMem() << "\n";
    cout << "tMem = " << index->tMem() << "\n";
    cout << "sMem = " << index->sMem() << "\n";
    cout << "suffixMem = " << index->suffixMem() << "\n";
}

inline int loadFile (string filePath, vector<string> &keys) {
    ifstream infile(filePath);
    string op;
    string key;
    uint64_t count = 0;
    int longestKeyLen = 0;
    while (count < TEST_SIZE && infile.good()) {
	infile >> key; //subject to change
	keys.push_back(key);
	if (key.length() > longestKeyLen)
	    longestKeyLen = key.length();
	count++;
    }

    return longestKeyLen;
}

inline int loadMonoInt (vector<uint64_t> &keys) {
    for (uint64_t i = 0; i < TEST_SIZE_INT; i++)
	keys.push_back(i);
    return sizeof(uint64_t);
}

inline int loadMonoSkipInt (vector<uint64_t> &keys) {
    uint64_t skip = 10;
    for (uint64_t i = skip - 1; i < (TEST_SIZE_INT * skip); i += skip)
	keys.push_back(i);
    return sizeof(uint64_t);
}

inline int loadRandInt (vector<uint64_t> &keys) {
    srand(0);
    for (uint64_t i = 0; i < TEST_SIZE_INT; i++) {
	uint64_t r = rand();
	keys.push_back(r);
    }
    sort(keys.begin(), keys.end());
    return sizeof(uint64_t);
}


//*****************************************************************
// SuRF TESTS
//*****************************************************************

TEST_F(SuRFUnitTest, LookupTest) {
    vector<string> keys;
    int longestKeyLen = loadFile(testFilePath, keys);

    //double start_time = get_now();
    SuRF *index = SuRF::buildSuRF(keys, longestKeyLen, INCLUDE_SUFFIX_LEN, HUF_CONFIG);
    //double end_time = get_now();
    //double tput = TEST_SIZE / (end_time - start_time);
    //cout << tput << "\n";

    printStatSuRF(index);

    //index->printU();
    //index->print();

    //start_time = get_now();
    for (int i = 0; i < TEST_SIZE; i++) {
    //for (int i = 27; i < 28; i++) {
	if (i > 0 && keys[i].compare(keys[i-1]) == 0)
	    continue;
	//ASSERT_TRUE(index->lookup((uint8_t*)keys[i].c_str(), keys[i].length()));
	ASSERT_TRUE(index->lookup(keys[i]));
    }
    //end_time = get_now();
    //tput = TEST_SIZE / (end_time - start_time);
    //cout << tput << "\n";

    index->destroy();
}

TEST_F(SuRFUnitTest, LookupMonoIntTest) {
    vector<uint64_t> keys;
    int longestKeyLen = loadMonoInt(keys);

    SuRF *index = SuRF::buildSuRF(keys, INCLUDE_SUFFIX_LEN);

    printStatSuRF(index);

    for (uint64_t i = 0; i < TEST_SIZE_INT; i++) {
	ASSERT_TRUE(index->lookup(keys[i]));
    }

    index->destroy();
}


TEST_F(SuRFUnitTest, LookupRandIntTest) {
    vector<uint64_t> keys;
    int longestKeyLen = loadRandInt(keys);

    SuRF *index = SuRF::buildSuRF(keys, INCLUDE_SUFFIX_LEN);

    printStatSuRF(index);

    random_shuffle(keys.begin(), keys.end());

    for (uint64_t i = 0; i < TEST_SIZE_INT; i++) {
	ASSERT_TRUE(index->lookup(keys[i]));
    }

    index->destroy();
}

TEST_F(SuRFUnitTest, LowerBoundTest) {
    vector<uint64_t> keys;
    int longestKeyLen = loadMonoSkipInt(keys);

    SuRF *index = SuRF::buildSuRF(keys, INCLUDE_SUFFIX_LEN);

    printStatSuRF(index);

    string curkey;
    char* key_str = new char[8];
    string keyString;
    SuRFIter iter(index);
    for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
	ASSERT_TRUE(index->lowerBound(keys[i] - 1, iter));
	curkey = iter.key();
	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
	keyString = string(key_str, 8);
	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
	ASSERT_TRUE(res.first == curkey.end());

	for (int j = 0; j < RANGE_SIZE; j++) {
	    if (i+j+1 < TEST_SIZE_INT) {
		ASSERT_TRUE(iter++);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i+j+1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	    else {
		ASSERT_FALSE(iter++);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[TEST_SIZE_INT-1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	}
    }

    index->destroy();
}

TEST_F(SuRFUnitTest, UpperBoundTest) {
    vector<uint64_t> keys;
    int longestKeyLen = loadMonoSkipInt(keys);

    SuRF *index = SuRF::buildSuRF(keys, INCLUDE_SUFFIX_LEN);

    printStatSuRF(index);

    string curkey;
    char* key_str = new char[8];
    string keyString;
    SuRFIter iter(index);
    for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
 	ASSERT_TRUE(index->upperBound(keys[i] + 1, iter));
	curkey = iter.key();
	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
	keyString = string(key_str, 8);
	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
	ASSERT_TRUE(res.first == curkey.end());

	for (int j = 0; j < RANGE_SIZE; j++) {
	    if (i+j+1 < TEST_SIZE_INT) {
		ASSERT_TRUE(iter++);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i+j+1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	    else {
		ASSERT_FALSE(iter++);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[TEST_SIZE_INT-1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	}
    }

    index->destroy();
}


TEST_F(SuRFUnitTest, ScanTest) {
    vector<string> keys;
    int longestKeyLen = loadFile(testFilePath, keys);

    SuRF *index = SuRF::buildSuRF(keys, longestKeyLen, INCLUDE_SUFFIX_LEN, HUF_CONFIG);

    printStatSuRF(index);

    string curkey;
    SuRFIter iter(index);
    for (int i = 0; i < TEST_SIZE - 1; i++) {
	if (i > 0 && keys[i].compare(keys[i-1]) == 0)
	    continue;
	//ASSERT_TRUE(index->lowerBound((uint8_t*)keys[i].c_str(), keys[i].length(), iter));
	ASSERT_TRUE(index->lowerBound(keys[i], iter));
	//curkey = iter.key();
	//auto res = mismatch(curkey.begin(), curkey.end(), keys[i].begin());
	//ASSERT_TRUE(res.first == curkey.end());

	for (int j = 0; j < RANGE_SIZE; j++) {
	    if (i+j+1 < TEST_SIZE) {
		ASSERT_TRUE(iter++);
		//curkey = iter.key();
		//res = mismatch(curkey.begin(), curkey.end(), keys[i+j+1].begin());
		//ASSERT_TRUE(res.first == curkey.end());
	    }
	    else {
		ASSERT_FALSE(iter++);
		//curkey = iter.key();
		//res = mismatch(curkey.begin(), curkey.end(), keys[TEST_SIZE-1].begin());
		//ASSERT_TRUE(res.first == curkey.end());
	    }
	}
    }

    index->destroy();
}


TEST_F(SuRFUnitTest, ScanMonoIntTest) {
    vector<uint64_t> keys;
    int longestKeyLen = loadMonoInt(keys);

    SuRF *index = SuRF::buildSuRF(keys, INCLUDE_SUFFIX_LEN);

    printStatSuRF(index);

    string curkey;
    char* key_str = new char[8];
    string keyString;
    SuRFIter iter(index);
    for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
	ASSERT_TRUE(index->lowerBound(keys[i], iter));
	curkey = iter.key();
	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
	keyString = string(key_str, 8);
	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
	ASSERT_TRUE(res.first == curkey.end());

	for (int j = 0; j < RANGE_SIZE; j++) {
	    if (i+j+1 < TEST_SIZE_INT) {
		ASSERT_TRUE(iter++);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i+j+1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	    else {
		ASSERT_FALSE(iter++);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[TEST_SIZE_INT-1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	}
    }

    index->destroy();
}

TEST_F(SuRFUnitTest, ScanReverseTest) {
    vector<string> keys;
    int longestKeyLen = loadFile(testFilePath, keys);

    SuRF *index = SuRF::buildSuRF(keys, longestKeyLen, INCLUDE_SUFFIX_LEN, HUF_CONFIG);

    printStatSuRF(index);

    string curkey;
    SuRFIter iter(index);
    for (int i = 0; i < TEST_SIZE - 1; i++) {
	if (i > 0 && keys[i].compare(keys[i-1]) == 0)
	    continue;

	//ASSERT_TRUE(index->upperBound((uint8_t*)keys[i].c_str(), keys[i].length(), iter));
	ASSERT_TRUE(index->upperBound(keys[i], iter));
	//curkey = iter.key();
	//auto res = mismatch(curkey.begin(), curkey.end(), keys[i].begin());
	//ASSERT_TRUE(res.first == curkey.end());

	for (int j = 0; j < RANGE_SIZE; j++) {
	    if (i-j-1 >= 0) {
		ASSERT_TRUE(iter--);
		//curkey = iter.key();
		//res = mismatch(curkey.begin(), curkey.end(), keys[i-j-1].begin());
		//ASSERT_TRUE(res.first == curkey.end());
	    }
	    else {
		ASSERT_FALSE(iter--);
		//curkey = iter.key();
		//res = mismatch(curkey.begin(), curkey.end(), keys[0].begin());
		//ASSERT_TRUE(res.first == curkey.end());
	    }
	}
    }

    index->destroy();
}

TEST_F(SuRFUnitTest, ScanMonoIntReverseTest) {
    vector<uint64_t> keys;
    int longestKeyLen = loadMonoInt(keys);

    SuRF *index = SuRF::buildSuRF(keys, INCLUDE_SUFFIX_LEN);

    printStatSuRF(index);

    string curkey;
    char* key_str = new char[8];
    string keyString;
    SuRFIter iter(index);
    for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
	ASSERT_TRUE(index->upperBound(keys[i], iter));
	curkey = iter.key();
	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
	keyString = string(key_str, 8);
	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
	ASSERT_TRUE(res.first == curkey.end());

	for (int j = 0; j < RANGE_SIZE; j++) {
	    if (i-j-1 >= 0) {
		ASSERT_TRUE(iter--);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i-j-1]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	    else {
		ASSERT_FALSE(iter--);
		curkey = iter.key();
		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[0]);
		keyString = string(key_str, 8);
		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
		ASSERT_TRUE(res.first == curkey.end());
	    }
	}
    }

    index->destroy();
}

// TEST_F(UnitTest, LookupTest) {
//     vector<string> keys;
//     int longestKeyLen = loadFile(testFilePath, keys);

//     //double start_time = get_now();
//     SuRF *index = load(keys, longestKeyLen, INCLUDE_SUFFIX_LEN, HUF_CONFIG);
//     //double end_time = get_now();
//     //double tput = TEST_SIZE / (end_time - start_time);
//     //cout << tput << "\n";

//     printStatSuRF(index);

//     //index->printU();
//     //index->print();

//     //start_time = get_now();
//     for (int i = 0; i < TEST_SIZE; i++) {
//     //for (int i = 27; i < 28; i++) {
// 	if (i > 0 && keys[i].compare(keys[i-1]) == 0)
// 	    continue;
// 	//ASSERT_TRUE(index->lookup((uint8_t*)keys[i].c_str(), keys[i].length()));
// 	ASSERT_TRUE(index->lookup(keys[i]));
//     }
//     //end_time = get_now();
//     //tput = TEST_SIZE / (end_time - start_time);
//     //cout << tput << "\n";

//     delete index;
// }


// TEST_F(UnitTest, LookupMonoIntTest) {
//     vector<uint64_t> keys;
//     int longestKeyLen = loadMonoInt(keys);

//     SuRF *index = load(keys, INCLUDE_SUFFIX_LEN);

//     printStatSuRF(index);

//     for (uint64_t i = 0; i < TEST_SIZE_INT; i++) {
// 	ASSERT_TRUE(index->lookup(keys[i]));
//     }
// }


// TEST_F(UnitTest, LookupRandIntTest) {
//     vector<uint64_t> keys;
//     int longestKeyLen = loadRandInt(keys);

//     SuRF *index = load(keys, INCLUDE_SUFFIX_LEN);

//     printStatSuRF(index);

//     random_shuffle(keys.begin(), keys.end());

//     for (uint64_t i = 0; i < TEST_SIZE_INT; i++) {
// 	ASSERT_TRUE(index->lookup(keys[i]));
//     }
// }

// TEST_F(UnitTest, LowerBoundTest) {
//     vector<uint64_t> keys;
//     int longestKeyLen = loadMonoSkipInt(keys);

//     SuRF *index = load(keys, INCLUDE_SUFFIX_LEN);

//     printStatSuRF(index);

//     string curkey;
//     char* key_str = new char[8];
//     string keyString;
//     SuRFIter iter(index);
//     for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
// 	ASSERT_TRUE(index->lowerBound(keys[i] - 1, iter));
// 	curkey = iter.key();
// 	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
// 	keyString = string(key_str, 8);
// 	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 	ASSERT_TRUE(res.first == curkey.end());

// 	for (int j = 0; j < RANGE_SIZE; j++) {
// 	    if (i+j+1 < TEST_SIZE_INT) {
// 		ASSERT_TRUE(iter++);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i+j+1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	    else {
// 		ASSERT_FALSE(iter++);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[TEST_SIZE_INT-1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	}
//     }
// }

// TEST_F(UnitTest, UpperBoundTest) {
//     vector<uint64_t> keys;
//     int longestKeyLen = loadMonoSkipInt(keys);

//     SuRF *index = load(keys, INCLUDE_SUFFIX_LEN);

//     printStatSuRF(index);

//     string curkey;
//     char* key_str = new char[8];
//     string keyString;
//     SuRFIter iter(index);
//     for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
//  	ASSERT_TRUE(index->upperBound(keys[i] + 1, iter));
// 	curkey = iter.key();
// 	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
// 	keyString = string(key_str, 8);
// 	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 	ASSERT_TRUE(res.first == curkey.end());

// 	for (int j = 0; j < RANGE_SIZE; j++) {
// 	    if (i+j+1 < TEST_SIZE_INT) {
// 		ASSERT_TRUE(iter++);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i+j+1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	    else {
// 		ASSERT_FALSE(iter++);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[TEST_SIZE_INT-1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	}
//     }
// }


// TEST_F(UnitTest, ScanTest) {
//     vector<string> keys;
//     int longestKeyLen = loadFile(testFilePath, keys);

//     SuRF *index = load(keys, longestKeyLen, INCLUDE_SUFFIX_LEN, HUF_CONFIG);

//     printStatSuRF(index);

//     string curkey;
//     SuRFIter iter(index);
//     for (int i = 0; i < TEST_SIZE - 1; i++) {
// 	if (i > 0 && keys[i].compare(keys[i-1]) == 0)
// 	    continue;
// 	//ASSERT_TRUE(index->lowerBound((uint8_t*)keys[i].c_str(), keys[i].length(), iter));
// 	ASSERT_TRUE(index->lowerBound(keys[i], iter));
// 	//curkey = iter.key();
// 	//auto res = mismatch(curkey.begin(), curkey.end(), keys[i].begin());
// 	//ASSERT_TRUE(res.first == curkey.end());

// 	for (int j = 0; j < RANGE_SIZE; j++) {
// 	    if (i+j+1 < TEST_SIZE) {
// 		ASSERT_TRUE(iter++);
// 		//curkey = iter.key();
// 		//res = mismatch(curkey.begin(), curkey.end(), keys[i+j+1].begin());
// 		//ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	    else {
// 		ASSERT_FALSE(iter++);
// 		//curkey = iter.key();
// 		//res = mismatch(curkey.begin(), curkey.end(), keys[TEST_SIZE-1].begin());
// 		//ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	}
//     }
// }


// TEST_F(UnitTest, ScanMonoIntTest) {
//     vector<uint64_t> keys;
//     int longestKeyLen = loadMonoInt(keys);

//     SuRF *index = load(keys, INCLUDE_SUFFIX_LEN);

//     printStatSuRF(index);

//     string curkey;
//     char* key_str = new char[8];
//     string keyString;
//     SuRFIter iter(index);
//     for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
// 	ASSERT_TRUE(index->lowerBound(keys[i], iter));
// 	curkey = iter.key();
// 	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
// 	keyString = string(key_str, 8);
// 	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 	ASSERT_TRUE(res.first == curkey.end());

// 	for (int j = 0; j < RANGE_SIZE; j++) {
// 	    if (i+j+1 < TEST_SIZE_INT) {
// 		ASSERT_TRUE(iter++);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i+j+1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	    else {
// 		ASSERT_FALSE(iter++);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[TEST_SIZE_INT-1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	}
//     }
// }

// TEST_F(UnitTest, ScanReverseTest) {
//     vector<string> keys;
//     int longestKeyLen = loadFile(testFilePath, keys);

//     SuRF *index = load(keys, longestKeyLen, INCLUDE_SUFFIX_LEN, HUF_CONFIG);

//     printStatSuRF(index);

//     string curkey;
//     SuRFIter iter(index);
//     for (int i = 0; i < TEST_SIZE - 1; i++) {
// 	if (i > 0 && keys[i].compare(keys[i-1]) == 0)
// 	    continue;

// 	//ASSERT_TRUE(index->upperBound((uint8_t*)keys[i].c_str(), keys[i].length(), iter));
// 	ASSERT_TRUE(index->upperBound(keys[i], iter));
// 	//curkey = iter.key();
// 	//auto res = mismatch(curkey.begin(), curkey.end(), keys[i].begin());
// 	//ASSERT_TRUE(res.first == curkey.end());

// 	for (int j = 0; j < RANGE_SIZE; j++) {
// 	    if (i-j-1 >= 0) {
// 		ASSERT_TRUE(iter--);
// 		//curkey = iter.key();
// 		//res = mismatch(curkey.begin(), curkey.end(), keys[i-j-1].begin());
// 		//ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	    else {
// 		ASSERT_FALSE(iter--);
// 		//curkey = iter.key();
// 		//res = mismatch(curkey.begin(), curkey.end(), keys[0].begin());
// 		//ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	}
//     }
// }

// TEST_F(UnitTest, ScanMonoIntReverseTest) {
//     vector<uint64_t> keys;
//     int longestKeyLen = loadMonoInt(keys);

//     SuRF *index = load(keys, INCLUDE_SUFFIX_LEN);

//     printStatSuRF(index);

//     string curkey;
//     char* key_str = new char[8];
//     string keyString;
//     SuRFIter iter(index);
//     for (int i = 0; i < TEST_SIZE_INT - 1; i++) {
// 	ASSERT_TRUE(index->upperBound(keys[i], iter));
// 	curkey = iter.key();
// 	reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i]);
// 	keyString = string(key_str, 8);
// 	auto res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 	ASSERT_TRUE(res.first == curkey.end());

// 	for (int j = 0; j < RANGE_SIZE; j++) {
// 	    if (i-j-1 >= 0) {
// 		ASSERT_TRUE(iter--);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[i-j-1]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	    else {
// 		ASSERT_FALSE(iter--);
// 		curkey = iter.key();
// 		reinterpret_cast<uint64_t*>(key_str)[0]=__builtin_bswap64(keys[0]);
// 		keyString = string(key_str, 8);
// 		res = mismatch(curkey.begin(), curkey.end(), keyString.begin());
// 		ASSERT_TRUE(res.first == curkey.end());
// 	    }
// 	}
//     }
// }


int main (int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
