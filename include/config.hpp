#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

namespace surf {

using level_t = uint32_t;
using position_t = uint32_t;

using label_t = uint8_t;
using suffix_t = uint8_t;
static const suffix_t kSuffixHashMask = 0xFF;

using word_t = uint64_t;
static const unsigned kWordSize = 64;
static const word_t kMsbMask = 0x8000000000000000;

static const uint32_t kSparseDenseRatio = 64;
static const label_t kTerminator = 255;

enum SuffixType {
    kNone = 0,
    kHash = 1,
    kReal = 2
};

} // namespace surf

#endif // CONFIG_H_
