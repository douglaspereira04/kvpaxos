#!/bin/bash

./compile.sh
    mkdir -p build/bin
mv ./build/src/main ./build/bin/single
cp -r experiments/test.sh build/bin/