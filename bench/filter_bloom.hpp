#ifndef FILTER_BLOOM_H_
#define FILTER_BLOOM_H_

#include <string>
#include <vector>

#include "bloom.hpp"

namespace bench {

class FilterBloom : public Filter {
public:
    // Requires that keys are sorted
    FilterBloom(const std::vector<std::string>& keys) {
	filter_ = new BloomFilter(kBitsPerKey);
	filter_->CreateFilter(keys, keys.size(), &filter_data_);
    }

    ~FilterBloom() {
	delete filter_;
    }

    bool lookup(const std::string& key) {
	return filter_->KeyMayMatch(key, filter_data_);
    }

    bool lookupRange(const std::string& left_key, const std::string& right_key) {
	std::cout << kRed << "A Bloom filter does not support range queries\n" << kNoColor;
	return false;
    }

    uint64_t getMemoryUsage() {
	return filter_data_.size();
    }

private:
    int kBitsPerKey = 10;

    BloomFilter* filter_;
    std::string filter_data_;
};

} // namespace bench

#endif // FILTER_BLOOM_H
