#!/bin/bash
source ./experiments.sh

_methods=(METIS)
_partitions=(8)
_versions=(non_stop)
_workloads=(ycsb_a)
_n_initial_keys=(1000000)
_parameters_file="non_stop_ycsb_a_d_parameters.txt"
_reps=1

experiments _methods _partitions _versions _workloads _n_initial_keys $_parameters_file $_reps
