mkdir ../workloads
curl -O --location https://github.com/brianfrankcooper/YCSB/releases/download/0.12.0/ycsb-0.12.0.tar.gz
tar xfvz ycsb-0.12.0.tar.gz
rm ycsb-0.12.0.tar.gz
mv ycsb-0.12.0 YCSB
