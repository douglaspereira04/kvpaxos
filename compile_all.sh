#!/bin/bash

#./async_imb_0_0 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
track_length=(1000 10000 100000)
q_size=(1000 10000 100000)
schedule_queue_size=50000000

./compile.sh ASYNC 0 0 ${schedule_queue_size} 0
mv ./build/bin/replica ./build/bin/async_0_0

for track_length_ in "${track_length[@]}"; do
    ./compile.sh ASYNC ${track_length_} 0 ${schedule_queue_size} 0
    mv ./build/bin/replica ./build/bin/async_${track_length_}_0
done;


for q_size_ in "${q_size[@]}"; do
    ./compile.sh ASYNC 0 ${q_size_} ${schedule_queue_size} 0
    mv ./build/bin/replica ./build/bin/async_0_${q_size_}
done;


./compile.sh OLD 0 0 ${schedule_queue_size} 0
mv ./build/bin/replica ./build/bin/old_0_0


cp -r experiments/* build/bin/