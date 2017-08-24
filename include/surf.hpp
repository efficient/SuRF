#ifndef SURF_H_
#define SURF_H_

#include <string>

#include "config.hpp"
#include "louds_dense.hpp"
#include "louds_sparse.hpp"

namespace surf {

class SuRF {
public:
    SuRF() {};

    SuRF(const std::vector<std::vector<word_t> > &bitmap_labels_pl,
	 const std::vector<std::vector<word_t> > &bitmap_child_indicator_bits_pl,
	 const std::vector<std::vector<word_t> > &prefixkey_indicator_bits_pl,
	 const std::vector<std::vector<label_t> > &labels_pl,
	 const std::vector<std::vector<word_t> > &child_indicator_bits_pl,
	 const std::vector<std::vector<word_t> > &louds_bits_pl,
	 const std::vector<std::vector<suffix_t> > &suffixes_pl,
	 const std::vector<position_t> &node_count_pl,
	 const level_t sparse_start_level,
	 const SuffixType suffix_config) {
	louds_dense_ = new LoudsDense(bitmap_labels_pl, bitmap_child_indicator_bits_pl, prefixkey_indicator_bits_pl, suffixes_pl, node_count_pl, suffix_config);
	louds_sparse_ = new LoudsSparse(labels_pl, child_indicator_bits_pl, louds_bits_pl, suffixes_pl, node_count_pl, sparse_start_level, suffix_config);
    }

    ~SuRF() {
	delete louds_dense_;
	delete louds_sparse_;
    }

    bool lookupKey(const std::string &key);
    bool lookupRange(const std::string &left_key, const std::string &right_key);
    uint32_t countRange(const std::string &left_key, const std::string &right_key);

    bool getLowerBoundKey(const std::string &key, std::string *out_key);

    uint64_t getMemoryUsage();

private:
    LoudsDense* louds_dense_;
    LoudsSparse* louds_sparse_;
};

bool SuRF::lookupKey(const std::string &key) {
    position_t connect_node_num = 0;
    if (!louds_dense_->lookupKey(key, connect_node_num))
	return louds_sparse_->lookupKey(key, connect_node_num);
    return true;
}

bool SuRF::lookupRange(const std::string &left_key, const std::string &right_key) {
    position_t left_connect_pos = 0;
    position_t right_connect_pos = 0;
    if (!louds_dense_->lookupRange(left_key, right_key, left_connect_pos, right_connect_pos))
	return louds_sparse_->lookupRange(left_key, right_key, left_connect_pos, right_connect_pos);
    return true;
}

uint32_t SuRF::countRange(const std::string &left_key, const std::string &right_key) {
    uint32_t count = 0;
    position_t left_connect_pos = 0;
    position_t right_connect_pos = 0;
    count += louds_dense_->countRange(left_key, right_key, left_connect_pos, right_connect_pos);
    count += louds_sparse_->countRange(left_key, right_key, left_connect_pos, right_connect_pos);
    return count;
}

bool SuRF::getLowerBoundKey(const std::string &key, std::string *output_key) {
    position_t connect_pos = 0;
    if (!louds_dense_->getLowerBoundKey(key, output_key, connect_pos))
	return louds_sparse_->getLowerBoundKey(key, output_key, connect_pos);
    return true;
}

uint64_t SuRF::getMemoryUsage() {
    return (sizeof(SuRF) + louds_dense_->getMemoryUsage() + louds_sparse_->getMemoryUsage());
}

} // namespace surf

#endif // SURF_H
