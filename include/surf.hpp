#ifndef SURF_H_
#define SURF_H_

#include <emmintrin.h>
#include <assert.h>
#include <string.h>

#include <vector>
#include <iostream>
#include <string>
#include <climits>

#include "common.h"
#include "popcount.h"

using namespace std;

class SuRFIter;
class SuRF;

//******************************************************
// Constants for SuRF
//******************************************************
const uint8_t TERM = 36; //$
//const int CUTOFF_RATIO = 64;
const int CUTOFF_RATIO = 10000000;
//const int MIN_PATH_LEN = 0;

const uint8_t CHECK_BITS_MASK = 0xFF; //8

//******************************************************
//Constants for rank and select
//******************************************************
const int kCacheLineSize = 64;
const int kWordSize = 64;

const int kBasicBlockSizeU = 64;
const int kBasicBlockBitsU = 6;
const int kBasicBlockMaskU = kBasicBlockSizeU - 1;
const int kWordCountPerBasicBlockU = kBasicBlockSizeU / kWordSize;

const int kBasicBlockSize = 512;
const int kBasicBlockBits = 9;
const int kBasicBlockMask = kBasicBlockSize - 1;
const int kWordCountPerBasicBlock = kBasicBlockSize / kWordSize;

const int kWordBits = 6;
const uint32_t kWordMask = ((uint32_t)1 << kWordBits) - 1;

const int skip = 64;
const int kSkipBits = 6;
const uint32_t kSkipMask = ((uint32_t)1 << kSkipBits) - 1;


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

inline uint32_t suffixHash(const string &key) {
    return Hash(key.c_str(), key.size(), 0xbc9f1d34);
}

inline uint32_t suffixHash(const char* key, const int keylen) {
    return Hash(key, keylen, 0xbc9f1d34);
}


//******************************************************
// Order-preserving Huffman Encoding
//******************************************************
typedef struct ht_node {
    bool isLeaf;
    uint16_t leftLabel; //inclusive
    uint16_t rightLabel; //non-inclusive
    uint64_t count;
    ht_node* leftChild;
    ht_node* rightChild;
} ht_node;

