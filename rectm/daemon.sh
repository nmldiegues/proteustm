#!/bin/bash

FLD=ProteusTM
BASE=$PWD
HOME=~
MAINPACKAGE=${BASE}/${FLD}
#Need java 7

if [ "${1}" != "-s" ]; then
export MAVEN_OPTS="-Xmx512m -XX:MaxPermSize=256m"
if [ $(uname) = "Darwin" ]; then
export JAVA_HOME="/Library/Java/JavaVirtualMachines/jdk1.7.0_51.jdk/Contents/Home"
fi
mvn  clean install -U
fi

ARGS="GAUSSIAN_BAG_KNN"

CONF=${MAINPACKAGE}/conf/
DATA=${MAINPACKAGE}/data/
TARGET=${MAINPACKAGE}/target
CP=${CONF}:${DATA}*
CP=$CP:${TARGET}/*
CP=$CP:${TARGET}/dependencies/*
EXEC=runtime.oracle.ProteusTMOracle
JVM_OPTS="-server -Xmx8G -Xms8G -XX:+UseParNewGC -XX:+UseConcMarkSweepGC"
time java ${JVM_OPTS} -cp $CP $EXEC ${ARGS}
