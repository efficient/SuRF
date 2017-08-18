#ifndef LOUDSSPARSE_H_
#define LOUDSSPARSE_H_

#include "config.hpp"
#include "labelVector.hpp"
#include "rank.hpp"
#include "suffixVector.hpp"

using namespace std;

class LoudsSparse {
public:
    LoudsSparse() {};

    LoudsSparse(const vector<vector<uint8_t> > &labelsPL,
		const vector<vector<uint64_t> > &childIndicatorBitsPL,
		const vector<vector<uint64_t> > &loudsBitsPL,
		const vector<vector<uint8_t> > &suffixesPL,
		const vector<uint32_t> &nodeCountPL,
		const uint32_t sparseStartLevel,
		const uint16_t suffixConfig);

    ~LoudsDense() {
	delete labels_;
	delete childIndicatorBits_;
	delete loudsBits_;
	delete suffixes_;
    }

    bool lookupKey(const string &key, const position_t &in_nodeNum) const;
    bool lookupRange(const string &leftKey, const string &rightKey, const position_t &in_leftPos, const position_t &in_rightPos) const;
    uint32_t countRange(const string &leftKey, const string &rightKey, const position_t &in_leftPos, const position_t &in_rightPos) const;

    bool getLowerBoundKey(const string &key, string& outputKey, const position_t in_pos) const;

    uint64_t getMemoryUsage();

private:
    position_t getChildPos(position_t pos) const;
    position_t nodeSize(position_t pos) const;

private:
    static const position_t RANK_BASIC_BLOCK_SIZE = 512;

    uint32_t height_;
    uint32_t startLevel_;
    position_t nodeCountLoudsDense_;
    position_t childCountLoudsDense_;

    LabelVector* labels_;
    BitVectorRank* childIndicatorBits_;
    BitVectorSelect* loudsBits_;
    SuffixVector* suffixes_;
};

LoudsSparse::LoudsSparse(const vector<vector<uint8_t> > &labelsPL,
			 const vector<vector<uint64_t> > &childIndicatorBitsPL,
			 const vector<vector<uint64_t> > &loudsBitsPL,
			 const vector<vector<uint8_t> > &suffixesPL,
			 const vector<uint32_t> &nodeCountPL,
			 const uint32_t sparseStartLevel,
			 const uint16_t suffixConfig) {
    height_ = labelsPL.size();
    startLevel_ = sparseStartLevel;

    nodeCountLoudsDense_ = 0;
    for (uint32_t level = 0; level < startLevel_; level++)
	nodeCountLoudsDense_ += nodeCountPL[level];

    childCountLoudsDense_ = nodeCountLoudsDense_ + nodeCountPL[startLevel];

    vector<uint32_t> numItemsPerLevel;
    for (uint32_t level = startLevel_; level < height_; level++)
	numItemsPerLevel.push_back(labelsPL[level].size() * 64);

    vector<vector<uint8_t> > labelsSparsePL;
    vector<vector<uint64_t> > childIndicatorBitsSparsePL;
    vector<vector<uint64_t> > loudsBitsSparsePL;
    for (uint32_t level = sparseStartLevel; level < height_; level++) {
	labelsSparsePL.push_back(labelsPL[level]);
	childIndicatorBitsSparsePL.push_back(childIndicatorBitsPL[level]);
	loudsBitsSparsePL.push_back(loudsBitsPL[level]);
    }

    labels_ = new LabelVector(labelsPL);
    childIndicatorBits_ = new BitVectorRank(RANK_BASIC_BLOCK_SIZE, childIndicatorBitsSparsePL, numItemsPerLevel);
    loudsBits_ = new BitVectorSelect(RANK_BASIC_BLOCK_SIZE, loudsBitsSparsePL, numItemsPerLevel); //TODO
    suffixes_ = new SuffixVector(suffixConfig, suffixesPL);
}

//TODO: need check off-by-one
position_t LoudsSparse::getChildNodeNum(position_t pos) const {
    return (childIndicatorBits_->rank(pos) + childCountLoudsDense_);
}

//TODO: need check off-by-one
position_t LoudsSparse::getFirstLabelPos(position_t nodeNum) const {
    return loudsBits_->select(nodeNum - nodeCountLoudsDense_);
}

//TODO: need check off-by-one
position_t LoudsSparse::getSuffixPos(position_t pos) const {
    return (pos - childIndicatorBitMaps_->rank(pos));
}

position_t LoudsSparse::nodeSize(position_t pos) const {
    assert(loudsBits_->readBit(pos));
    return loudsBits_->distanceToNextOne(pos);
}

bool LoudsSparse::lookupKey(const string &key, const position_t &in_nodeNum) const {
    position_t nodeNum = in_nodeNum;
    position_t pos = getChildPos(nodeNum);
    for (uint32_t level = startLevel_; level < key.length(); level++) {
	if (!labels_->search(key[level], pos, nodeSize(pos)))
	    return false;

	if (!childIndicatorBitMaps_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(nodeNum, pos), key, level + 1);

	nodeNum = getChildNodeNum(pos);
	pos = getFirstLabelPos(nodeNum);
    }

    if ((labels_->read(pos) == TERMINATOR) && (!childIndicatorBitMaps_->readBit(pos)))
	return suffixes_->checkEquality(getSuffixPos(nodeNum, pos), key, level + 1);
}

bool LoudsSparse::lookupRange(const string &leftKey, const string &rightKey, const position_t &in_leftPos, const position_t &in_rightPos) const {
    return true;
}

uint32_t LoudsSparse::countRange(const string &leftKey, const string &rightKey, const position_t &in_leftPos, const position_t &in_rightPos) const {
    return 0;
}

bool LoudsSparse::getLowerBoundKey(const string &key, string& outputKey, const position_t in_pos) const {
    return true;
}

uint64_t LoudsDense::getMemoryUsage() {
    return (sizeof(this)
	    + labels_->size()
	    + childIndicatorBits_->size()
	    + loudsBits_->size()
	    + suffixes_->size());
}

#endif