inline bool build_ht(vector<string> &keys, uint8_t* hufLen, char* hufTable) {
    if (!hufLen || !hufTable) {
	cout << "Error: unallocated buffer(s)\n";
	return false;
    }

    for (int i = 0; i < 256; i++) {
	hufLen[i] = 0;
	hufTable[i] = 0;
	hufTable[i+i] = 0;
    }

    //collect frequency stats
    uint64_t freq[256];
    for (int i = 0; i < 256; i++)
	freq[i] = 0;

    for (int k = 0; k < (int)keys.size(); k++) {
	string key = keys[k];
	for (int j = 0; j < (int)key.length(); j++)
	    freq[(uint8_t)key[j]]++;
    }

    //for (int i = 0; i < 256; i ++)
    //cout << i << ": " << freq[i] << "\n";

    vector<ht_node*> nodeList;
    vector<ht_node*> tempNodeList;
    vector<ht_node*> allNodeList;
    //init huffman tree leaf nodes
    uint16_t c = 0;
    for (int i = 0; i < 256; i++) {
	ht_node* n = new ht_node();
	n->isLeaf = true;
	n->leftLabel = c;
	n->rightLabel = c + 1;
	n->count = freq[i];
	n->leftChild = NULL;
	n->rightChild = NULL;
	nodeList.push_back(n);
	allNodeList.push_back(n);
	c++;
    }

    //merge adjacent low-frequency nodes to build huffman tree
    while (nodeList.size() > 1) {
	uint64_t lowest_freq_sum = ULLONG_MAX;
	for (int i = 0; i < (int)nodeList.size() - 1; i++) {
	    uint64_t s = nodeList[i]->count + nodeList[i+1]->count;
	    if (s < lowest_freq_sum)
		lowest_freq_sum = s;
	}

	//cout << "lowest_freq_sum = " << lowest_freq_sum << "\n";
	//cout << "nodeList.size() = " << nodeList.size() << "\n";

	int pos = 0;
	while (pos < (int)nodeList.size() - 1) {
	    uint64_t s = nodeList[pos]->count + nodeList[pos+1]->count;
	    if (s <= lowest_freq_sum) {
		ht_node* n = new ht_node();
		n->isLeaf = false;
		n->leftLabel = nodeList[pos]->leftLabel;
		n->rightLabel = nodeList[pos+1]->rightLabel;
		n->count = s;
		n->leftChild = nodeList[pos];
		n->rightChild = nodeList[pos+1];
		tempNodeList.push_back(n);
		allNodeList.push_back(n);
		pos += 2;
	    }
	    else {
		tempNodeList.push_back(nodeList[pos]);
		pos++;
	    }
	}

	if (pos == (int)nodeList.size() - 1)
	    tempNodeList.push_back(nodeList[pos]);

	nodeList = tempNodeList;
	tempNodeList.clear();
    }

    //generate huffman table
    ht_node* root;
    if (nodeList.size() > 0)
	root = nodeList[0];
    else {
	cout << "Error: empty node list\n";
	return false;
    }

    //print huffman tree
    // vector<ht_node*> printList;
    // vector<ht_node*> tempPrintList;
    // printList.push_back(root);
    // while (printList.size() > 0) {
    // 	for (int i = 0; i < (int)printList.size(); i++) {
    // 	    ht_node* node = printList[i];
    // 	    cout << "[" << node->leftLabel << ", " << node->rightLabel << ") ";
    // 	    if (node->leftChild)
    // 		tempPrintList.push_back(node->leftChild);
    // 	    if (node->rightChild)
    // 		tempPrintList.push_back(node->rightChild);
    // 	}
    // 	printList = tempPrintList;
    // 	tempPrintList.clear();
    // 	cout << "\n-------------------------------------------------------------\n";
    // }

    uint8_t byte_mask = 0x80;
    for (int i = 0; i < 256; i++) {
	//cout << "i = " << i << "============================================\n";
	ht_node* node = root;
	while (!node->isLeaf) {
	    ht_node* left = node->leftChild;
	    ht_node* right = node->rightChild;

	    //cout << "left range = " << "[" << left->leftLabel << ", " << left->rightLabel << ")\t";
	    //cout << "right range = " << "[" << right->leftLabel << ", " << right->rightLabel << ")\n";

	    if (left && i >= left->leftLabel && i < left->rightLabel) {
		hufLen[i]++;
		node = left;
	    }
	    else if (right && i >= right->leftLabel && i < right->rightLabel) {
		if (hufLen[i] == 0)
		    hufTable[i+i] |= byte_mask;
		else if (hufLen[i] < 8) {
		    hufTable[i+i] |= (byte_mask >> hufLen[i]);
		}
		else if (hufLen[i] == 8)
		    hufTable[i+i+1] |= byte_mask;
		else {
		    hufTable[i+i+1] |= (byte_mask >> (hufLen[i] - 8));
		}
		hufLen[i]++;
		node = right;
	    }
	    else {
		cout << "ERROR: tree navigation\n";
		return false;
	    }
	}
    }

    //garbage collect ht_nodes
    for (int i = 0; i < (int)allNodeList.size(); i++)
	delete allNodeList[i];

    return true;
}

inline void print_ht(uint8_t* hufLen, char* hufTable) {
    if (!hufLen || !hufTable) {
	cout << "Error: unallocated buffer(s)\n";
	return;
    }

    for (int i = 0; i < 256; i++) {
	cout << i << ": " << (uint16_t)hufLen[i] << "; " << hex << (uint16_t)(uint8_t)hufTable[i+i] << " " << (uint16_t)(uint8_t)hufTable[i+i+1] << " " << dec;
	//cout << i << ": ";
	uint8_t byte_mask = 0x80;
	char code = hufTable[i+i];
	for (int j = 0; j < hufLen[i]; j++) {
	    //cout << hex << (uint16_t)byte_mask << " ";
	    if (j == 8) {
		code = hufTable[i+i+1];
		byte_mask = 0x80;
	    }
	    if (code & byte_mask)
		cout << "1";
	    else
		cout << "0";
	    byte_mask >>= 1;
	}
	cout << "\n";
    }
}

