#!/bin/bash

./compile.sh TBB OLD OLD_GRAPH
mv ./build/bin/replica ./build/bin/old

./compile.sh TBB FREE OLD_GRAPH
mv ./build/bin/replica ./build/bin/free

./compile.sh TBB NON_STOP OLD_GRAPH
mv ./build/bin/replica ./build/bin/non_stop

./compile.sh TBB NON_STOP_WINDOWED OLD_GRAPH
mv ./build/bin/replica ./build/bin/non_stop_windowed

./compile.sh TBB NON_STOP_WINDOWED_BOUNDED OLD_GRAPH
mv ./build/bin/replica ./build/bin/non_stop_windowed_bounded

cp -r experiments/* build/bin/