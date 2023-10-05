#!/bin/bash

./compile.sh TBB OLD OLD_GRAPH
mv ./build/bin/replica ./build/bin/old

./compile.sh TBB FREE OLD_GRAPH
mv ./build/bin/replica ./build/bin/free

./compile.sh TBB NON_STOP OLD_GRAPH
mv ./build/bin/replica ./build/bin/non_stop
