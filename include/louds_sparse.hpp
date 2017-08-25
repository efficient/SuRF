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

    bool lookupKey(const std::string& key, const position_t in_node_num) const;
    bool lookupRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const;
    uint32_t countRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const;

    bool getLowerBoundKey(const std::string& key, std::string* output_key, const position_t in_pos) const;

    uint64_t getMemoryUsage();

private:
    position_t getChildNodeNum(const position_t pos) const;
    position_t getFirstLabelPos(const position_t node_num) const;
    position_t getSuffixPos(const position_t pos) const;
    position_t nodeSize(const position_t pos) const;

private:
    static const position_t kRankBasicBlockSize = 512;

    level_t height_;
    level_t start_level_;
    position_t node_count_dense_;
    position_t child_count_dense_;

    LabelVector* labels_;
    BitVectorRank* child_indicator_bits_;
    BitVectorSelect* louds_bits_;
    SuffixVector* suffixes_;
};


LoudsSparse::LoudsSparse(const SuRFBuilder* builder) {
    height_ = builder->getLabels().size();
    start_level_ = builder->getSparseStartLevel();

    node_count_dense_ = 0;
    for (level_t level = 0; level < start_level_; level++)
	node_count_dense_ += builder->getNodeCounts()[level];

    child_count_dense_ = node_count_dense_ + builder->getNodeCounts()[start_level_];

    std::vector<position_t> num_items_per_level;
    for (level_t level = 0; level < height_; level++)
	num_items_per_level.push_back(builder->getLabels()[level].size() * kWordSize);

    labels_ = new LabelVector(builder->getLabels(), start_level_, height_);
    child_indicator_bits_ = new BitVectorRank(kRankBasicBlockSize, builder->getChildIndicatorBits(), num_items_per_level, start_level_, height_);
    louds_bits_ = new BitVectorSelect(kRankBasicBlockSize, builder->getLoudsBits(), num_items_per_level, start_level_, height_); //TODO
    suffixes_ = new SuffixVector(builder->getSuffixConfig(), builder->getSuffixes(), start_level_, height_);
}

//TODO: need check off-by-one
position_t LoudsSparse::getChildNodeNum(const position_t pos) const {
    return (child_indicator_bits_->rank(pos) + child_count_dense_);
}

//TODO: need check off-by-one
position_t LoudsSparse::getFirstLabelPos(const position_t node_num) const {
    return louds_bits_->select(node_num - node_count_dense_);
}

//TODO: need check off-by-one
position_t LoudsSparse::getSuffixPos(const position_t pos) const {
    return (pos - child_indicator_bits_->rank(pos));
}

position_t LoudsSparse::nodeSize(const position_t pos) const {
    assert(louds_bits_->readBit(pos));
    return louds_bits_->distanceToNextOne(pos);
}

bool LoudsSparse::lookupKey(const std::string& key, const position_t in_node_num) const {
    position_t node_num = in_node_num;
    position_t pos = getFirstLabelPos(node_num);
    level_t level = 0;
    for (level = start_level_; level < key.length(); level++) {
	if (!labels_->search(key[level], pos, nodeSize(pos)))
	    return false;

	if (!child_indicator_bits_->readBit(pos)) //if trie branch terminates
	    return suffixes_->checkEquality(getSuffixPos(pos), key, level + 1);

	node_num = getChildNodeNum(pos);
	pos = getFirstLabelPos(node_num);
    }

    if ((labels_->read(pos) == kTerminator) && (!child_indicator_bits_->readBit(pos)))
	return suffixes_->checkEquality(getSuffixPos(pos), key, level + 1);
}

bool LoudsSparse::lookupRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const {
    return true;
}

uint32_t LoudsSparse::countRange(const std::string& left_key, const std::string& right_key, const position_t in_left_pos, const position_t in_right_pos) const {
    return 0;
}

bool LoudsSparse::getLowerBoundKey(const std::string& key, std::string* output_key, const position_t in_pos) const {
    return true;
}

uint64_t LoudsSparse::getMemoryUsage() {
    return (sizeof(this)
	    + labels_->size()
	    + child_indicator_bits_->size()
	    + louds_bits_->size()
	    + suffixes_->size());
}

} // namespace surf

#endif // LOUDSSPARSE_H_
