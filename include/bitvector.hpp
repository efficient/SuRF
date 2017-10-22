#ifndef BITVECTOR_H_
#define BITVECTOR_H_

#include <assert.h>

#include <vector>

#include "config.hpp"

namespace surf {

class Bitvector {
public:
    Bitvector() : num_bits_(0), bits_(NULL) {};

    Bitvector(const std::vector<std::vector<word_t> >& bitvector_per_level, 
	      const std::vector<position_t>& num_bits_per_level, 
	      const level_t start_level = 0, 
	      level_t end_level = 0/* non-inclusive */) {
	if (end_level == 0)
	    end_level = bitvector_per_level.size();
	computeTotalNumBits(num_bits_per_level, start_level, end_level);
	bits_ = new word_t[numWords()];
	memset(bits_, 0, numWords() * (kWordSize / 8));
	concatenateBitvectors(bitvector_per_level, num_bits_per_level, start_level, end_level);
    }

    ~Bitvector() {
	delete[] bits_;
    }

    position_t numBits() const {
	return num_bits_;
    }

    position_t numWords() const {
	if (num_bits_ % kWordSize == 0)
	    return (num_bits_ / kWordSize);
	else
	    return (num_bits_ / kWordSize + 1);
    }

    // in bytes
    position_t size() const {
        return (sizeof(Bitvector) + numWords() * (kWordSize / 8));
    }

    inline bool readBit(const position_t pos) const;

    position_t distanceToNextSetBit(const position_t pos) const;

private:
    void computeTotalNumBits(const std::vector<position_t>& num_bits_per_level, 
				   const level_t start_level, 
				   const level_t end_level/* non-inclusive */);

    void concatenateBitvectors(const std::vector<std::vector<word_t> >& bitvector_per_level, 
			       const std::vector<position_t>& num_bits_per_level, 
			       const level_t start_level, 
			       const level_t end_level/* non-inclusive */);
protected:
    position_t num_bits_;
    word_t* bits_;
};

bool Bitvector::readBit (const position_t pos) const {
    assert(pos < num_bits_);
    position_t word_id = pos / kWordSize;
    position_t offset = pos & (kWordSize - 1);
    return bits_[word_id] & (kMsbMask >> offset);
}

position_t Bitvector::distanceToNextSetBit (const position_t pos) const {
    assert(pos < num_bits_);
    position_t distance = 1;

    position_t word_id = (pos + 1) / kWordSize;
    if (word_id >= numWords())
	return distance;
    position_t offset = (pos + 1) % kWordSize;

    //first word left-over bits
    word_t test_bits = bits_[word_id] << offset;
    if (test_bits > 0) {
	return (distance + __builtin_clzll(test_bits));
    } else {
	if (word_id == numWords() - 1)
	    return (num_bits_ - pos);
	distance += (kWordSize - offset);
    }

    while (true) {
	word_id++;
	if (word_id >= numWords())
	    return distance;
	test_bits = bits_[word_id];
	if (test_bits > 0)
	    return (distance + __builtin_clzll(test_bits));
	distance += kWordSize;
    }
}

void Bitvector::computeTotalNumBits(const std::vector<position_t>& num_bits_per_level, 
					  const level_t start_level, 
					  const level_t end_level/* non-inclusive */) {
    num_bits_ = 0;
    for (level_t level = start_level; level < end_level; level++)
	num_bits_ += num_bits_per_level[level];
}

void Bitvector::concatenateBitvectors(const std::vector<std::vector<word_t> >& bitvector_per_level, 
			   const std::vector<position_t>& num_bits_per_level, 
			   const level_t start_level, 
			   const level_t end_level/* non-inclusive */) {
    position_t bit_shift = 0;
    position_t word_id = 0;
    for (level_t level = start_level; level < end_level; level++) {
	if (num_bits_per_level[level] == 0) continue;
	position_t num_complete_words = num_bits_per_level[level] / kWordSize;
	for (position_t word = 0; word < num_complete_words; word++) {
	    bits_[word_id] |= (bitvector_per_level[level][word] >> bit_shift);
	    word_id++;
	    if (bit_shift > 0)
		bits_[word_id] |= (bitvector_per_level[level][word] << (kWordSize - bit_shift));
	}

	word_t bits_remain = num_bits_per_level[level] - num_complete_words * kWordSize;
	if (bits_remain > 0) {
	    word_t last_word = bitvector_per_level[level][num_complete_words];
	    bits_[word_id] |= (last_word >> bit_shift);
	    if (bit_shift + bits_remain < kWordSize) {
		bit_shift += bits_remain;
	    } else {
		word_id++;
		bits_[word_id] |= (last_word << (kWordSize - bit_shift));
		bit_shift = bit_shift + bits_remain - kWordSize;
	    }
	}
    }
}

} // namespace surf

#endif // BITVECTOR_H_
