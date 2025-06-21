#!/bin/bash

#./async_imb_0_0 configs/config.toml 2 100000 1000000 METIS ycsb_a_100000_requests.txt 0 167227088 1000 0.05
q_size=(0 100000)


mkdir -p build/bin
for q_size_ in "${q_size[@]}"; do
    ./compile.sh ON ${q_size_}
    mv ./build/src/main ./build/bin/rep_${q_size_}
done;

./compile.sh OFF 0 0
mv ./build/src/main ./build/bin/stat

cp -r experiments/test.sh build/bin/