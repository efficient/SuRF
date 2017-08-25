#ifndef SURFBUILDER_H_
#define SURFBUILDER_H_

#include <assert.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "hash.hpp"

namespace surf {

class SuRFBuilder {
public: 
    SuRFBuilder() : sparse_start_level_(0), suffix_config_(kNone) {};
    explicit SuRFBuilder(SuffixType suffix_config) : sparse_start_level_(0), suffix_config_(suffix_config) {};

    ~SuRFBuilder() {};

    void build(const std::vector<std::string>& keys);

    static bool readBit(const std::vector<word_t>& bits, position_t pos) {
	assert(pos < (bits.size() * kWordSize));
	position_t word_id = pos / kWordSize;
	position_t offset = pos % kWordSize;
	return (bits[word_id] & (kMsbMask >> offset));
    }

    static void setBit(std::vector<word_t>& bits, position_t pos) {
	assert(pos < (bits.size() * kWordSize));
	position_t word_id = pos / kWordSize;
	position_t offset = pos % kWordSize;
	bits[word_id] |= (kMsbMask >> offset);
    }

    level_t getTreeHeight() const {
	return labels_.size();
    }

    const std::vector<std::vector<word_t> >& getBitmapLabels() const {
	return bitmap_labels_;
    }

    const std::vector<std::vector<word_t> >& getBitmapChildIndicatorBits() const {
	return bitmap_child_indicator_bits_;
    }

    const std::vector<std::vector<word_t> >& getPrefixkeyIndicatorBits() const {
	return prefixkey_indicator_bits_;
    }

    const std::vector<std::vector<label_t> >& getLabels() const {
	return labels_;
    }

    const std::vector<std::vector<word_t> >& getChildIndicatorBits() const {
	return child_indicator_bits_;
    }

    const std::vector<std::vector<word_t> >& getLoudsBits() const {
	return louds_bits_;
    }

    const std::vector<std::vector<suffix_t> >& getSuffixes() const {
	return suffixes_;
    }

    const std::vector<position_t>& getNodeCounts() const {
	return node_counts_;
    }

    level_t getSparseStartLevel() const {
	return sparse_start_level_;
    }

    SuffixType getSuffixConfig() const {
	return suffix_config_;
    }

private:
    static bool isSameKey(const std::string& a, const std::string& b) {
	return a.compare(b) == 0;
    }

    position_t getNumItems(const level_t level) const;
    void addLevel();
    void setChildIndicatorBit(level_t level, position_t pos);
    void setLoudsBit(level_t level, position_t pos);
    void moveToNextItemSlot(const level_t level);
    bool isCharCommonPrefix(const char c, const level_t level) const;
    level_t skipCommonPrefix(const std::string& key);
    void insertKeyByte(const char c, const level_t level, const bool is_start_of_node);
    level_t insertKeyBytesToTrieUntilUnique(const std::string& key, const std::string& next_key, const level_t start_level);
    void insertSuffix(const std::string& key, const level_t level);
    void buildSparsePerLevel(const std::vector<std::string>& keys);

    uint64_t computeDenseMem(const level_t downto_level) const;
    uint64_t computeSparseMem(const level_t start_level) const;
    void determineCutoffLevel();
    void initDenseVectors(const level_t level);
    void fillInLabelAndChildIndicator(const level_t level, const position_t node_num, const position_t pos);
    bool isStartOfNode(const level_t level, const position_t pos) const;
    bool isTerminator(const level_t level, const position_t pos) const;
    void fillInDenseVectors();

private:
    std::vector<std::vector<word_t> > bitmap_labels_;
    std::vector<std::vector<word_t> > bitmap_child_indicator_bits_;
    std::vector<std::vector<word_t> > prefixkey_indicator_bits_;

    std::vector<std::vector<label_t> > labels_;
    std::vector<std::vector<word_t> > child_indicator_bits_;
    std::vector<std::vector<word_t> > louds_bits_;

    std::vector<std::vector<suffix_t> > suffixes_; //current version of SuRF only supports one-byte suffixes

    std::vector<position_t> node_counts_;
    std::vector<bool> is_last_item_terminator_;

