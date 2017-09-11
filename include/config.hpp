#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

namespace surf {

using level_t = uint32_t;
using position_t = uint32_t;

using label_t = uint8_t;
static const position_t kFanout = 256;

using suffix_t = uint8_t;
static const suffix_t kSuffixHashMask = 0xFF;

using word_t = uint64_t;
static const unsigned kWordSize = 64;
static const word_t kMsbMask = 0x8000000000000000;

static const bool kIncludeDense = true;
static const uint32_t kSparseDenseRatio = 64;
static const label_t kTerminator = 255;

enum SuffixType {
    kNone = 0,
    kHash = 1,
    kReal = 2
};

std::string uint64ToString(uint64_t key) {
    uint64_t endian_swapped_key = __builtin_bswap64(key);
    return std::string(reinterpret_cast<const char*>(&endian_swapped_key), 8);
}

uint64_t stringToUint64(std::string str_key) {
    uint64_t int_key = 0;
    memcpy(reinterpret_cast<char*>(&int_key), str_key.data(), 8);
    return __builtin_bswap64(int_key);
}

} // namespace surf

#endif // CONFIG_H_
