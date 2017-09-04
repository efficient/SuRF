#ifndef SUFFIXVECTOR_H_
#define SUFFIXVECTOR_H_

#include <assert.h>

#include "config.hpp"
#include "hash.hpp"

namespace surf {

//Current version only supports one-byte suffixes
class SuffixVector {
public:
    SuffixVector() : type_(kNone), num_bytes_(0), suffixes_(NULL) {};

    SuffixVector(const SuffixType type, const std::vector<std::vector<suffix_t> >& suffixes_per_level,
		 const level_t start_level = 0, 
		 level_t end_level = 0/* non-inclusive */) {
	if (end_level == 0)
	    end_level = suffixes_per_level.size();

	type_ = type;

	num_bytes_ = 0;
	for (level_t level = start_level; level < end_level; level++)
	    num_bytes_ += suffixes_per_level[level].size();

	suffixes_ = new suffix_t[num_bytes_];

	position_t pos = 0;
	for (level_t level = start_level; level < end_level; level++) {
	    for (position_t idx = 0; idx < suffixes_per_level[level].size(); idx++) {
		suffixes_[pos] = suffixes_per_level[level][idx];
		pos++;
	    }
	}
    }

    ~SuffixVector() {
	delete[] suffixes_;
    }

    bool checkEquality(const position_t pos, const std::string& key, const level_t level = 0) const {
	assert(pos < num_bytes_);
	switch (type_) {
	case kHash: {
	    suffix_t h = suffixHash(key) & kSuffixHashMask;
	    if (h == suffixes_[pos])
		return true;
	    return false;
	}
	case kReal: {
	    if (suffixes_[pos] == 0) //0 means no suffix stored
		return true;
	    assert(level < key.length());
	    if (key[level] == suffixes_[pos])
		return true;
	    return false;
	}
	default:
	    return false;
	}

    }

    position_t size() const {
	return (sizeof(SuffixVector) + num_bytes_);
    }

private:
    SuffixType type_;
    position_t num_bytes_;
    suffix_t* suffixes_;
};

} // namespace surf

#endif // SUFFIXVECTOR_H_
