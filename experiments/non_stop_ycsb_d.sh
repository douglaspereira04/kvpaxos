#!/bin/bash
# ./non_stop_0_0 configs/config.toml 4 1000000 0 METIS ycsb_d_1000000_requests.txt
methods=(METIS KAHIP)
partitions=(8)
versions=(non_stop)
workloads=(ycsb_d)
n_initial_keys=(1000000)
deltap=(0)
track_length=(0 1000 10000 100000)
q_size=(0)
scenarios=3
reps=1

for w in "${workloads[@]}"; do
	for initial in "${n_initial_keys[@]}"; do
		if [ ! -f "${w}_${initial}_requests.txt" ]; then
			./${versions[0]}_0_0 workloads/${w}_${initial}.toml
			mv requests.txt ${w}_${initial}_requests.txt
		fi
	done;
done;

for i in $(seq $reps); do
	echo rep ${i}
	for initial in "${n_initial_keys[@]}"; do
		for p in "${partitions[@]}"; do
			for s in $(seq 0 $scenarios); do
				for m in "${methods[@]}"; do
					for w in "${workloads[@]}"; do
						for v in "${versions[@]}"; do
							mkdir -p output/${w}/${m}
							echo ${w}_${m}_${initial}_${deltap[s]}_${v}_${p}_${i}_${track_length[s]}_${q_size[s]}
							if [ ! -f "output/${w}/${m}/${track_length[s]}_${q_size[s]}_${initial}_${deltap[s]}_${v}_${p}_${i}.csv" ]; then
								./${v}_${track_length[s]}_${q_size[s]} configs/config.toml ${p} ${initial} ${deltap[s]} ${m} ${w}_${initial}_requests.txt > output/${w}/${m}/${track_length[s]}_${q_size[s]}_${initial}_${deltap[s]}_${v}_${p}_${i}.csv
								mv details.csv output/${w}/${m}/details_${track_length[s]}_${q_size[s]}_${initial}_${deltap[s]}_${v}_${p}_${i}.csv
								cp -r output /users/douglasp/jan_12/
							fi
						done;
					done;
				done;
			done;
		done;
	done;
done;
