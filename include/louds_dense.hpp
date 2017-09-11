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
    bool getKeyGreaterThan(const std::string& key, std::string& output_key, position_t& out_pos) const;
    bool getKeyGreaterThanOrEqualTo(const std::string& key, std::string& output_key, position_t& out_pos) const;
    bool lookupRange(const std::string& left_key, const std::string& right_key, position_t& out_left_pos, position_t& out_right_pos) const;
    uint32_t countRange(const std::string& left_key, const std::string& right_key, position_t& out_left_pos, position_t& out_right_pos) const;

    uint64_t getMemoryUsage();

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getSuffixPos(const position_t node_num, const position_t pos) const;

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
	pos = (node_num * kNodeFanout) + (label_t)key[level];
	if (level >= key.length()) { //if run out of searchKey bytes
	    if (prefixkey_indicator_bits_->readBit(node_num)) //if the prefix is also a key
		return suffixes_->checkEquality(getSuffixPos(node_num, pos), key, level + 1);
	    else
		return false;
	}

	if (!label_bitmaps_->readBit(pos)) //if key byte does not exist
	    return false;

	if (!child_indicator_bitmaps_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(node_num, pos), key, level + 1);

	node_num = getChildNodeNum(pos);
    }
    //search will continue in LoudsSparse
    out_node_num = node_num;
    return true;
}

bool LoudsDense::getKeyGreaterThan(const std::string& key, std::string& output_key, position_t& out_pos) const {
    return true;
}

bool LoudsDense::getKeyGreaterThanOrEqualTo(const std::string& key, std::string& output_key, position_t& out_pos) const {
    return true;
}

bool LoudsDense::lookupRange(const std::string& left_key, const std::string& right_key, position_t& out_left_pos, position_t& out_right_pos) const {
    return true;
}

uint32_t LoudsDense::countRange(const std::string& left_key, const std::string& right_key, position_t& out_left_pos, position_t& out_right_pos) const {
    return 0;
}

uint64_t LoudsDense::getMemoryUsage() {
    return (sizeof(this)
	    + label_bitmaps_->size()
	    + child_indicator_bitmaps_->size()
	    + prefixkey_indicator_bits_->size()
	    + suffixes_->size());
}

//TODO: need check off-by-one
position_t LoudsDense::getChildNodeNum(const position_t pos) const {
    return child_indicator_bitmaps_->rank(pos);
}

//TODO: need check off-by-one
position_t LoudsDense::getSuffixPos(const position_t node_num, const position_t pos) const {
    return (label_bitmaps_->rank(pos)
	    - child_indicator_bitmaps_->rank(pos)
	    + prefixkey_indicator_bits_->rank(node_num)
	    - 1);
}

} //namespace surf

#endif // LOUDSDENSE_H_
