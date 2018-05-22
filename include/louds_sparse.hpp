#ifndef LOUDSSPARSE_H_
#define LOUDSSPARSE_H_

#include <string>

#include "config.hpp"
#include "label_vector.hpp"
#include "rank.hpp"
#include "select.hpp"
#include "suffix.hpp"
#include "surf_builder.hpp"

namespace surf {

class LoudsSparse {
public:
    class Iter {
    public:
	Iter() : is_valid_(false) {};
	Iter(LoudsSparse* trie) : is_valid_(false), trie_(trie), start_node_num_(0), 
				  key_len_(0), is_at_terminator_(false) {
	    start_level_ = trie_->getStartLevel();
	    for (level_t level = start_level_; level < trie_->getHeight(); level++) {
		key_.push_back(0);
		pos_in_trie_.push_back(0);
	    }
	}

	void clear();
	bool isValid() const { return is_valid_; };
	int compare(const std::string& key) const;
	std::string getKey() const;
        int getSuffix(word_t* suffix) const;
	std::string getKeyWithSuffix(unsigned* bitlen) const;

	position_t getStartNodeNum() const { return start_node_num_; };
	void setStartNodeNum(position_t node_num) { start_node_num_ = node_num; };

	void setToFirstLabelInRoot();
	void setToLastLabelInRoot();
	void moveToLeftMostKey();
	void moveToRightMostKey();
	void operator ++(int);
	void operator --(int);

    private:
	void append(const position_t pos);
	void append(const label_t label, const position_t pos);
	void set(const level_t level, const position_t pos);

    private:
	bool is_valid_; // True means the iter currently points to a valid key
	LoudsSparse* trie_;
	level_t start_level_;
	position_t start_node_num_; // Passed in by the dense iterator; default = 0
	level_t key_len_; // Start counting from start_level_; does NOT include suffix

	std::vector<label_t> key_;
	std::vector<position_t> pos_in_trie_;
	bool is_at_terminator_;

	friend class LoudsSparse;
    };

public:
    LoudsSparse() {};
    LoudsSparse(const SuRFBuilder* builder);

    ~LoudsSparse() {}

    // point query: trie walk starts at node "in_node_num" instead of root
    // in_node_num is provided by louds-dense's lookupKey function
    bool lookupKey(const std::string& key, const position_t in_node_num) const;
    // return value indicates potential false positive
    bool moveToKeyGreaterThan(const std::string& key, 
			      const bool inclusive, LoudsSparse::Iter& iter) const;

    level_t getHeight() const { return height_; };
    level_t getStartLevel() const { return start_level_; };
    uint64_t serializedSize() const;
    uint64_t getMemoryUsage() const;

    void serialize(char*& dst) const {
	memcpy(dst, &height_, sizeof(height_));
	dst += sizeof(height_);
	memcpy(dst, &start_level_, sizeof(start_level_));
	dst += sizeof(start_level_);
	memcpy(dst, &node_count_dense_, sizeof(node_count_dense_));
	dst += sizeof(node_count_dense_);
	memcpy(dst, &child_count_dense_, sizeof(child_count_dense_));
	dst += sizeof(child_count_dense_);
	align(dst);
	labels_->serialize(dst);
	child_indicator_bits_->serialize(dst);
	louds_bits_->serialize(dst);
	suffixes_->serialize(dst);
	align(dst);
    }

