#ifndef LOUDSSPARSEBUILDER_H_
#define LOUDSSPARSEBUILDER_H_

#include <assert.h>
#include <vector>

#include "config.hpp"
#include "hash.h"
#include "SuRF.hpp"

using namespace std;

class SuRFBuilder {
public: 
    SuRFBuilder() : cutoffLevel_(0), suffixConfig_(0) {};
    SuRFBuilder(SuffixType suffixConfig) : cutoffLevel_(0), suffixConfig_(suffixConfig) {};

    ~SuRFBuilder() {};

    static SuRF* build(const vector<string> &keys);

private:
    uint32_t getTreeHeight() {
	return (uint32_t)labels_.size();
    }

    uint32_t getNumItems(uint32_t level) {
	return (uint32_t)labels_[level].size();
    }

    void addLevel() {
	labels_.push_back(vector<uint8_t>());
	childIndicatorBits_.push_back(vector<uint64_t>());
	loudsBits_.push_back(vector<uint64_t>());
	suffixes_.push_back(vector<uint8_t>());

	nodeCount_.push_back(0);
	isLastItemTerminator_.push_back(false);

	childIndicatorBits_[getTreeHeight() - 1].push_back(0);
	loudsBits_[getTreeHeight() - 1].push_back(0);
    }

    void setChildIndicatorBit(uint32_t level, uint32_t pos) {
	assert(level < getTreeHeight());
	uint32_t byteId = pos / 64;
	uint32_t byteOffset = pos % 64;
	assert(byteId < childIndicatorBits_[level].size());
	childIndicatorBits_[level][byteId] |= MSB_MASK >> byteOffset;
    }

    void setLoudsBit(uint32_t level, uint32_t pos) {
	assert(level < getTreeHeight());
	uint32_t byteId = pos / 64;
	uint32_t byteOffset = pos % 64;
	assert(byteId < loudsBits_[level].size());
	loudsBits_[level][byteId] |= MSB_MASK >> byteOffset;
    }

    void moveToNextItemSlot(const uint32_t level) {
	assert(level < getTreeHeight());
	uint32_t numItems = getNumItems(level);
	if (numItems % 64 == 0) {
	    childIndicatorBits_[level].push_back(0);
	    loudsBits_[level].push_back(0);
	}
    }

    static bool isSameKey(const string &a, const string &b) {
	return a.compare(b) == 0;
    }

    bool isCharCommonPrefix(const char c, const uint32_t level) const {
	return level < getTreeHeight() 
	    && !isLastItemTerminator_[level] 
	    && c == labels_[level].back();
    }

    uint32_t skipCommonPrefix(const string &key) {
	uint32_t level = 0;
	while (level < key.length() && isCharCommonPrefix(key[level], level)) {
	    setChildIndicatorBit(level, getNumItems(level) - 1);
	    level++;
	}
	return level;
    }

    void insertKeyByte(const char c, const uint32_t level, const bool isStartOfNode) {
	while (level >= getTreeHeight())
	    addLevel();

	//set parent node's child indicator
	if (level > 0)
	    setChildIndicatorBit(level - 1, getNumItems(level - 1) - 1);

	labels_[level].push_back(c);
	if (isStartOfNode) {
	    setLoudsBit(level, getNumItems(level - 1) - 1);
	    nodeCount_[level]++;
	}

	moveToNextItemSlot(level);
    }

    uint32_t insertKeyBytesToTrieUntilUnique(const string &key, const string &nextKey, const uint32_t startLevel) {
	assert(startLevel < key.length());

	uint32_t level = startLevel;
	bool isStartOfNode = false;
	insertKeyByte(key[level], level, isStartOfNode);

	isStartOfNode = true;
	while (level < key.length() && level < nextKey.length() && key[level] == nextKey[level]) {
	    insertKeyByte(key[level], level, isStartOfNode);
	    level++;
	}

	if (level < key.length())
	    insertKeyByte(key[level], level, isStartOfNode);
	else
	    insertKeyByte(TERMINATOR, level, isStartOfNode);

	level++;
	return level;
    }

    void insertSuffix(const string &key, const uint32_t level) {
	switch (suffixConfig_) {
	case SuffixType_Hash: {
	    uint8_t h = suffixHash(key) & 0xFF;
	    suffixes_[level].push_back(h);
	    break;
	}
	case SuffixType_Real: {
	    if (level < key.length())
		suffixes_[level].push_back(key[level]);
	    else
		suffixes_[level].push_back((uint8_t)0);
	    break;
	}
	default:
	    ; // SuffixType_None
	}
    }

    void buildSparsePerLevel(const vector<string> &keys) {
	for (uint32_t i = 0; i < keys.size(); i++) {
	    if (i > 0 && isSameKey(keys[i], keys[i-1])) 
		continue;
	    uint32_t level = skipCommonPrefix(keys[i]);
	    if (i < keys.size() - 1)
		level = insertKeyBytesToTrieUntilUnique(keys[i], keys[i+1], level);
	    else
		level = insertKeyBytesToTrieUntilUnique(keys[i], string(), level);
	    insertSuffix(keys[i], level);
	}
    }

