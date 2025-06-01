mkdir -p output
#                #requests  #initial keys    request file       #rate mean   #rate seed
./replica_1000000 10000000     1000000       ycsb_a_requests.txt         0       167227088 > output/ycsb_a.csv
cp details.csv output/details_ycsb_a.csv
./replica_1000000 10000000     1000000       ycsb_d_requests.txt         0       167227088 > output/ycsb_d.csv
cp details.csv output/details_ycsb_d.csv
./replica_1000000 1000000      100000        ycsb_e_requests.txt         0       167227088 > output/ycsb_e.csv
cp details.csv output/details_ycsb_e.csv

cp -r output /users/douglasp/output