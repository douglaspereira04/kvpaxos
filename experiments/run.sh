#!/bin/bash
source ./experiments.sh

node=$(get_node_id)
case $node in

  0)
    ./async_ycsb_a.sh $1
    ;;

  1)
    ./async_ycsb_d.sh $1
    ;;

  2)
    ./async_ycsb_e.sh $1
    ;;

  3)
    ./old_ycsb_a.sh $1
    ;;

  4)
    ./old_ycsb_d.sh $1
    ;;

  5)
    ./old_ycsb_e.sh $1
    ;;

  6)
    ./rr_ycsb_a.sh $1
    ;;

  7)
    ./rr_ycsb_d.sh $1
    ;;

  8)
    ./rr_ycsb_e.sh $1
    ;;

  *)
    echo -n "No Experiment"
    ;;
esac
