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
	    could_be_fp_ = false;
	}

	void clear();
	bool isValid() const;
	bool getFpFlag() const;
	int compare(const std::string& key) const;
	std::string getKey() const;
	int getSuffix(word_t* suffix) const;
	std::string getKeyWithSuffix(unsigned* bitlen) const;

	// Returns true if the status of the iterator after the operation is valid
	bool operator ++(int);
	bool operator --(int);

    private:
	void passToSparse();
	bool incrementDenseIter();
	bool incrementSparseIter();
	bool decrementDenseIter();
	bool decrementSparseIter();

    private:
	// true implies that dense_iter_ is valid
	LoudsDense::Iter dense_iter_;
	LoudsSparse::Iter sparse_iter_;
	bool could_be_fp_;

	friend class SuRF;
    };

public:
    SuRF() {};

    //------------------------------------------------------------------
    // Input keys must be SORTED
    //------------------------------------------------------------------
    SuRF(const std::vector<std::string>& keys) {
	create(keys, kIncludeDense, kSparseDenseRatio, kNone, 0, 0);
    }

    SuRF(const std::vector<std::string>& keys, const SuffixType suffix_type,
	 const level_t hash_suffix_len, const level_t real_suffix_len) {
	create(keys, kIncludeDense, kSparseDenseRatio, suffix_type, hash_suffix_len, real_suffix_len);
    }
    
    SuRF(const std::vector<std::string>& keys,
	 const bool include_dense, const uint32_t sparse_dense_ratio,
	 const SuffixType suffix_type, const level_t hash_suffix_len, const level_t real_suffix_len) {
	create(keys, include_dense, sparse_dense_ratio, suffix_type, hash_suffix_len, real_suffix_len);
    }

    ~SuRF() {}

    void create(const std::vector<std::string>& keys,
		const bool include_dense, const uint32_t sparse_dense_ratio,
		const SuffixType suffix_type,
                const level_t hash_suffix_len, const level_t real_suffix_len);

    bool lookupKey(const std::string& key) const;
    // This function searches in a conservative way: if inclusive is true
    // and the stored key prefix matches key, iter stays at this key prefix.
    SuRF::Iter moveToKeyGreaterThan(const std::string& key, const bool inclusive) const;
    SuRF::Iter moveToKeyLessThan(const std::string& key, const bool inclusive) const;
    SuRF::Iter moveToFirst() const;
    SuRF::Iter moveToLast() const;
    bool lookupRange(const std::string& left_key, const bool left_inclusive, 
		     const std::string& right_key, const bool right_inclusive);

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
	surf->iter_ = SuRF::Iter(surf);
	return surf;
    }

    void destroy() {
	louds_dense_->destroy();
	louds_sparse_->destroy();
    }

private:
    LoudsDense* louds_dense_;
    LoudsSparse* louds_sparse_;
    SuRFBuilder* builder_;
    SuRF::Iter iter_;
};

void SuRF::create(const std::vector<std::string>& keys, 
		  const bool include_dense, const uint32_t sparse_dense_ratio,
		  const SuffixType suffix_type,
                  const level_t hash_suffix_len, const level_t real_suffix_len) {
    builder_ = new SuRFBuilder(include_dense, sparse_dense_ratio,
                              suffix_type, hash_suffix_len, real_suffix_len);
    builder_->build(keys);
    louds_dense_ = new LoudsDense(builder_);
    louds_sparse_ = new LoudsSparse(builder_);
    iter_ = SuRF::Iter(this);
    delete builder_;
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
    iter.could_be_fp_ = louds_dense_->moveToKeyGreaterThan(key, inclusive, iter.dense_iter_);

    if (!iter.dense_iter_.isValid())
	return iter;
    if (iter.dense_iter_.isComplete())
	return iter;

    if (!iter.dense_iter_.isSearchComplete()) {
	iter.passToSparse();
	iter.could_be_fp_ = louds_sparse_->moveToKeyGreaterThan(key, inclusive, iter.sparse_iter_);
	if (!iter.sparse_iter_.isValid())
	    iter.incrementDenseIter();
	return iter;
    } else if (!iter.dense_iter_.isMoveLeftComplete()) {
	iter.passToSparse();
	iter.sparse_iter_.moveToLeftMostKey();
	return iter;
    }

    assert(false); // shouldn't reach here
    return iter;
}

SuRF::Iter SuRF::moveToKeyLessThan(const std::string& key, const bool inclusive) const {
    SuRF::Iter iter = moveToKeyGreaterThan(key, false);
    if (!iter.isValid()) {
	iter = moveToLast();
	return iter;
    }
    if (!iter.getFpFlag()) {
	iter--;
	if (lookupKey(key))
	    iter--;
    }
    return iter;
}

