#ifndef HASH_H_
#define HASH_H_

#include <string>

namespace surf {

//******************************************************
//HASH FUNCTION FROM LEVELDB
//******************************************************
inline uint32_t DecodeFixed32(const char* ptr) {
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
}

inline uint32_t Hash(const char* data, size_t n, uint32_t seed) {
    // Similar to murmur hash
    const uint32_t m = 0xc6a4a793;
    const uint32_t r = 24;
    const char* limit = data + n;
    uint32_t h = seed ^ (n * m);

    // Pick up four bytes at a time
    while (data + 4 <= limit) {
	uint32_t w = DecodeFixed32(data);
	data += 4;
	h += w;
	h *= m;
	h ^= (h >> 16);
    }

    // Pick up remaining bytes
    switch (limit - data) {
    case 3:
	h += static_cast<unsigned char>(data[2]) << 16;
    case 2:
	h += static_cast<unsigned char>(data[1]) << 8;
    case 1:
	h += static_cast<unsigned char>(data[0]);
	h *= m;
	h ^= (h >> r);
	break;
    }
    return h;
}

inline uint32_t suffixHash(const std::string &key) {
    return Hash(key.c_str(), key.size(), 0xbc9f1d34);
}

inline uint32_t suffixHash(const char* key, const int keylen) {
    return Hash(key, keylen, 0xbc9f1d34);
}

} // namespace surf

#endif // HASH_H_

