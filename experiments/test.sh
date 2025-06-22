#!/bin/bash

#                    #requests  #initial keys    request file       #rate mean   #rate seed
#./single_100000      1000000      100000      ycsb_a_requests.txt     0        167227088

arrival_rate_seed=1672270886
workloads=(ycsb_a ycsb_d ycsb_e)
version=$1
n_initial_keys=(1000000 1000000 100000)
requests=(10000000 10000000 100000)

mkdir -p output
for ((i=0; i<${#workloads[@]}; i++)); do
    file_name=single_${version}_w${workloads[$i]}.csv
    echo ./single_${version} ${requests[$i]} ${n_initial_keys[$i]}  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed
    ./single_$version ${requests[$i]} ${n_initial_keys[$i]}  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed > output/$file_name
    if [ $? -ne 0 ]; then
        echo "ERROR"
        break
    fi
    mv details.csv output/details_$file_name
    rm partition_output_*
done;
mkdir -p $2
cp -r output $2