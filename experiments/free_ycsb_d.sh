#!/bin/bash
methods=(METIS)
partitions=(8 2)
versions=(free)
workloads=(ycsb_d)
n_initial_keys=(1000000)
repartition_intervals=(100000 500000 1000000 5000000 10000000)
reps=1

for w in "${workloads[@]}"; do
	for initial in "${n_initial_keys[@]}"; do
		if [ ! -f "${w}_${initial}_requests.txt" ]; then
			./old workloads/${w}_${initial}.toml
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
							mkdir -p output/${w}/${m}
							echo ${w}_${m}_${initial}_${deltap}_${v}_${p}_${i}
							if [ ! -f "output/${w}/${m}/${initial}_${deltap}_${v}_${p}_${i}.csv" ]; then
								./${v} configs/config.toml ${p} ${initial} ${deltap} ${m} ${w}_${initial}_requests.txt > output/${w}/${m}/${initial}_${deltap}_${v}_${p}_${i}.csv
								mv details.csv output/${w}/${m}/details_${initial}_${deltap}_${v}_${p}_${i}.csv
								cp -r output /users/douglasp/blocking/
							fi
						done;
					done;
				done;
			done;
		done;
	done;
done;
