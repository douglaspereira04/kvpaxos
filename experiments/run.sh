#!/bin/bash
source ./experiments.sh

node=$(get_node_id)
_methods=(METIS)
_partitions=(8)
_n_initial_keys=(1000000)
_queue_heads_distance=(10000)
_imbalance_thresholds=(0)
_reps=1

_arrival_rate_seed=1672270886

case $node in

  0)
    _versions=(async)
    _workloads=(ycsb_a)
    _parameters_file="async_ycsb_a_d_parameters.txt"
    ;;

  1)
    _versions=(async)
    _workloads=(ycsb_d)
    _parameters_file="async_ycsb_a_d_parameters.txt"
    ;;

  2)
    _versions=(async)
    _workloads=(ycsb_e)
    _parameters_file="async_ycsb_e_parameters.txt"
    ;;

  3)
    _versions=(async_imb)
    _workloads=(ycsb_a)
    _parameters_file="async_ycsb_a_d_parameters.txt"
    _imbalance_thresholds=(0.05 0.02)
    ;;

  4)
    _versions=(async_imb)
    _workloads=(ycsb_d)
    _parameters_file="async_ycsb_a_d_parameters.txt"
    _imbalance_thresholds=(0.05 0.02)
    ;;

  5)
    _versions=(async_imb)
    _workloads=(ycsb_e)
    _parameters_file="async_ycsb_e_parameters.txt"
    _imbalance_thresholds=(0.05 0.02)
    ;;

  6)
    _versions=(old)
    _workloads=(ycsb_a)
    _parameters_file="old_ycsb_a_d_parameters.txt"
    ;;

  7)
    _versions=(old)
    _workloads=(ycsb_d)
    _parameters_file="old_ycsb_a_d_parameters.txt"
    ;;

  8)
    _versions=(old)
    _workloads=(ycsb_e)
    _parameters_file="old_ycsb_e_parameters.txt"
    ;;

  9)
    _methods=(ROUND_ROBIN)
    _partitions=(1 8)
    _versions=(old)
    _workloads=(ycsb_a)
    _parameters_file="rr_parameters.txt"
    ;;

  10)
    _methods=(ROUND_ROBIN)
    _partitions=(1 8)
    _versions=(old)
    _workloads=(ycsb_d)
    _parameters_file="rr_parameters.txt"
    ;;

  11)
    _methods=(ROUND_ROBIN)
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


if [[ "${_workloads[0]}" == "ycsb_a" ]]; then
  if [ $1 = "pt0" ]; then
      _arrival_rates=(0 100000)
  elif [ $1 = "pt2" ]; then
      _arrival_rates=(200000 600000)
  elif [ $1 = "pt2" ]; then
      _arrival_rates=(300000 400000 500000)
  else
      _arrival_rates=(800000 1000000)
  fi
elif [[ "${_workloads[0]}" == "ycsb_d" ]]; then
  if [ $1 = "pt0" ]; then
      _arrival_rates=(0 300000)
  elif [ $1 = "pt1" ]; then
      _arrival_rates=(400000 800000)
  elif [ $1 = "pt2" ]; then
      _arrival_rates=(500000 600000 700000)
  else
      _arrival_rates=(1000000 1200000)
  fi
else
  if [ $1 = "pt0" ]; then
      _arrival_rates=(0 20000)
  elif [ $1 = "pt1" ]; then
      _arrival_rates=(30000 70000)
  elif [ $1 = "pt2" ]; then
      _arrival_rates=(40000 50000 60000)
  else
      _arrival_rates=(90000 110000)
  fi
fi

experiments _methods _partitions _versions _workloads _n_initial_keys _arrival_rates _queue_heads_distance _imbalance_thresholds $_arrival_rate_seed $_parameters_file $_reps