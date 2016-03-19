#!/bin/bash
#Generate different parameters configuration for the genome test
GENE_LENGTHS="1024 4096 16384 65536"      #Length of a gene, 10,12,14,16
MIN_SEGMENTS_NUMBERS="1048576 4194304 16777216 268435456"   #Min number of segments, 2^4,6,8,10
SEGMENT_LENGTHS="16 64 256 1024" #Lenght of segments, 2^20,22,24,28

PREFIX="params"
if [ "$#" -ne 0 ]; then
   INIT_INDEX=$1
   OUT=$2
else
   INIT_INDEX="1"
   OUT="gen_bench_temp"
fi
TEST_INDEX=${INIT_INDEX}
STRING=""  # Default: "g16384 -s64 -n16777216 -t"    ===> 2^14 2^6 2^24

for g in ${GENE_LENGTHS}; do
   for n in ${MIN_SEGMENTS_NUMBERS}; do
      for s in ${SEGMENT_LENGTHS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"-g${g} -s${s} -n${n} -t\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
      done #g
   done #n
done #s

PREFIX="folder"
TEST_INDEX=${INIT_INDEX}

for g in ${GENE_LENGTHS}; do
   for n in ${MIN_SEGMENTS_NUMBERS}; do
      for s in ${SEGMENT_LENGTHS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"benchmarks/stamp/genome\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #g
      done #n
done #s

PREFIX="benchmarks"
TEST_INDEX=${INIT_INDEX}

for g in ${GENE_LENGTHS}; do
   for n in ${MIN_SEGMENTS_NUMBERS}; do
      for s in ${SEGMENT_LENGTHS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"genome\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #g
      done #n
done #s


PREFIX="bStr"
TEST_INDEX=${INIT_INDEX}

for g in ${GENE_LENGTHS}; do
   for n in ${MIN_SEGMENTS_NUMBERS}; do
      for s in ${SEGMENT_LENGTHS}; do
         STRING="${PREFIX}[${TEST_INDEX}]=\"genome_g${g}_n${n}_s${s}\""
         ((TEST_INDEX++))
         echo ${STRING}  >> ${OUT}
         done #g
      done #n
   done #s


echo ${TEST_INDEX}