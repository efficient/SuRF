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
#include "hash.h"
#include "ophuf.h"

#include "rank.hpp"

using namespace std;

class SuRF;
class SuRFIter;

//******************************************************
// Constants for SuRF
//******************************************************
const uint8_t TERM = 255; //prefix termination indicator
const int CUTOFF_LEVEL = -1; //-1 means using the default cutoff ratio
const int CUTOFF_RATIO = 64;
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
// SuRF: Succinct Range Filter
//******************************************************
class SuRF {

//******************************************************
// Public Interface
//******************************************************
public:
    //new
    static SuRF* buildSuRF(vector<string> &keys, int longestKeyLen, uint16_t suf_config, bool huf_config);
    static SuRF* buildSuRF(vector<uint64_t> &keys, uint16_t suffixConfig);

    void destroy();

    //point query
    bool lookup(string &key);
    bool lookup(const uint8_t* key, const int keylen);
    bool lookup(const uint64_t key);

    //range query
    bool lowerBound(string &key, SuRFIter &iter);
    bool lowerBound(const uint8_t* key, const int keylen, SuRFIter &iter);
    bool lowerBound(const uint64_t key, SuRFIter &iter);
    bool upperBound(string &key, SuRFIter &iter);
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


    uint8_t* hufLen();
    char* hufTable();

    //debug
    void printU();
    void print();

private:
    SuRF();
    SuRF(int16_t cl, uint16_t th, int8_t fsp, int32_t lsp, 
         uint32_t ncu, uint32_t ccu, uint16_t sc, bool hc,
         uint32_t cUnb, uint32_t cUmem, uint32_t tUnb, uint32_t tUmem,
         uint32_t oUnb, uint32_t oUmem, uint32_t sUm, uint32_t cmem,
         uint32_t tnb, uint32_t tmem, uint32_t smem, uint32_t sbbc, uint32_t sm);
    ~SuRF();

    //new
    static inline bool insertChar_cond(const uint8_t ch, vector<uint8_t> &c, vector<uint64_t> &t, vector<uint64_t> &s, uint64_t &pos, uint64_t &is_term, uint64_t &nc);
    static inline bool insertChar(const uint8_t ch, bool isTerm, vector<uint8_t> &c, vector<uint64_t> &t, vector<uint64_t> &s, uint64_t &pos, uint64_t &nc);

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

    //class members
    //-------------------------------------------------
    uint16_t treeHeight_;
    int16_t cutoffLevel_;
    uint32_t nodeCountDense_;
    uint32_t childCountDense_;
    uint16_t suffixConfig_; // 0: no suffix; 1: suffix length; 2: one-byte hash bits; 3: one-byte suffix
    bool useHuf_;

    //TODO: remove
    int8_t first_suffix_pos_; // negative means in suffixesU_
    int32_t last_suffix_pos_; // negative means in suffixesU_

    BitVectorRank labelDense_;

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
    //uint32_t last_suffix_pos;
    int32_t last_suffix_pos;

    friend class SuRF;
};

#endif // SURF_H_
