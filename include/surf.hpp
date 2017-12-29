#ifndef SURF_H_
#define SURF_H_

#include <string>
#include <vector>

#include "config.hpp"
#include "louds_dense.hpp"
#include "louds_sparse.hpp"
#include "surf_builder.hpp"

namespace surf {

class SuRF {
public:
    class Iter {
    public:
	Iter() {};
	Iter(const SuRF* filter) {
	    dense_iter_ = LoudsDense::Iter(filter->louds_dense_);
	    sparse_iter_ = LoudsSparse::Iter(filter->louds_sparse_);
	}

	bool isValid() const;
	int compare(const std::string& key);
	std::string getKey() const;
	int getSuffix(word_t* suffix) const; // TODO: need to add unit test
	std::string getKeyWithSuffix(unsigned* bitlen) const;

	// Returns true if the status of the iterator after the operation is valid
	bool operator ++(int);

    private:
	void passToSparse();
	bool incrementDenseIter();
	bool incrementSparseIter();

    private:
	// true implies that dense_iter_ is valid
	LoudsDense::Iter dense_iter_;
	LoudsSparse::Iter sparse_iter_;

	friend class SuRF;
    };

public:
    SuRF() {};

    SuRF(const std::vector<std::string>& keys, 
	 const bool include_dense, const uint32_t sparse_dense_ratio,
	 const SuffixType suffix_type, const level_t suffix_len) {
	create(keys, include_dense, sparse_dense_ratio, suffix_type, suffix_len);
    }

    ~SuRF() {
	//delete louds_dense_;
	//delete louds_sparse_;
    }

    void create(const std::vector<std::string>& keys, 
		const bool include_dense, const uint32_t sparse_dense_ratio,
		const SuffixType suffix_type, const level_t suffix_len);

    bool lookupKey(const std::string& key) const;
    // This function searches in a conservative way: if inclusive is true
    // and the stored key prefix matches key, iter stays at this key prefix.
    SuRF::Iter moveToKeyGreaterThan(const std::string& key, const bool inclusive) const;
    bool lookupRange(const std::string& left_key, const bool left_inclusive, 
		     const std::string& right_key, const bool right_inclusive) const;
    uint32_t countRange(const std::string& left_key, const bool left_inclusive, 
			const std::string& right_key, const bool right_inclusive) const;

    uint64_t serializedSize() const;
    uint64_t getMemoryUsage() const;
    level_t getHeight() const;
    level_t getSparseStartLevel() const;

    char* serialize() const {
	uint64_t size = serializedSize();
	char* data = new char[size];
	char* cur_data = data;
	louds_dense_->serialize(cur_data);
	louds_sparse_->serialize(cur_data);
	assert(cur_data - data == (int64_t)size);
	return data;
    }

    static SuRF* deSerialize(char* src) {
	SuRF* surf = new SuRF();
	surf->louds_dense_ = LoudsDense::deSerialize(src);
	surf->louds_sparse_ = LoudsSparse::deSerialize(src);
	return surf;
    }

    void destroy() {
	louds_dense_->destroy();
	louds_sparse_->destroy();
    }

private:
    LoudsDense* louds_dense_;
    LoudsSparse* louds_sparse_;
    SuRFBuilder* builder;
};

void SuRF::create(const std::vector<std::string>& keys, 
		  const bool include_dense, const uint32_t sparse_dense_ratio,
		  const SuffixType suffix_type, const level_t suffix_len) {
    builder = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, suffix_len);
    builder->build(keys);
    louds_dense_ = new LoudsDense(builder);
    louds_sparse_ = new LoudsSparse(builder);
    delete builder;
}

bool SuRF::lookupKey(const std::string& key) const {
    position_t connect_node_num = 0;
    if (!louds_dense_->lookupKey(key, connect_node_num))
	return false;
    else if (connect_node_num != 0)
	return louds_sparse_->lookupKey(key, connect_node_num);
    return true;
}

