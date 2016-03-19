#!/bin/sh


workspace="/home/ndiegues/tuner"

backend[1]="tsxbaseline"
backend[2]="tsxhle"
backend[3]="tsxaux"
backend[4]="tsxcombi"

benchmarks[1]="-w r -t false -m false"
benchmarks[2]="-w rw -t false -m false"
benchmarks[3]="-w w -t false -m false"
benchmarks[4]="-w r -t true -m false"
benchmarks[5]="-w rw -t true -m false"
benchmarks[6]="-w w -t true -m false"
benchmarks[7]="-w r -t false -m true"
benchmarks[8]="-w rw -t false -m true"
benchmarks[9]="-w w -t false -m true"
benchmarks[10]="-w r -t true -m true"
benchmarks[11]="-w rw -t true -m true"
benchmarks[12]="-w w -t true -m true"

bStr[1]="r-f-f"
bStr[2]="rw-f-f"
bStr[3]="w-f-f"
bStr[4]="r-t-f"
bStr[5]="rw-t-f"
bStr[6]="w-t-f"
bStr[7]="r-f-t"
bStr[8]="rw-f-t"
bStr[9]="w-f-t"
bStr[10]="r-t-t"
bStr[11]="rw-t-t"
bStr[12]="w-t-t"


wait_until_finish() {
    pid3=$1
    echo "process is $pid3"
    LIMIT=30
    for ((j = 0; j < $LIMIT; ++j)); do
        kill -s 0 $pid3
        rc=$?
        if [[ $rc != 0 ]] ; then
            echo "returning"
            return;
        fi
        sleep 1s
    done
    kill $pid3
}

for be in 1 2 3 4
do
for retries in 4 5 6 7 8 9 #{4..16}
do
for apriori in 1 #(( apriori=1; apriori < $retries; apriori++ ))
do
    for b in {1..3}
    do
        for t in {1..8}
        do
            cd $workspace;
            cd stmbench7;
            make clean; make STM=${backend[$be]} RETRIES=$retries APRIORI=$apriori PROFILE=fast
            for a in {1..3}
            do
                echo "${bStr[$b]} | backend ${backend[$be]} | htm_retries $retries | apriori tries $apriori | threads $t | attempt $a"
                ./sb7_tt -r false -s b -d 20000 ${benchmarks[$b]} -n $t > ../auto-results/${backend[$be]}-${bStr[$b]}-retries$retries-$apriori-th$t-att$a.data &
                pid3=$!; wait_until_finish $pid3; wait $pid3; rc=$?
                if [[ $rc != 0 ]] ; then
                    echo "Error within: backend ${backend[$be]} | ${bStr[$b]} | htm_retries $retries | apriori tries $apriori | threads $t | attempt $a" >> ../auto-results/error.out
                fi
           done
        done
    done
done
done
done
