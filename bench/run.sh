#!bin/bash

echo 'Bloom Filter, random int, point queries'
../build/bench/workload Bloom 1 mixed 50 0 randint point zipfian

echo 'SuRF, random int, point queries'
../build/bench/workload SuRF 1 mixed 50 0 randint point zipfian

echo 'SuRFHash, 4-bit suffixes, random int, point queries'
../build/bench/workload SuRFHash 4 mixed 50 0 randint point zipfian

echo 'SuRFReal, 4-bit suffixes, random int, point queries'
../build/bench/workload SuRFReal 4 mixed 50 0 randint point zipfian

echo 'SuRFMixed, 2-bit hash suffixes and 2-bit real suffixes, random int, point queries'
../build/bench/workload SuRFMixed 2 mixed 50 0 randint mix zipfian


# echo 'Bloom Filter, email, point queries'
# ../build/bench/workload Bloom 1 mixed 50 0 email point zipfian

# echo 'SuRF, email, point queries'
# ../build/bench/workload SuRF 1 mixed 50 0 email point zipfian

# echo 'SuRFHash, 4-bit suffixes, email, point queries'
# ../build/bench/workload SuRFHash 4 mixed 50 0 email point zipfian

# echo 'SuRFReal, 4-bit suffixes, email, point queries'
# ../build/bench/workload SuRFReal 4 mixed 50 0 email point zipfian

# echo 'SuRFMixed, 2-bit hash suffixes and 2-bit real suffixes, email, point queries'
# ../build/bench/workload SuRFMixed 2 mixed 50 0 email mix zipfian


echo 'SuRFReal, 4-bit suffixes, random int, range queries'
../build/bench/workload SuRFReal 4 mixed 50 0 randint range zipfian

# echo 'SuRFReal, 4-bit suffixes, email, point queries'
# ../build/bench/workload SuRFReal 4 mixed 50 0 email range zipfian

