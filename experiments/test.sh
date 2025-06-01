mkdir -p output
#                #requests  #initial keys    request file       #rate mean   #rate seed
./single_1000000 10000000     1000000       ycsb_a_requests.txt         0       167227088 > output/ycsb_a_single_1000000.csv
cp details.csv output/details_ycsb_a_single_1000000.csv
./single_1000000 10000000     1000000       ycsb_d_requests.txt         0       167227088 > output/ycsb_d_single_1000000.csv
cp details.csv output/details_ycsb_d_single_1000000.csv
./single_1000000 1000000      100000        ycsb_e_requests.txt         0       167227088 > output/ycsb_e_single_1000000.csv
cp details.csv output/details_ycsb_e_single_1000000.csv

cp -r output /users/douglasp/single_thread_kv_1000000/