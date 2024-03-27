#!/bin/bash
source ./experiments.sh

_methods=(METIS)
_partitions=(8)
_versions=(old)
_workloads=(ycsb_e)
_n_initial_keys=(1000000)
_parameters_file="old_ycsb_e_parameters.txt"
_reps=1

experiments _methods _partitions _versions _workloads _n_initial_keys $_parameters_file $_reps
