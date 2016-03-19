#!/bin/bash
#Generate different parameters configuration for the vacation test
NUM_QUERIES_PER_TXS="4 8 16 32"   #Number of user queries/transaction
PERCENTAGE_QUERIED_RELATIONS="20 40 60 80"  #Percentage of relations queried
NUM_POX_RELATIONS="1024 1048576 1073741824"  #Number of possible relations, 2^10,20,30
NUM_TRANSACTIONS="4194304"  #Number of transactions
PERCENTAGE_USER_TRANSACTIONS="10 50 90"  #Percentage of user transactions


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

for n in ${NUM_QUERIES_PER_TXS}; do
   for u in ${PERCENTAGE_USER_TRANSACTIONS}; do
      for r in ${NUM_POX_RELATIONS}; do
         for q in ${PERCENTAGE_QUERIED_RELATIONS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"-n${n} -q${q} -u${u} -r${r} -t${NUM_TRANSACTIONS} -c\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #q
      done #r
   done #u
done #n

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}

for n in ${NUM_QUERIES_PER_TXS}; do
   for u in ${PERCENTAGE_USER_TRANSACTIONS}; do
      for r in ${NUM_POX_RELATIONS}; do
         for q in ${PERCENTAGE_QUERIED_RELATIONS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/stamp/vacation\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #q
      done #r
   done #u
done #n

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}

for n in ${NUM_QUERIES_PER_TXS}; do
   for u in ${PERCENTAGE_USER_TRANSACTIONS}; do
      for r in ${NUM_POX_RELATIONS}; do
         for q in ${PERCENTAGE_QUERIED_RELATIONS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"vacation\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #q
      done #r
   done #u
done #n


PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}

for n in ${NUM_QUERIES_PER_TXS}; do
   for u in ${PERCENTAGE_USER_TRANSACTIONS}; do
      for r in ${NUM_POX_RELATIONS}; do
         for q in ${PERCENTAGE_QUERIED_RELATIONS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"vacation_n${n}_q${q}_u${u}_r${r}_t${NUM_TRANSACTIONS}\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #q
      done #r
   done #u
done #n


echo ${TEST_INDEX}