    static LoudsSparse* deSerialize(char*& src) {
	LoudsSparse* louds_sparse = new LoudsSparse();
	memcpy(&(louds_sparse->height_), src, sizeof(louds_sparse->height_));
	src += sizeof(louds_sparse->height_);
	memcpy(&(louds_sparse->start_level_), src, sizeof(louds_sparse->start_level_));
	src += sizeof(louds_sparse->start_level_);
	memcpy(&(louds_sparse->node_count_dense_), src, sizeof(louds_sparse->node_count_dense_));
	src += sizeof(louds_sparse->node_count_dense_);
	memcpy(&(louds_sparse->child_count_dense_), src, sizeof(louds_sparse->child_count_dense_));
	src += sizeof(louds_sparse->child_count_dense_);
	align(src);
	louds_sparse->labels_ = LabelVector::deSerialize(src);
	louds_sparse->child_indicator_bits_ = BitvectorRank::deSerialize(src);
	louds_sparse->louds_bits_ = BitvectorSelect::deSerialize(src);
	louds_sparse->suffixes_ = BitvectorSuffix::deSerialize(src);
	align(src);
	return louds_sparse;
    }

    void destroy() {
	labels_->destroy();
	child_indicator_bits_->destroy();
	louds_bits_->destroy();
	suffixes_->destroy();
    }

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getFirstLabelPos(const position_t node_num) const;
    position_t getLastLabelPos(const position_t node_num) const;
    position_t getSuffixPos(const position_t pos) const;
    position_t nodeSize(const position_t pos) const;

    void moveToLeftInNextSubtrie(position_t pos, const position_t node_size, 
				 const label_t label, LoudsSparse::Iter& iter) const;
    // return value indicates potential false positive
    bool compareSuffixGreaterThan(const position_t pos, const std::string& key, 
				  const level_t level, const bool inclusive, 
				  LoudsSparse::Iter& iter) const;

private:
    static const position_t kRankBasicBlockSize = 512;
    static const position_t kSelectSampleInterval = 64;

    level_t height_; // trie height
    level_t start_level_; // louds-sparse encoding starts at this level
    // number of nodes in louds-dense encoding
    position_t node_count_dense_;
    // number of children(1's in child indicator bitmap) in louds-dense encoding
    position_t child_count_dense_;

