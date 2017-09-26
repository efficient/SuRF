#ifndef LOUDSSPARSE_H_
#define LOUDSSPARSE_H_

#include <string>

#include "config.hpp"
#include "label_vector.hpp"
#include "rank.hpp"
#include "select.hpp"
#include "suffix_vector.hpp"
#include "surf_builder.hpp"

namespace surf {

class LoudsSparse {
public:
    class Iter {
    public:
	Iter() : is_valid_(false) {};
	Iter(LoudsSparse* trie) : is_valid_(false), trie_(trie), start_node_num_(0), 
				  key_len_(0), is_at_terminator(false) {
	    start_level_ = trie_->getStartLevel();
	    for (level_t level = start_level_; level < trie_->getHeight(); level++) {
		key_.push_back(0);
		pos_in_trie_.push_back(0);
	    }
	}

	bool isValid() const { return is_valid_; };
	std::string getKey() const;

	position_t getStartNodeNum() const { return start_node_num_; };
	void setStartNodeNum(position_t node_num) { start_node_num_ = node_num; };

	void moveToLeftMostKey();
	void operator ++(int);

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
	bool is_at_terminator;

	friend class LoudsSparse;
    };

public:
    LoudsSparse() {};
    LoudsSparse(const SuRFBuilder* builder);

    ~LoudsSparse() {
	delete labels_;
	delete child_indicator_bits_;
	delete louds_bits_;
	delete suffixes_;
    }

    // point query: trie walk starts at node "in_node_num" instead of root
    // in_node_num is provided by louds-dense's lookupKey function
    bool lookupKey(const std::string& key, const position_t in_node_num) const;

    // Returns true if iter is valid
    void moveToKeyGreaterThan(const std::string& key, const bool inclusive, LoudsSparse::Iter& iter) const;
    bool lookupRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const;
    uint32_t countRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const;

    level_t getHeight() const { return height_; };
    level_t getStartLevel() const { return start_level_; };
    uint64_t getMemoryUsage() const;

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getFirstLabelPos(const position_t node_num) const;
    position_t getSuffixPos(const position_t pos) const;
    position_t nodeSize(const position_t pos) const;

    void moveToLeftInNextSubtrie(position_t pos, const position_t node_size, const label_t label, LoudsSparse::Iter& iter) const;
    void compareSuffixGreaterThan(const position_t pos, const std::string& key, const level_t level, const bool inclusive, LoudsSparse::Iter& iter) const;

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
    SuffixVector* suffixes_;
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

    std::vector<position_t> num_items_per_level;
    for (level_t level = 0; level < height_; level++)
	num_items_per_level.push_back(builder->getLabels()[level].size());

    labels_ = new LabelVector(builder->getLabels(), start_level_, height_);
    child_indicator_bits_ = new BitvectorRank(kRankBasicBlockSize, builder->getChildIndicatorBits(), num_items_per_level, start_level_, height_);
    louds_bits_ = new BitvectorSelect(kSelectSampleInterval, builder->getLoudsBits(), num_items_per_level, start_level_, height_);
    suffixes_ = new SuffixVector(builder->getSuffixConfig(), builder->getSuffixes(), start_level_, height_);
}

bool LoudsSparse::lookupKey(const std::string& key, const position_t in_node_num) const {
    position_t node_num = in_node_num;
    position_t pos = getFirstLabelPos(node_num);
    level_t level = 0;
    for (level = start_level_; level < key.length(); level++) {
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

void LoudsSparse::moveToKeyGreaterThan(const std::string& key, const bool inclusive, LoudsSparse::Iter& iter) const {
    position_t node_num = iter.getStartNodeNum();
    position_t pos = getFirstLabelPos(node_num);
    level_t level;
    for (level = start_level_; level < key.length(); level++) {
	position_t node_size = nodeSize(pos);
	// if no exact match
	if (!labels_->search((label_t)key[level], pos, node_size))
	    return moveToLeftInNextSubtrie(pos, node_size, key[level], iter);

	iter.append(key[level], pos);

	// if trie branch terminates
	if (!child_indicator_bits_->readBit(pos))
	    return compareSuffixGreaterThan(pos, key, level+1, inclusive, iter);

	// move to child
	node_num = getChildNodeNum(pos);
	pos = getFirstLabelPos(node_num);
    }
    if ((labels_->read(pos) == kTerminator) && (!child_indicator_bits_->readBit(pos))) {
	iter.append(kTerminator, pos);
	iter.is_at_terminator = true;
    }
    if (!inclusive)
	return iter++;
    iter.is_valid_ = true;
}

bool LoudsSparse::lookupRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const {
    return true;
}

uint32_t LoudsSparse::countRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const {
    return 0;
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

position_t LoudsSparse::getSuffixPos(const position_t pos) const {
    return (pos - child_indicator_bits_->rank(pos));
}

position_t LoudsSparse::nodeSize(const position_t pos) const {
    assert(louds_bits_->readBit(pos));
    return louds_bits_->distanceToNextSetBit(pos);
}

void LoudsSparse::moveToLeftInNextSubtrie(position_t pos, const position_t node_size, const label_t label, LoudsSparse::Iter& iter) const {
    // if no label is greater than key[level] in this node
    if (!labels_->searchGreaterThan(label, pos, node_size)) {
	iter.append(pos + node_size - 1);
	return iter++;
    } else {
	iter.append(pos);
	return iter.moveToLeftMostKey();
    }
}

void LoudsSparse::compareSuffixGreaterThan(const position_t pos, const std::string& key, const level_t level, const bool inclusive, LoudsSparse::Iter& iter) const {
    if (suffixes_->getType() == kReal) {
	position_t suffix_pos = getSuffixPos(pos);
	int compare = suffixes_->compare(suffix_pos, key[level]);
	if ((compare < 0) || (compare == 0 && !inclusive))
	    return iter++;
    } else {
	if (!inclusive)
	    return iter++;
    }
    iter.is_valid_ = true;
}

//============================================================================

std::string LoudsSparse::Iter::getKey() const {
    if (!is_valid_) 
	return std::string();
    level_t len = key_len_;
    if (is_at_terminator) 
	len--;
    std::string ret_str = std::string((const char*)key_.data(), (size_t)len);
    position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1]);
    if (trie_->suffixes_->getType() == kReal && trie_->suffixes_->read(suffix_pos) > 0)
	ret_str += std::string((const char*)(trie_->suffixes_->move(suffix_pos)), sizeof(suffix_t));
    return ret_str;
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
    assert((level - start_level_) < key_.size());
    key_[level - start_level_] = trie_->labels_->read(pos);
    pos_in_trie_[level - start_level_] = pos;
}

void LoudsSparse::Iter::moveToLeftMostKey() {
    if (key_len_ == 0) {
	position_t pos = trie_->getFirstLabelPos(start_node_num_);
	label_t label = trie_->labels_->read(pos);
	append(label, pos);
    }

    level_t level = start_level_ + key_len_ - 1;
    position_t pos = pos_in_trie_[key_len_ - 1];
    label_t label = trie_->labels_->read(pos);

    if (!trie_->child_indicator_bits_->readBit(pos)) {
	if (label == kTerminator)
	    is_at_terminator = true;
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
	    if (label == kTerminator)
		is_at_terminator = true;
	    is_valid_ = true;
	    return;
	}
	append(label, pos);
	level++;
    }
    assert(false); // shouldn't have reached here
}

void LoudsSparse::Iter::operator ++(int) {
    assert(key_len_ > 0);
    is_at_terminator = false;
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

} // namespace surf

#endif // LOUDSSPARSE_H_
