#ifndef SURFSTATIC_H_
#define SURFSTATIC_H_

#include <string>

#include "config.hpp"
#include "loudsDense.hpp"
#include "loudsSparse.hpp"

using namespace std;

class SuRF {
public:
    SuRF() {};

    SuRF(const vector<vector<uint64_t> > &bitMap_labelsPL,
	 const vector<vector<uint64_t> > &bitMap_childIndicatorBitsPL,
	 const vector<vector<uint64_t> > &prefixKeyIndicatorBitsPL,
	 const vector<vector<uint8_t> > &labelsPL,
	 const vector<vector<uint64_t> > &childIndicatorBitsPL,
	 const vector<vector<uint64_t> > &loudsBitsPL,
	 const vector<vector<uint8_t> > &suffixesPL,
	 const vector<uint32_t> &nodeCountPL,
	 const uint32_t sparseStartLevel,
	 const SuffixType suffixConfig) {
	loudsDense_ = new LoudsDense(bitMap_labelsPL, bitMap_childIndicatorBitsPL, prefixKeyIndicatorBitsPL, suffixesPL, suffixConfig);
	loudsSparse_ = new LoudsSparse(labelsPL, childIndicatorBitsPL, loudsBitsPL, suffixesPL, nodeCountPL, sparseStartLevel, suffixConfig);
    }

    ~SuRF() {
	delete loudsDense_;
	delete loudsSparse_;
    }

    bool lookupKey(const string &key);
    bool lookupRange(const string &leftKey, const string &rightKey);
    uint32_t countRange(const string &leftKey, const string &rightKey);

    bool getLowerBoundKey(const string &key, string &outKey);

    uint64_t getMemoryUsage();

private:
    LoudsDense* loudsDense_;
    LoudsSparse* loudsSparse_;
};

bool SuRF::lookupKey(const string &key) {
    position_t connectNodeNum = 0;
    if (!loudsDense_->lookupKey(key, connectNodeNum))
	return loudsSparse_->lookupKey(key, connectNodeNum);
    return true;
}

bool SuRF::lookupRange(const string &leftKey, const string &rightKey) {
    position_t leftConnectPos = 0;
    position_t rightConnectPos = 0;
    if (!loudsDense_->lookupRange(key, leftConnectPos, rightConnectPos))
	return loudsSparse_->lookupRange(key, leftConnectPos, rightConnectPos);
    return true;
}

uint32_t SuRF::countRange(const string &leftKey, const string &rightKey) {
    uint32_t count = 0;
    position_t leftConnectPos = 0;
    position_t rightConnectPos = 0;
    count += loudsDense_->countRange(key, leftConnectPos, rightConnectPos);
    count += loudsSparse_->countRange(key, leftConnectPos, rightConnectPos);
    return count;
}

bool SuRF::getLowerBoundKey(const string &key, string &outputKey) {
    position_t connectPos = 0;
    if (!loudsDense_->getLowerBoundKey(key, outputKey, connectPos))
	return loudsSparse_->getLowerBoundKey(key, outputKey, connectPos);
    return true;
}

uint64_t SuRF::getMemoryUsage() {
    return (sizeof(SuRF) + loudsDense_->getMemoryUsage() + loudsSparse_->getMemoryUsage());
}

