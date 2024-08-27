#!/bin/bash
source ./experiments.sh

_methods=(METIS)
_partitions=(8)
_versions=(batch)
_workloads=(ycsb_e)
_n_initial_keys=(1000000)
_parameters_file="batch_ycsb_e_parameters.txt"
_reps=1
_arrival_rates=(15122 36115 57108)
_arrival_rate_seed=1672270886

experiments _methods _partitions _versions _workloads _n_initial_keys _arrival_rates $_arrival_rate_seed $_parameters_file $_reps