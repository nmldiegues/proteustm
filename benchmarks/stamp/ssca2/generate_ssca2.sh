#!/bin/bash
#Generate different parameters configuration for the vacation test
PROBLEM_SCALE="10 20 40"   #Problem [s]cale
INTER_CLIQUE_PERC="0.1 0.5 1.0"  #Probability [i]nter-clique
UNIDIRECTIONAL_PROB="0.1 0.5 1.0"  #Probability [u]nidirectional
#I *guess* these have to be related to the problem scale, so the values will be scale/(1,2,3,4)
MAX_PATH_LENGTH="1 2 4"  #Max path [l]ength
MAX_PARALLEL_EDGES="1 2 4"  #Max [p]arallel edges


PREFIX="params"
if [ "$#" -ne 0 ]; then
   INIT_INDEX=$1
   OUT=$2
else
   INIT_INDEX="1"
   OUT="gen_bench_temp"
fi
TEST_INDEX=${INIT_INDEX}
STRING=""  # Default: "-s20 -i1.0 -u1.0 -l3 -p3 -t"

for s in ${PROBLEM_SCALE}; do
   for i in ${INTER_CLIQUE_PERC}; do
      for u in ${UNIDIRECTIONAL_PROB}; do
         for l in ${MAX_PATH_LENGTH}; do
            for p in ${MAX_PARALLEL_EDGES};do
               actual_l=$(( s/l ))
               actual_p=$(( s/p ))
               STRING="${PREFIX}[${TEST_INDEX}]=\"-s${s} -i${i} -u${u} -l${actual_l} -p${actual_p} -t\""
               ((TEST_INDEX++))
               echo ${STRING}  >> ${OUT}
            done #p
         done #q
      done #r
   done #u
done #n

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}

for s in ${PROBLEM_SCALE}; do
   for i in ${INTER_CLIQUE_PERC}; do
      for u in ${UNIDIRECTIONAL_PROB}; do
         for l in ${MAX_PATH_LENGTH}; do
            for p in ${MAX_PARALLEL_EDGES};do
               STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/stamp/ssca2\""
               ((TEST_INDEX++))
               echo ${STRING}  >> ${OUT}
            done #p
         done #q
      done #r
   done #u
done #n

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}

for s in ${PROBLEM_SCALE}; do
   for i in ${INTER_CLIQUE_PERC}; do
      for u in ${UNIDIRECTIONAL_PROB}; do
         for l in ${MAX_PATH_LENGTH}; do
            for p in ${MAX_PARALLEL_EDGES};do
               STRING="${PREFIX}[${TEST_INDEX}]=\"ssca2\""
               ((TEST_INDEX++))
               echo ${STRING}  >> ${OUT}
            done #p
         done #q
      done #r
   done #u
done #n


PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}

for s in ${PROBLEM_SCALE}; do
   for i in ${INTER_CLIQUE_PERC}; do
      for u in ${UNIDIRECTIONAL_PROB}; do
         for l in ${MAX_PATH_LENGTH}; do
            for p in ${MAX_PARALLEL_EDGES};do
               actual_l=$(( s/l ))
               actual_p=$(( s/p ))
               STRING="${PREFIX}[${TEST_INDEX}]=\"ssca2_s${s}_i${i}_u${u}_l${actual_l}_p${actual_p}\""
               ((TEST_INDEX++))
               echo ${STRING}  >> ${OUT}
         done #p
         done #q
      done #r
   done #u
done #n

echo ${TEST_INDEX}