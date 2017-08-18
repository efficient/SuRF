#ifndef _CONFIG_H_
#define _CONFIG_H_

using position_t = uint32_t;

using suffix_t = uint8_t;
static const suffix_t SUFFIX_HASH_MASK = 0xFF;

static const uint32_t SPARSE_DENSE_RATIO = 64;
static const uint8_t TERMINATOR = 255;
static const uint64_t MSB_MASK = 0x8000000000000000;
static const int WORD_SIZE = 64;

enum SuffixType {
    SuffixType_None = 0,
    SuffixType_Hash = 1,
    SuffixType_Real = 2
};

#endif
