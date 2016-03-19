#!/bin/bash
#Generate different parameters configuration for the hashmap test
DURATIONS="5000000"  #Pre-existent default value
INIT_SIZES="2 4"  #Number of elements to insert before test, expressed as portion of the total range
RANGES="3 4 5 6" #Range of integer values inserted in set; has to be higher than init size; expressed as power of 10
UPDATE_RATES="2 5 10 50 90 95" #Percentage of update transactions


PREFIX="params"

if [ "$#" -ne 0 ]; then
   INIT_INDEX=$1
   OUT=$2
else
   INIT_INDEX="1"
   OUT="gen_bench_temp"
fi

TEST_INDEX=${INIT_INDEX}
STRING=""  # Default: "-d 5000000 -i 10000 -r 1000000 -u 10 -n"

for d in ${DURATIONS}; do
   for s in ${INIT_SIZES}; do
      for rr in ${RANGES}; do
         for u in ${UPDATE_RATES}; do
            r=$(echo "10^${rr}" | bc)
            i=$(echo $(( ${r} / ${s} )))
            STRING="${PREFIX}[${TEST_INDEX}]=\"-d ${d} -i ${i} -r ${r} -u ${u} -n\""
            ((TEST_INDEX++))
            echo ${STRING}  >> ${OUT}
         done #u
      done #r
   done #i
done #d

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}

for d in ${DURATIONS}; do
   for s in ${INIT_SIZES}; do
      for rr in ${RANGES}; do
         for u in ${UPDATE_RATES}; do
            r=$(echo "10^${rr}" | bc)
            i=$(echo $(( ${r} / ${s} )))
            STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/datastructures/hashmap\""
            ((TEST_INDEX++))
            echo ${STRING}  >> ${OUT}
         done #u
      done #r
   done #i
done #d

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}

for d in ${DURATIONS}; do
   for s in ${INIT_SIZES}; do
      for rr in ${RANGES}; do
         for u in ${UPDATE_RATES}; do
            r=$(echo "10^${rr}" | bc)
            i=$(echo $(( ${r} / ${s} )))
            STRING="${PREFIX}[${TEST_INDEX}]=\"hashmap\""
            ((TEST_INDEX++))
            echo ${STRING}  >> ${OUT}
         done #u
      done #r
   done #i
done #d


PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}


for d in ${DURATIONS}; do
   for s in ${INIT_SIZES}; do
      for rr in ${RANGES}; do
         for u in ${UPDATE_RATES}; do
            r=$(echo "10^${rr}" | bc)
            i=$(echo $(( ${r} / ${s} )))
            STRING="${PREFIX}[${TEST_INDEX}]=\"hashmap_d${d}_i${i}_r${r}_u${u}\""
            ((TEST_INDEX++))
            echo ${STRING}  >> ${OUT}
         done #u
      done #r
   done #i
done #d

echo ${TEST_INDEX}