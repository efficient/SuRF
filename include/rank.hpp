#ifndef RANK_H_
#define RANK_H_

#include <assert.h>
#include <vector>

#include "popcount.h"
#include "bitVector.hpp"

class BitVectorRank : public BitVector {
public:
    BitVectorRank() : basicBlockSize_(0), rankLut_(NULL) {};

    BitVectorRank(uint32_t basicBlockSize, vector<vector<uint64_t> > bitVectorPerLevel, vector<uint32_t> numBitsPerLevel) : BitVector(bitVectorPerLevel, numBitsPerLevel) {
	basicBlockSize_ = basicBlockSize;
	initRankLut();
    }

    ~BitVectorRank() {};

    uint32_t rank(uint32_t pos) {
        assert(pos <= numBits_);
        uint32_t wordPerBasicBlock = basicBlockSize_ / WORD_SIZE;
        uint32_t blockId = pos / basicBlockSize_;
        uint32_t offset = pos & (basicBlockSize_ - 1);
        return rankLut_[blockId] + popcountLinear(bits_, blockId * wordPerBasicBlock, offset);
    }

    uint32_t size() {
        uint32_t bitVectorMem = numBits_ / 8;
        uint32_t rankLutMem = numBits_ / basicBlockSize_ * sizeof(uint32_t);
        return bitVectorMem + rankLutMem;
    }

private:
    void initRankLut() {
        uint32_t wordPerBasicBlock = basicBlockSize_ / WORD_SIZE;
        uint32_t numBlocks = numBits_ / basicBlockSize_;
	rankLut_ = new uint32_t[numBlocks];

        uint32_t cumuRank = 0;
        for (uint32_t i = 0; i < numBlocks; i++) {
            rankLut_[i] = cumuRank;
            cumuRank += popcountLinear(bits_, i * wordPerBasicBlock, basicBlockSize_);
        }
    }

    uint32_t basicBlockSize_;
    uint32_t* rankLut_; //rank look-up table
};

#endif // RANK_H_
