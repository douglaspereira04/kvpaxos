#!/bin/bash

#                        #requests  #partitions   #initial keys   interval in us   method      request file    #rate mean   #rate seed   #dh
#./rep_100000_100000      1000000        2           100000         10000000         METIS   ycsb_a_requests.txt     0        167227088  100000

track_length=()
q_size=(0 100000)
n_initial_keys=1000000
arrival_rate_seed=1672270886
method=KAHIP
versions=($1)
partitions=($2)
callback=($3)
workloads=(ycsb_a ycsb_d ycsb_e)
deltat=(1000000 1000000 100000)
requests=(10000000 10000000 1000000)

mkdir -p output
rm -r /tmp/repart_kv_storage
for v_ in "${versions[@]}"; do
    for c_ in "${callback[@]}"; do
        for ((i=0; i<${#workloads[@]}; i++)); do
            if [ "${workloads[$i]}" = "ycsb_e" ]; then
                track_length=(0 100000)
            else
                track_length=(0 1000000)
            fi
            for p_ in "${partitions[@]}"; do
                if [ "$v_" = "rep" ]; then
                    for track_length_ in "${track_length[@]}"; do
                        for q_size_ in "${q_size[@]}"; do
                            file_name=${v_}_c${c_}_t${track_length_}_q${q_size_}_p${p_}_dt${deltat[$i]}_w${workloads[$i]}.csv
                            echo ./${v_}_${track_length_}_${q_size_} ${requests[$i]}  $p_  $n_initial_keys  ${deltat[$i]}  $method  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed  $track_length_ $c_
                            ./${v_}_${track_length_}_${q_size_} ${requests[$i]}  $p_  $n_initial_keys  ${deltat[$i]}  $method  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed  $track_length_ $c_ > output/$file_name
                            if [ $? -ne 0 ]; then
                                echo "ERROR"
                            fi
                            mv details.csv output/details_$file_name
                            rm partition_output_*
                            rm -r /tmp/repart_kv_storage
                        done;
                    done;
                else
                    file_name=${v_}_c${c_}_p${p_}_w${workloads[$i]}.csv
                    echo ./${v_} ${requests[$i]}  $p_  $n_initial_keys  0  $method  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed  0 $c_
                    ./${v_} ${requests[$i]}  $p_  $n_initial_keys  0  $method  ${workloads[$i]}_requests.txt  0  $arrival_rate_seed  0 $c_ > output/$file_name
                    if [ $? -ne 0 ]; then
                        echo "ERROR"
                    fi
                    mv details.csv output/details_$file_name
                    rm partition_output_*
                    rm -r /tmp/repart_kv_storage
                fi

            done;
        done;
    done;
done;
mkdir -p $4
cp -r output $4
