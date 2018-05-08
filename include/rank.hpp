#ifndef RANK_H_
#define RANK_H_

#include "bitvector.hpp"

#include <assert.h>

#include <vector>

#include "popcount.h"

namespace surf {

class BitvectorRank : public Bitvector {
public:
    BitvectorRank() : basic_block_size_(0), rank_lut_(nullptr) {};

    BitvectorRank(const position_t basic_block_size, 
		  const std::vector<std::vector<word_t> >& bitvector_per_level, 
		  const std::vector<position_t>& num_bits_per_level,
		  const level_t start_level = 0,
		  const level_t end_level = 0/* non-inclusive */) 
	: Bitvector(bitvector_per_level, num_bits_per_level, start_level, end_level) {
	basic_block_size_ = basic_block_size;
	initRankLut();
    }

    ~BitvectorRank() {}

    // Counts the number of 1's in the bitvector up to position pos.
    // pos is zero-based; count is one-based.
    // E.g., for bitvector: 100101000, rank(3) = 2
    position_t rank(position_t pos) const {
        assert(pos < num_bits_);
        position_t word_per_basic_block = basic_block_size_ / kWordSize;
        position_t block_id = pos / basic_block_size_;
        position_t offset = pos & (basic_block_size_ - 1);
        return (rank_lut_[block_id] 
		+ popcountLinear(bits_, block_id * word_per_basic_block, offset + 1));
    }

    position_t rankLutSize() const {
	return ((num_bits_ / basic_block_size_ + 1) * sizeof(position_t));
    }

    position_t serializedSize() const {
	position_t size = sizeof(num_bits_) + sizeof(basic_block_size_) 
	    + bitsSize() + rankLutSize();
	sizeAlign(size);
	return size;
    }

    position_t size() const {
	return (sizeof(BitvectorRank) + bitsSize() + rankLutSize());
    }

    void prefetch(position_t pos) const {
	__builtin_prefetch(bits_ + (pos / kWordSize));
	__builtin_prefetch(rank_lut_ + (pos / basic_block_size_));
    }

    void serialize(char*& dst) const {
	memcpy(dst, &num_bits_, sizeof(num_bits_));
	dst += sizeof(num_bits_);
	memcpy(dst, &basic_block_size_, sizeof(basic_block_size_));
	dst += sizeof(basic_block_size_);
	memcpy(dst, bits_, bitsSize());
	dst += bitsSize();
	memcpy(dst, rank_lut_, rankLutSize());
	dst += rankLutSize();
	align(dst);
    }

    static BitvectorRank* deSerialize(char*& src) {
	BitvectorRank* bv_rank = new BitvectorRank();
	memcpy(&(bv_rank->num_bits_), src, sizeof(bv_rank->num_bits_));
	src += sizeof(bv_rank->num_bits_);
	memcpy(&(bv_rank->basic_block_size_), src, sizeof(bv_rank->basic_block_size_));
	src += sizeof(bv_rank->basic_block_size_);
	bv_rank->bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(src));
	src += bv_rank->bitsSize();
	bv_rank->rank_lut_ = const_cast<position_t*>(reinterpret_cast<const position_t*>(src));
	src += bv_rank->rankLutSize();
	align(src);
	return bv_rank;
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
        for (position_t i = 0; i < num_blocks - 1; i++) {
            rank_lut_[i] = cumu_rank;
            cumu_rank += popcountLinear(bits_, i * word_per_basic_block, basic_block_size_);
        }
	rank_lut_[num_blocks - 1] = cumu_rank;
    }

    position_t basic_block_size_;
    position_t* rank_lut_; //rank look-up table
};

} // namespace surf

#endif // RANK_H_
