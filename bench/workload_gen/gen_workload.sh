#!bin/bash

python gen_load.py randint uniform
python gen_txn.py randint uniform
python gen_txn.py randint zipfian
python gen_txn.py randint latest

python poisson.py
python gen_load.py timestamp uniform
python gen_txn.py timestamp uniform
python gen_txn.py timestamp zipfian
python gen_txn.py timestamp latest

python gen_load.py email uniform
python gen_txn.py email uniform
python gen_txn.py email zipfian
python gen_txn.py email latest
