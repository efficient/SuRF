#include "bench.hpp"
#include "filter_factory.hpp"

//#define VERBOSE 1

static std::vector<std::string> txn_keys;
static std::vector<std::string> upper_bound_keys;

typedef struct ThreadArg {
    int thread_id;
    bench::Filter* filter;
    int start_pos;
    int end_pos;
    int query_type;
    int64_t out_positives;
    double tput;
} ThreadArg;

void* execute_workload(void* arg) {
    ThreadArg* thread_arg = (ThreadArg*)arg;
    int64_t positives = 0;
    double start_time = bench::getNow();
    if (thread_arg->query_type == 0) { // point
	for (int i = thread_arg->start_pos; i < thread_arg->end_pos; i++)
	    positives += (int)thread_arg->filter->lookup(txn_keys[i]);
    } else { // range
	for (int i = thread_arg->start_pos; i < thread_arg->end_pos; i++)
	    positives += (int)thread_arg->filter->lookupRange(txn_keys[i], 
							      upper_bound_keys[i]);
    }
    double end_time = bench::getNow();
    double tput = (thread_arg->end_pos - thread_arg->start_pos) / (end_time - start_time) / 1000000; // Mops/sec

#ifdef VERBOSE
    std::cout << "Thread #" << thread_arg->thread_id << bench::kGreen 
    	      << ": Throughput = " << bench::kNoColor << tput << "\n";
#else
    std::cout << tput << "\n";
#endif

    thread_arg->out_positives = positives;
    thread_arg->tput = tput;
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 10) {
	std::cout << "Usage:\n";
	std::cout << "1. filter type: SuRF, SuRFHash, SuRFReal, Bloom\n";
	std::cout << "2. suffix length: 0 < len <= 64 (for SuRFHash and SuRFReal only)\n";
	std::cout << "3. workload type: mixed, alterByte (only for email key)\n";
	std::cout << "4. percentage of keys inserted: 0 < num <= 100\n";
	std::cout << "5. byte position (conting from last, only for alterByte): num\n";
	std::cout << "6. key type: randint, email\n";
	std::cout << "7. query type: point, range\n";
	std::cout << "8. distribution: uniform, zipfian, latest\n";
	std::cout << "9. number of threads\n";
	return -1;
    }

    std::string filter_type = argv[1];
    uint32_t suffix_len = (uint32_t)atoi(argv[2]);
    std::string workload_type = argv[3];
    unsigned percent = atoi(argv[4]);
    unsigned byte_pos = atoi(argv[5]);
    std::string key_type = argv[6];
    std::string query_type = argv[7];
    std::string distribution = argv[8];
    int num_threads = atoi(argv[9]);

    // check args ====================================================
    if (filter_type.compare(std::string("SuRF")) != 0
	&& filter_type.compare(std::string("SuRFHash")) != 0
	&& filter_type.compare(std::string("SuRFReal")) != 0
	&& filter_type.compare(std::string("Bloom")) != 0
	&& filter_type.compare(std::string("ARF")) != 0) {
	std::cout << bench::kRed << "WRONG filter type\n" << bench::kNoColor;
	return -1;
    }

    if (suffix_len == 0 || suffix_len > 64) {
	std::cout << bench::kRed << "WRONG suffix length\n" << bench::kNoColor;
	return -1;
    }

    if (workload_type.compare(std::string("mixed")) != 0
	&& workload_type.compare(std::string("alterByte")) == 0) {
	std::cout << bench::kRed << "WRONG workload type\n" << bench::kNoColor;
	return -1;
    }

    if (percent > 100) {
	std::cout << bench::kRed << "WRONG percentage\n" << bench::kNoColor;
	return -1;
    }

    if (key_type.compare(std::string("randint")) != 0
	&& key_type.compare(std::string("timestamp")) != 0
	&& key_type.compare(std::string("email")) != 0) {
	std::cout << bench::kRed << "WRONG key type\n" << bench::kNoColor;
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
    std::string load_file = "workloads/load_";
    load_file += key_type;
    std::vector<std::string> load_keys;
    if (key_type.compare(std::string("email")) == 0)
	bench::loadKeysFromFile(load_file, false, load_keys);
    else
	bench::loadKeysFromFile(load_file, true, load_keys);

    std::string txn_file = "workloads/txn_";
    txn_file += key_type;
    txn_file += "_";
    txn_file += distribution;

    if (key_type.compare(std::string("email")) == 0)
	bench::loadKeysFromFile(txn_file, false, txn_keys);
    else
	bench::loadKeysFromFile(txn_file, true, txn_keys);

    std::vector<std::string> insert_keys;
    bench::selectKeysToInsert(percent, insert_keys, load_keys);

    if (workload_type.compare(std::string("alterByte")) == 0)
	bench::modifyKeyByte(txn_keys, byte_pos);

    // compute upperbound keys for range queries =================
    if (query_type.compare(std::string("range")) == 0) {
	for (int i = 0; i < (int)txn_keys.size(); i++)
	    upper_bound_keys.push_back(bench::getUpperBoundKey(key_type, txn_keys[i]));
    }

    // create filter ==============================================
    bench::Filter* filter = bench::FilterFactory::createFilter(filter_type, suffix_len, insert_keys);

#ifdef VERBOSE
    std::cout << bench::kGreen << "Memory = " << bench::kNoColor << filter->getMemoryUsage() << std::endl;
#endif

    // execute transactions =======================================
    pthread_t* threads = new pthread_t[num_threads];
    pthread_attr_t attr;
    // Initialize and set thread joinable
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    ThreadArg* thread_args = new ThreadArg[num_threads];
    int num_txns = (int)txn_keys.size();
    int num_txns_per_thread = num_txns / num_threads;
    for (int i = 0; i < num_threads; i++) {
	thread_args[i].thread_id = i;
	thread_args[i].filter = filter;
	thread_args[i].start_pos = num_txns_per_thread * i;
	thread_args[i].end_pos = num_txns_per_thread * (i + 1);
	if (query_type.compare(std::string("point")) == 0)
	    thread_args[i].query_type = 0;
	else
	    thread_args[i].query_type = 1;
	thread_args[i].out_positives = 0;
	thread_args[i].tput = 0;
    }

    for (int i = 0; i < num_threads; i++) {
	int rc = pthread_create(&threads[i], NULL, execute_workload, (void*)(&thread_args[i]));
	if (rc) {
	    std::cout << "Error: unable to create thread " << rc << std::endl;
	    exit(-1);
	}
    }

    // free attribute and wait for the other threads
    pthread_attr_destroy(&attr);
    for (int i = 0; i < num_threads; i++) {
	void* status;
	int rc = pthread_join(threads[i], &status);
	if (rc) {
	    std::cout << "Error:unable to join " << rc << endl;
	    exit(-1);
	}
    }

    double tput = 0;
    for (int i = 0; i < num_threads; i++) {
	tput += thread_args[i].tput;
    }

#ifdef VERBOSE
    std::cout << bench::kGreen << "Throughput = " << bench::kNoColor << tput << "\n";

    int positives = 0;
    for (int i = 0; i < num_threads; i++) {
	positives += (thread_args[i].out_positives);
    }

    // compute true positives ======================================
    std::map<std::string, bool> ht;
    for (int i = 0; i < (int)insert_keys.size(); i++)
	ht[insert_keys[i]] = true;

    int64_t true_positives = 0;
    std::map<std::string, bool>::iterator ht_iter;
    if (query_type.compare(std::string("point")) == 0) {
	for (int i = 0; i < (int)txn_keys.size(); i++) {
	    ht_iter = ht.find(txn_keys[i]);
	    true_positives += (ht_iter != ht.end());
	}
    } else if (query_type.compare(std::string("range")) == 0) {
	for (int i = 0; i < (int)txn_keys.size(); i++) {
	    ht_iter = ht.upper_bound(txn_keys[i]);
	    if (ht_iter != ht.end()) {
		std::string fetched_key = ht_iter->first;
		true_positives += (fetched_key.compare(upper_bound_keys[i]) < 0);
	    }
	}
    }
    int64_t false_positives = positives - true_positives;
    assert(false_positives >= 0);
    int64_t true_negatives = txn_keys.size() - true_positives;
    double fp_rate = 0;
    if (false_positives > 0)
	fp_rate = false_positives / (true_negatives + false_positives + 0.0);

    std::cout << "positives = " << positives << "\n";
    std::cout << "true positives = " << true_positives << "\n";
    std::cout << "false positives = " << false_positives << "\n";
    std::cout << "true negatives = " << true_negatives << "\n";
    std::cout << bench::kGreen << "False Positive Rate = " << bench::kNoColor << fp_rate << "\n";
#else
    std::cout << tput << "\n";
    std::cout << bench::kGreen << bench::kNoColor << "\n\n";
#endif

    delete[] threads;
    delete[] thread_args;

    pthread_exit(NULL);
    return 0;
}
