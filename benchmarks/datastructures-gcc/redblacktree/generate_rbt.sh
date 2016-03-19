#!/bin/bash
#Generate different parameters configuration for the vacation test
#SIZES="16777216 268435456 4294967296"   #size of the rbt  2^24 28 32
UPDATE_RATES="2 5 10 50 90 95" #Update ops percentage
INIT_SIZES="2 4"  #Number of elements to insert before test, expressed as portion of the total range
RANGES="3 4 5 6" #Range of integer values inserted in set; has to be higher than init size



PREFIX="params"
if [ "$#" -ne 0 ]; then
   INIT_INDEX=$1
   OUT=$2
else
   INIT_INDEX="1"
   OUT="gen_bench_temp"
fi
TEST_INDEX=${INIT_INDEX}
STRING=""  # Default: "-n4 -q60 -u90 -r1048576 -t4194304 -c"

for s in ${INIT_SIZES}; do
   for rr in ${RANGES}; do
      for u in ${UPDATE_RATES}; do
         #s=$(echo "2^{i}" | bc)
         r=$(echo "10^${rr}" | bc)
         i=$(echo $(( ${r} / ${s} )))
         STRING="${PREFIX}[${TEST_INDEX}]=\"-d 30000000 -i ${i} -r ${r} -u ${u} -n\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
      done #u
   done #rr
done #i

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}

for s in ${INIT_SIZES}; do
   for rr in ${RANGES}; do
      for u in ${UPDATE_RATES}; do
         #s=$(echo "2^{i}" | bc)
         r=$(echo "10^${rr}" | bc)
         i=$(echo $(( ${r} / ${s} )))
         STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/datastructures/redblacktree\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
      done #u
   done #rr
done #i

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}

for s in ${INIT_SIZES}; do
   for rr in ${RANGES}; do
      for u in ${UPDATE_RATES}; do
         #s=$(echo "2^{i}" | bc)
         r=$(echo "10^${rr}" | bc)
         i=$(echo $(( ${r} / ${s} )))
         STRING="${PREFIX}[${TEST_INDEX}]=\"redblacktree\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
      done #u
   done #rr
done #i


PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}

for s in ${INIT_SIZES}; do
   for rr in ${RANGES}; do
      for u in ${UPDATE_RATES}; do
         #s=$(echo "2^{i}" | bc)
         r=$(echo "10^${rr}" | bc)
         i=$(echo $(( ${r} / ${s} )))
         STRING="${PREFIX}[${TEST_INDEX}]=\"rbt_${i}_${r}_${u}\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
      done #u
   done #rr
done #i


echo ${TEST_INDEX}