#!/bin/bash
methods=(KAHIP METIS)
partitions=(8 4 2)
versions=(non_stop)
workloads=(ycsb_a ycsb_d ycsb_e)
n_initial_keys=(1000000 1000)
repartition_intervals=(0)
reps=1

for w in "${workloads[@]}"; do
	for initial in "${n_initial_keys[@]}"; do
		./old workloads/${w}_${initial}.toml
		mv requests.txt ${w}_${initial}_requests.txt
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
							if [ ! -f "output/${w}/${m}/${initial}_${deltap}_${v}_${p}_${i}.txt" ]; then
								./${v} configs/config.toml ${p} ${initial} ${deltap} ${m} ${w}_${initial}_requests.txt > output/${w}/${m}/${initial}_${deltap}_${v}_${p}_${i}.csv
								mv details.txt output/${w}/${m}/details_${initial}_${deltap}_${v}_${p}_${i}.txt
								cp -r output /users/douglasp/
							fi
						done;
					done;
				done;
			done;
		done;
	done;
done;
