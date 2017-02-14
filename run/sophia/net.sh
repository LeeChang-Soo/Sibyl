#!/bin/bash

SCRIPT_PATH=${0%/*}
BIN_PATH=$SCRIPT_PATH/../../bin
RUN_PATH=$SCRIPT_PATH/../../run
SOPHIA_PY=$SCRITP_PATH/../../../Sophia/src/sophia.py
HOME_PATH=$SCRIPT_PATH/../../..

DATE=`date +%Y%m%d`
LOG_PATH=$HOME_PATH/log/$DATE

DEV=gpu0
TCP_ADDRESS=210.57.226.251
TCP_PORT=50505

rm $BIN_PATH/log/*

THEANO_FLAGS=floatX=float32,device=$DEV,lib.cnmem=1,print_active_device=False python -u $SOPHIA_PY &

$BIN_PATH/sophia $RUN_PATH/reward.config $RUN_PATH/reshaper_0.config $SCRIPT_PATH/workspace.list $TCP_ADDRESS $TCP_PORT 

mkdir -p $LOG_PATH
cp -a $BIN_PATH/state $LOG_PATH
cp -a $BIN_PATH/log $LOG_PATH
