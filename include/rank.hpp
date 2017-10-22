#ifndef RANK_H_
#define RANK_H_

#include "bitvector.hpp"

#include <assert.h>

#include <vector>

#include "popcount.h"

namespace surf {

class BitvectorRank : public Bitvector {
public:
    BitvectorRank() : basic_block_size_(0), rank_lut_(NULL) {};

    BitvectorRank(const position_t basic_block_size, 
		  const std::vector<std::vector<word_t> >& bitvector_per_level, 
		  const std::vector<position_t>& num_bits_per_level,
		  const level_t start_level = 0,
		  const level_t end_level = 0/* non-inclusive */) 
	: Bitvector(bitvector_per_level, num_bits_per_level, start_level, end_level) {
	basic_block_size_ = basic_block_size;
	initRankLut();
    }

    ~BitvectorRank() {
	//delete[] rank_lut_;
    }

    // Counts the number of 1's in the bitvector up to position pos.
    // pos is zero-based; count is one-based.
    // E.g., for bitvector: 100101000, rank(3) = 2
    position_t rank(position_t pos) {
        assert(pos < num_bits_);
        position_t word_per_basic_block = basic_block_size_ / kWordSize;
        position_t block_id = pos / basic_block_size_;
        position_t offset = pos & (basic_block_size_ - 1);
        return (rank_lut_[block_id] 
		+ popcountLinear(bits_, block_id * word_per_basic_block, offset + 1));
    }

    // in bytes
    position_t size() {
        //position_t bitvector_mem = (num_bits_ / kWordSize) * (kWordSize / 8);
	//if (num_bits_ % kWordSize == 0)
	//bitvector_mem += (kWordSize / 8);
	position_t bitvector_mem = numWords() * (kWordSize / 8);
        position_t rank_lut_mem = (num_bits_ / basic_block_size_ + 1) * sizeof(uint32_t);
        return (sizeof(BitvectorRank) + bitvector_mem + rank_lut_mem);
    }

    inline void prefetch(position_t pos) {
	__builtin_prefetch(bits_ + (pos / kWordSize));
	__builtin_prefetch(rank_lut_ + (pos / basic_block_size_));
    }

    void serialize(std::string* dst) const {
	uint64_t num_bits_size = sizeof(num_bits_);
	uint64_t bits_size = numWords() * (kWordSize / 8);
	uint64_t basic_block_size_size = sizeof(basic_block_size_);
	uint64_t rank_lut_size = (num_bits_ / basic_block_size_ + 1) * sizeof(uint32_t);
	uint64_t size = num_bits_size + bits_size + basic_block_size_size + rank_lut_size;
	dst->resize(size, 0);
	uint64_t offset = 0;
	
	memcpy(&(*dst)[offset], &num_bits_, num_bits_size);
	offset += num_bits_size;
	memcpy(&(*dst)[offset], &basic_block_size_, basic_block_size_size);
	offset += basic_block_size_size;
	memcpy(&(*dst)[offset], bits_, bits_size);
	offset += bits_size;
	memcpy(&(*dst)[offset], rank_lut_, rank_lut_size);
    }

    static void deSerialize(const std::string& src, uint64_t& offset, BitvectorRank* bv_rank) {
	uint64_t num_bits_size = sizeof(bv_rank->num_bits_);
	uint64_t basic_block_size_size = sizeof(bv_rank->basic_block_size_);
	const char* data = src.data();
	memcpy(&(bv_rank->num_bits_), &data[offset], num_bits_size);
	offset += num_bits_size;
	memcpy(&(bv_rank->basic_block_size_), &data[offset], basic_block_size_size);
	offset += basic_block_size_size;
	uint64_t bits_size = bv_rank->numWords() * (kWordSize / 8);
	bv_rank->bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(&data[offset]));
	offset += bits_size;
	bv_rank->rank_lut_ = const_cast<position_t*>(reinterpret_cast<const position_t*>(&data[offset]));
	uint64_t rank_lut_size = (bv_rank->num_bits_ / bv_rank->basic_block_size_ + 1) * sizeof(uint32_t);
	offset += rank_lut_size;
    }

    void destroy() {
	delete[] bits_;
	delete[] rank_lut_;
    }

private:
    void initRankLut() {
        position_t word_per_basic_block = basic_block_size_ / kWordSize;
        position_t num_blocks = num_bits_ / basic_block_size_ + 1;
	rank_lut_ = new position_t[num_blocks];

        position_t cumu_rank = 0;
        for (position_t i = 0; i < num_blocks; i++) {
            rank_lut_[i] = cumu_rank;
            cumu_rank += popcountLinear(bits_, i * word_per_basic_block, basic_block_size_);
        }
    }

    position_t basic_block_size_;
    position_t* rank_lut_; //rank look-up table
};

} // namespace surf

#endif // RANK_H_
