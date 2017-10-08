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
	Iter(SuRF* filter) {
	    dense_iter_ = LoudsDense::Iter(filter->louds_dense_);
	    sparse_iter_ = LoudsSparse::Iter(filter->louds_sparse_);
	}

	bool isValid() const;
	int compare(const std::string& key);
	std::string getKey() const;

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
	builder = new SuRFBuilder(include_dense, sparse_dense_ratio, suffix_type, suffix_len);
	builder->build(keys);
	louds_dense_ = new LoudsDense(builder);
	louds_sparse_ = new LoudsSparse(builder);
	delete builder;
    }

    ~SuRF() {
	delete louds_dense_;
	delete louds_sparse_;
    }

    bool lookupKey(const std::string& key);
    SuRF::Iter moveToKeyGreaterThan(const std::string& key, const bool inclusive);
    bool lookupRange(const std::string& left_key, const bool left_inclusive, 
		     const std::string& right_key, const bool right_inclusive);
    uint32_t countRange(const std::string& left_key, const bool left_inclusive, 
			const std::string& right_key, const bool right_inclusive);

    uint64_t getMemoryUsage();
    level_t getHeight();
    level_t getSparseStartLevel();

private:
    LoudsDense* louds_dense_;
    LoudsSparse* louds_sparse_;
    SuRFBuilder* builder;
};


bool SuRF::lookupKey(const std::string& key) {
    position_t connect_node_num = 0;
    if (!louds_dense_->lookupKey(key, connect_node_num))
	return false;
    else if (connect_node_num != 0)
	return louds_sparse_->lookupKey(key, connect_node_num);
    return true;
}

SuRF::Iter SuRF::moveToKeyGreaterThan(const std::string& key, const bool inclusive) {
    SuRF::Iter iter(this);
    louds_dense_->moveToKeyGreaterThan(key, inclusive, iter.dense_iter_);

    if (!iter.dense_iter_.isValid()) return iter;
    if (iter.dense_iter_.isComplete()) return iter;

    if (!iter.dense_iter_.isSearchComplete()) {
	iter.passToSparse();
	louds_sparse_->moveToKeyGreaterThan(key, inclusive, iter.sparse_iter_);
	if (!iter.sparse_iter_.isValid())
	    iter.incrementDenseIter();
	return iter;
    } else if (!iter.dense_iter_.isMoveLeftComplete()) {
	iter.passToSparse();
	iter.sparse_iter_.moveToLeftMostKey();
	return iter;
    }
    assert(false); // shouldn't have reached here
}

bool SuRF::lookupRange(const std::string& left_key, const bool left_inclusive, 
		       const std::string& right_key, const bool right_inclusive) {
    SuRF::Iter iter = moveToKeyGreaterThan(left_key, left_inclusive);
    int compare = iter.compare(right_key);
    if (right_inclusive)
	return (compare <= 0);
    else
	return (compare < 0);
}

// TODO
uint32_t SuRF::countRange(const std::string& left_key, const bool left_inclusive, 
			  const std::string& right_key, const bool right_inclusive) {
    return 0;
}

uint64_t SuRF::getMemoryUsage() {
    return (sizeof(SuRF) + louds_dense_->getMemoryUsage() + louds_sparse_->getMemoryUsage());
}

level_t SuRF::getHeight() {
    return louds_sparse_->getHeight();
}

level_t SuRF::getSparseStartLevel() {
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
