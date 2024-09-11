#!/bin/bash
source ./experiments.sh

_methods=(METIS)
_partitions=(8)
_versions=(async)
_workloads=(ycsb_a)
_n_initial_keys=(1000000)
_parameters_file="async_ycsb_a_d_parameters.txt"
_reps=1
if [ $1 = "pt2" ]; then
    _arrival_rates=(400000 500000 600000)
else
    _arrival_rates=(100000 200000 300000)
fi
_arrival_rate_seed=1672270886

experiments _methods _partitions _versions _workloads _n_initial_keys _arrival_rates $_arrival_rate_seed $_parameters_file $_reps
