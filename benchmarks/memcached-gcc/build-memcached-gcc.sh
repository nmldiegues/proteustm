#!/bin/sh

htm_retries=5
htm_capacity_abort_strategy=2
adaptivity_disabled=0
adaptivity_starting_mode=0
#phase duration in msec  for stmb7
change_workload= #"CHANGEWORKLOAD=-DCHANGEWORKLOAD WORKLOAD_PHASE_DURATION=-DWORKLOAD_PHASE_DURATION=10000"
#in usec
controller_sleep= #"CONTROLLER_SLEEP=-DCONTROLLER_SLEEP=2000000"
#workload tracking and its parameters
kpi_tracking= #"KPI_TRACKING=-DKPI_TRACKING"
kpi_tracking_parameters= #"SMOOTHING=-DSMOOTHING=0.1 CONSECUTIVE_THRESHOLD=-DCONSECUTIVE_THRESHOLD=1 ANOMALY_THRESHOLD=-DANOMALY_THRESHOLD=0.1"

if [ $# -eq 4 ] ; then
    htm_retries=$1 # e.g.: 5
    htm_capacity_abort_strategy=$2 # e.g.: 0 for "give up"
    adaptivity_disabled=$3 # e.g.: 1 to disable
    adaptivity_starting_mode=$4 # e.g.: 3 for SwissTM
fi

cd code;
root_dir="../../.."
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
cd $root_dir/benchmarks/memcached-gcc/code
rm memcached

gcc -std=gnu99 -DHAVE_CONFIG_H -I. -DNDEBUG -g -O2 -pthread -mrtm -pthread hash.c slabs.c items.c assoc.c thread.c daemon.c stats.c util.c cache.c memcached.c -o memcached -lgcov  -levent -lrt -lm -fgnu-tm -L $root_dir/backends/greentm/gcc-abi/ -litm -I$root_dir/../include/ -L$root_dir/../lib/ -L$root_dir/stms/norec/ -L$root_dir/stms/tinystm/lib -L$root_dir/stms/tl2/ -L$root_dir/stms/swisstm/lib/ -L$root_dir/rapl-power/ -lrapl -lstm -lnorec -ltl2 -lwlpdstm -lm -ltcmalloc
