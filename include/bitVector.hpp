#ifndef BITVECTOR_H_
#define BITVECTOR_H_

#include <assert.h>

#include "config.h"

using namespace std;

class BitVector {
public:
    BitVector() : numBits_(0), bits_(NULL) {};

    BitVector(vector<vector<uint64_t> > bitVectorPerLevel, vector<uint32_t> numBitsPerLevel) {
	numBits_ = computeTotalNumBits(numBitsPerLevel);
	bits_ = new uint64_t[numBits_/64 + 1];
	concatenateBitVectors(bitVectorPerLevel, numBitsPerLevel);
    }

    ~BitVector() {
	delete[] bits_;
    }

    void setBit(uint32_t pos) {
        assert(pos <= numBits_);
        uint32_t wordId = pos / WORD_SIZE;
        uint32_t offset = pos & (WORD_SIZE - 1);
        bits_[wordId] |= (MSB_MASK >> offset);
    }

    bool readBit(uint32_t pos) {
        assert(pos <= numBits_);
        uint32_t wordId = pos / WORD_SIZE;
        uint32_t offset = pos & (WORD_SIZE - 1);
        return bits_[wordId] & (MSB_MASK >> offset);
    }

    uint32_t size() {
        return numBits_ / 8;
    }

private:
    inline uint32_t computeTotalNumBits(vector<uint32_t> numBitsPerLevel) {
	for (uint32_t level = 0; level < numBitsPerLevel.size(); level++) {
	    numBits_ += numBitsPerLevel[level];
	}	
    }

    inline void concatenateBitVectors(vector<vector<uint64_t> > bitVectorPerLevel, vector<uint32_t> numBitsPerLevel) {
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
