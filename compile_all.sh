#!/bin/bash

#./async_imb_0_0 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
q_size=(0 100000)


for q_size_ in "${q_size[@]}"; do
        ./compile.sh ${q_size_} ${schedule_queue_size}
        mkdir -p build/bin
        mv ./build/src/replica ./build/bin/single_${q_size_}
done;

cp -r experiments/test.sh build/bin/