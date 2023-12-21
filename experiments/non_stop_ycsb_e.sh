#!/bin/bash
methods=(METIS)
partitions=(8)
versions=(non_stop)
workloads=(ycsb_e)
n_initial_keys=(1000000)
repartition_intervals=(0)
track_length=(0 1000 10000)
q_size=(0 1000 10000)
reps=1

for w in "${workloads[@]}"; do
	for initial in "${n_initial_keys[@]}"; do
		if [ ! -f "${w}_${initial}_requests.txt" ]; then
			./${versions[0]}_${track_length[0]}_${q_size[0]} workloads/${w}_${initial}.toml
			mv requests.txt ${w}_${initial}_requests.txt
		fi
	done;
done;

for i in $(seq $reps); do
	echo rep ${i}
	for initial in "${n_initial_keys[@]}"; do
		for p in "${partitions[@]}"; do
			for deltap in "${repartition_intervals[@]}"; do
				for m in "${methods[@]}"; do
					for w in "${workloads[@]}"; do
						for v in "${versions[@]}"; do
							for track_length_ in "${track_length[@]}"; do
								for q_size_ in "${q_size[@]}"; do
									mkdir -p output/${w}/${m}
									echo ${w}_${m}_${initial}_${deltap}_${v}_${p}_${i}_${track_length_}_${q_size_}
									if [ ! -f "output/${w}/${m}/${track_length_}_${q_size_}_${initial}_${deltap}_${v}_${p}_${i}.csv" ]; then
										./${v}_${track_length_}_${q_size_} configs/config.toml ${p} ${initial} ${deltap} ${m} ${w}_${initial}_requests.txt > output/${w}/${m}/${track_length_}_${q_size_}_${initial}_${deltap}_${v}_${p}_${i}.csv
										mv details.csv output/${w}/${m}/details_${track_length_}_${q_size_}_${initial}_${deltap}_${v}_${p}_${i}.csv
										cp -r output /users/douglasp/dez_2/
									fi
								done;
							done;
						done;
					done;
				done;
			done;
		done;
	done;
done;
