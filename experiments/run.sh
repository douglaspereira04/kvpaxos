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

  9)
    ./async_ycsb_a.sh pt2
    ;;

  10)
    ./async_ycsb_d.sh pt2
    ;;

  11)
    ./async_ycsb_e.sh pt2
    ;;

  12)
    ./old_ycsb_a.sh pt2
    ;;

  13)
    ./old_ycsb_d.sh pt2
    ;;

  14)
    ./old_ycsb_e.sh pt2
    ;;

  15)
    ./rr_ycsb_a.sh pt2
    ;;

  16)
    ./rr_ycsb_d.sh pt2
    ;;

  17)
    ./rr_ycsb_e.sh pt2
    ;;

  *)
    echo -n "No Experiment"
    ;;
esac