    uint64_t computeDenseMem(const uint32_t downToLevel) const {
	assert(downToLevel < getTreeHeight());
	uint64_t mem = 0;
	for (uint32_t level = 0; level < downToLevel; level++) {
	    mem += (2 * 256 * nodeCount_[level]);
	    if (level > 0)
		mem += (nodeCount_[level - 1] / 8 + 1);
	    mem += suffixes_[level].size();
	}
	return mem;
    }

    uint64_t computeSparseMem(const uint32_t startLevel) const {
	uint64_t mem = 0;
	for (uint32_t level = startLevel; level < getTreeHeight(); level++) {
	    uint32_t numItems = labels_[level].size();
	    mem += (numItems + 2 * numItems / 8 + 1);
	    mem += suffixes_[level].size();
	}
	return mem;
    }

    void determineCuttofLevel() {
	uint32_t cutoffLevel = 0;
	uint64_t denseMem = computeDenseMem(cutoffLevel);
	uint64_t sparseMem = computeSparseMem(cutoffLevel);
	while (denseMem * SPARSE_DENSE_RATIO < sparseMem) {
	    cutoffLevel++;
	    denseMem = computeDenseMem(cutoffLevel);
	    sparseMem = computeSparseMem(cutoffLevel);
	}
	sparseStartLevel_ = cutoffLevel--;
    }

    void initDenseVectors(const uint32_t level) {
	bitMap_labels_.push_back(vector<uint64_t>);
	bitMap_childIndicatorBits_.push_back(vector<uint64_t>);
	prefixKeysIndicatorBits.push_back(vector<uint64_t>);

	for (uint32_t nc = 0; nc < nodeCount_[level]; nc++) {
	    for (int i = 0; i < 4; i++) {
		bitMap_labels_[level].push_back(0);
		bitMap_childIndicatorBits_[level].push_back(0);
	    }
	    if (nc % 64 == 0)
		prefixKeyIndicatorBits[level].push_back(0);
	}
    }

    bool readBit(const vector<uint64_t> &bits, position_t pos) const {
	assert(pos < bits.size());
	position_t wordId = pos / 64;
	position_t offset = pos % 64;
	return (bits[wordId] & (MSB_MASK >> offset));
    }

    void setBit(vector<uint64_t> &bits, position_t pos) {
	assert(pos < bits.size());
	position_t wordId = pos / 64;
	position_t offset = pos % 64;
	bits[wordId] |= (MSB_MASK >> offset);
    }

    void fillInLabelAndChildIndicator(const uint32_t level, const position_t nodeNum, const position_t pos) {
	uint8_t label = labels_[level][pos];
	setBit(bitMap_labels_[level], nodeNum * 256 + label);
	if (readBit(childIndicatorBits_[level], pos))
	    setBit(bitMap_childIndicatorBits_[level], nodeNum * 256 + label);
    }

    bool isStartOfNode(const uint32_t level, const position_t pos) const {
	return readBit(loudsBits_[level], pos);
    }

    bool isTerminator(const uint32_t level, const position_t pos) const {
	uint8_t label = labels_[level][pos];
	return ((label == TERMINATOR) && !readBit(childIndicatorBits_[level], pos));
    }

    void fillInDenseVectors() {
	for (uint32_t level = 0; level < sparseStartLevel; level++) {
	    initDenseVectors();
	    position_t nodeNum = 0;
	    fillInLabelAndChildIndicator(level, nodeNum, 0);
	    for (position_t pos = 1; pos < getNumItems(level); pos++) {
		if (isStartOfNode(level, pos)) {
		    nodeNum++;
		    if (isTerminator(level, pos)) {
			setBit(prefixKeyIndicatorBits_[level], nodeNum);
			continue;
		    }
		}
		fillInLabelAndChildIndicator(level, nodeNum, pos);
	    }
	}
    }

private:
    vector<vector<uint64_t> > bitMap_labels_;
    vector<vector<uint64_t> > bitMap_childIndicatorBits_;
    vector<vector<uint64_t> > prefixKeyIndicatorBits_;

    vector<vector<uint8_t> > labels_;
    vector<vector<uint64_t> > childIndicatorBits_;
    vector<vector<uint64_t> > loudsBits_;

    vector<vector<uint8_t> > suffixes_; //current version of SuRF only supports one-byte suffixes

    vector<uint32_t> nodeCount_;
    vector<bool> isLastItemTerminator_;

    uint32_t sparseStartLevel_;

    SuffixType suffixConfig_;
};

static SuRF* SuRFBuilder::build(const vector<string> &keys) {
    assert(keys.size() > 0);
    buildSparsePerLevel(keys);
    determineCutoffLevel();
    fillInDenseVectors();

    SuRF* filter = new SuRF(bitMap_labels, bitMap_childIndicatorBits_, prefixKeyIndicatorBits_, labels_, childIndicatorBits_, loudsBits_, suffixes_, sparseStartLevel_, nodeCount_, suffixConfig_);
    return filter;
}

#endif
