#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <vector>
#include <fstream>
#include <iostream>
#include <utility>
#include <algorithm>
#include <random>
#include <climits>
#include <cstdlib>
#include <unordered_set>
#include <map>

namespace bench {

static const uint64_t kNumIntRecords = 100000000;
static const uint64_t kNumEmailRecords = 25000000;
static const uint64_t kNumTxns = 10000000;
static const uint64_t kIntRangeSize = 92233697311;
static const uint64_t kEmailRangeSize = 128;

//static const uint64_t kRandintRangeSize = 328 * 1024 * 1024 * (uint64_t)1024;
//static const char* kWordloadDir = "workloads/";

// for pretty print
static const char* kGreen ="\033[0;32m";
static const char* kRed ="\033[0;31m";
static const char* kNoColor ="\033[0;0m";

// for time measurement
double getNow() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

std::string uint64ToString(uint64_t key) {
    uint64_t endian_swapped_key = __builtin_bswap64(key);
    return std::string(reinterpret_cast<const char*>(&endian_swapped_key), 8);
}

uint64_t stringToUint64(std::string str_key) {
    uint64_t int_key = 0;
    memcpy(reinterpret_cast<char*>(&int_key), str_key.data(), 8);
    return __builtin_bswap64(int_key);
}

void loadKeysFromFile(const std::string& file_name, const bool is_key_int, 
		      std::vector<std::string> &keys) {
    std::ifstream infile(file_name);
    std::string key;
    uint64_t count = 0;
    if (is_key_int) {
	while (count < kNumIntRecords && infile.good()) {
	    uint64_t int_key;
	    infile >> int_key;
	    key = uint64ToString(int_key);
	    keys.push_back(key);
	    count++;
	}
    } else {
	while (count < kNumEmailRecords && infile.good()) {
	    infile >> key;
	    keys.push_back(key);
	    count++;
	}
    }
}

void loadKeysFromFile(const std::string& file_name, uint64_t num_records,
		      std::vector<uint64_t> &keys) {
    std::ifstream infile(file_name);
    uint64_t count = 0;
    while (count < num_records && infile.good()) {
	uint64_t key;
	infile >> key;
	keys.push_back(key);
	count++;
    }
}

// 0 < percent <= 100
void selectKeysToInsert(const unsigned percent, 
			std::vector<std::string> &insert_keys, 
			std::vector<std::string> &keys) {
    random_shuffle(keys.begin(), keys.end());
    uint64_t num_insert_keys = keys.size() * percent / 100;
    for (uint64_t i = 0; i < num_insert_keys; i++)
	insert_keys.push_back(keys[i]);

    keys.clear();
    sort(insert_keys.begin(), insert_keys.end());
}

// 0 < percent <= 100
void selectIntKeysToInsert(const unsigned percent, 
			   std::vector<uint64_t> &insert_keys, 
			   std::vector<uint64_t> &keys) {
    random_shuffle(keys.begin(), keys.end());
    uint64_t num_insert_keys = keys.size() * percent / 100;
    for (uint64_t i = 0; i < num_insert_keys; i++)
	insert_keys.push_back(keys[i]);

    keys.clear();
    sort(insert_keys.begin(), insert_keys.end());
}

// pos > 0, position counting from the last byte
void modifyKeyByte(std::vector<std::string> &keys, int pos) {
    for (int i = 0; i < (int)keys.size(); i++) {
	int keylen = keys[i].length();
	if (keylen > pos)
	    keys[i][keylen - 1 - pos] = '+';
	else
	    keys[i][0] = '+';
    } 
}

std::string getUpperBoundKey(const std::string& key_type, const std::string& key) {
    std::string ret_str = key;
    if (key_type.compare(std::string("email")) == 0) {
	ret_str[ret_str.size() - 1] += (char)kEmailRangeSize;
    } else {
	uint64_t int_key = stringToUint64(key);
	int_key += kIntRangeSize;
	ret_str = uint64ToString(int_key);
    }
    return ret_str;
}

} // namespace bench
