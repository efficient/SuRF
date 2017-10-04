#ifndef LOUDSDENSE_H_
#define LOUDSDENSE_H_

#include <string>

#include "config.hpp"
#include "rank.hpp"
#include "suffix_vector.hpp"
#include "surf_builder.hpp"

namespace surf {

class LoudsDense {
public:
    class Iter {
    public:
	Iter() : is_valid_(false) {};
	Iter(LoudsDense* trie) : is_valid_(false), is_search_complete_(false), 
				 is_move_left_complete_(false), trie_(trie), 
				 send_out_node_num_(0), key_len_(0), 
				 is_at_prefix_key_(false) {
				 
	    for (level_t level = 0; level < trie_->getHeight(); level++) {
		key_.push_back(0);
		pos_in_trie_.push_back(0);
	    }
	}

	bool isValid() const { return is_valid_; };
	bool isSearchComplete() const { return is_search_complete_; };
	bool isMoveLeftComplete() const { return is_move_left_complete_; };
	bool isComplete() const { return (is_search_complete_ && is_move_left_complete_); };


	std::string getKey() const;
	position_t getSendOutNodeNum() const { return send_out_node_num_; };

	void moveToLeftMostKey();
	void operator ++(int);

    private:
	void append(position_t pos);
	void set(level_t level, position_t pos);
	void setSendOutNodeNum(position_t node_num) { send_out_node_num_ = node_num; };
	void setFlags(const bool is_valid, const bool is_search_complete, 
		      const bool is_move_left_complete);

    private:
	// True means the iter either points to a valid key 
	// or to a prefix with length trie_->getHeight()
	bool is_valid_;
	// If false, call moveToKeyGreaterThan in LoudsSparse to complete
	bool is_search_complete_; 
	// If false, call moveToLeftMostKey in LoudsSparse to complete
	bool is_move_left_complete_; 
	LoudsDense* trie_;
	position_t send_out_node_num_;
	level_t key_len_; // Does NOT include suffix

	std::vector<label_t> key_;
	std::vector<position_t> pos_in_trie_;
	bool is_at_prefix_key_;

	friend class LoudsDense;
    };

public:
    LoudsDense() {};
    LoudsDense(const SuRFBuilder* builder);

    ~LoudsDense() {
	delete label_bitmaps_;
	delete child_indicator_bitmaps_;
	delete prefixkey_indicator_bits_;
	delete suffixes_;
    }

    // Returns whether key exists in the trie so far
    // out_node_num == 0 means search terminates in louds-dense.
    bool lookupKey(const std::string& key, position_t& out_node_num) const;

    void moveToKeyGreaterThan(const std::string& key, const bool inclusive, LoudsDense::Iter& iter) const;
    uint32_t countRange(const std::string& left_key, const std::string& right_key, position_t& out_left_pos, position_t& out_right_pos) const;

    uint64_t getHeight() const { return height_; };
    uint64_t getMemoryUsage() const;

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getSuffixPos(const position_t pos, const bool is_prefix_key) const;
    position_t getNextPos(const position_t pos) const;

    void compareSuffixGreaterThan(const position_t pos, const std::string& key, const level_t level, const bool inclusive, LoudsDense::Iter& iter) const;

private:
    static const position_t kNodeFanout = 256;
    static const position_t kRankBasicBlockSize  = 512;

    level_t height_;