inline bool encode(string &key_huf, string &key, uint8_t* hufLen, char* hufTable) {
    //compute code length
    int code_len_bit = 0;
    for (int i = 0; i < key.size(); i++) {
	int c = (uint8_t)key[i];
	code_len_bit += hufLen[c];
    }
    //round-up to byte-aligned
    int code_len_byte = 0;
    if (code_len_bit % 8 == 0)
	code_len_byte = code_len_bit / 8;
    else
	code_len_byte = code_len_bit / 8 + 1;

    //init result string
    key_huf.resize(code_len_byte);
    for (int i = 0; i < code_len_byte; i++)
	key_huf[i] = 0;

    //encode
    int pos = 0;
    int byte = pos / 8;
    int offset = pos - (byte * 8);
    for (int i = 0; i < key.size(); i++) {
	int c = (uint8_t)key[i];
	code_len_bit = hufLen[c];
	uint8_t code_byte1 = (uint8_t)hufTable[c+c];
	uint8_t code_byte2 = (uint8_t)hufTable[c+c+1];
	
	if (code_len_bit < 8) {
	    uint8_t mask = code_byte1 >> offset;
	    key_huf[byte] |= mask;

	    if (code_len_bit > 8 - offset) {
		mask = code_byte1 << (8 - offset);
		key_huf[byte + 1] |= mask;
	    }
	    pos += code_len_bit;
	    byte = pos / 8;
	    offset = pos - (byte * 8);
	}
	else {
	    uint8_t mask = code_byte1 >> offset;
	    key_huf[byte] |= mask;
	    if (offset > 0) {
		mask = code_byte1 << (8 - offset);
		key_huf[byte + 1] |= mask;
	    }
	    pos += 8;
	    byte = pos / 8;
	    offset = pos - (byte * 8);
	    code_len_bit -= 8;

	    mask = code_byte2 >> offset;
	    key_huf[byte] |= mask;

	    if (code_len_bit > 8 - offset) {
		mask = code_byte2 << (8 - offset);
		key_huf[byte + 1] |= mask;
	    }
	    pos += code_len_bit;
	    byte = pos / 8;
	    offset = pos - (byte * 8);
	}
    }

    // cout << "key = " << key << "\t";
    // cout << "key_huf = ";
    // for (int i = 0; i < key_huf.size(); i++) {
    // 	cout << (uint16_t)(uint8_t)key_huf[i] << " ";
    // }
    // cout << "\n";

    return true;
}

inline bool encodeList(vector<string> &keys_huf, vector<string> &keys, uint8_t* hufLen, char* hufTable) {
    for (int i = 0; i < (int)keys.size(); i++) {
	string key_huf;
	if (!encode(key_huf, keys[i], hufLen, hufTable)) return false;
	keys_huf.push_back(key_huf);
    }
    return true;
}


//******************************************************
// Initilization functions for SuRF
//******************************************************
SuRF* load(vector<string> &keys, int longestKeyLen, uint16_t suf_config, bool huf_config);
SuRF* load(vector<uint64_t> &keys, uint16_t suf_config);

//helpers
inline bool insertChar_cond(const uint8_t ch, vector<uint8_t> &c, vector<uint64_t> &t, vector<uint64_t> &s, uint64_t &pos, uint64_t &nc);
inline bool insertChar(const uint8_t ch, bool isTerm, vector<uint8_t> &c, vector<uint64_t> &t, vector<uint64_t> &s, uint64_t &pos, uint64_t &nc);


//******************************************************
// Fast Succinct Trie Class
//******************************************************
class SuRF {
public:
    SuRF();
    SuRF(int16_t cl, uint16_t th, int8_t fsp, int32_t lsp, 
	 uint32_t ncu, uint32_t ccu, uint16_t sc, bool hc,
	 uint32_t cUnb, uint32_t cUmem, uint32_t tUnb, uint32_t tUmem,
	 uint32_t oUnb, uint32_t oUmem, uint32_t sUm, uint32_t cmem,
	 uint32_t tnb, uint32_t tmem, uint32_t smem, uint32_t sbbc, uint32_t sm);
    virtual ~SuRF();

