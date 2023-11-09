#!/bin/bash

./compile.sh FELDMAN OLD OLD_GRAPH
mv ./build/bin/replica ./build/bin/old

./compile.sh FELDMAN FREE OLD_GRAPH
mv ./build/bin/replica ./build/bin/free

./compile.sh FELDMAN NON_STOP OLD_GRAPH
mv ./build/bin/replica ./build/bin/non_stop

cp -r experiments/* build/bin/