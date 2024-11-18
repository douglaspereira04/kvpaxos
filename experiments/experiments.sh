#!/bin/bash

get_node_id () {
    hname="$(hostname)"
    IFS='.' read -r -a words <<< "$hname"
    node=${words[0]}
    node=${node//"node"/}
    echo $node
}

experiments () {
    local -n methods=$1
    local -n partitions=$2
    local -n versions=$3
    local -n workloads=$4
    local -n n_initial_keys=$5
    local -n arrival_rates=$6
    local -n q_heads_ds=$7
    local -n imbalance_thresholds=$8
    arrival_rate_seed=$9
    parameters_file=${10}
    reps=${11}


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
            for arrival_rate in "${arrival_rates[@]}"; do
                for p in "${partitions[@]}"; do
                    while read -r interval window queue; do
                        for m in "${methods[@]}"; do
                            for w in "${workloads[@]}"; do
                                for v in "${versions[@]}"; do
                                    for q_heads_d in "${q_heads_ds[@]}"; do
                                        for imbalance_threshold in "${imbalance_thresholds[@]}"; do
                                            output_dir="output"
                                            output_file="${arrival_rate}_${initial}_${w}_${m}_${p}_${v}_${window}_${queue}_${interval}_${q_heads_d}_${imbalance_threshold}"
                                            mkdir -p $output_dir
                                            echo ${output_file}
                                            if [ ! -f "${output_dir}/details_${output_file}" ]; then
                                                rm -f -- ${output_dir}/${output_file}.csv
                                                echo ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} ${q_heads_d} ${imbalance_threshold}
                                                ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} ${q_heads_d} ${imbalance_threshold} > ${output_dir}/${output_file}.csv
                                                mv details.csv ${output_dir}/details_${output_file}
                                                mkdir -p /users/douglasp/nov2/output
                                                cp -r output /users/douglasp/nov2/
                                            fi
                                        done;
                                    done;
                                done;
                            done;
                        done;
                    done < "$parameters_file";
                done;
            done;
        done;
    done;
}