    friend inline bool insertChar_cond(const uint8_t ch, vector<uint8_t> &c, vector<uint64_t> &t, vector<uint64_t> &s, uint64_t &pos, uint64_t &nc);
    friend inline bool insertChar(const uint8_t ch, bool isTerm, vector<uint8_t> &c, vector<uint64_t> &t, vector<uint64_t> &s, uint64_t &pos, uint64_t &nc);

    friend SuRF* load(vector<string> &keys, int longestKeyLen, uint16_t suf_config, bool huf_config);
    friend SuRF* load(vector<uint64_t> &keys, uint16_t suf_config);

    //point query
    bool lookup(string &key);
    bool lookup(const uint8_t* key, const int keylen);
    bool lookup(const uint64_t key);
    bool lookup(const uint8_t* key, const int keylen, uint64_t &position);
    bool lookup(const uint64_t key, uint64_t &position);

    //range query
    bool lowerBound(const uint8_t* key, const int keylen, SuRFIter &iter);
    bool lowerBound(const uint64_t key, SuRFIter &iter);
    bool upperBound(const uint8_t* key, const int keylen, SuRFIter &iter);
    bool upperBound(const uint64_t key, SuRFIter &iter);


    //mem stats
    //-------------------------------------------------
    uint32_t cMemU();
    uint32_t cRankMemU();
    uint32_t tMemU();
    uint32_t tRankMemU();
    uint32_t oMemU();
    uint32_t oRankMemU();
    uint32_t suffixMemU();

    uint64_t cMem();
    uint32_t tMem();
    uint32_t tRankMem();
    uint32_t sMem();
    uint32_t sSelectMem();
    uint32_t suffixMem();

    uint64_t mem();

    //debug
    void printU();
    void print();

private:
    //bit/byte vector accessors
    //-------------------------------------------------
    inline uint64_t* cUbits_();
    inline uint32_t* cUrankLUT_();
    inline uint64_t* tUbits_();
    inline uint32_t* tUrankLUT_();
    inline uint64_t* oUbits_();
    inline uint32_t* oUrankLUT_();
    inline uint8_t* cbytes_();
    inline uint64_t* tbits_();
    inline uint32_t* trankLUT_();
    inline uint64_t* sbits_();
    inline uint32_t* sselectLUT_();
    inline uint8_t* suffixesU_();
    inline uint8_t* suffixes_();

    //rank and select init
    //-------------------------------------------------
    inline void cUinit(uint64_t* bits, uint32_t nbits);
    inline void tUinit(uint64_t* bits, uint32_t nbits);
    inline void oUinit(uint64_t* bits, uint32_t nbits);
    inline void tinit(uint64_t* bits, uint32_t nbits);
    inline void sinit(uint64_t* bits, uint32_t nbits);

    //rank and select support
    //-------------------------------------------------
    inline uint32_t cUrank(uint32_t pos);
    inline uint32_t tUrank(uint32_t pos);
    inline uint32_t oUrank(uint32_t pos);
    inline uint32_t trank(uint32_t pos);
    inline uint32_t sselect(uint32_t pos);

    //helpers
    //-------------------------------------------------
    inline bool isCbitSetU(uint64_t nodeNum, uint8_t kc);
    inline bool isTbitSetU(uint64_t nodeNum, uint8_t kc);
    inline bool isObitSetU(uint64_t nodeNum);
    inline bool isSbitSet(uint64_t pos);
    inline bool isTbitSet(uint64_t pos);
    inline uint64_t suffixPosU(uint64_t nodeNum, uint64_t pos);
    inline uint64_t suffixPos(uint64_t pos);

    inline uint64_t childNodeNumU(uint64_t pos);
    inline uint64_t childNodeNum(uint64_t pos);
    inline uint64_t childpos(uint64_t nodeNum);

    inline int nodeSize(uint64_t pos);
    inline bool simdSearch(uint64_t &pos, uint64_t size, uint8_t target);
    inline bool binarySearch(uint64_t &pos, uint64_t size, uint8_t target);
    inline bool linearSearch(uint64_t &pos, uint64_t size, uint8_t target);
    inline bool nodeSearch(uint64_t &pos, int size, uint8_t target);
    inline bool nodeSearch_lowerBound(uint64_t &pos, int size, uint8_t target);
    inline bool nodeSearch_upperBound(uint64_t &pos, int size, uint8_t target);