SuRF::Iter SuRF::moveToFirst() const {
    SuRF::Iter iter(this);
    if (louds_dense_->getHeight() > 0) {
	iter.dense_iter_.setToFirstLabelInRoot();
	iter.dense_iter_.moveToLeftMostKey();
	if (iter.dense_iter_.isMoveLeftComplete())
	    return iter;
	iter.passToSparse();
	iter.sparse_iter_.moveToLeftMostKey();
    } else {
	iter.sparse_iter_.setToFirstLabelInRoot();
	iter.sparse_iter_.moveToLeftMostKey();
    }
    return iter;
}

SuRF::Iter SuRF::moveToLast() const {
    SuRF::Iter iter(this);
    if (louds_dense_->getHeight() > 0) {
	iter.dense_iter_.setToLastLabelInRoot();
	iter.dense_iter_.moveToRightMostKey();
	if (iter.dense_iter_.isMoveRightComplete())
	    return iter;
	iter.passToSparse();
	iter.sparse_iter_.moveToRightMostKey();
    } else {
	iter.sparse_iter_.setToLastLabelInRoot();
	iter.sparse_iter_.moveToRightMostKey();
    }
    return iter;
}

bool SuRF::lookupRange(const std::string& left_key, const bool left_inclusive, 
		       const std::string& right_key, const bool right_inclusive) {
    iter_.clear();
    louds_dense_->moveToKeyGreaterThan(left_key, left_inclusive, iter_.dense_iter_);
    if (!iter_.dense_iter_.isValid()) return false;
    if (!iter_.dense_iter_.isComplete()) {
	if (!iter_.dense_iter_.isSearchComplete()) {
	    iter_.passToSparse();
	    louds_sparse_->moveToKeyGreaterThan(left_key, left_inclusive, iter_.sparse_iter_);
	    if (!iter_.sparse_iter_.isValid()) {
		iter_.incrementDenseIter();
	    }
	} else if (!iter_.dense_iter_.isMoveLeftComplete()) {
	    iter_.passToSparse();
	    iter_.sparse_iter_.moveToLeftMostKey();
	}
    }
    if (!iter_.isValid()) return false;
    int compare = iter_.compare(right_key);
    if (compare == kCouldBePositive)
	return true;
    if (right_inclusive)
	return (compare <= 0);
    else
	return (compare < 0);
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

void SuRF::Iter::clear() {
    dense_iter_.clear();
    sparse_iter_.clear();
}

bool SuRF::Iter::getFpFlag() const {
    return could_be_fp_;
}

bool SuRF::Iter::isValid() const {
    return dense_iter_.isValid() 
	&& (dense_iter_.isComplete() || sparse_iter_.isValid());
}

int SuRF::Iter::compare(const std::string& key) const {
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

void SuRF::Iter::passToSparse() {
    sparse_iter_.setStartNodeNum(dense_iter_.getSendOutNodeNum());
}

bool SuRF::Iter::incrementDenseIter() {
    if (!dense_iter_.isValid()) 
	return false;

    dense_iter_++;
    if (!dense_iter_.isValid()) 
	return false;
    if (dense_iter_.isMoveLeftComplete()) 
	return true;

    passToSparse();
    sparse_iter_.moveToLeftMostKey();
    return true;
}

bool SuRF::Iter::incrementSparseIter() {
    if (!sparse_iter_.isValid()) 
	return false;
    sparse_iter_++;
    return sparse_iter_.isValid();
}

bool SuRF::Iter::operator ++(int) {
    if (!isValid()) 
	return false;
    if (incrementSparseIter()) 
	return true;
    return incrementDenseIter();
}

bool SuRF::Iter::decrementDenseIter() {
    if (!dense_iter_.isValid()) 
	return false;

    dense_iter_--;
    if (!dense_iter_.isValid()) 
	return false;
    if (dense_iter_.isMoveRightComplete()) 
	return true;

    passToSparse();
    sparse_iter_.moveToRightMostKey();
    return true;
}

bool SuRF::Iter::decrementSparseIter() {
    if (!sparse_iter_.isValid()) 
	return false;
    sparse_iter_--;
    return sparse_iter_.isValid();
}

bool SuRF::Iter::operator --(int) {
    if (!isValid()) 
	return false;
    if (decrementSparseIter()) 
	return true;
    return decrementDenseIter();
}

} // namespace surf

#endif // SURF_H
