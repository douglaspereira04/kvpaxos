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
    local -n imb_versions=$4
    local -n workloads=$5
    local -n n_initial_keys=$6
    local -n arrival_rates=$7
    local -n q_heads_ds=$8
    local -n imbalance_thresholds=$9
    local -n max_sucessive_imbalances=${10}
    arrival_rate_seed=${11}
    parameters_file=${12}
    reps=${13}
    experiment_name=${14}


    for w in "${workloads[@]}"; do
        for initial in "${n_initial_keys[@]}"; do
            while read -r interval window queue; do
                for v in "${versions[@]}"; do
                    if [ ! -f "${w}_${initial}_requests.txt" ]; then
                        ./${v}_${window}_${queue} workloads/${w}_${initial}.toml
                        mv requests.txt ${w}_${initial}_requests.txt
                    fi
                done;
                for v in "${imb_versions[@]}"; do
                    for max_sucessive_imbalance in "${max_sucessive_imbalances[@]}"; do
                        if [ ! -f "${w}_${initial}_requests.txt" ]; then
                            ./${v}_${window}_${queue}_${max_sucessive_imbalance} workloads/${w}_${initial}.toml
                            mv requests.txt ${w}_${initial}_requests.txt
                        fi
                    done;
                done;
            done < "$parameters_file";
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
                                for q_heads_d in "${q_heads_ds[@]}"; do
                                    for v in "${versions[@]}"; do
                                        output_dir="output"
                                        output_file="${arrival_rate}_${initial}_${w}_${m}_${p}_${v}_${window}_${queue}_${interval}_${q_heads_d}_0_0"
                                        mkdir -p $output_dir
                                        echo ${output_file}
                                        if [ ! -f "${output_dir}/details_${output_file}" ]; then
                                            rm -f -- ${output_dir}/${output_file}.csv
                                            echo ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} ${q_heads_d} 0
                                            ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} ${q_heads_d} 0 > ${output_dir}/${output_file}.csv
                                            mv details.csv ${output_dir}/details_${output_file}
                                            mkdir -p /users/douglasp/${experiment_name}/output
                                            cp -r output /users/douglasp/${experiment_name}/
                                        fi
                                    done;
                                    for v in "${imb_versions[@]}"; do
                                        for imbalance_threshold in "${imbalance_thresholds[@]}"; do
                                            for max_sucessive_imbalance in "${max_sucessive_imbalances[@]}"; do
                                                output_dir="output"
                                                output_file="${arrival_rate}_${initial}_${w}_${m}_${p}_${v}_${window}_${queue}_${interval}_${q_heads_d}_${imbalance_threshold}_${max_sucessive_imbalance}"
                                                mkdir -p $output_dir
                                                echo ${output_file}
                                                if [ ! -f "${output_dir}/details_${output_file}" ]; then
                                                    rm -f -- ${output_dir}/${output_file}.csv
                                                    echo ./${v}_${window}_${queue}_${max_sucessive_imbalance} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} ${q_heads_d} ${imbalance_threshold}
                                                    ./${v}_${window}_${queue}_${max_sucessive_imbalance} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt ${arrival_rate} ${arrival_rate_seed} ${q_heads_d} ${imbalance_threshold} > ${output_dir}/${output_file}.csv
                                                    mv details.csv ${output_dir}/details_${output_file}
                                                    mkdir -p /users/douglasp/${experiment_name}/output
                                                    cp -r output /users/douglasp/${experiment_name}/
                                                fi
                                            done;
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