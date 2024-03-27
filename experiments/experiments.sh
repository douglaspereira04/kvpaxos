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
    parameters_file=$6
    reps=$7

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
                while read -r interval window queue; do
                    for m in "${methods[@]}"; do
                        for w in "${workloads[@]}"; do
                            for v in "${versions[@]}"; do
                                mkdir -p output/${w}/${m}
                                echo ${w}_${m}_${initial}_${interval}_${v}_${p}_${i}_${window}_${queue}
                                if [ ! -f "output/${w}/${m}/${window}_${queue}_${initial}_${interval}_${v}_${p}_${i}.csv" ]; then
                                    ./${v}_${window}_${queue} configs/config.toml ${p} ${initial} ${interval} ${m} ${w}_${initial}_requests.txt > output/${w}/${m}/${window}_${queue}_${initial}_${interval}_${v}_${p}_${i}.csv
                                    mv details.csv output/${w}/${m}/details_${window}_${queue}_${initial}_${interval}_${v}_${p}_${i}.csv
                                    cp -r output /users/douglasp/mar_27/
                                fi
                            done;
                        done;
                    done;
                done < "$parameters_file";
            done;
        done;
    done;
}