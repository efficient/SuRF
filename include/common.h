#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define MSB_MASK_64 0x8000000000000000

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

inline void setBit(uint64_t &byte, int pos) {
    byte = byte | (MSB_MASK_64 >> pos);
}

inline bool readBit(uint64_t &byte, int pos) {
    return byte & (MSB_MASK_64 >> pos);
}

inline bool isLabelExist(uint64_t *bits, uint8_t c) {
    int group = c >> 6;
    int idx = c & 63;
    return bits[group] & (MSB_MASK_64 >> idx);
}

inline bool isLabelExist_lowerBound(uint64_t *bits, uint8_t c, uint8_t &pos) {
    int group = c >> 6;
    int idx = c & 63;
    uint64_t b64 = bits[group];
    if (b64 & (MSB_MASK_64 >> idx)) {
	pos = c;
	return true;
    }
    else {
	b64 <<= idx;
	if (b64) {
	    pos = c + __builtin_clzll(b64);
	    return true;
	}
	for (int i = (group + 1); i < 4; i++) {
	    b64 = bits[i];
	    if (b64) {
		pos = (i << 6) + __builtin_clzll(b64);
		return true;
	    }
	}
    }
    return false;
}

inline bool isLabelExist_upperBound(uint64_t *bits, uint8_t c, uint8_t &pos) {
    int group = c >> 6;
    int idx = c & 63;
    uint64_t b64 = bits[group];
    if (b64 & (MSB_MASK_64 >> idx)) {
	pos = c;
	return true;
    }
    else {
	b64 >>= (63 - idx);
	if (b64) {
	    pos = c - __builtin_ctzll(b64);
	    return true;
	}
	for (int i = (group - 1); i >= 0; i--) {
	    b64 = bits[i];
	    if (b64) {
		pos = ((i + 1) << 6) - 1 - __builtin_ctzll(b64);
		return true;
	    }
	}
    }
    return false;
}

inline void setLabel(uint64_t *bits, uint8_t c) {
    int group = c >> 6;
    int idx = c & 63;
    bits[group] |= ((uint64_t)1 << (63 - idx));
}

inline int commonPrefixLen(std::string &a, std::string &b) {
    int len = a.length();
    if (b.length() < len)
	len = b.length();
    for (int i = 0; i < len; i++) {
	if (a[i] != b[i])
	    return i;
    }
    return len;
}

#endif /* _COMMON_H_ */

