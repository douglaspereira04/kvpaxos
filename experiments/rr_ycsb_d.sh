#!/bin/bash
source ./experiments.sh

_methods=(ROUND_ROBIN)
_partitions=(1 8)
_versions=(old)
_workloads=(ycsb_d)
_n_initial_keys=(1000000)
_parameters_file="rr_parameters.txt"
_reps=1
_arrival_rate=0
_arrival_rate_seed=1672270886
_arrival_rate_inc_interval=0
_arrival_rate_inc=0

experiments _methods _partitions _versions _workloads _n_initial_keys $_arrival_rate $_arrival_rate_seed $_arrival_rate_inc_interval $_arrival_rate_inc $_parameters_file $_reps
