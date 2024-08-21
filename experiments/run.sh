#!/bin/bash
source ./experiments.sh

node=$(get_node_id)

case $node in

  0)
    ./async_ycsb_a.sh
    ;;

  1)
    ./async_ycsb_d.sh
    ;;

  2)
    ./async_ycsb_e.sh
    ;;

  3)
    ./non_stop_ycsb_a.sh
    ;;

  4)
    ./non_stop_ycsb_d.sh
    ;;

  5)
    ./non_stop_ycsb_e.sh
    ;;

  6)
    ./old_ycsb_a.sh
    ;;

  7)
    ./old_ycsb_d.sh
    ;;

  8)
    ./old_ycsb_e.sh
    ;;

  9)
    ./rr_ycsb_a.sh
    ;;

  10)
    ./rr_ycsb_d.sh
    ;;

  11)
    ./rr_ycsb_e.sh
    ;;

  *)
    echo -n "No Experiment"
    ;;
esac
