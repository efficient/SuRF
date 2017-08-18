#ifndef LABELVECTOR_H_
#define LABELVECTOR_H_

#include <assert.h>
#include <emmintrin.h>

#include "config.hpp"

using namespace std;

class LabelVector {
public:
    LabelVector() : numBytes_(0), labels_(NULL) {};

    LabelVector(const vector<vector<uint8_t> > &labelsPerLevel) {
	numBytes_ = 0;
	for (int level = 0; level < labelsPerLevel.size(); level++)
	    numBytes += labelsPerLevel[level].size();

	labels_ = new uint8_t[numBytes];

	position_t pos = 0;
	for (int level = 0; level < labelsPerLevel.size(); level++) {
	    for (int idx = 0; idx < labelsPerLevel[level].size(); idx++) {
		labels_[pos] = labelsPerLevel[level][idx];
		pos++;
	    }
	}
    }

    ~LabelVector() {
	delete[] labels_;
    }

    position_t size() const {
	return (sizeof(LabelVector) + numBytes_);
    }

    uint8_t read (const position_t pos) const {
	return labels_[pos];
    }

    uint8_t operator[] (const position_t pos) const {
	return labels_[pos];
    }

    bool search(const uint8_t target, position_t &pos, const position_t searchLen) const;

private:
    bool binarySearch(const uint8_t target, position_t &pos, const position_t searchLen) const;
    bool simdSearch(const uint8_t target, position_t &pos, const position_t searchLen) const;
    bool linearSearch(const uint8_t target, position_t &pos, const position_t searchLen) const;

private:
    position_t numBytes_;
    uint8_t* labels_;
};

//TODO: need unit test
bool LabelVector::binarySearch(const uint8_t target, position_t &pos, const position_t searchLen) const {
    position_t l = pos;
    position_t r = pos + searchLen;
    while (l < r) {
	position_t m = (l + r) >> 1;
	if (target < labels_[m])
	    r = m;
	else if (target == labels_[m]) {
	    pos = m;
	    return true;
	}
	else
	    l = m + 1;
    }
    return false;
}

//TODO: need unit test
bool LabelVector::simdSearch(const uint8_t target, position_t &pos, const position_t searchLen) const {
    position_t numLabelsSearched = 0;
    position_t numLabelsLeft = searchLen;
    while ((numLabelsLeft >> 4) > 0) {
	uint8_t* startPtr = labels_ + pos + numLabelsSearched;
	__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(target), 
				     _mm_loadu_si128(reinterpret_case<__m128i*>(startPtr)));
	unsigned checkBits = _mm_movemask_epi8(cmp);
	if (checkBits) {
	    pos += (numLabelsSearched + __builtin_ctz(checkBits));
	    return true;
	}
	numLabelsSearched += 16;
	numLabelsLeft -= 16;
    }

    if (numLabelsLeft > 0) {
	uint8_t* startPtr = labels_ + pos + numLabelsSearched;
	__m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(target), 
				     _mm_loadu_si128(reinterpret_case<__m128i*>(startPtr)));
	unsigned leftoverBitsMask = (1 << numLabelsLeft) - 1;
	unsigned checkBits = _mm_movemask_epi8(cmp) & leftoverBitsMask;
	if (checkBits) {
	    pos += (numLabelsSearched + __builtin_ctz(checkBits));
	    return true;
	}
    }

    return false;
}

//TODO: need unit test
bool LabelVector::linearSearch(const uint8_t target, position_t &pos, const position_t searchLen) const {
    for (position_t i = 0; i < searchLen; i++) {
	if (target == labels_[pos + i]) {
	    pos += i;
	    return true;
	}
    }
    return false;
}

//TODO: need performance test
bool LabelVector::search(const uint8_t target, position_t &pos, position_t searchLen) const {
    //skip terminator label
    if ((searchLen > 1) && (labels_[pos] == TERMINATOR)) {
	pos++;
	searchLen--;
    }

    //TODO: need new test data
    if (searchLen < 3)
	return linearSearch(target, pos, searchLen);
    if (searchLen < 12)
	return binarySearch(target, pos, searchLen);
    else
	return simdSearch(target, pos, searchLen);
}

#endif
