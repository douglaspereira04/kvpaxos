#!/bin/bash
source ./experiments.sh

_methods=(METIS)
_partitions=(16)
_versions=(non_stop)
_workloads=(ycsb_d)
_n_initial_keys=(1000000)
_parameters_file="non_stop_ycsb_a_d_parameters.txt"
_reps=1
_arrival_rates=(0)
_arrival_rate_seed=1672270886

experiments _methods _partitions _versions _workloads _n_initial_keys _arrival_rates $_arrival_rate_seed $_parameters_file $_reps