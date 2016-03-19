#!/bin/bash
#Generate different parameters configuration for the intruder test
ATTACK_PERCENTAGES="10 25 50 75 90"   #Percentage of attacks
MAX_DATA_LENGHTS="64 128 256 512 1024 2048 4096"  #Lenghts of data
NUM_FLOWS="4096 32768 262144 2097152"  #Number of flows, 2^15,2^18,2^21,2^12
RANDOM_SEEDS="1"  #Random seed default


PREFIX="params"
if [ "$#" -ne 0 ]; then
   INIT_INDEX=$1
   OUT=$2
else
   INIT_INDEX="1"
   OUT="gen_bench_temp"
fi
TEST_INDEX=${INIT_INDEX}
STRING=""  # Default: "-a10 -l128 -n262144 -s1 -t"   262144=2^18

for a in ${ATTACK_PERCENTAGES}; do
   for l in ${MAX_DATA_LENGHTS}; do
      for n in ${NUM_FLOWS}; do
         for s in ${RANDOM_SEEDS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"-a${a} -l${l} -n${n} -s${s} -t\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #a
      done #l
   done #n
done #s

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}

for a in ${ATTACK_PERCENTAGES}; do
   for l in ${MAX_DATA_LENGHTS}; do
      for n in ${NUM_FLOWS}; do
         for s in ${RANDOM_SEEDS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/stamp/intruder\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #a
      done #l
   done #n
done #s

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}

for a in ${ATTACK_PERCENTAGES}; do
   for l in ${MAX_DATA_LENGHTS}; do
      for n in ${NUM_FLOWS}; do
         for s in ${RANDOM_SEEDS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"intruder\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #a
      done #l
   done #n
done #s


PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}

for a in ${ATTACK_PERCENTAGES}; do
   for l in ${MAX_DATA_LENGHTS}; do
      for n in ${NUM_FLOWS}; do
         for s in ${RANDOM_SEEDS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"intruder_a${a}_l${l}_n${n}_s${s}\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #a
      done #l
   done #n
done #s

echo ${TEST_INDEX}