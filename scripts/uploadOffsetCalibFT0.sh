#!/bin/bash

#####INPUT VARS########
CALIB_OBJECT_NAME="calibChannelTimeOffset"
PATH_CCDB="/FT0/Calib/ChannelTimeOffsetTest"
SERVER_CCDB="ccdb-test.cern.ch:8080"
CALIB_RUN_LIST="calibRunList.csv"
CALIB_RUN_LIST_DELIMETER=";"
CALIB_FILE_MASK="calibFT0_run%i.root"
SLEEP_SECONDS=5 # sleep between uploadings, for decreasing request rate, if zero then no sleeping
META_INFO_AUTHOR="Artur Furs"
#######################
UPLOAD_COMMAND_BASE="o2-ccdb-upload --key ${CALIB_OBJECT_NAME} --path ${PATH_CCDB} --host ${SERVER_CCDB}"
#######################

RUNNUM_RECORD=( $(tail -n +2 ${CALIB_RUN_LIST} | cut -d "${CALIB_RUN_LIST_DELIMETER}" -f1) )
SOR_RECORD=( $(tail -n +2 ${CALIB_RUN_LIST} | cut -d "${CALIB_RUN_LIST_DELIMETER}" -f2) )
EOR_RECORD=( $(tail -n +2 ${CALIB_RUN_LIST} | cut -d "${CALIB_RUN_LIST_DELIMETER}" -f3) )
echo "Runnums : ${RUNNUM_RECORD[@]}"
echo "SORs : ${SOR_RECORD[@]}"
echo "EORs : ${EOR_RECORD[@]}"
N_ENTRIES=${#RUNNUM_RECORD[@]}
let LAST_ELEMENT=$((N_ENTRIES-1))

for index in $(seq 0 $LAST_ELEMENT); do
  runnum=${RUNNUM_RECORD[index]}
  sor=${SOR_RECORD[index]}
  eor=${EOR_RECORD[index]}

  filename=( $(printf "${CALIB_FILE_MASK}" "${runnum}") )
  upload_command=$UPLOAD_COMMAND_BASE
  upload_command+=" --file ${filename} --starttimestamp ${sor} --endtimestamp ${eor} --meta \"runNumber=${runnum};Author=${META_INFO_AUTHOR}\""
  echo "Executing command: ${upload_command}"
  if (( $SLEEP_SECONDS>0 )); then
    sleep $SLEEP_SECONDS
  fi
  eval $upload_command
done