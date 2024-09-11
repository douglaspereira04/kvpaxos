#!/bin/bash

#./async_0_0 configs/config.toml 2 1000000 1000 METIS requests.txt 0 167227088
track_length=(0 1000 10000 100000 1000000 10000000)
q_size=(0 1000 10000 100000 1000000 10000000)
schedule_queue_size=50000000
schedulers=(OLD ASYNC)


for track_length_ in "${track_length[@]}"; do
    for q_size_ in "${q_size[@]}"; do
        for scheduler in "${schedulers[@]}"; do
            ./compile.sh ${scheduler} ${track_length_} ${q_size_} ${schedule_queue_size}
            mv ./build/bin/replica ./build/bin/${scheduler,,}_${track_length_}_${q_size_}
        done;
    done;
done;

cp -r experiments/* build/bin/