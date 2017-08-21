#ifndef LOUDSDENSE_H_
#define LOUDSDENSE_H_

#include "config.hpp"
#include "rank.hpp"
#include "suffixVector.hpp"

using namespace std;

class LoudsDense {
public:
    LoudsDense() {};

    LoudsDense(const vector<vector<uint64_t> > &bitmap_labelsPL,
	       const vector<vector<uint64_t> > &bitmap_childIndicatorBitsPL,
	       const vector<vector<uint64_t> > &prefixKeyIndicatorBitsPL,
	       const vector<vector<uint8_t> > &suffixesPL,
	       const vector<uint32_t> &nodeCountPL,
	       const SuffixType suffixConfig);

    ~LoudsDense() {
	delete labelBitmaps_;
	delete childIndicatorBitmaps_;
	delete prefixKeyIndicatorBits_;
	delete suffixes_;
    }

    bool lookupKey(const string &key, position_t &out_nodeNum) const;
    bool lookupRange(const string &leftKey, const string &rightKey, position_t &out_leftPos, position_t &out_rightPos) const;
    uint32_t countRange(const string &leftKey, const string &rightKey, position_t &out_leftPos, position_t &out_rightPos) const;

    bool getLowerBoundKey(const string &key, string &outputKey, position_t &out_pos) const;

    uint64_t getMemoryUsage();

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getSuffixPos(const position_t nodeNum, const position_t pos) const;

private:
    static const position_t NODE_FANOUT = 256;
    static const position_t NODE_LOG_FANOUT = 8;
    static const uint32_t RANK_BASIC_BLOCK_SIZE = 512;

    uint32_t height_;

    BitVectorRank* labelBitmaps_;
    BitVectorRank* childIndicatorBitmaps_;
    BitVectorRank* prefixKeyIndicatorBits_; //1 bit per internal node
    SuffixVector* suffixes_;
};

LoudsDense::LoudsDense(const vector<vector<uint64_t> > &bitmap_labelsPL,
		       const vector<vector<uint64_t> > &bitmap_childIndicatorBitsPL,
		       const vector<vector<uint64_t> > &prefixKeyIndicatorBitsPL,
		       const vector<vector<uint8_t> > &suffixesPL,
		       const vector<uint32_t> &nodeCountPL,
		       const SuffixType suffixConfig) {
    height_ = bitmap_labelsPL.size();
    vector<uint32_t> numBitsPerLevel;
    for (uint32_t level = 0; level < height_; level++)
	numBitsPerLevel.push_back(bitmap_labelsPL[level].size() * 64);

    labelBitmaps_ = new BitVectorRank(RANK_BASIC_BLOCK_SIZE, bitmap_labelsPL, numBitsPerLevel);
    childIndicatorBitmaps_ = new BitVectorRank(RANK_BASIC_BLOCK_SIZE, bitmap_childIndicatorBitsPL, numBitsPerLevel);
    prefixKeyIndicatorBits_ = new BitVectorRank(RANK_BASIC_BLOCK_SIZE, prefixKeyIndicatorBitsPL, nodeCountPL);
    suffixes_ = new SuffixVector(suffixConfig, suffixesPL);
}

//TODO: need check off-by-one
position_t LoudsDense::getChildNodeNum(const position_t pos) const {
    return childIndicatorBitmaps_->rank(pos);
}

//TODO: need check off-by-one
position_t LoudsDense::getSuffixPos(const position_t nodeNum, const position_t pos) const {
    return (labelBitmaps_->rank(pos)
	    - childIndicatorBitmaps_->rank(pos)
	    + prefixKeyIndicatorBits_->rank(nodeNum)
	    - 1);
}

bool LoudsDense::lookupKey(const string &key, position_t &out_nodeNum) const {
    position_t nodeNum = 0;
    position_t pos = 0;
    for (uint32_t level = 0; level < height_; level++) {
	if (level >= key.length()) { //if run out of searchKey bytes
	    if (prefixKeyIndicatorBits_->readBit(nodeNum)) //if the prefix is also a key
		return suffixes_->checkEquality(getSuffixPos(nodeNum, pos), key, level + 1);
	    else
		return false;
	}

	nodeNum = getChildNodeNum(pos);
	pos = (nodeNum << NODE_LOG_FANOUT) + key[level];

	if (!labelBitmaps_->readBit(pos)) //if key byte does not exist
	    return false;

	if (!childIndicatorBitmaps_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(nodeNum, pos), key, level + 1);
    }
    //search will continue in LoudsSparse
    out_nodeNum = nodeNum;
    return false;
}

bool LoudsDense::lookupRange(const string &leftKey, const string &rightKey, position_t &out_leftPos, position_t &out_rightPos) const {
    return true;
}

uint32_t LoudsDense::countRange(const string &leftKey, const string &rightKey, position_t &out_leftPos, position_t &out_rightPos) const {
    return 0;
}

bool LoudsDense::getLowerBoundKey(const string &key, string &outputKey, position_t &out_pos) const {
    return true;
}

uint64_t LoudsDense::getMemoryUsage() {
    return (sizeof(this)
	    + labelBitmaps_->size()
	    + childIndicatorBitmaps_->size()
	    + prefixKeyIndicatorBits_->size()
	    + suffixes_->size());
}

#endif
