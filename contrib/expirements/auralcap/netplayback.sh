#!/bin/bash

SCRIPT_PATH=/usr/local/share/monopticon/sound-filters.zeek

set -e
set -x

function run() {
  IFACE=$1
  P=$(shuf -i10000-99999 -n1)
  TMP_DIR=/tmp/aural-$P
  mkdir $TMP_DIR
  cd $TMP_DIR
  /opt/zeek/bin/zeek -i $IFACE -b $SCRIPT_PATH &
  capstats -I 1 -i $IFACE 2>&1 | grep --line-buffered -oP ' pkts=\K\d+(?= )' | auralcap $TMP_DIR
}

function usage() {
    echo "USAGE: net-playback <iface>"
}

# Check if the function exists (bash specific)
if [[ $# -eq 0 ]]
then
  usage
  exit 1
else
  if [ -z "$1" ]
  then
    echo "No arguments supplied"
    usage
    exit 1
  else
    run $1
  fi
fi
