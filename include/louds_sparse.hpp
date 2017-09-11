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
    bool getKeyGreaterThan(const std::string& key, std::string& output_key, const position_t in_pos) const;
    bool getKeyGreaterThanOrEqualTo(const std::string& key, std::string& output_key, const position_t in_pos) const;
    bool lookupRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const;
    uint32_t countRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const;

    uint64_t getMemoryUsage();

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getFirstLabelPos(const position_t node_num) const;
    position_t getSuffixPos(const position_t pos) const;
    position_t nodeSize(const position_t pos) const;

    bool getLeftmostKeyInSubtree(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const;
    bool getNextKey(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const;
    bool compareSuffixAndGetKeyGT(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const;
    bool compareSuffixAndGetKeyGTE(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const;

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

	if (!child_indicator_bits_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(pos), key, level + 1);

	node_num = getChildNodeNum(pos);
	pos = getFirstLabelPos(node_num);
    }
    if ((labels_->read(pos) == kTerminator) && (!child_indicator_bits_->readBit(pos)))
	return suffixes_->checkEquality(getSuffixPos(pos), key, level + 1);
    return false;
}

bool LoudsSparse::getKeyGreaterThan(const std::string& key, std::string& output_key, const position_t in_pos) const {
    output_key.clear();
    std::vector<position_t> pos_per_level;
    for (level_t i = 0; i < height_; i++)
	pos_per_level.push_back(0);

    position_t node_num = in_node_num;
    position_t pos = getFirstLabelPos(node_num);
    level_t level = 0;
    for (level = start_level_; level < key.length(); level++) {
	if (!labels_->search((label_t)key[level], pos, nodeSize(pos)))
	    return getLeftmostKeyInSubtree(pos, output_key, pos_per_level);

	output_key += key[level];
	pos_per_level[level] = pos;

	if (!child_indicator_bits_->readBit(pos)) //if trie branch terminates
	    return compareSuffixAndGetKeyGT(pos, output_key, pos_per_level);

	node_num = getChildNodeNum(pos);
	pos = getFirstLabelPos(node_num);
    }
    return compareSuffixAndGetKeyGT(pos, output_key, pos_per_level);
}

bool LoudsSparse::getKeyGreaterThanOrEqualTo(const std::string& key, std::string& output_key, const position_t in_pos) const {
    return true;
}

bool LoudsSparse::lookupRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const {
    return true;
}

uint32_t LoudsSparse::countRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const {
    return 0;
}

uint64_t LoudsSparse::getMemoryUsage() {
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

bool LoudsSparse::getLeftmostKeyInSubtree(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const {
    
}

bool LoudsSparse::getNextKey(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const {
    
}

bool LoudsSparse::compareSuffixAndGetKeyGT(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const {
    if (suffixes_->type() == kReal) {
	int compare = suffixes_->compare(getSuffixPos(pos), key, level + 1);
	if (compare > 0) {
	    output_key += suffixes_[pos];
	    return true;
	} else {
	    return getNextKey(pos, output_key, pos_per_level);
	}
    }
    return getNextKey(pos, output_key, pos_per_level);
}

bool LoudsSparse::compareSuffixAndGetKeyGTE(position_t pos, std::string& output_key, std::vector<level_t>& pos_per_level) const {
    if (suffixes_->type() == kReal) {
	int compare = suffixes_->compare(getSuffixPos(pos), key, level + 1);
	if (compare >= 0) {
	    output_key += suffixes_[pos];
	    return true;
	} else {
	    return getNextKey(pos, output_key, pos_per_level);
	}
    }
    return true;
}

} // namespace surf

#endif // LOUDSSPARSE_H_
