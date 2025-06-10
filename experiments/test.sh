#!/bin/bash

#                        #requests  #partitions   #initial keys   interval in us   method      request file    #rate mean   #rate seed   #dh
#./rep_100000_100000      1000000        2           100000         10000000         METIS   ycsb_a_requests.txt     0        167227088  100000

track_length=(0 100000)
q_size=(0 100000)
n_initial_keys=1000000
deltah=100000
arrival_rate_seed=1672270886
method=METIS
version=$1
partitions=($2)
workloads=(ycsb_a ycsb_d ycsb_e)
deltat=(1000000 1000000 100000)
requests=(50000000 50000000 5000000)

mkdir -p output
rm -r /tmp/repart_kv_storage
for ((i=0; i<${#workloads[@]}; i++)); do
    for p_ in "${partitions[@]}"; do
        if [ "$version" = "rep" ]; then
            for track_length_ in "${track_length[@]}"; do
                for q_size_ in "${q_size[@]}"; do
                    file_name=${version}_t${track_length_}_q${q_size_}_p${p_}_dt${deltat[$i]}_w${workloads[$i]}.csv
                    ./${version}_${track_length_}_${q_size_} ${requests[$i]}  $p_  $n_initial_keys  ${deltat[$i]}  $method  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed  $deltah > output/$file_name
                    if [ $? -ne 0 ]; then
                        echo "ERROR"
                    fi
                    mv details.csv output/details_$file_name
                    rm partition_output_*
                    rm -r /tmp/repart_kv_storage
                done;
            done;
        else
            file_name=${version}_p${p_}_w${workloads[$i]}.csv
            ./${version} ${requests[$i]}  $p_  $n_initial_keys  0  $method  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed  0 > output/$file_name
            if [ $? -ne 0 ]; then
                echo "ERROR"
            fi
            mv details.csv output/details_$file_name
            rm partition_output_*
            rm -r /tmp/repart_kv_storage
        fi

    done;
done;
mkdir -p $3
cp -r output $3
