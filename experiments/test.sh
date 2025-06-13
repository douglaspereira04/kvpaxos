#!/bin/bash

#                    #requests  #initial keys    request file       #rate mean   #rate seed
#./single_100000      1000000      100000      ycsb_a_requests.txt     0        167227088

n_initial_keys=1000000
arrival_rate_seed=1672270886
snapshop_scan=(0 1)
workloads=(ycsb_a ycsb_d ycsb_e)
requests=(50000000 50000000 5000000)

mkdir -p output
for snapshop_scan_ in "${snapshop_scan[@]}"; do
    for ((i=0; i<${#workloads[@]}; i++)); do
        file_name=single_${snapshop_scan_}_w${workloads[$i]}.csv
        echo ./single ${requests[$i]} $n_initial_keys  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed $snapshop_scan_
        ./single ${requests[$i]} $n_initial_keys  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed $snapshop_scan_ > output/$file_name
        if [ $? -ne 0 ]; then
            echo "ERROR"
            break
        fi
        mv details.csv output/details_$file_name
        rm partition_output_*
    done;
done;
mkdir -p $1
cp -r output $1