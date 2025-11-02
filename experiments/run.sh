#!/bin/bash
source ./experiments.sh

_experiment_name=$1
node=$(get_node_id)
_methods=(METIS)
_partitions=(8)
_n_initial_keys=(1000000)
_queue_heads_distance=(0)
_imbalance_thresholds=(0.1 0.05)
_max_sucessive_imbalances=(10 100)
_arrival_rates=(0)
_reps=1
_versions=()
_imb_versions=()

_arrival_rate_seed=1672270886
#node=$((node+9))
case $node in

  0)
    _versions=(async)
    _queue_heads_distance=(0 100 10000 100000 50000000)
    _arrival_rates=(293000)
    _workloads=(ycsb_a)
    _parameters_file="async_ycsb_a_d_parameters.txt"
    ;;

  1)
    _versions=(async)
    _queue_heads_distance=(0 100 10000 100000 50000000)
    _arrival_rates=(464000)
    _workloads=(ycsb_d)
    _parameters_file="async_ycsb_a_d_parameters.txt"
    ;;

  2)
    _versions=(async)
    _queue_heads_distance=(0 100 10000 100000 50000000)
    _arrival_rates=(14000)
    _workloads=(ycsb_e)
    _parameters_file="async_ycsb_e_parameters.txt"
    ;;

  3)
    _versions=(old)
    _queue_heads_distance=(0 100 10000 100000 50000000)
    _arrival_rates=(293000)
    _workloads=(ycsb_a)
    _parameters_file="old_ycsb_a_d_parameters.txt"
    ;;

  4)
    _versions=(old)
    _queue_heads_distance=(0 100 10000 100000 50000000)
    _arrival_rates=(464000)
    _workloads=(ycsb_d)
    _parameters_file="old_ycsb_a_d_parameters.txt"
    ;;

  5)
    _versions=(old)
    _queue_heads_distance=(0 100 10000 100000 50000000)
    _arrival_rates=(14000)
    _workloads=(ycsb_e)
    _parameters_file="old_ycsb_e_parameters.txt"
    ;;

  6)
    _methods=(ROUND_ROBIN)
    _arrival_rates=(293000)
    _partitions=(1 8)
    _versions=(old)
    _workloads=(ycsb_a)
    _parameters_file="rr_parameters.txt"
    ;;

  7)
    _methods=(ROUND_ROBIN)
    _arrival_rates=(464000)
    _partitions=(1 8)
    _versions=(old)
    _workloads=(ycsb_d)
    _parameters_file="rr_parameters.txt"
    ;;

  8)
    _methods=(ROUND_ROBIN)
    _arrival_rates=(14000)
    _partitions=(1 8)
    _versions=(old)
    _workloads=(ycsb_e)
    _parameters_file="rr_parameters.txt"
    ;;

  *)
    echo -n "No Experiment"
    exit 1
    ;;
esac

experiments _methods _partitions _versions _imb_versions _workloads _n_initial_keys _arrival_rates _queue_heads_distance _imbalance_thresholds _max_sucessive_imbalances $_arrival_rate_seed $_parameters_file $_reps $_experiment_name