    LabelVector* labels_;
    BitvectorRank* child_indicator_bits_;
    BitvectorSelect* louds_bits_;
    BitvectorSuffix* suffixes_;
};


LoudsSparse::LoudsSparse(const SuRFBuilder* builder) {
    height_ = builder->getLabels().size();
    start_level_ = builder->getSparseStartLevel();

    node_count_dense_ = 0;
    for (level_t level = 0; level < start_level_; level++)
	node_count_dense_ += builder->getNodeCounts()[level];

    if (start_level_ == 0)
	child_count_dense_ = 0;
    else
	child_count_dense_ = node_count_dense_ + builder->getNodeCounts()[start_level_] - 1;

    labels_ = new LabelVector(builder->getLabels(), start_level_, height_);

    std::vector<position_t> num_items_per_level;
    for (level_t level = 0; level < height_; level++)
	num_items_per_level.push_back(builder->getLabels()[level].size());

    child_indicator_bits_ = new BitvectorRank(kRankBasicBlockSize, builder->getChildIndicatorBits(), 
					      num_items_per_level, start_level_, height_);
    louds_bits_ = new BitvectorSelect(kSelectSampleInterval, builder->getLoudsBits(), 
				      num_items_per_level, start_level_, height_);

    if (builder->getSuffixType() == kNone) {
	suffixes_ = new BitvectorSuffix();
    } else {
	level_t hash_suffix_len = builder->getHashSuffixLen();
        level_t real_suffix_len = builder->getRealSuffixLen();
        level_t suffix_len = hash_suffix_len + real_suffix_len;
	std::vector<position_t> num_suffix_bits_per_level;
	for (level_t level = 0; level < height_; level++)
	    num_suffix_bits_per_level.push_back(builder->getSuffixCounts()[level] * suffix_len);

	suffixes_ = new BitvectorSuffix(builder->getSuffixType(), hash_suffix_len, real_suffix_len,
                                        builder->getSuffixes(),
					num_suffix_bits_per_level, start_level_, height_);
    }
}

bool LoudsSparse::lookupKey(const std::string& key, const position_t in_node_num) const {
    position_t node_num = in_node_num;
    position_t pos = getFirstLabelPos(node_num);
    level_t level = 0;
    for (level = start_level_; level < key.length(); level++) {
	//child_indicator_bits_->prefetch(pos);
	if (!labels_->search((label_t)key[level], pos, nodeSize(pos)))
	    return false;

	// if trie branch terminates
	if (!child_indicator_bits_->readBit(pos))
	    return suffixes_->checkEquality(getSuffixPos(pos), key, level + 1);

	// move to child
	node_num = getChildNodeNum(pos);
	pos = getFirstLabelPos(node_num);
    }
    if ((labels_->read(pos) == kTerminator) && (!child_indicator_bits_->readBit(pos)))
	return suffixes_->checkEquality(getSuffixPos(pos), key, level + 1);
    return false;
}

bool LoudsSparse::moveToKeyGreaterThan(const std::string& key, 
				       const bool inclusive, LoudsSparse::Iter& iter) const {
    position_t node_num = iter.getStartNodeNum();
    position_t pos = getFirstLabelPos(node_num);

    level_t level;
    for (level = start_level_; level < key.length(); level++) {
	position_t node_size = nodeSize(pos);
	// if no exact match
	if (!labels_->search((label_t)key[level], pos, node_size)) {
	    moveToLeftInNextSubtrie(pos, node_size, key[level], iter);
	    return false;
	}

	iter.append(key[level], pos);

	// if trie branch terminates
	if (!child_indicator_bits_->readBit(pos))
	    return compareSuffixGreaterThan(pos, key, level+1, inclusive, iter);

	// move to child
	node_num = getChildNodeNum(pos);
	pos = getFirstLabelPos(node_num);
    }

    if ((labels_->read(pos) == kTerminator) && (!child_indicator_bits_->readBit(pos))
	&& !louds_bits_->readBit(pos + 1)) {
	iter.append(kTerminator, pos);
	iter.is_at_terminator_ = true;
	if (!inclusive)
	    iter++;
	iter.is_valid_ = true;
	return false;
    }

    if (key.length() <= level) {
	iter.moveToLeftMostKey();
	return false;
    }

    iter.is_valid_ = true;
    return true;
}

uint64_t LoudsSparse::serializedSize() const {
    uint64_t size = sizeof(height_) + sizeof(start_level_)
	+ sizeof(node_count_dense_) + sizeof(child_count_dense_)
	+ labels_->serializedSize()
	+ child_indicator_bits_->serializedSize()
	+ louds_bits_->serializedSize()
	+ suffixes_->serializedSize();
    	sizeAlign(size);
	return size;
}

uint64_t LoudsSparse::getMemoryUsage() const {
    return (sizeof(this)
	    + labels_->size()
	    + child_indicator_bits_->size()
	    + louds_bits_->size()
	    + suffixes_->size());
}

position_t LoudsSparse::getChildNodeNum(const position_t pos) const {
    return (child_indicator_bits_->rank(pos) + child_count_dense_);
}

position_t LoudsSparse::getFirstLabelPos(const position_t node_num) const {
    return louds_bits_->select(node_num + 1 - node_count_dense_);
}

position_t LoudsSparse::getLastLabelPos(const position_t node_num) const {
    position_t next_rank = node_num + 2 - node_count_dense_;
    if (next_rank > louds_bits_->numOnes())
	return (louds_bits_->numBits() - 1);
    return (louds_bits_->select(next_rank) - 1);
}

position_t LoudsSparse::getSuffixPos(const position_t pos) const {
    return (pos - child_indicator_bits_->rank(pos));
}

position_t LoudsSparse::nodeSize(const position_t pos) const {
    assert(louds_bits_->readBit(pos));
    return louds_bits_->distanceToNextSetBit(pos);
}

void LoudsSparse::moveToLeftInNextSubtrie(position_t pos, const position_t node_size, 
					  const label_t label, LoudsSparse::Iter& iter) const {
    // if no label is greater than key[level] in this node
    if (!labels_->searchGreaterThan(label, pos, node_size)) {
	iter.append(pos + node_size - 1);
	return iter++;
    } else {
	iter.append(pos);
	return iter.moveToLeftMostKey();
    }
}

bool LoudsSparse::compareSuffixGreaterThan(const position_t pos, const std::string& key, 
					   const level_t level, const bool inclusive, 
					   LoudsSparse::Iter& iter) const {
    position_t suffix_pos = getSuffixPos(pos);
    int compare = suffixes_->compare(suffix_pos, key, level);
    if ((compare != kCouldBePositive) && (compare < 0)) {
	iter++;
	return false;
    }
    iter.is_valid_ = true;
    return true;
}

//============================================================================

void LoudsSparse::Iter::clear() {
    is_valid_ = false;
    key_len_ = 0;
    is_at_terminator_ = false;
}

int LoudsSparse::Iter::compare(const std::string& key) const {
    if (is_at_terminator_ && (key_len_ - 1) < (key.length() - start_level_))
	return -1;
    std::string iter_key = getKey();
    std::string key_sparse = key.substr(start_level_);
    std::string key_sparse_same_length = key_sparse.substr(0, iter_key.length());
    int compare = iter_key.compare(key_sparse_same_length);
    if (compare != 0) 
	return compare;
    position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1]);
    return trie_->suffixes_->compare(suffix_pos, key_sparse, key_len_);
}

