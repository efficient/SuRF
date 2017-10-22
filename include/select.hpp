#ifndef SELECT_H_
#define SELECT_H_

#include "bitvector.hpp"

#include <assert.h>

#include <vector>

#include "config.hpp"
#include "popcount.h"

namespace surf {

class BitvectorSelect : public Bitvector {
public:
    BitvectorSelect() : sample_interval_(0), num_ones_(0), select_lut_(NULL) {};

    BitvectorSelect(const position_t sample_interval, 
		    const std::vector<std::vector<word_t> >& bitvector_per_level, 
		    const std::vector<position_t>& num_bits_per_level,
		    const level_t start_level = 0,
		    const level_t end_level = 0/* non-inclusive */) 
	: Bitvector(bitvector_per_level, num_bits_per_level, start_level, end_level) {
	sample_interval_ = sample_interval;
	initSelectLut();
    }

    ~BitvectorSelect() {
	//delete[] select_lut_;
    }

    // Returns the postion of the rank-th 1 bit.
    // posistion is zero-based; rank is one-based.
    // E.g., for bitvector: 100101000, select(3) = 5
    position_t select(position_t rank) {
	assert(rank > 0);
	assert(rank <= num_ones_);
	position_t lut_idx = rank / sample_interval_;
	position_t rank_left = rank % sample_interval_;
	// The first slot in select_lut_ stores the position of the first 1 bit.
	// Slot i > 0 stores the position of (i * sample_interval_)-th 1 bit
	if (lut_idx == 0)
	    rank_left--;

	position_t pos = select_lut_[lut_idx];

	if (rank_left == 0)
	    return pos;

	position_t word_id = pos / kWordSize;
	position_t offset = pos % kWordSize;
	if (offset == kWordSize - 1) {
	    word_id++;
	    offset = 0;
	} else {
	    offset++;
	}
	word_t word = bits_[word_id] << offset >> offset; //zero-out most significant bits
	position_t ones_count_in_word = popcount(word);
	while (ones_count_in_word < rank_left) {
	    word_id++;
	    word = bits_[word_id];
	    rank_left -= ones_count_in_word;
	    ones_count_in_word = popcount(word);
	}
	return (word_id * kWordSize + select64_popcount_search(word, rank_left));
    }

    position_t size() {
        //position_t bitvector_mem = (num_bits_ / kWordSize) * (kWordSize / 8);
	//if (num_bits_ % kWordSize == 0)
	//bitvector_mem += (kWordSize / 8);
	position_t bitvector_mem = numWords() * (kWordSize / 8);
        position_t select_lut_mem = (num_ones_ / sample_interval_ + 1) * sizeof(uint32_t);
        return (sizeof(BitvectorSelect) + bitvector_mem + select_lut_mem);
    }

    void serialize(std::string* dst) const {
	uint64_t num_bits_size = sizeof(num_bits_);
	uint64_t bits_size = numWords() * (kWordSize / 8);
	uint64_t sample_interval_size = sizeof(sample_interval_);
	uint64_t num_ones_size = sizeof(num_ones_);
	uint64_t select_lut_size = (num_ones_ / sample_interval_ + 1) * sizeof(uint32_t);
	uint64_t size = num_bits_size + bits_size + sample_interval_size
	    + num_ones_size + select_lut_size;
	dst->resize(size, 0);
	uint64_t offset = 0;
	memcpy(&(*dst)[offset], &num_bits_, num_bits_size);
	offset += num_bits_size;
	memcpy(&(*dst)[offset], &sample_interval_, sample_interval_size);
	offset += sample_interval_size;
	memcpy(&(*dst)[offset], &num_ones_, num_ones_size);
	offset += num_ones_size;
	memcpy(&(*dst)[offset], bits_, bits_size);
	offset += bits_size;
	memcpy(&(*dst)[offset], select_lut_, select_lut_size);
    }

    static void deSerialize(const std::string& src, uint64_t& offset, BitvectorSelect* bv_select) {
	uint64_t num_bits_size = sizeof(bv_select->num_bits_);
	uint64_t sample_interval_size = sizeof(bv_select->sample_interval_);
	uint64_t num_ones_size = sizeof(bv_select->num_ones_);
	const char* data = src.data();
	memcpy(&(bv_select->num_bits_), &data[offset], num_bits_size);
	offset += num_bits_size;
	memcpy(&(bv_select->sample_interval_), &data[offset], sample_interval_size);
	offset += sample_interval_size;
	memcpy(&(bv_select->num_ones_), &data[offset], num_ones_size);
	offset += num_ones_size;
	uint64_t bits_size = bv_select->numWords() * (kWordSize / 8);
	bv_select->bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(&data[offset]));
	offset += bits_size;
	bv_select->select_lut_ = const_cast<position_t*>(reinterpret_cast<const position_t*>(&data[offset]));
	uint64_t select_lut_size = (bv_select->num_ones_ / bv_select->sample_interval_ + 1) * sizeof(uint32_t);
	offset += select_lut_size;
    }

    void destroy() {
	delete[] bits_;
	delete[] select_lut_;
    }

private:
    // This function currently assumes that the first bit in the
    // bitvector is one.
    void initSelectLut() {
	position_t num_words = num_bits_ / kWordSize;
	if (num_bits_ % kWordSize != 0)
	    num_words++;

	std::vector<position_t> select_lut_vector;
	select_lut_vector.push_back(0); //ASSERT: first bit is 1
	position_t sampling_ones = sample_interval_;
	position_t cumu_ones_upto_word = 0;
	for (position_t i = 0; i < num_words; i++) {
	    position_t num_ones_in_word = popcount(bits_[i]);
	    while (sampling_ones <= (cumu_ones_upto_word + num_ones_in_word)) {
		int diff = sampling_ones - cumu_ones_upto_word;
		position_t result_pos = i * kWordSize + select64_popcount_search(bits_[i], diff);
		select_lut_vector.push_back(result_pos);
		sampling_ones += sample_interval_;
	    }
	    cumu_ones_upto_word += popcount(bits_[i]);
	}

	num_ones_ = cumu_ones_upto_word;
	position_t num_samples = select_lut_vector.size();
	select_lut_ = new position_t[num_samples];
	for (position_t i = 0; i < num_samples; i++)
	    select_lut_[i] = select_lut_vector[i];
    }

private:
    position_t sample_interval_;
    position_t num_ones_;
    position_t* select_lut_; //select look-up table
};

} // namespace surf

#endif // SELECT_H_
