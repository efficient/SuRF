#ifndef LOUDSSPARSE_H_
#define LOUDSSPARSE_H_

#include <vector>
#include <string>

#include "config.hpp"
#include "label_vector.hpp"
#include "rank.hpp"
#include "select.hpp"
#include "suffix_vector.hpp"

namespace surf {

class LoudsSparse {
public:
    LoudsSparse() {};

    LoudsSparse(const std::vector<std::vector<label_t> > &labels_pl,
		const std::vector<std::vector<word_t> > &child_indicator_bits_pl,
		const std::vector<std::vector<word_t> > &louds_bits_pl,
		const std::vector<std::vector<suffix_t> > &suffixes_pl,
		const std::vector<position_t> &node_count_pl,
		const level_t sparse_start_level,
		const SuffixType suffix_config);

    ~LoudsSparse() {
	delete labels_;
	delete child_indicator_bits_;
	delete louds_bits_;
	delete suffixes_;
    }

    bool lookupKey(const std::string &key, const position_t &in_node_num) const;
    bool lookupRange(const std::string &left_key, const std::string &right_key, const position_t &in_left_pos, const position_t &in_right_pos) const;
    uint32_t countRange(const std::string &left_key, const std::string &right_key, const position_t &in_left_pos, const position_t &in_right_pos) const;

    bool getLowerBoundKey(const std::string &key, std::string *output_key, const position_t in_pos) const;

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

LoudsSparse::LoudsSparse(const std::vector<std::vector<label_t> > &labels_pl,
			 const std::vector<std::vector<word_t> > &child_indicator_bits_pl,
			 const std::vector<std::vector<word_t> > &louds_bits_pl,
			 const std::vector<std::vector<suffix_t> > &suffixes_pl,
			 const std::vector<position_t> &node_count_pl,
			 const level_t sparse_start_level,
			 const SuffixType suffix_config) {
    height_ = labels_pl.size();
    start_level_ = sparse_start_level;

    node_count_dense_ = 0;
    for (level_t level = 0; level < start_level_; level++)
	node_count_dense_ += node_count_pl[level];

    child_count_dense_ = node_count_dense_ + node_count_pl[start_level_];

    std::vector<position_t> num_items_per_level;
    for (level_t level = start_level_; level < height_; level++)
	num_items_per_level.push_back(labels_pl[level].size() * kWordSize);

    std::vector<std::vector<label_t> > labels_sparse_pl;
    std::vector<std::vector<word_t> > child_indicator_bits_sparse_pl;
    std::vector<std::vector<word_t> > louds_bits_sparse_pl;
    for (level_t level = sparse_start_level; level < height_; level++) {
	labels_sparse_pl.push_back(labels_pl[level]);
	child_indicator_bits_sparse_pl.push_back(child_indicator_bits_pl[level]);
	louds_bits_sparse_pl.push_back(louds_bits_pl[level]);
    }

    labels_ = new LabelVector(labels_pl);
    child_indicator_bits_ = new BitVectorRank(kRankBasicBlockSize, child_indicator_bits_sparse_pl, num_items_per_level);
    louds_bits_ = new BitVectorSelect(kRankBasicBlockSize, louds_bits_sparse_pl, num_items_per_level); //TODO
    suffixes_ = new SuffixVector(suffix_config, suffixes_pl);
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

bool LoudsSparse::lookupKey(const std::string &key, const position_t &in_node_num) const {
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

bool LoudsSparse::lookupRange(const std::string &left_key, const std::string &right_key, const position_t &in_left_pos, const position_t &in_right_pos) const {
    return true;
}

uint32_t LoudsSparse::countRange(const std::string &left_key, const std::string &right_key, const position_t &in_left_pos, const position_t &in_right_pos) const {
    return 0;
}

bool LoudsSparse::getLowerBoundKey(const std::string &key, std::string *output_key, const position_t in_pos) const {
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
