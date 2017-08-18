#ifdef SELECT_H_
#define SELECT_H_

#include <assert.h>
#include <vector>

#include "popcount.h"
#include "bitVector.hpp"

class BitVectorSelect : public BitVector {
public:
    BitVectorSelect() : sampleInterval_(0), numOnes_(0), selectLut_(NULL) {};

    BitVectorSelect(const uint32_t sampleInterval, const vector<vector<uint64_t> > &bitVectorPerLevel, const vector<uint32_t> &numBitsPerLevel) : BitVector(bitVectorPerLevel, numBitsPerLevel) {
	sampleInterval_ = sampleInterval;
	initSelectLut();
    }

    ~BitVectorSelect() {
	delete[] selectLut_;
    }

    uint32_t select(uint32_t rank) {
	assert(rank < numOnes_);
	uint32_t lutIdx = rank / sampleInterval_;
	uint32_t rankLeft = rank % sampleInterval_;

	uint32_t pos = selectLut_[lutIdx];

	if (rankLeft == 0)
	    return pos;

	uint32_t wordId = pos / WORD_SIZE;
	uint32_t offset = pos % WROD_SIZE;
	uint64_t word = bits_[wordId] << offset >> offset; //zero-out most significant bits
	uint32_t OnesCountInWord = popcount(word);
	while (OnesCountInWord < rankLeft) {
	    wordId++;
	    word = bits_[wordId];
	    rankLeft -= OnesCountInWord;
	}

	return (wordId * WORD_SIZE + select64_popcount_search(word, rankLeft));
    }

    uint32_t size() {
        uint32_t bitVectorMem = numBits_ / 8;
        uint32_t selectLutMem = (numOnes_ / sampleInterval_ + 1) * sizeof(uint32_t);
        return bitVectorMem + selectLutMem;
    }

private:
    void initSelectLut() {
	uint32_t numWords = numBits_ / WORD_SIZE;
	if (numBits % WORD_SIZE != 0)
	    numWords++;

	vector<uint32_t> selectLutVector;
	selectLutVector.push_back(0); //ASSERT: first bit is 1
	uint32_t samplingOnes = sampleInterval_;
	uint32_t cumuOnesUptoWord = 0;
	for (uint32_t i = 0; i < numWords; i++) {
	    uint32_t numOnesInWord = popcount(bits_[i]);
	    while (samplingOnes <= (cumuOnesUptoWord + numOnesInWord)) {
		int diff = samplingOnes - cumuOnesUptoWord;
		uint32_t resultPos = i * WORD_SIZE + select64_popcount_search(bits_[i], diff);
		selectLutVector.push_back(resultPos);
		samplingOnes += sampleInterval_;
	    }
	    cumuOnesUptoWord += popcount(bits_[i]);
	}

	numOnes_ = cumuOnesUptoWord;
	uint32_t numSamples = selectLutVector.size();
	selectLut_ = new uint32_t[numSamples];
	for (uint32_t i = 0; i < numSamples; i++)
	    selectLut_ = selectLutVector[i];
    }

private:
    uint32_t sampleInterval_;
    uint32_t numOnes_;
    uint32_t* selectLut_; //select look-up table
};

#endif // SELECT_H_
