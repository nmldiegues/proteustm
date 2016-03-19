#!/bin/sh
FOLDERS="skiplist linkedlist redblacktree hashmap"

rm lib/*.o || true

for F in $FOLDERS
do
    cd $F
    rm *.o || true
    rm $F
    cd ..
done
