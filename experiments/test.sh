mkdir -p output

#                      #requests  #partitions   #initial keys   interval in us   method      request file    #rate mean   #rate seed   #dh   #threshold
./rep_100000_100000_100 1000000        16           1000000         10000         METIS   ycsb_a_requests.txt     0        167227088  100000     0.20 > output/ycsb_a.csv
cp details.csv output/details_ycsb_a.csv
./rep_100000_100000_100 1000000        16           1000000         10000         METIS   ycsb_a_requests.txt     0        167227088  100000     0.20 > output/ycsb_d.csv
cp details.csv output/details_ycsb_d.csv
./rep_100000_100000_100 1000000        16           1000000         10000         METIS   ycsb_a_requests.txt     0        167227088  100000     0.20 > output/ycsb_e.csv
cp details.csv output/details_ycsb_e_rep_100000_100000_100.csv