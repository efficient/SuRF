#ifndef FILTER_SURF_H_
#define FILTER_SURF_H_

#include <string>
#include <vector>

#include "surf.hpp"

namespace bench {

class FilterSuRF : public Filter {
public:
    // Requires that keys are sorted
    FilterSuRF(const std::vector<std::string>& keys,
	       const surf::SuffixType suffix_type,
               const uint32_t hash_suffix_len, const uint32_t real_suffix_len) {
	// uses default sparse-dense size ratio
	filter_ = new surf::SuRF(keys, surf::kIncludeDense, surf::kSparseDenseRatio,
				 suffix_type, hash_suffix_len, real_suffix_len);
    }

    ~FilterSuRF() {
	filter_->destroy();
	delete filter_;
    }

    bool lookup(const std::string& key) {
	return filter_->lookupKey(key);
    }

    bool lookupRange(const std::string& left_key, const std::string& right_key) {
	//return filter_->lookupRange(left_key, false, right_key, false);
	return filter_->lookupRange(left_key, true, right_key, true);
    }

    uint64_t getMemoryUsage() {
	return filter_->getMemoryUsage();
    }

private:
    surf::SuRF* filter_;
};

} // namespace bench

#endif // FILTER_SURF_H
