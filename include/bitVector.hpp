#ifndef BITVECTOR_H_
#define BITVECTOR_H_

#include <assert.h>

#include "config.hpp"

using namespace std;

class BitVector {
public:
    BitVector() : numBits_(0), bits_(NULL) {};

    BitVector(const vector<vector<uint64_t> > &bitVectorPerLevel, const vector<uint32_t> &numBitsPerLevel) {
	numBits_ = computeTotalNumBits(numBitsPerLevel);
	bits_ = new uint64_t[numBits_/64 + 1];
	concatenateBitVectors(bitVectorPerLevel, numBitsPerLevel);
    }

    ~BitVector() {
	delete[] bits_;
    }

    uint32_t size() const {
        return (sizeof(BitVector) + (numBits_ >> 3));
    }

    uint32_t numWords() const {
	return (numBits_ >> 6);
    }

    //TODO: maybe not necessary
    void setBit(const uint32_t pos) {
        assert(pos <= numBits_);
        uint32_t wordId = pos / WORD_SIZE;
        uint32_t offset = pos & (WORD_SIZE - 1);
        bits_[wordId] |= (MSB_MASK >> offset);
    }

    bool readBit (const uint32_t pos) const {
        assert(pos <= numBits_);
        uint32_t wordId = pos / WORD_SIZE;
        uint32_t offset = pos & (WORD_SIZE - 1);
        return bits_[wordId] & (MSB_MASK >> offset);
    }

    bool operator[] (const uint32_t pos) const {
        assert(pos <= numBits_);
        uint32_t wordId = pos / WORD_SIZE;
        uint32_t offset = pos & (WORD_SIZE - 1);
        return bits_[wordId] & (MSB_MASK >> offset);
    }

    uint32_t distanceToNextOne(const uint32_t pos) const {
	assert(pos <= numBits_);
	uint32_t distance = 1;

	uint64_t wordId = (pos + 1) / 64;
	if (wordId >= numWords()) return distance;
	uint64_t offset = (pos + 1) % 64;

	//first word left-over bits
	uint64_t testBits = bits_[wordId] << offset;
	if (testBits > 0)
	    return (distance + __builtin_clzll(testBits));
	else
	    distance += (64 - offset);

	while (true) {
	    wordId++;
	    if (wordId >= numWords()) return distance;
	    testBits = bits_[wordId];
	    if (testBits > 0)
		return (distance + __builtin_clzll(testBits));
	    distance += 64;
	}
    }

private:
    uint32_t computeTotalNumBits(vector<uint32_t> numBitsPerLevel) {
	for (uint32_t level = 0; level < numBitsPerLevel.size(); level++) {
	    numBits_ += numBitsPerLevel[level];
	}	
    }

    void concatenateBitVectors(vector<vector<uint64_t> > bitVectorPerLevel, vector<uint32_t> numBitsPerLevel) {
        uint32_t bitShift = 0;
        uint32_t wordId = 0;
        for (uint32_t level = 0; level < bitVectorPerLevel.size(); level++) {
            uint32_t numCompleteWords = numBitsPerLevel[level] / WORD_SIZE;
            for (uint32_t word = 0; word < numCompleteWords; word++) {
                bits_[wordId] |= (bitVectorPerLevel[level][word] >> bitShift);
                wordId++;
                bits_[wordId] |= (bitVectorPerLevel[level][word] << (WORD_SIZE - bitShift));
            }
            uint64_t lastWord = bitVectorPerLevel[level][numCompleteWords];
            uint64_t bitsRemain = numBitsPerLevel[level] - numCompleteWords * WORD_SIZE;
            bits_[wordId] |= (lastWord >> bitShift);
            if (bitShift + bitsRemain < WORD_SIZE) {
                bitShift += bitsRemain;
            }
            else {
                wordId++;
                bits_[wordId] |= (lastWord << (WORD_SIZE - bitShift));
                bitShift = bitShift + bitsRemain - WORD_SIZE;
            }
        }
    }

protected:
    uint32_t numBits_;
    uint64_t* bits_;
};

#endif // BITVECTOR_H_
