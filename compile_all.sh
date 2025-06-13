#!/bin/bash

#./async_imb_0_0 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
track_length=(100000)
q_size=(100000)


for q_size_ in "${q_size[@]}"; do
    for track_length_ in "${track_length[@]}"; do
            ./compile.sh ON ${track_length_} ${q_size_}
            mkdir -p build/bin
            mv ./build/src/main ./build/bin/rep_${track_length_}_${q_size_}
    done;
done;

./compile.sh OFF 0 0
mkdir -p build/bin
mv ./build/src/main ./build/bin/stat

cp -r experiments/test.sh build/bin/