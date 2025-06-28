#!/bin/bash

#./async_imb_0_0 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
q_size=(0)
engines=(TKRZW)

mkdir -p build/bin
for e_ in "${engines[@]}"; do
    for q_size_ in "${q_size[@]}"; do
        ./compile.sh ${e_} ON ${q_size_}
        mv ./build/src/main ./build/bin/rep_${e_}_${q_size_}
    done;
done;

#for e_ in "${engines[@]}"; do
#    ./compile.sh ${e_} OFF 0
#    mv ./build/src/main ./build/bin/stat_${e_}
#done;

cp -r experiments/test.sh build/bin/