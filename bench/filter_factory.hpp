#ifndef FILTER_FACTORY_H_
#define FILTER_FACTORY_H_

#include "filter.hpp"
#include "filter_bloom.hpp"
#include "filter_surf.hpp"

namespace bench {

class FilterFactory {
public:
    static Filter* createFilter(const std::string& filter_type,
				const uint32_t suffix_len,
				const std::vector<std::string>& keys) {
	if (filter_type.compare(std::string("SuRF")) == 0)
	    return new FilterSuRF(keys, surf::kNone, 0, 0);
	else if (filter_type.compare(std::string("SuRFHash")) == 0)
	    return new FilterSuRF(keys, surf::kHash, suffix_len, 0);
	else if (filter_type.compare(std::string("SuRFReal")) == 0)
	    return new FilterSuRF(keys, surf::kReal, 0, suffix_len);
        else if (filter_type.compare(std::string("SuRFMixed")) == 0)
	    return new FilterSuRF(keys, surf::kMixed, suffix_len, suffix_len);
	else if (filter_type.compare(std::string("Bloom")) == 0)
	    return new FilterBloom(keys);
	else
	    return new FilterSuRF(keys, surf::kReal, 0, suffix_len); // default
    }
};

} // namespace bench

#endif // FILTER_FACTORY_H
