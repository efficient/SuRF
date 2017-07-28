#ifdef SELECT_H_
#define SELECT_H_

#include <assert.h>
#include <vector>

#include "popcount.h"
#include "bitVector.hpp"

class BitVectorSelect : public BitVector {
public:
    BitVectorSelect() : basicBlockSize_(0), lut_(NULL) {};
    ~BitVectorSelect() {};

    void init(uint32_t numBits, uint32_t basicBlockSize, void* addr) {
        basicBlockSize_ = basicBlockSize;
        lut_ = (uint32_t*)addr;
        uint32_t numBlocks = numBits / basicBlockSize_;
        BitVector::init(numBits, (uint64_t*)(lut_ + numBlocks));
    }

    void initSelectLut() {
        uint32_t numBlocks = numBits_ / basicBlockSize_;
        uint32_t wordPerBasicBlock = basicBlockSize_ / WORD_SIZE;
        uint32_t cumuRank = 0;
        for (uint32_t i = 0; i < numBlocks; i++) {
            lut_[i] = cumuRank;
            cumuRank += popcountLinear(bits_, i * wordPerBasicBlock, basicBlockSize_);
        }
    }

    uint32_t rank(uint32_t pos) {
        assert(pos <= numBits_);
        uint32_t wordPerBasicBlock = basicBlockSize_ / WORD_SIZE;
        uint32_t blockId = pos / basicBlockSize_;
        uint32_t offset = pos & (basicBlockSize_ - 1);
        return lut_[blockId] + popcountLinear(bits_, blockId * wordPerBasicBlock, offset);
    }

    uint32_t size() {
        uint32_t bitVectorMem = numBits_ / 8;
        uint32_t selectLutMem = (numOnes_ / sampleInterval_ + 1) * sizeof(uint32_t);
        return bitVectorMem + selectLutMem;
    }

private:
    uint32_t sampleInterval_;
    uint32_t numOnes_;
    uint32_t* selectLut_; //select look-up table
};

#endif // SELECT_H_