std::string LoudsSparse::Iter::getKey() const {
    if (!is_valid_)
	return std::string();
    level_t len = key_len_;
    if (is_at_terminator_)
	len--;
    return std::string((const char*)key_.data(), (size_t)len);
}

int LoudsSparse::Iter::getSuffix(word_t* suffix) const {
    if ((trie_->suffixes_->getType() == kReal) || (trie_->suffixes_->getType() == kMixed)) {
	position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1]);
	*suffix = trie_->suffixes_->readReal(suffix_pos);
	return trie_->suffixes_->getRealSuffixLen();
    }
    *suffix = 0;
    return 0;
}

std::string LoudsSparse::Iter::getKeyWithSuffix(unsigned* bitlen) const {
    std::string iter_key = getKey();
    if ((trie_->suffixes_->getType() == kReal) || (trie_->suffixes_->getType() == kMixed)) {
	position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1]);
	word_t suffix = trie_->suffixes_->readReal(suffix_pos);
	if (suffix > 0) {
	    level_t suffix_len = trie_->suffixes_->getRealSuffixLen();
	    *bitlen = suffix_len % 8;
	    suffix <<= (64 - suffix_len);
	    char* suffix_str = reinterpret_cast<char*>(&suffix);
	    suffix_str += 7;
	    unsigned pos = 0;
	    while (pos < suffix_len) {
		iter_key.append(suffix_str, 1);
		suffix_str--;
		pos += 8;
	    }
	}
    }
    return iter_key;
}

void LoudsSparse::Iter::append(const position_t pos) {
    assert(key_len_ < key_.size());
    key_[key_len_] = trie_->labels_->read(pos);
    pos_in_trie_[key_len_] = pos;
    key_len_++;
}

void LoudsSparse::Iter::append(const label_t label, const position_t pos) {
    assert(key_len_ < key_.size());
    key_[key_len_] = label;
    pos_in_trie_[key_len_] = pos;
    key_len_++;
}

void LoudsSparse::Iter::set(const level_t level, const position_t pos) {
    assert(level < key_.size());
    key_[level] = trie_->labels_->read(pos);
    pos_in_trie_[level] = pos;
}

void LoudsSparse::Iter::setToFirstLabelInRoot() {
    assert(start_level_ == 0);
    pos_in_trie_[0] = 0;
    key_[0] = trie_->labels_->read(0);
}

