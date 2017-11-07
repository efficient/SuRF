#ifndef SUFFIX_H_
#define SUFFIX_H_

#include "bitvector.hpp"

#include <assert.h>

#include <vector>

#include "config.hpp"
#include "hash.hpp"

namespace surf {

// Max suffix_len_ = 64 bits
// For kReal suffixes, if the stored key is not long enough to provide
// suffix_len_ suffix bits, its suffix field is cleared (i.e., all 0's)
// to indicate that there is no suffix info associated with the key.
class BitvectorSuffix : public Bitvector {
public:
    BitvectorSuffix() : type_(kNone), suffix_len_(0) {};

    BitvectorSuffix(const SuffixType type, const level_t suffix_len,
		 const std::vector<std::vector<word_t> >& bitvector_per_level, 
		 const std::vector<position_t>& num_bits_per_level,
		 const level_t start_level = 0, 
		 level_t end_level = 0/* non-inclusive */)
	: Bitvector(bitvector_per_level, num_bits_per_level, start_level, end_level) {
	assert(suffix_len <= kWordSize);
	type_ = type;
	suffix_len_ = suffix_len;
    }

private:
    static inline word_t constructHashSuffix(const std::string& key, const level_t len) {
	word_t suffix = suffixHash(key);
	suffix <<= (kWordSize - len - kHashShift);
	suffix >>= (kWordSize - len);
	return suffix;
    }

    static inline word_t constructRealSuffix(const std::string& key, const level_t level, const level_t len) {
	if (key.length() < level || ((key.length() - level) * 8) < len)
	    return 0;
	word_t suffix = 0;
	level_t num_complete_bytes = len / 8;
	if (num_complete_bytes > 0) {
	    suffix += (word_t)(label_t)key[level];
	    for (position_t i = 1; i < num_complete_bytes; i++) {
		suffix <<= 8;
		suffix += (word_t)(uint8_t)key[level + i];
	    }
	}
	level_t offset = len % 8;
	if (offset > 0) {
	    suffix <<= offset;
	    word_t remaining_bits = 0;
	    remaining_bits = (word_t)(uint8_t)key[level + num_complete_bytes];
	    remaining_bits >>= (8 - offset);
	    suffix += remaining_bits;
	}
	return suffix;
    }

public:
    static word_t constructSuffix(const std::string& key, const level_t level, 
				  const SuffixType type, const level_t len) {
	switch (type) {
	case kHash:
	    return constructHashSuffix(key, len);
	case kReal:
	    return constructRealSuffix(key, level, len);
	default:
	    return 0;
    }
    }

    SuffixType getType() const {
	return type_;
    }

    level_t getSuffixLen() const {
	return suffix_len_;
    }

    position_t serializedSize() const {
	position_t size = sizeof(num_bits_) + sizeof(type_) + sizeof(suffix_len_) + bitsSize();
	sizeAlign(size);
	return size;
    }

    position_t size() const {
	return (sizeof(BitvectorSuffix) + bitsSize());
    }

    word_t read(const position_t idx) const;
    bool checkEquality(const position_t idx, const std::string& key, const level_t level) const;

    // Compare stored suffix to querying suffix.
    // kReal suffix type only.
    int compare(const position_t idx, const std::string& key, const level_t level) const;

    void serialize(char*& dst) const {
	memcpy(dst, &num_bits_, sizeof(num_bits_));
	dst += sizeof(num_bits_);
	memcpy(dst, &type_, sizeof(type_));
	dst += sizeof(type_);
	memcpy(dst, &suffix_len_, sizeof(suffix_len_));
	dst += sizeof(suffix_len_);
	if (type_ != kNone) {
	    memcpy(dst, bits_, bitsSize());
	    dst += bitsSize();
	}
	align(dst);
    }

    static BitvectorSuffix* deSerialize(char*& src) {
	BitvectorSuffix* sv = new BitvectorSuffix();
	memcpy(&(sv->num_bits_), src, sizeof(sv->num_bits_));
	src += sizeof(sv->num_bits_);
	memcpy(&(sv->type_), src, sizeof(sv->type_));
	src += sizeof(sv->type_);
	memcpy(&(sv->suffix_len_), src, sizeof(sv->suffix_len_));
	src += sizeof(sv->suffix_len_);
	if (sv->type_ != kNone) {
	    sv->bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(src));
	    src += sv->bitsSize();
	}
	align(src);
	return sv;
    }

    void destroy() {
	if (type_ != kNone) 
	    delete[] bits_;
    }

private:
    SuffixType type_;
    level_t suffix_len_; // in bits
};

word_t BitvectorSuffix::read(const position_t idx) const {
    if (type_ == kNone) return 0;
    if (idx * suffix_len_ >= num_bits_) return 0;
    //assert(idx * suffix_len_ < num_bits_);
    position_t bit_pos = idx * suffix_len_;
    position_t word_id = bit_pos / kWordSize;
    position_t offset = bit_pos & (kWordSize - 1);
    word_t ret_word = (bits_[word_id] << offset) >> (kWordSize - suffix_len_);
    if (offset + suffix_len_ > kWordSize)
	ret_word += (bits_[word_id+1] >> (kWordSize - offset - suffix_len_));
    return ret_word;
}

bool BitvectorSuffix::checkEquality(const position_t idx, const std::string& key, const level_t level) const {
    if (type_ == kNone) return true;
    if (idx * suffix_len_ >= num_bits_) return false;
    //assert(idx * suffix_len_ < num_bits_);
    word_t stored_suffix = read(idx);
    if (type_ == kReal) {
	// if no suffix info for the stored key
	if (stored_suffix == 0) return true;
	// if the querying key is shorter than the stored key
	if (key.length() < level || ((key.length() - level) * 8) < suffix_len_) return false;
    }
    word_t querying_suffix = constructSuffix(key, level, type_, suffix_len_);
    if (stored_suffix == querying_suffix) return true;
    return false;
}

// If no real suffix is stored for the key, compare returns 0.
int BitvectorSuffix::compare(const position_t idx, const std::string& key, const level_t level) const {
    if (type_ == kNone || type_ == kHash) return 0;
    if (idx * suffix_len_ >= num_bits_) return 0;
    //assert(idx * suffix_len_ < num_bits_);
    word_t stored_suffix = read(idx);
    if (stored_suffix == 0) return 0;
    word_t querying_suffix = constructRealSuffix(key, level, suffix_len_);
    if (stored_suffix < querying_suffix) return -1;
    else if (stored_suffix == querying_suffix) return 0;
    else return 1;
}

} // namespace surf

#endif // SUFFIXVECTOR_H_