    inline bool binarySearch_lowerBound(uint64_t &pos, uint64_t size, uint8_t target);
    inline bool binarySearch_upperBound(uint64_t &pos, uint64_t size, uint8_t target);
    inline bool linearSearch_lowerBound(uint64_t &pos, uint64_t size, uint8_t target);
    inline bool linearSearch_upperBound(uint64_t &pos, uint64_t size, uint8_t target);

    inline bool nextItemU(uint64_t nodeNum, uint8_t kc, uint8_t &cc);
    inline bool prevItemU(uint64_t nodeNum, uint8_t kc, uint8_t &cc);

    inline bool nextLeftU(int keypos, uint64_t pos, SuRFIter* iter);
    inline bool nextLeft(int keypos, uint64_t pos, SuRFIter* iter);
    inline bool nextRightU(int keypos, uint64_t pos, SuRFIter* iter);
    inline bool nextRight(int keypos, uint64_t pos, SuRFIter* iter);

    inline bool nextNodeU(int keypos, uint64_t nodeNum, SuRFIter* iter);
    inline bool nextNode(int keypos, uint64_t pos, SuRFIter* iter);
    inline bool prevNodeU(int keypos, uint64_t nodeNum, SuRFIter* iter);
    inline bool prevNode(int keypos, uint64_t pos, SuRFIter* iter);

    //members
    //-------------------------------------------------
    int16_t cutoff_level_;
    uint16_t tree_height_;
    int8_t first_suffix_pos_; // negative means in suffixesU_
    int32_t last_suffix_pos_; // negative means in suffixesU_
    uint32_t nodeCountU_;
    uint32_t childCountU_;
    uint16_t suffixConfig_; // 0: no suffix; 1: suffix length; 2: one-byte hash bits; 3: one-byte suffix
    bool hufConfig_;

    //D-Labels
    uint32_t cUnbits_;
    uint32_t cUpCount_;
    uint32_t cUmem_;

    //D-HasChild
    uint32_t tUnbits_;
    uint32_t tUpCount_;
    uint32_t tUmem_;

    //D-IsPrefixKey
    uint32_t oUnbits_;
    uint32_t oUpCount_;
    uint32_t oUmem_;

    //D-values
    uint32_t suffixUmem_;

    //S-Labels
    uint32_t cmem_;

    //S-HasChild
    uint32_t tnbits_;
    uint32_t tpCount_;
    uint32_t tmem_;

    //S-LOUDS
    uint32_t  snbits_;
    uint32_t  spCount_;
    uint32_t  smem_;
    uint32_t  sselectLUTCount_;

    //S-values
    uint32_t suffixmem_;

    //Huffman Table
    uint8_t hufLen_[256]; // code length of each entry in the huffman table; max = 16 bits
    char hufTable_[512];

    //data
    char data_[0];

    friend class SuRFIter;
};

typedef struct {
    int32_t keyPos;
    int32_t sufPos;
    bool isO;
} Cursor;


class SuRFIter {
public:
    SuRFIter();
    SuRFIter(SuRF* idx);

    void clear ();

    inline void setVU (int keypos, uint64_t nodeNum, uint64_t pos);
    inline void setVU_R (int keypos, uint64_t nodeNum, uint64_t pos);
    inline void setKVU (int keypos, uint64_t nodeNum, uint64_t pos, bool o);
    inline void setKVU_R (int keypos, uint64_t nodeNum, uint64_t pos, bool o);
    inline void setV (int keypos, uint64_t pos);
    inline void setV_R (int keypos, uint64_t pos);
    inline void setKV (int keypos, uint64_t pos);
    inline void setKV_R (int keypos, uint64_t pos);

    string key ();
    uint8_t suffix ();
    bool operator ++ (int);
    bool operator -- (int);

private:
    SuRF* index;
    vector<Cursor> positions;

    uint32_t len;
    bool isBegin;
    bool isEnd;

    uint32_t cBoundU;
    uint64_t cBound;
    int cutoff_level;
    uint32_t tree_height;
    int8_t first_suffix_pos;
    uint32_t last_suffix_pos;

    friend class SuRF;
};

#endif  // SURF_H_
