#!/bin/bash
source ./experiments.sh

_methods=(ROUND_ROBIN)
_partitions=(1 8)
_versions=(old)
_workloads=(ycsb_a)
_n_initial_keys=(1000000)
_parameters_file="rr_parameters.txt"
_reps=1

experiments _methods _partitions _versions _workloads _n_initial_keys $_parameters_file $_reps
