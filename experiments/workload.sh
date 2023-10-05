#!/bin/bash
workloads=(ycsb_a ycsb_d ycsb_e)
n_initial_keys=(1000 1000000)

for w in "${workloads[@]}"; do
	for initial in "${n_initial_keys[@]}"; do
		./old workloads/${w}_${initial}.toml
		mv requests.txt ${w}_${initial}_requests.txt
	done;
done;
