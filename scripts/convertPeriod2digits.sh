#!/bin/bash

# RUNLIST_FILE="run3_list_LHC22m_2.csv"
RUNLIST_FILE=$1
DELIMETER=";"
COMMAND="./ctf2dig.sh"

PERIOD_RECORD=( $(tail -n +2 ${RUNLIST_FILE} | cut -d ';' -f1) )
FILL_RECORD=( $(tail -n +2 ${RUNLIST_FILE} | cut -d ';' -f2) )
RUNNUM_RECORD=( $(tail -n +2 ${RUNLIST_FILE} | cut -d ';' -f3) )
echo "Periods : ${PERIOD_RECORD[@]}"
echo "FIlls : ${FILL_RECORD[@]}"
echo "Runnums : ${RUNNUM_RECORD[@]}"
N_ENTRIES=${#PERIOD_RECORD[@]}
let LAST_ELEMENT=$((N_ENTRIES-1))
echo $LAST_ELEMENT

for entry in $(seq 0 $LAST_ELEMENT); do
#  if [[ ${PERIOD_RECORD[entry]} == $PERIOD ]]; then
  if [[ ${PERIOD_RECORD[entry]} != "LHC22a" ]]; then
    COMMAND_ENTRY="${COMMAND} ${PERIOD_RECORD[entry]} ${RUNNUM_RECORD[entry]}"
    echo ${COMMAND_ENTRY}
    eval $COMMAND_ENTRY
  else
    COMMAND_ENTRY="${COMMAND} /alice/data/2022/MAY/${RUNNUM_RECORD[entry]}/raw"
    echo ${COMMAND_ENTRY}
    eval $COMMAND_ENTRY
  fi
done