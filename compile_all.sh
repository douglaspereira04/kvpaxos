#!/bin/bash

engines=(TKRZW LMDB LEVEL_DB)

mkdir -p build/bin
for e_ in "${engines[@]}"; do
    ./compile.sh ${e_}
    mv ./build/src/main ./build/bin/single_${e_}
done;

cp -r experiments/test.sh build/bin/