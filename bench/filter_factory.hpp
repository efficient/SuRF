#ifndef FILTER_FACTORY_H_
#define FILTER_FACTORY_H_

#include "filter.hpp"
#include "filter_arf.hpp"
#include "filter_bloom.hpp"
#include "filter_surf.hpp"

namespace bench {

class FilterFactory {
public:
    static Filter* createFilter(const std::string& filter_type, 
				const std::vector<std::string>& keys) {
	if (filter_type.compare(std::string("SuRF")) == 0)
	    return new FilterSuRF(keys, surf::kNone);
	else if (filter_type.compare(std::string("SuRFHash")) == 0)
	    return new FilterSuRF(keys, surf::kHash);
	else if (filter_type.compare(std::string("SuRFReal")) == 0)
	    return new FilterSuRF(keys, surf::kReal);
	else if (filter_type.compare(std::string("Bloom")) == 0)
	    return new FilterBloom(keys);
	else if (filter_type.compare(std::string("ARF")) == 0)
	    return new FilterARF(keys);
	else
	    return new FilterSuRF(keys, surf::kReal); // default
    }
};

} // namespace bench

#endif // FILTER_FACTORY_H
