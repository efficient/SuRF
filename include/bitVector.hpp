#ifndef BITVECTOR_H_
#define BITVECTOR_H_

#include <assert.h>

using namespace std;

class BitVector {
protected:
    const int WORD_SIZE = 64;
    const uint64_t MSB_MASK = 0x8000000000000000;

public:
    BitVector() : numBits_(0), bits_(NULL) {};
    ~BitVector() {};

    void init(uint32_t numBits, uint64_t* addr) {
        numBits_ = numBits;
        bits_ = addr;
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

    // number of bits per level must be multiples of WORD_SIZE
    void load(vector<vector<uint64_t> > bitVectorPerLevel) {
        uint32_t wordId = 0;
        // combine bit-vectors at each trie level into one (consecutive) bit-vector
        for (uint32_t level = 0; level < bitVectorPerLevel.size(); level++) {
            for (uint32_t word = 0; word < bitVectorPerLevel[level].size(); word++) {
                bits_[wordId] = bitVectorPerLevel[level][word];
                wordId++;
            }
        }
    }

    void load(vector<vector<uint64_t> > bitVectorPerLevel, vector<uint32_t> numBitsPerLevel) {
        uint32_t bitShift = 0;
        uint32_t wordId = 0;
        // combine bit-vectors at each trie level into one (consecutive) bit-vector
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

    uint32_t size() {
        return numBits_ / 8;
    }

protected:
    uint32_t numBits_;
    uint64_t* bits_;
};

#endif // BITVECTOR_H_