    level_t sparse_start_level_;

    SuffixType suffix_config_;
};


position_t SuRFBuilder::getNumItems(const level_t level) const {
    return labels_[level].size();
}

void SuRFBuilder::addLevel() {
    labels_.push_back(std::vector<label_t>());
    child_indicator_bits_.push_back(std::vector<word_t>());
    louds_bits_.push_back(std::vector<word_t>());
    suffixes_.push_back(std::vector<suffix_t>());

    node_counts_.push_back(0);
    is_last_item_terminator_.push_back(false);

    child_indicator_bits_[getTreeHeight() - 1].push_back(0);
    louds_bits_[getTreeHeight() - 1].push_back(0);
}

void SuRFBuilder::setChildIndicatorBit(level_t level, position_t pos) {
    assert(level < getTreeHeight());
    position_t byte_id = pos / kWordSize;
    position_t byte_offset = pos % kWordSize;
    assert(byte_id < child_indicator_bits_[level].size());
    child_indicator_bits_[level][byte_id] |= kMsbMask >> byte_offset;
}

void SuRFBuilder::setLoudsBit(level_t level, position_t pos) {
    assert(level < getTreeHeight());
    position_t byte_id = pos / kWordSize;
    position_t byte_offset = pos % kWordSize;
    assert(byte_id < louds_bits_[level].size());
    louds_bits_[level][byte_id] |= kMsbMask >> byte_offset;
}

void SuRFBuilder::moveToNextItemSlot(const level_t level) {
    assert(level < getTreeHeight());
    position_t num_items = getNumItems(level);
    if (num_items % kWordSize == 0) {
	child_indicator_bits_[level].push_back(0);
	louds_bits_[level].push_back(0);
    }
}

bool SuRFBuilder::isCharCommonPrefix(const char c, const level_t level) const {
    return (level < getTreeHeight())
	&& (!is_last_item_terminator_[level])
	&& (c == labels_[level].back());
}

level_t SuRFBuilder::skipCommonPrefix(const std::string& key) {
    level_t level = 0;
    while (level < key.length() && isCharCommonPrefix(key[level], level)) {
	setChildIndicatorBit(level, getNumItems(level) - 1);
	level++;
    }
    return level;
}

void SuRFBuilder::insertKeyByte(const char c, const level_t level, const bool is_start_of_node) {
    if (level >= getTreeHeight())
	addLevel();

    assert(level < getTreeHeight());

    //set parent node's child indicator
    if (level > 0)
	setChildIndicatorBit(level - 1, getNumItems(level - 1) - 1);

    labels_[level].push_back(c);
    if (is_start_of_node) {
	setLoudsBit(level, getNumItems(level) - 1);
	node_counts_[level]++;
    }

    moveToNextItemSlot(level);
}

level_t SuRFBuilder::insertKeyBytesToTrieUntilUnique(const std::string& key, const std::string& next_key, const level_t start_level) {
    assert(start_level < key.length());

    level_t level = start_level;
    bool is_start_of_node = false;
    if (level >= getTreeHeight())
	is_start_of_node = true;
    insertKeyByte(key[level], level, is_start_of_node);
    level++;

    is_start_of_node = true;
    while (level < key.length() && level < next_key.length() && key[level] == next_key[level]) {
	insertKeyByte(key[level], level, is_start_of_node);
	level++;
    }

    if (level < key.length())
	insertKeyByte(key[level], level, is_start_of_node);
    else
	insertKeyByte(kTerminator, level, is_start_of_node);

    level++;
    return level;
}

void SuRFBuilder::insertSuffix(const std::string& key, const level_t level) {
    switch (suffix_config_) {
    case kHash: {
	suffix_t h = suffixHash(key) & 0xFF;
	suffixes_[level].push_back(h);
	break;
    }
    case kReal: {
	if (level < key.length())
	    suffixes_[level].push_back(key[level]);
	else
	    suffixes_[level].push_back(0);
	break;
    }
    default:
	; // kNone
    }
}

void SuRFBuilder::buildSparsePerLevel(const std::vector<std::string>& keys) {
    for (uint32_t i = 0; i < keys.size(); i++) {
	if (i > 0 && isSameKey(keys[i], keys[i-1])) 
	    continue;
	level_t level = skipCommonPrefix(keys[i]);
	if (i < keys.size() - 1)
	    level = insertKeyBytesToTrieUntilUnique(keys[i], keys[i+1], level);
	else
	    level = insertKeyBytesToTrieUntilUnique(keys[i], std::string(), level);
	insertSuffix(keys[i], level);
    }
}

uint64_t SuRFBuilder::computeDenseMem(const level_t downto_level) const {
    assert(downto_level < getTreeHeight());
    uint64_t mem = 0;
    for (level_t level = 0; level < downto_level; level++) {
	mem += (2 * kFanout * node_counts_[level]);
	if (level > 0)
	    mem += (node_counts_[level - 1] / 8 + 1);
	mem += suffixes_[level].size();
    }
    return mem;
}

uint64_t SuRFBuilder::computeSparseMem(const level_t start_level) const {
    uint64_t mem = 0;
    for (level_t level = start_level; level < getTreeHeight(); level++) {
	position_t num_items = labels_[level].size();
	mem += (num_items + 2 * num_items / 8 + 1);
	mem += suffixes_[level].size();
    }
    return mem;
}

void SuRFBuilder::determineCutoffLevel() {
    level_t cutoff_level = 0;
    uint64_t dense_mem = computeDenseMem(cutoff_level);
    uint64_t sparse_mem = computeSparseMem(cutoff_level);
    while (dense_mem * kSparseDenseRatio < sparse_mem) {
	cutoff_level++;
	dense_mem = computeDenseMem(cutoff_level);
	sparse_mem = computeSparseMem(cutoff_level);
    }
    sparse_start_level_ = cutoff_level--;
}

void SuRFBuilder::initDenseVectors(const level_t level) {
    bitmap_labels_.push_back(std::vector<word_t>());
    bitmap_child_indicator_bits_.push_back(std::vector<word_t>());
    prefixkey_indicator_bits_.push_back(std::vector<word_t>());

    for (position_t nc = 0; nc < node_counts_[level]; nc++) {
	for (int i = 0; i < kFanout; i += kWordSize) {
	    bitmap_labels_[level].push_back(0);
	    bitmap_child_indicator_bits_[level].push_back(0);
	}
	if (nc % kWordSize == 0)
	    prefixkey_indicator_bits_[level].push_back(0);
    }
}

void SuRFBuilder::fillInLabelAndChildIndicator(const level_t level, const position_t node_num, const position_t pos) {
    label_t label = labels_[level][pos];
    setBit(bitmap_labels_[level], node_num * kFanout + label);
    if (readBit(child_indicator_bits_[level], pos))
	setBit(bitmap_child_indicator_bits_[level], node_num * kFanout + label);
}

bool SuRFBuilder::isStartOfNode(const level_t level, const position_t pos) const {
    return readBit(louds_bits_[level], pos);
}

bool SuRFBuilder::isTerminator(const level_t level, const position_t pos) const {
    label_t label = labels_[level][pos];
    return ((label == kTerminator) && !readBit(child_indicator_bits_[level], pos));
}

void SuRFBuilder::fillInDenseVectors() {
    for (level_t level = 0; level < sparse_start_level_; level++) {
	initDenseVectors(level);
	position_t node_num = 0;
	fillInLabelAndChildIndicator(level, node_num, 0);
	for (position_t pos = 1; pos < getNumItems(level); pos++) {
	    if (isStartOfNode(level, pos)) {
		node_num++;
		if (isTerminator(level, pos)) {
		    setBit(prefixkey_indicator_bits_[level], node_num);
		    continue;
		}
	    }
	    fillInLabelAndChildIndicator(level, node_num, pos);
	}
    }
}

void SuRFBuilder::build(const std::vector<std::string>& keys) {
    assert(keys.size() > 0);

    suffixes_.push_back(std::vector<suffix_t>());
    buildSparsePerLevel(keys);
    determineCutoffLevel();
    fillInDenseVectors();
}

} // namespace surf

#endif // SURFBUILDER_H_
