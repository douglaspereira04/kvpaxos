#!/bin/bash
source ./experiments.sh

_methods=(METIS)
_partitions=(8)
_versions=(non_stop)
_workloads=(ycsb_e)
_n_initial_keys=(1000000)
_parameters_file="non_stop_ycsb_e_parameters.txt"
_reps=1
_arrival_rate=0
_arrival_rate_seed=1672270886
_arrival_rate_inc_interval=0
_arrival_rate_inc=0

experiments _methods _partitions _versions _workloads _n_initial_keys $_arrival_rate $_arrival_rate_seed $_arrival_rate_inc_interval $_arrival_rate_inc $_parameters_file $_reps
