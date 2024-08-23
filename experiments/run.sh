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
    ./batch_ycsb_a.sh
    ;;

  4)
    ./batch_ycsb_d.sh
    ;;

  5)
    ./batch_ycsb_e.sh
    ;;

  6)
    ./non_stop_ycsb_a.sh
    ;;

  7)
    ./non_stop_ycsb_d.sh
    ;;

  8)
    ./non_stop_ycsb_e.sh
    ;;

  9)
    ./old_ycsb_a.sh
    ;;

  10)
    ./old_ycsb_d.sh
    ;;

  11)
    ./old_ycsb_e.sh
    ;;

  12)
    ./rr_ycsb_a.sh
    ;;

  13)
    ./rr_ycsb_d.sh
    ;;

  14)
    ./rr_ycsb_e.sh
    ;;

  *)
    echo -n "No Experiment"
    ;;
esac
