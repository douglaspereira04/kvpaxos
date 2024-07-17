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
    arrival_rates=$6
    arrival_rate_seed=$7
    parameters_file=${8}
    reps=${9}

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
                                    output_dir="output"
                                    output_file="${arrival_rate}_${initial}_${w}_${m}_${p}_${v}_${window}_${queue}_${interval}"
                                    mkdir -p $output_dir
                                    echo ${w}_${m}_${initial}_${interval}_${v}_${p}_${i}_${window}_${queue}
                                    if [ ! -f "${output_dir}/${output_file}" ]; then
                                        echo ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed}
                                        ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} > ${output_dir}/${output_file}.csv
                                        mv details.csv ${output_dir}/details_${output_file}
                                        mkdir -p /users/douglasp/jul/output
                                        cp -r output /users/douglasp/jul/
                                    fi
                                done;
                            done;
                        done;
                    done < "$parameters_file";
                done;
            done;
        done;
    done;
}