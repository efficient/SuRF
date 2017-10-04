#ifndef FILTER_ARF_H_
#define FILTER_ARF_H_

#include <string>
#include <vector>

namespace bench {

class FilterARF : public Filter {
public:
    // Requires that keys are sorted
    FilterARF(const std::vector<std::string>& keys) {
	;
    }

    ~FilterARF() {
	;
    }

    bool lookup(const std::string& key) {
	return false;
    }

    bool lookupRange(const std::string& left_key, const std::string& right_key) {
	return false;
    }

    uint64_t getMemoryUsage() {
	return 0;
    }

private:

};

} // namespace bench

#endif // FILTER_ARF_H
