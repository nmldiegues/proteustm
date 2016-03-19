htm_retries=5
htm_capacity_abort_strategy=2
adaptivity_disabled=0
adaptivity_starting_mode=0
#phase duration in msec  for stmb7
change_workload= #"CHANGEWORKLOAD=-DCHANGEWORKLOAD WORKLOAD_PHASE_DURATION=-DWORKLOAD_PHASE_DURATION=10000"
#in usec
controller_sleep="CONTROLLER_SLEEP=-DCONTROLLER_SLEEP=4000000"
#workload tracking and its parameters
kpi_tracking="KPI_TRACKING=-DKPI_TRACKING"
kpi_tracking_parameters="SMOOTHING=-DSMOOTHING=0.0 CONSECUTIVE_THRESHOLD=-DCONSECUTIVE_THRESHOLD=1 ANOMALY_THRESHOLD=-DANOMALY_THRESHOLD=0.25"

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
cd $root_dir/benchmarks/tpcc-gcc/code
rm tpcc



# gcc flags used for both debug and opt builds
CXXFLAGS="-g -fgnu-tm -O2 -w -I$HOME/boost_1_57_0/ -L$HOME/boost_1_57_0/stage/lib/"
LDFLAGS="-lboost_system -mrtm -pthread  -L../../../stms/norec/ -L../../../stms/tinystm/lib -L../../../stms/tl2/ -L../../../stms/swisstm/lib/ -L../../../rapl-power/ -L ../../../backends/greentm/gcc-abi/ -litm -lstm -lnorec -ltl2 -lwlpdstm -lm -ltcmalloc -lrapl -pthread"

#g++ $CXXFLAGS btree_test.cc stupidunit.cc -o btree_test $LDFLAGS -lstdc++
#g++ $CXXFLAGS randomgenerator_test.cc randomgenerator.cc stupidunit.cc -o randomgenerator_test $LDFLAGS -lstdc++
#g++ $CXXFLAGS tpccclient_test.cc tpccclient.cc randomgenerator.cc stupidunit.cc -o tpccclient_test $LDFLAGS -lstdc++
#g++ $CXXFLAGS tpcctables_test.cc tpcctables.cc tpccdb.cc randomgenerator.cc stupidunit.cc -o tpcctables_test $LDFLAGS -lstdc++
#g++ $CXXFLAGS tpccgenerator_test.cc tpccgenerator.cc tpcctables.cc tpccdb.cc randomgenerator.cc stupidunit.cc -o tpccgenerator_test $LDFLAGS -lstdc++
g++ $CXXFLAGS memory.cc pair.cc list.cc hashtable.cc tpcc.cc tpccclient.cc tpccgenerator.cc tpcctables.cc tpccdb.cc clock.cc randomgenerator.cc stupidunit.cc -o tpcc $LDFLAGS -lstdc++