    BitvectorRank* label_bitmaps_;
    BitvectorRank* child_indicator_bitmaps_;
    BitvectorRank* prefixkey_indicator_bits_; //1 bit per internal node
    SuffixVector* suffixes_;
};


LoudsDense::LoudsDense(const SuRFBuilder* builder) {
    height_ = builder->getSparseStartLevel();
    std::vector<position_t> num_bits_per_level;
    for (level_t level = 0; level < height_; level++)
	num_bits_per_level.push_back(builder->getBitmapLabels()[level].size() * kWordSize);

    label_bitmaps_ = new BitvectorRank(kRankBasicBlockSize, builder->getBitmapLabels(), num_bits_per_level, 0, height_);
    child_indicator_bitmaps_ = new BitvectorRank(kRankBasicBlockSize, builder->getBitmapChildIndicatorBits(), num_bits_per_level, 0, height_);
    prefixkey_indicator_bits_ = new BitvectorRank(kRankBasicBlockSize, builder->getPrefixkeyIndicatorBits(), builder->getNodeCounts(), 0, height_);
    suffixes_ = new SuffixVector(builder->getSuffixConfig(), builder->getSuffixes(), 0, height_);
}

bool LoudsDense::lookupKey(const std::string& key, position_t& out_node_num) const {
    position_t node_num = 0;
    position_t pos = 0;
    for (level_t level = 0; level < height_; level++) {
	pos = (node_num * kNodeFanout);
	if (level >= key.length()) { //if run out of searchKey bytes
	    if (prefixkey_indicator_bits_->readBit(node_num)) //if the prefix is also a key
		return suffixes_->checkEquality(getSuffixPos(pos, true), key, level + 1);
	    else
		return false;
	}
	pos += (label_t)key[level];

	if (!label_bitmaps_->readBit(pos)) //if key byte does not exist
	    return false;

	if (!child_indicator_bitmaps_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(pos, false), key, level + 1);

	node_num = getChildNodeNum(pos);
    }
    //search will continue in LoudsSparse
    out_node_num = node_num;
    return true;
}

void LoudsDense::moveToKeyGreaterThan(const std::string& key, const bool inclusive, LoudsDense::Iter& iter) const {
    position_t node_num = 0;
    position_t pos = 0;
    for (level_t level = 0; level < height_; level++) {
	// if is_at_prefix_key_, pos is at the next valid position in the child node
	pos = node_num * kNodeFanout;
	if (level >= key.length()) { //if run out of searchKey bytes
	    iter.append(getNextPos(pos - 1));
	    iter.is_at_prefix_key_ = true;
	    if (!inclusive)
		return iter++;
	    return iter.setFlags(true, true, true); // valid, search complete, moveLeft complete
	}

	pos += (label_t)key[level];
	iter.append(pos);

	// if no exact match
	if (!label_bitmaps_->readBit(pos))
	    return iter++;

	//if trie branch terminates
	if (!child_indicator_bitmaps_->readBit(pos)) {
	    return compareSuffixGreaterThan(pos, key, level+1, inclusive, iter);
	}

	node_num = getChildNodeNum(pos);
    }

    //search will continue in LoudsSparse
    iter.setSendOutNodeNum(node_num);
    iter.setFlags(true, false, true); // valid, search INCOMPLETE, moveLeft complete
}

// TODO
uint32_t LoudsDense::countRange(const std::string& left_key, const std::string& right_key, position_t& out_left_pos, position_t& out_right_pos) const {
    return 0;
}

uint64_t LoudsDense::getMemoryUsage() const {
    return (sizeof(this)
	    + label_bitmaps_->size()
	    + child_indicator_bitmaps_->size()
	    + prefixkey_indicator_bits_->size()
	    + suffixes_->size());
}

position_t LoudsDense::getChildNodeNum(const position_t pos) const {
    return child_indicator_bitmaps_->rank(pos);
}

position_t LoudsDense::getSuffixPos(const position_t pos, const bool is_prefix_key) const {
    position_t node_num = pos / kNodeFanout;
    position_t suffix_pos = (label_bitmaps_->rank(pos)
			     - child_indicator_bitmaps_->rank(pos)
			     + prefixkey_indicator_bits_->rank(node_num)
			     - 1);
    if (is_prefix_key && label_bitmaps_->readBit(pos))
	suffix_pos--;
    return suffix_pos;
}

position_t LoudsDense::getNextPos(const position_t pos) const {
    return pos + label_bitmaps_->distanceToNextSetBit(pos);
}

void LoudsDense::compareSuffixGreaterThan(const position_t pos, const std::string& key, const level_t level, const bool inclusive, LoudsDense::Iter& iter) const {
    if (suffixes_->getType() == kReal) {
	position_t suffix_pos = getSuffixPos(pos, false);
	int compare = suffixes_->compare(suffix_pos, key[level]);
	if ((compare < 0) || (compare == 0 && !inclusive))
	    return iter++;
    } else {
	if (!inclusive)
	    return iter++;
    }
    iter.setFlags(true, true, true); // valid, search complete, moveLeft complete    
}

//============================================================================

std::string LoudsDense::Iter::getKey() const {
    if (!is_valid_)
	return std::string();
    level_t len = key_len_;
    if (is_at_prefix_key_) 
	len--;
    std::string ret_str = std::string((const char*)key_.data(), (size_t)len);
    if (isComplete() && !is_at_prefix_key_) {
	position_t suffix_pos = trie_->getSuffixPos(pos_in_trie_[key_len_ - 1], is_at_prefix_key_);
	if (trie_->suffixes_->getType() == kReal && trie_->suffixes_->read(suffix_pos) > 0)
	    ret_str += std::string((const char*)(trie_->suffixes_->move(suffix_pos)), sizeof(suffix_t));
    }
    return ret_str;
}

void LoudsDense::Iter::append(position_t pos) {
    assert(key_len_ < key_.size());
    key_[key_len_] = (label_t)(pos % kNodeFanout);
    pos_in_trie_[key_len_] = pos;
    key_len_++;
}

void LoudsDense::Iter::set(level_t level, position_t pos) {
    assert(level < key_.size());
    key_[level] = (label_t)(pos % kNodeFanout);
    pos_in_trie_[level] = pos;
}

void LoudsDense::Iter::setFlags(const bool is_valid, const bool is_search_complete, 
				const bool is_move_left_complete) {
    is_valid_ = is_valid;
    is_search_complete_ = is_search_complete;
    is_move_left_complete_ = is_move_left_complete;
}

void LoudsDense::Iter::moveToLeftMostKey() {
    assert(key_len_ > 0);
    level_t level = key_len_ - 1;
    position_t pos = pos_in_trie_[level];

    if (!trie_->child_indicator_bitmaps_->readBit(pos))
	return setFlags(true, true, true); // valid, search complete, moveLeft complete

    while (level < trie_->getHeight() - 1) {
	position_t node_num = trie_->getChildNodeNum(pos);
	//if the current prefix is also a key
	if (trie_->prefixkey_indicator_bits_->readBit(node_num)) {
	    append(trie_->getNextPos(node_num * kNodeFanout - 1));
	    is_at_prefix_key_ = true;
	    return setFlags(true, true, true); // valid, search complete, moveLeft complete
	}

	pos = trie_->getNextPos(node_num * kNodeFanout - 1);
	append(pos);

	// if trie branch terminates
	if (!trie_->child_indicator_bitmaps_->readBit(pos))
	    return setFlags(true, true, true); // valid, search complete, moveLeft complete

	level++;
    }
    send_out_node_num_ = trie_->getChildNodeNum(pos);
    setFlags(true, true, false); // valid, search complete, moveLeft INCOMPLETE
}

void LoudsDense::Iter::operator ++(int) {
    assert(key_len_ > 0);
    if (is_at_prefix_key_) {
	is_at_prefix_key_ = false;
	return moveToLeftMostKey();
    }
    position_t pos = pos_in_trie_[key_len_ - 1];
    position_t next_pos = trie_->getNextPos(pos);
    // if crossing node boundary
    while ((next_pos / kNodeFanout) > (pos / kNodeFanout)) {
	key_len_--;
	if (key_len_ == 0) {
	    is_valid_ = false;
	    return;
	}
	pos = pos_in_trie_[key_len_ - 1];
	next_pos = trie_->getNextPos(pos);
    }
    set(key_len_ - 1, next_pos);
    return moveToLeftMostKey();
}

} //namespace surf

#endif // LOUDSDENSE_H_
