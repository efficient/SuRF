#ifndef SURF_H_
#define SURF_H_

#include <string>
#include <vector>

#include "config.hpp"
#include "louds_dense.hpp"
#include "louds_sparse.hpp"
#include "surf_builder.hpp"

namespace surf {

class SuRF {
public:
    SuRF() {};

    SuRF(const std::vector<std::string>& keys, bool include_dense, 
	 uint32_t sparse_dense_ratio,SuffixType suffix_config) {
	builder = new SuRFBuilder(suffix_config, sparse_dense_ratio, suffix_config);
	builder->build(keys);
	louds_dense_ = new LoudsDense(builder);
	louds_sparse_ = new LoudsSparse(builder);
	delete builder;
    }

    ~SuRF() {
	delete louds_dense_;
	delete louds_sparse_;
    }

    bool lookupKey(const std::string& key);
    bool getLowerBoundKey(const std::string& key, std::string* out_key);
    bool lookupRange(const std::string& left_key, const std::string& right_key);
    uint32_t countRange(const std::string& left_key, const std::string& right_key);

    uint64_t getMemoryUsage();

private:
    LoudsDense* louds_dense_;
    LoudsSparse* louds_sparse_;
    SuRFBuilder* builder;
};


bool SuRF::lookupKey(const std::string& key) {
    position_t connect_node_num = 0;
    if (!louds_dense_->lookupKey(key, connect_node_num))
	return false;
    else if (connect_node_num != 0)
	return louds_sparse_->lookupKey(key, connect_node_num);
    return true;
}

bool SuRF::getLowerBoundKey(const std::string& key, std::string* output_key) {
    position_t connect_pos = 0;
    if (!louds_dense_->getLowerBoundKey(key, output_key, connect_pos))
	return louds_sparse_->getLowerBoundKey(key, output_key, connect_pos);
    return true;
}

bool SuRF::lookupRange(const std::string& left_key, const std::string& right_key) {
    position_t left_connect_pos = 0;
    position_t right_connect_pos = 0;
    if (!louds_dense_->lookupRange(left_key, right_key, left_connect_pos, right_connect_pos))
	return louds_sparse_->lookupRange(left_key, right_key, left_connect_pos, right_connect_pos);
    return true;
}

uint32_t SuRF::countRange(const std::string& left_key, const std::string& right_key) {
    uint32_t count = 0;
    position_t left_connect_pos = 0;
    position_t right_connect_pos = 0;
    count += louds_dense_->countRange(left_key, right_key, left_connect_pos, right_connect_pos);
    count += louds_sparse_->countRange(left_key, right_key, left_connect_pos, right_connect_pos);
    return count;
}

uint64_t SuRF::getMemoryUsage() {
    return (sizeof(SuRF) + louds_dense_->getMemoryUsage() + louds_sparse_->getMemoryUsage());
}

} // namespace surf

#endif // SURF_H
