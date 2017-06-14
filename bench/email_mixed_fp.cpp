#include "microbench.hpp"

using namespace std;

//==============================================================
// EXEC
//==============================================================
inline void exec(int wl, vector<string> &init_keys, vector<string> &keys, int percent) {
    vector<string> insert_init_keys;
    for (int i = 0; i < (INIT_LIMIT_EMAIL * percent / 100); i++)
	insert_init_keys.push_back(init_keys[i]);

    //put back
    sort(insert_init_keys.begin(), insert_init_keys.end());

#ifdef USE_BLOOM
    BloomFilter *idx = new BloomFilter(BITS_PER_KEY);
    string filter;
    idx->CreateFilter(insert_init_keys, insert_init_keys.size(), &filter);
#else
    SuRF *idx = load(insert_init_keys, LONGEST_KEY_LEN, INCLUDE_SUFFIX_LEN, HUFFMAN_CONFIG);
    cout << "memory = " << (idx->mem() / 1000000.0) << "MB\n";
#endif

    //LOAD MAP----------------
    map<string, bool> ht;
    for (int i = 0; i < insert_init_keys.size(); i++)
	ht[insert_init_keys[i]] = true;

    //COMPUTE TRUE POSITIVES----------------
    uint64_t tp = 0;
    map<string, bool>::iterator ht_iter;
    if (wl < 2) {
	for (int i = 0; i < LIMIT; i++) {
	    ht_iter = ht.find(keys[i]);
	    tp += (ht_iter != ht.end());
	}
    }
    else {
	for (int i = 0; i < LIMIT; i++) {
	    string upper = keys[i].c_str();
	    upper[upper.length() - 1]++;
	    ht_iter = ht.lower_bound(keys[i]);
	    if (ht_iter != ht.end()) {
		//string lower = ht_iter->first.c_str();
		string lower = ht_iter->first;
		tp += (lower.compare(upper) < 0);
	    }
	}
    }

    //READ TEST----------------

    //pre-load encoded keys
    //vector<string> keys_huf;
    //encodeList(keys_huf, keys, idx->hufLen(), idx->hufTable());

    uint64_t pp = 0;

#ifdef USE_BLOOM
    if (wl < 2) {
	for (int i = 0; i < LIMIT; i++)
	    pp += (int)idx->KeyMayMatch(keys[i], filter);
    }
#else
    SuRFIter iter(idx);
    if (wl < 2) {
	//pre-load encoded keys
	//for (int i = 0; i < LIMIT; i++)
	//pp += (int)idx->lookup(keys_huf[i]);

	for (int i = 0; i < LIMIT; i++)
	    pp += (int)idx->lookup(keys[i]);
    }
    else {
	uint8_t upper_str[8];
	for (int i = 0; i < LIMIT; i++) {
	    idx->lowerBound(keys[i], iter);
	    string upper = keys[i].c_str();
	    upper[upper.length() - 1]++;
	    string lower = iter.key();
	    pp += (lower.compare(upper) < 0);
	}
    }
#endif

    cout << "ALL positives = " << pp << "\n";
    cout << "TRUE positives = " << tp << "\n";
    cout << "FALSE positives = " << (pp - tp) << "\n";
    cout << "TRUE negatives = " << (LIMIT - tp) << "\n";
    cout << "FP rate = " << (((pp - tp) + 0.0) / (LIMIT - tp + (pp - tp))) << "\n";

    cout << (((pp - tp) + 0.0) / (LIMIT - tp + (pp - tp))) << "\n";
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
	cout << "Usage:\n";
	cout << "1. point or range query: p/r\n";
	cout << "2. uniform or zipf distribution: u/z\n";
	return 1;
    }

    int wl = 0;
    if (strcmp(argv[1], "p") == 0) {
	if (strcmp(argv[2], "u") == 0)
	    wl = 1;
	else if (strcmp(argv[2], "z") == 0)
	    wl = 0;
    }
    else if (strcmp(argv[1], "r") == 0) {
	if (strcmp(argv[2], "u") == 0)
	    wl = 3;
	else if (strcmp(argv[2], "z") == 0)
	    wl = 2;
    }

    vector<string> init_keys;
    vector<string> keys;
    vector<int> ranges;
    vector<int> ops;

    load_init_email(wl, INIT_LIMIT_EMAIL, init_keys);
    load_txn_email(wl, LIMIT, keys, ranges, ops);

    //put back
    random_shuffle(init_keys.begin(), init_keys.end());

    //for (int p = 10; p < 100; p += 10)
    //exec(wl, init_keys, keys, p);
    exec(wl, init_keys, keys, 50);

    return 0;
}
