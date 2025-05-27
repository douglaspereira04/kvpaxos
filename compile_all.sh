#!/bin/bash

#./async_imb_0_0 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
q_size=(10000)

for q_size_ in "${q_size[@]}"; do
    ./compile.sh ${q_size_}
    mv ./build/bin/replica ./build/bin/rep_${q_size_}
done;

cp -r experiments/* build/bin/