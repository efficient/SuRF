#include "bench.hpp"
#include "ARF.h"
#include "Database.h"
#include "Query.h"

static const int kARFSize = 70000000;
static const int kInputSize = 10000000;
static const int kTxnSize = 10000000;
static const int kTrainingSize = 2000000;
static const uint64_t kDomain = (ULLONG_MAX / 2 - 1);
static const uint64_t kRangeSize = 922336973116;

int main(int argc, char *argv[]) {
    if (argc != 4) {
	std::cout << "Usage:\n";
	std::cout << "1. percentage of keys inserted: 0 < num <= 100\n";
	std::cout << "2. query type: point, range\n";
	std::cout << "3. distribution: uniform, zipfian, latest\n";
	return -1;
    }

    unsigned percent = atoi(argv[1]);
    std::string query_type = argv[2];
    std::string distribution = argv[3];

    // check args ====================================================
    if (percent > 100) {
	std::cout << bench::kRed << "WRONG percentage\n" << bench::kNoColor;
	return -1;
    }

    if (query_type.compare(std::string("point")) != 0
	&& query_type.compare(std::string("range")) != 0) {
	std::cout << bench::kRed << "WRONG query type\n" << bench::kNoColor;
	return -1;
    }

    if (distribution.compare(std::string("uniform")) != 0
	&& distribution.compare(std::string("zipfian")) != 0
	&& distribution.compare(std::string("latest")) != 0) {
	std::cout << bench::kRed << "WRONG distribution\n" << bench::kNoColor;
	return -1;
    }

    // load keys from files =======================================
    std::string load_file = "workloads/load_randint";
    std::vector<uint64_t> load_keys;
    bench::loadKeysFromFile(load_file, kInputSize, load_keys);
    std::cout << "load_keys size = " << load_keys.size() << "\n";

    sort(load_keys.begin(), load_keys.end());
    uint64_t max_key = load_keys[load_keys.size() - 1];
    std::cout << std::hex << "max key = " << max_key << std::dec << "\n";
    uint64_t max_gap = load_keys[load_keys.size() - 1] - load_keys[0];
    std::cout << "max gap = " << max_gap << "\n";
    uint64_t avg_gap = max_gap / kInputSize;
    std::cout << "avg gap = " << avg_gap << "\n";

    std::string txn_file = "workloads/txn_randint_";
    txn_file += distribution;
    std::vector<uint64_t> txn_keys;
    bench::loadKeysFromFile(txn_file, kTxnSize, txn_keys);
    std::cout << "txn_keys size = " << txn_keys.size() << "\n";

    std::vector<uint64_t> insert_keys;
    bench::selectIntKeysToInsert(percent, insert_keys, load_keys);
    std::cout << "insert_keys size = " << insert_keys.size() << "\n";

    // compute upperbound keys for range queries =================
    std::vector<uint64_t> upper_bound_keys;
    if (query_type.compare(std::string("range")) == 0) {
	for (int i = 0; i < (int)txn_keys.size(); i++) {
	    txn_keys[i]++;
	    uint64_t upper_bound = txn_keys[i] + kRangeSize;
	    upper_bound_keys.push_back(upper_bound);
	}
    } else {
	for (int i = 0; i < (int)txn_keys.size(); i++) {
	    upper_bound_keys.push_back(txn_keys[i]);
	}
    }

    // create filter ==============================================
    arf::Database* db = new arf::Database(insert_keys);
    arf::ARF* filter = new arf::ARF(0, kDomain, db);

    // build perfect ARF ==========================================
    double start_time = bench::getNow();
    filter->perfect(db);
    double end_time = bench::getNow();
    double time_diff = end_time - start_time;
    std::cout << "build perfect time = " << time_diff << " s\n";

    // training ===================================================
    start_time = bench::getNow();
    for (int i = 0; i < kTrainingSize; i++) {
	if (i % 100000 == 0)
	    std::cout << "i = " << i << std::endl;
	bool qR = db->rangeQuery(txn_keys[i], upper_bound_keys[i]);
	filter->handle_query(txn_keys[i], upper_bound_keys[i], qR, true);
    }
    filter->reset_training_phase();
    filter->truncate(kARFSize);
    filter->end_training_phase();
    filter->print_size();
    end_time = bench::getNow();
    time_diff = end_time - start_time;
    std::cout << "training time = " << time_diff << " s\n";
    std::cout << "training throughput = " << ((kTrainingSize + 0.0) / time_diff) << " txns/s\n";

    // execute transactions =======================================
    int64_t positives = 0;
    start_time = bench::getNow();
    for (int i = kTrainingSize; i < kTxnSize; i++) {
	positives += (int)filter->handle_query(txn_keys[i], upper_bound_keys[i], true, false);
    }
    end_time = bench::getNow();
    time_diff = end_time - start_time;
    std::cout << "time = " << time_diff << " s\n";
    std::cout << "throughput = " << bench::kGreen << ((kTrainingSize + 0.0) / time_diff) << bench::kNoColor << " txns/s\n";
    end_time = bench::getNow();

    // compute true positives ======================================
    int64_t tps = 0;
    int64_t tns = 0;
    for (int i = kTrainingSize; i < kTxnSize; i++) {
	bool dR = db->rangeQuery(txn_keys[i], upper_bound_keys[i]);
	if (dR)
	    tps++;
	else
	    tns++;
    }
    int64_t fps = positives - tps;

    std::cout << "positives = " << positives << "\n";
    std::cout << "true positives = " << tps << "\n";
    std::cout << "true negatives = " << tns << "\n";
    std::cout << "false positives = " << fps << "\n";

    double fp_rate = 0;
    if (fps >= 0)
	fp_rate = fps / (tns + fps + 0.0);
    else
	std::cout << "ERROR: fps < 0\n";
    std::cout << "False Positive Rate = " << fp_rate << "\n";

    return 0;
}
