#!/bin/bash

#./async_imb_0_10000 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
track_length=(0 1000 100000)
q_size=(0 1000 100000)
schedule_queue_size=50000000
schedulers=(OLD ASYNC ASYNC_IMB)


for track_length_ in "${track_length[@]}"; do
    for q_size_ in "${q_size[@]}"; do
        for scheduler in "${schedulers[@]}"; do
            ./compile.sh ${scheduler} ${track_length_} ${q_size_} ${schedule_queue_size}
            mv ./build/bin/replica ./build/bin/${scheduler,,}_${track_length_}_${q_size_}
        done;
    done;
done;

cp -r experiments/* build/bin/