#ifndef LOUDSSPARSEBUILDER_H_
#define LOUDSSPARSEBUILDER_H_

#include <assert.h>
#include <vector>

#include "config.h"
#include "hash.h"

using namespace std;

class LoudsSparseBuilder {
public: 
    LoudsSparseBuilder() : suffixConfig_(0) {};
    LoudsSparseBuilder(uint16_t suffixConfig) : suffixConfig_(suffixConfig) {};

    bool build(const vector<string> &keys) {
	for (uint32_t i = 0; i < keys.size(); i++) {
	    if (i > 0 && isSameKey(keys[i], keys[i-1])) 
		continue;
	    string key = keys[i];
	    uint32_t level = skipCommonPrefix(key);
	    level = insertKeyBytesToTrieUntilUnique(key, level);
	    insertSuffix(key, level);
	}
    }

    uint32_t getTreeHeight() {
	return (uint32_t)labels_.size();
    }

private:
    inline void setChildIndicatorBit(uint32_t level, uint32_t pos) {
	uint32_t byteId = pos / 64;
	uint32_t byteOffset = pos % 64;
	childIndicatorBits_[level][byteId] |= MSB_MASK >> byteOffset;
    }

    inline void setLoudsBit(uint32_t level, uint32_t pos) {
	uint32_t byteId = pos / 64;
	uint32_t byteOffset = pos % 64;
	loudsBits_[level][byteId] |= MSB_MASK >> byteOffset;
    }

    inline void addLevel() {
	labels.push_back(vector<uint8_t>());
	childIndicatorBits_.push_back(vector<uint64_t>());
	loudsBits_.push_back(vector<uint64_t>());
	suffixes_.push_back(vector<uint8_t>());

	numItems_.push_back(0);
	isLastItemTerminator_.push_back(false);

	childIndicatorBits_[getTreeHeight() - 1].push_back(0);
	loudsBits_[getTreeHeight() - 1].push_back(0);
    }

    inline void incrementNumItems(const uint32_t level) {
	numItems_[level]++;
	if (numItems_[level] % 64 == 0) {
	    childIndicatorBits_[level].push_back(0);
	    loudsBits_[level].push_back(0);
	}
    }

    inline bool isSameKey(const string &a, const string &b) {
	return a.compare(b) == 0;
    }

    inline bool isCharCommonPrefix(const char c, const uint32_t level) {
	return level < getTreeHeight() 
	    && !isLastItemTerminator[level] 
	    && c == labels_[level].back();
    }

    inline uint32_t skipCommonPrefix(const string &key) {
	uint32_t level = 0;
	while (level < key.length() && isCharCommonPrefix(key[level], level)) {
	    setChildIndicatorBit(level, numItems_[level] - 1);
	    level++;
	}
	return level;
    }

    inline insertKeyByte(const char c, const uint32_t level) {
	while (level >= getTreeHeight()) {
	    addlevel();
	}
	setChildIndicatorBit(level - 1, numItems_[level - 1] - 1);
	labels_[level].push_back(c);
	setLoudsBit(level, numItems_[level]);
	incrementNumItems(level);
    }

    inline uint32_t insertKeyBytesToTrieUntilUnique(const string &key, const string &nextKey, const uint32_t startLevel) {
	uint32_t level = startLevel;
	while (level < key.length() && level < nextKey.length() && key[level] == nextKey[level]) {
	    insertKeyByte(key[level], level);
	    level++;
	}

	if (level < key.length())
	    insertKeyByte(key[level], level);
	else
	    insertKeyByte(TERMINATOR, level);

	level++;
	return level;
    }

    inline void insertSuffix(const string &key, const uint32_t level) {
	switch (suffixConfig_) {
	case SuffixType_Hash:
	    uint8_t h = suffixHash(key) & 0xFF;
	    suffixes_[level].push_back(h);
	    break;
	case SuffixType_Real:
	    if (level < key.length())
		suffixes_[level].push_back(key[level]);
	    else
		suffixes_[level].push_back((uint8_t)0);
	    break;
	default:
	    // SuffixType_None
	}
    }

private:
    vector<vector<uint8_t> > labels_;
    vector<vector<uint64_t> > childIndicatorBits_;
    vector<vector<uint64_t> > loudsBits_;

    //current version of SuRF only supports one-byte suffixes
    vector<vector<uint8_t> > suffixes_;

    vector<uint32_t> numItems_;
    vector<bool> isLastItemTerminator_;

    uint16_t suffixConfig_;
};
