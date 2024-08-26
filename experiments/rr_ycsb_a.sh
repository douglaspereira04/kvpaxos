#!/bin/bash
source ./experiments.sh

_methods=(ROUND_ROBIN)
_partitions=(1 8)
_versions=(old)
_workloads=(ycsb_a)
_n_initial_keys=(1000000)
_parameters_file="rr_parameters.txt"
_reps=1
_arrival_rates=(389695 423145 456595)
_arrival_rate_seed=1672270886

experiments _methods _partitions _versions _workloads _n_initial_keys $_arrival_rates $_arrival_rate_seed $_parameters_file $_reps