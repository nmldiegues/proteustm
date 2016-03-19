#!/bin/sh
FOLDERS="skiplist linkedlist redblacktree hashmap"

htm_retries=5
htm_capacity_abort_strategy=2
adaptivity_disabled=0
adaptivity_starting_mode=0
#in usec
controller_sleep="CONTROLLER_SLEEP=-DCONTROLLER_SLEEP=10000000"
#workload tracking and its parameters
kpi_tracking="KPI_TRACKING=-DKPI_TRACKING"
kpi_tracking_parameters="SMOOTHING=-DSMOOTHING=0.1 CONSECUTIVE_THRESHOLD=-DCONSECUTIVE_THRESHOLD=1 ANOMALY_THRESHOLD=-DANOMALY_THRESHOLD=0.1"

if [ $# -eq 4 ] ; then
    htm_retries=$1 # e.g.: 5
    htm_capacity_abort_strategy=$2 # e.g.: 0 for "give up"
    adaptivity_disabled=$3 # e.g.: 1 to disable
    adaptivity_starting_mode=$4 # e.g.: 3 for SwissTM
fi

root_dir="../../"
cd $root_dir/backends/greentm/gcc-abi/
make clean;
if [ $# -eq 1 ] ; then
    make -f Makefile ${change_workload} ${controller_sleep} ${kpi_tracking} ${kpi_tracking_parameters}
else
    make_command="make -f Makefile HTM_RETRIES=-DHTM_RETRIES=$htm_retries RETRY_POLICY=-DRETRY_POLICY=$htm_capacity_abort_strategy STARTING_MODE=-DSTARTING_MODE=$adaptivity_starting_mode ${change_workload} ${controller_sleep} ${kpi_tracking} ${kpi_tracking_parameters}"
    if [ $adaptivity_disabled -eq 1 ] ; then
        make_command="$make_command NO_ADAPTIVITY=-DNO_ADAPTIVITY=1"
    fi
    $make_command
fi
cd ../$root_dir/benchmarks/datastructures-gcc

rm lib/*.o || true

for F in $FOLDERS
do
    cd $F
    rm *.o || true
    rm $F
    make -f Makefile
    rc=$?
    if [[ $rc != 0 ]] ; then
        echo ""
        echo "=================================== ERROR BUILDING $F - $name ===================================="
        echo ""
        exit 1
    fi
    cd ..
done

