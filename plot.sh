#!/bin/bash
cmake .
make -j4

mkdir -p results/hashmap/all/dist-45-45-10/
mkdir -p results/hashmap/all/dist-90-5-5/
mkdir -p results/hashmap/all/dist-5-90-5/
mkdir -p results/hashmap/fine/dist-45-45-10/
mkdir -p results/hashmap/fine/dist-90-5-5/
mkdir -p results/hashmap/fine/dist-5-90-5/
mkdir -p results/queue/
mkdir -p results/zoom/

#./hashmap_plot
#./queue_plot
./kvpaxos_plot