void LoudsSparse::Iter::setToLastLabelInRoot() {
    assert(start_level_ == 0);
    pos_in_trie_[0] = trie_->getLastLabelPos(0);
    key_[0] = trie_->labels_->read(pos_in_trie_[0]);
}

void LoudsSparse::Iter::moveToLeftMostKey() {
    if (key_len_ == 0) {
	position_t pos = trie_->getFirstLabelPos(start_node_num_);
	label_t label = trie_->labels_->read(pos);
	append(label, pos);
    }

    level_t level = key_len_ - 1;
    position_t pos = pos_in_trie_[level];
    label_t label = trie_->labels_->read(pos);

    if (!trie_->child_indicator_bits_->readBit(pos)) {
	if ((label == kTerminator) && !trie_->louds_bits_->readBit(pos + 1))
	    is_at_terminator_ = true;
	is_valid_ = true;
	return;
    }

    while (level < trie_->getHeight()) {
	position_t node_num = trie_->getChildNodeNum(pos);
	pos = trie_->getFirstLabelPos(node_num);
	label = trie_->labels_->read(pos);
	// if trie branch terminates
	if (!trie_->child_indicator_bits_->readBit(pos)) {
	    append(label, pos);
	    if ((label == kTerminator) && !trie_->louds_bits_->readBit(pos + 1))
		is_at_terminator_ = true;
	    is_valid_ = true;
	    return;
	}
	append(label, pos);
	level++;
    }
    assert(false); // shouldn't reach here
}

void LoudsSparse::Iter::moveToRightMostKey() {
    if (key_len_ == 0) {
	position_t pos = trie_->getFirstLabelPos(start_node_num_);
	pos = trie_->getLastLabelPos(start_node_num_);
	label_t label = trie_->labels_->read(pos);
	append(label, pos);
    }

    level_t level = key_len_ - 1;
    position_t pos = pos_in_trie_[level];
    label_t label = trie_->labels_->read(pos);

    if (!trie_->child_indicator_bits_->readBit(pos)) {
	if ((label == kTerminator) && !trie_->louds_bits_->readBit(pos + 1))
	    is_at_terminator_ = true;
	is_valid_ = true;
	return;
    }

    while (level < trie_->getHeight()) {
	position_t node_num = trie_->getChildNodeNum(pos);
	pos = trie_->getLastLabelPos(node_num);
	label = trie_->labels_->read(pos);
	// if trie branch terminates
	if (!trie_->child_indicator_bits_->readBit(pos)) {
	    append(label, pos);
	    if ((label == kTerminator) && !trie_->louds_bits_->readBit(pos + 1))
		is_at_terminator_ = true;
	    is_valid_ = true;
	    return;
	}
	append(label, pos);
	level++;
    }
    assert(false); // shouldn't reach here
}

void LoudsSparse::Iter::operator ++(int) {
    assert(key_len_ > 0);
    is_at_terminator_ = false;
    position_t pos = pos_in_trie_[key_len_ - 1];
    pos++;
    while (pos >= trie_->louds_bits_->numBits() || trie_->louds_bits_->readBit(pos)) {
	key_len_--;
	if (key_len_ == 0) {
	    is_valid_ = false;
	    return;
	}
	pos = pos_in_trie_[key_len_ - 1];
	pos++;
    }
    set(key_len_ - 1, pos);
    return moveToLeftMostKey();
}

void LoudsSparse::Iter::operator --(int) {
    assert(key_len_ > 0);
    is_at_terminator_ = false;
    position_t pos = pos_in_trie_[key_len_ - 1];
    if (pos == 0) {
	is_valid_ = false;
	return;
    }
    while (trie_->louds_bits_->readBit(pos)) {
	key_len_--;
	if (key_len_ == 0) {
	    is_valid_ = false;
	    return;
	}
	pos = pos_in_trie_[key_len_ - 1];
    }
    pos--;
    set(key_len_ - 1, pos);
    return moveToRightMostKey();
}

} // namespace surf

#endif // LOUDSSPARSE_H_
