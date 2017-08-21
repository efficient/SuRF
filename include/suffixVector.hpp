#ifndef SUFFIXVECTOR_H_
#define SUFFIXVECTOR_H_

#include <assert.h>

#include "config.hpp"
#include "hash.h"

using namespace std;

//Current version only supports one-byte suffixes
class SuffixVector {
public:
    SuffixVector() : type_(SuffixType_None), numBytes_(0), suffixes_(NULL) {};

    SuffixVector(const SuffixType type, const vector<vector<uint8_t> > &suffixesPerLevel) {
	type_ = type;

	numBytes_ = 0;
	for (int level = 0; level < suffixesPerLevel.size(); level++)
	    numBytes_ += suffixesPerLevel[level].size();

	suffixes_ = new uint8_t[numBytes_];

	position_t pos = 0;
	for (int level = 0; level < suffixesPerLevel.size(); level++) {
	    for (int idx = 0; idx < suffixesPerLevel[level].size(); idx++) {
		suffixes_[pos] = suffixesPerLevel[level][idx];
		pos++;
	    }
	}
    }

    ~SuffixVector() {
	delete[] suffixes_;
    }

    bool checkEquality(const position_t pos, const string &key, const uint32_t level) const {
	assert(pos < numBytes_);
	switch (type_) {
	case SuffixType_Hash: {
	    suffix_t h = suffixHash(key) & SUFFIX_HASH_MASK;
	    if (h == suffixes_[pos])
		return true;
	    return false;
	}
	case SuffixType_Real: {
	    if (suffixes_[pos] == 0) //0 means no suffix stored
		return true;
	    assert(level < key.length());
	    if (key[level] == suffixes_[pos])
		return true;
	    return false;
	}
	default:
	    return false;
	}

    }

    position_t size() const {
	return (sizeof(SuffixVector) + numBytes_);
    }

private:
    SuffixType type_;
    position_t numBytes_;
    suffix_t* suffixes_;
};

#endif
