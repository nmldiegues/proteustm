#!/bin/sh
name=$1

cd code;
make clean

rm Defines.common.mk
rm Makefile.flags
rm src/thread/thread.h
rm src/thread/thread_backend.cc
rm src/thread/tm.h
rm src/thread/rectm.h
