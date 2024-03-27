#!/bin/bash
source ./experiments.sh

node=$(get_node_id)

case $node in

  0)
    ./non_stop_ycsb_a.sh
    ;;

  1)
    ./non_stop_ycsb_d.sh
    ;;

  2)
    ./non_stop_ycsb_e.sh
    ;;

  3)
    ./old_ycsb_a.sh
    ;;

  4)
    ./old_ycsb_d.sh
    ;;

  5)
    ./old_ycsb_e.sh
    ;;

  6)
    ./rr_ycsb_a.sh
    ;;

  7)
    ./rr_ycsb_d.sh
    ;;

  8)
    ./rr_ycsb_e.sh
    ;;

  *)
    echo -n "No Experiment"
    ;;
esac