SuRF::Iter SuRF::moveToKeyGreaterThan(const std::string& key, const bool inclusive) const {
    SuRF::Iter iter(this);
    louds_dense_->moveToKeyGreaterThan(key, inclusive, iter.dense_iter_);

    if (!iter.dense_iter_.isValid()) return iter;
    if (iter.dense_iter_.isComplete()) return iter;

    if (!iter.dense_iter_.isSearchComplete()) {
	iter.passToSparse();
	louds_sparse_->moveToKeyGreaterThan(key, inclusive, iter.sparse_iter_);
	if (!iter.sparse_iter_.isValid()) {
	    iter.incrementDenseIter();
	}
	return iter;
    } else if (!iter.dense_iter_.isMoveLeftComplete()) {
	iter.passToSparse();
	iter.sparse_iter_.moveToLeftMostKey();
	return iter;
    }
    assert(false); // shouldn't have reached here
    return iter;
}

bool SuRF::lookupRange(const std::string& left_key, const bool left_inclusive, 
		       const std::string& right_key, const bool right_inclusive) const {
    SuRF::Iter iter = moveToKeyGreaterThan(left_key, left_inclusive);
    if (!iter.isValid()) return false;
    int compare = iter.compare(right_key);
    if (right_inclusive)
	return (compare <= 0);
    else
	return (compare < 0);
}

// TODO
uint32_t SuRF::countRange(const std::string& left_key, const bool left_inclusive, 
			  const std::string& right_key, const bool right_inclusive) const {
    return 0;
}

uint64_t SuRF::serializedSize() const {
    return (louds_dense_->serializedSize()
	    + louds_sparse_->serializedSize());
}

uint64_t SuRF::getMemoryUsage() const {
    return (sizeof(SuRF) + louds_dense_->getMemoryUsage() + louds_sparse_->getMemoryUsage());
}

level_t SuRF::getHeight() const {
    return louds_sparse_->getHeight();
}

level_t SuRF::getSparseStartLevel() const {
    return louds_sparse_->getStartLevel();
}

//============================================================================

bool SuRF::Iter::isValid() const {
    return dense_iter_.isValid() 
	&& (dense_iter_.isComplete() || sparse_iter_.isValid());
}

int SuRF::Iter::compare(const std::string& key) {
    assert(isValid());
    int dense_compare = dense_iter_.compare(key);
    if (dense_iter_.isComplete() || dense_compare != 0) 
	return dense_compare;
    return sparse_iter_.compare(key);
}

std::string SuRF::Iter::getKey() const {
    if (!isValid())
	return std::string();
    if (dense_iter_.isComplete())
	return dense_iter_.getKey();
    return dense_iter_.getKey() + sparse_iter_.getKey();
}

int SuRF::Iter::getSuffix(word_t* suffix) const {
    if (!isValid())
	return 0;
    if (dense_iter_.isComplete())
	return dense_iter_.getSuffix(suffix);
    return sparse_iter_.getSuffix(suffix);
}

std::string SuRF::Iter::getKeyWithSuffix(unsigned* bitlen) const {
    *bitlen = 0;
    if (!isValid())
	return std::string();
    if (dense_iter_.isComplete())
	return dense_iter_.getKeyWithSuffix(bitlen);
    return dense_iter_.getKeyWithSuffix(bitlen) + sparse_iter_.getKeyWithSuffix(bitlen);
}

inline void SuRF::Iter::passToSparse() {
    sparse_iter_.setStartNodeNum(dense_iter_.getSendOutNodeNum());
}

inline bool SuRF::Iter::incrementDenseIter() {
    if (!dense_iter_.isValid()) return false;

    dense_iter_++;
    if (!dense_iter_.isValid()) return false;
    if (dense_iter_.isMoveLeftComplete()) return true;

    passToSparse();
    sparse_iter_.moveToLeftMostKey();
    return true;
}

inline bool SuRF::Iter::incrementSparseIter() {
    if (!sparse_iter_.isValid()) return false;
    sparse_iter_++;
    return sparse_iter_.isValid();
}

bool SuRF::Iter::operator ++(int) {
    if (!isValid()) return false;
    if (incrementSparseIter()) return true;
    return incrementDenseIter();
}

} // namespace surf

#endif // SURF_H
