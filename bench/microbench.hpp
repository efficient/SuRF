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

//#define USE_BLOOM 1

#ifdef USE_BLOOM
#include "bloom.hpp"
#else
#include "surf.hpp"
#endif

#define INIT_LIMIT 50000000
#define INIT_LIMIT_EMAIL 25000000
//#define INIT_LIMIT_EMAIL 100
#define LIMIT 10000000

//SuRF config
#define INCLUDE_SUFFIX_LEN 0
#define HUFFMAN_CONFIG true
#define LONGEST_KEY_LEN 129

//bloom filter config
#define BITS_PER_KEY 10

//random string config
#define STR_LEN_MIN 10
#define STR_LEN_MAX 10

using namespace std;

//Range query
static const uint64_t RANGE_SIZE = 328 * 1024 * 1024 * (uint64_t)1024;

static const char* GREEN="\033[0;32m";
static const char* RED="\033[0;31m";
static const char* NC="\033[0;0m";

static const char* DIR = "workloads/";

static const char ALPHA_NUM[] =
    "0123456789"
    "!@#$%^&*"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

//==============================================================
inline double get_now() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

//==============================================================
#ifndef USE_BLOOM
inline void printSuRFStat(SuRF* index) {
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
#endif

//==============================================================
// LOAD INIT
//==============================================================
inline void load_init(int wl, int limit, vector<uint64_t> &init_keys) {
    string init_file;

    init_file = DIR;
    // 0 = c, 1 = e
    if (wl == 0)
	init_file += "load_randint_workloadc";
    else if (wl == 1)
	init_file += "load_randint_workloadc_uniform";
    else if (wl == 2)
	init_file += "load_randint_workloade";
    else if (wl == 3)
	init_file += "load_randint_workloade_uniform";
    else
	init_file += "load_randint_workloadc";

    ifstream infile_load(init_file);

    string op;
    uint64_t key;

    string insert("INSERT");

    int count = 0;
    while ((count < limit) && infile_load.good()) {
	infile_load >> op >> key;
	if (op.compare(insert) != 0) {
	    cout << "READING LOAD FILE FAIL!\n";
	    return;
	}
	init_keys.push_back(key);
	count++;
    }

    sort(init_keys.begin(), init_keys.end());
}


//==============================================================
// LOAD TXN
//==============================================================
inline void load_txn(int wl, int limit, vector<uint64_t> &keys, vector<int> &ranges, vector<int> &ops) {
    string txn_file;

    txn_file = DIR;
    // 0 = c, 1 = e
    if (wl == 0)
	txn_file += "txn_randint_workloadc";
    else if (wl == 1)
	txn_file += "txn_randint_workloadc_uniform";
    else if (wl == 2)
	txn_file += "txn_randint_workloade";
    else if (wl == 3)
	txn_file += "txn_randint_workloade_uniform";
    else
	txn_file += "txn_randint_workloadc";

    ifstream infile_txn(txn_file);

    string op;
    uint64_t key;
    int range;

    string read("READ");
    string scan("SCAN");

    int count = 0;
    while ((count < limit) && infile_txn.good()) {
	infile_txn >> op >> key;
	if (op.compare(read) == 0) {
	    ops.push_back(1);
	    keys.push_back(key);
	}
	else if (op.compare(scan) == 0) {
	    infile_txn >> range;
	    ops.push_back(2);
	    keys.push_back(key);
	    ranges.push_back(range);
	}
	else {
	    cout << "UNRECOGNIZED CMD!\n";
	    return;
	}
	count++;
    }
}


//==============================================================
// LOAD INIT EMAIL
//==============================================================
inline void load_init_email(int wl, int limit, vector<string> &init_keys) {
    string init_file;

    init_file = DIR;
    // 0 = c, 1 = e
    if (wl == 0)
	init_file += "load_email_workloadc";
    else if (wl == 1)
	init_file += "load_email_workloadc_uniform";
    else if (wl == 2)
	init_file += "load_email_workloade";
    else if (wl == 3)
	init_file += "load_email_workloade_uniform";
    else
	init_file += "load_email_workloadc";

    ifstream infile_load(init_file);

    string op;
    string key;

    string insert("INSERT");

    int count = 0;
    while ((count < limit) && infile_load.good()) {
	infile_load >> op >> key;
	if (op.compare(insert) != 0) {
	    cout << "READING LOAD FILE FAIL!\n";
	    return;
	}
	init_keys.push_back(key);
	count++;
    }

    sort(init_keys.begin(), init_keys.end());
}

//==============================================================
// LOAD EMAIL
//==============================================================
inline void load_txn_email(int wl, int limit, vector<string> &keys, vector<int> &ranges, vector<int> &ops) {
    string txn_file;

    txn_file = DIR;
    // 0 = c, 1 = e
    if (wl == 0)
	txn_file += "txn_email_workloadc";
    else if (wl == 1)
	txn_file += "txn_email_workloadc_uniform";
    else if (wl == 2)
	txn_file += "txn_email_workloade";
    else if (wl == 3)
	txn_file += "txn_email_workloade_uniform";
    else
	txn_file += "txn_email_workloadc";

    ifstream infile_txn(txn_file);

    string op;
    string key;
    int range;

    string read("READ");
    string scan("SCAN");

    int count = 0;
    while ((count < limit) && infile_txn.good()) {
	infile_txn >> op >> key;
	if (op.compare(read) == 0) {
	    ops.push_back(1);
	    keys.push_back(key);
	}
	else if (op.compare(scan) == 0) {
	    infile_txn >> range;
	    ops.push_back(2);
	    keys.push_back(key);
	    ranges.push_back(range);
	}
	else {
	    cout << "UNRECOGNIZED CMD!\n";
	    return;
	}
	count++;
    }
}


//==============================================================
// LOAD TXN RANDOM INT
//==============================================================
inline void load_txn_randInt(int limit, vector<uint64_t> &keys) {
    random_device rd;
    mt19937_64 e2(rd());
    uniform_int_distribution<unsigned long long> dist(0, ULLONG_MAX);

    for (uint64_t i = 0; i < limit; i++) {
	uint64_t r = dist(e2);
	keys.push_back(r);
    }
}

//==============================================================
// LOAD TXN RANDOM STRING
//==============================================================
inline void load_txn_randStr(int limit, vector<string> &keys) {
    random_device rd;
    mt19937_64 str_len_gen(rd());
    uniform_int_distribution<unsigned int> dist_len(STR_LEN_MIN, STR_LEN_MAX);

    int charLen = sizeof(ALPHA_NUM) - 1;

    random_device rd2;
    mt19937_64 char_gen(rd2());
    uniform_int_distribution<unsigned int> dist_char(0, charLen);

    uint64_t strLen = 0;
    for (uint64_t i = 0; i < limit; i++) {
	string key;
	strLen = dist_len(str_len_gen);
	for (uint64_t j = 0; j < strLen; j++)
	    key += ALPHA_NUM[dist_char(char_gen)];
	keys.push_back(key);
    }
}
