#!/bin/bash
workloads=(ycsb_a ycsb_d ycsb_e)
n_initial_keys=(1000000)

for w in "${workloads[@]}"; do
	for initial in "${n_initial_keys[@]}"; do
		if [ ! -f "${w}_${initial}_requests.txt" ]; then
			./non_stop_0_0 workloads/${w}_${initial}.toml
			mv requests.txt ${w}_${initial}_requests.txt
		fi
	done;
done;