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
	type_ = type;
	num_bytes_ = 0;

	if (type_ == kNone)
	    return;

	if (end_level == 0)
	    end_level = suffixes_per_level.size();

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
	if (type_ != kNone)
	    delete[] suffixes_;
    }

    SuffixType getType() const {
	return type_;
    }

    position_t size() const {
	return (sizeof(SuffixVector) + num_bytes_);
    }

    suffix_t read(const position_t pos) const {
	return suffixes_[pos];
    }

    suffix_t* move(const position_t pos) const {
	return (suffixes_ + pos);
    }

    bool checkEquality(const position_t pos, const std::string& key, const level_t level) const;
    int compare(const position_t pos, const label_t label) const;

private:
    SuffixType type_;
    position_t num_bytes_;
    suffix_t* suffixes_;
};

bool SuffixVector::checkEquality(const position_t pos, const std::string& key, const level_t level) const {
    switch (type_) {
    case kNone:
	return true;
    case kHash: {
	assert(pos < num_bytes_);
	suffix_t h = suffixHash(key) & kSuffixHashMask;
	if (suffixes_[pos] == h)
	    return true;
	return false;
    }
    case kReal: {
	assert(pos < num_bytes_);
	if (suffixes_[pos] == 0) //0 means no suffix stored
	    return true;
	if (level >= key.length())
	    return false;
	if (suffixes_[pos] == (label_t)key[level])
	    return true;
	return false;
    }
    default:
	return false;
    }
}

int SuffixVector::compare(const position_t pos, const label_t label) const {
    assert(pos < num_bytes_);
    assert(type_ == kReal);

    if (suffixes_[pos] < label)
	return -1;
    else if (suffixes_[pos] == label)
	return 0;
    else
	return 1;
}

} // namespace surf

#endif // SUFFIXVECTOR_H_
