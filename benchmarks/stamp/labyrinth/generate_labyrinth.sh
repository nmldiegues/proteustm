#!/bin/bash
INPUT_FLD=inputs
#Generate different parameters configuration for the vacation test
X_AXIS="4 8 16 32"   #size of x
Y_AXIS="4 8 16 32 64"  #size of y
Z_AXIS="3 5 7 10"  #size of z
NUM_JOINS="64 512 1024"  #Number of points


cd ${INPUT_FLD}
for x in ${X_AXIS}; do
   for y in ${Y_AXIS}; do
      for z in ${Z_AXIS};do
         for n in ${NUM_JOINS};do
         python generate.py ${x} ${y} ${z} ${n} > d-random-x${x}-y${y}-z${z}-n${n}.txt
         done #n
      done #z
   done #y
done #x
cd -


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

for x in ${X_AXIS}; do
   for y in ${Y_AXIS}; do
      for z in ${Z_AXIS};do
         for n in ${NUM_JOINS};do
         STRING="${PREFIX}[${TEST_INDEX}]=\"-i inputs/d-random-x${x}-y${y}-z${z}-n${n}.txt -t\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #n
      done #z
   done #y
done #x

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}


for x in ${X_AXIS}; do
   for y in ${Y_AXIS}; do
      for z in ${Z_AXIS};do
         for n in ${NUM_JOINS};do
         STRING="${PREFIX}[${TEST_INDEX}]=\"labyrinth\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #n
      done #z
   done #y
done #x

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}


for x in ${X_AXIS}; do
   for y in ${Y_AXIS}; do
      for z in ${Z_AXIS};do
         for n in ${NUM_JOINS};do
         STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/stamp/labyrinth\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #n
      done #z
   done #y
done #x



PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}

for x in ${X_AXIS}; do
   for y in ${Y_AXIS}; do
      for z in ${Z_AXIS};do
         for n in ${NUM_JOINS};do
         STRING="${PREFIX}[${TEST_INDEX}]=\"labyrinth_d_random_x${x}_y${y}_z${z}_n${n}\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #n
      done #z
   done #y
done #x

echo ${TEST_INDEX}