#!/bin/bash

MAIN_PATH="/alice/data"
YEAR=2022
N_FILES=100
OUTPUT_DIR="/data/work/run3/calibration/calib4/LHC22o"
if [ -z "$2" ]
then
  FULL_PATH_TO_RAW=$1
  RUNNUM=$(echo "${FULL_PATH_TO_RAW}" | grep -E -o '/[0-9]{6}/' | grep -E -o '[0-9]{6}')
  PERIOD=$(echo "${FULL_PATH_TO_RAW}" | grep -E -o '${MAIN_PATH}/${YEAR}/LHC[0-9]{2}[a-z]{1,2}/' | grep -E -o 'LHC[0-9]{2}[a-z]{1,2}')
else
  PERIOD=$1
  RUNNUM=$2
  PATH_TO_RAW="alien://${MAIN_PATH}/${YEAR}/${PERIOD}/${RUNNUM}/raw"
  FULL_PATH_TO_RAW=$PATH_TO_RAW
fi

INPUT_FILELIST="list${RUNNUM}"
COMMAND_DIRS_10MINS="alien_ls -c ${FULL_PATH_TO_RAW} | grep -o -E \".*raw/[0-9]{4}/\""
DIRS=( $( eval $COMMAND_DIRS_10MINS ) )
echo "Full path: ${FULL_PATH_TO_RAW}"

N_ENTRIES=${#DIRS[@]}
echo $N_ENTRIES
FIRST_ENTRY=""
FIRST_INDEX=0
SECOND_ENTRY=""
SECOND_INDEX=1
FIRST_IN_ARR_10MINS=""
RUN_DIR="run${RUNNUM}"
OUTPUT_PATH_RUN="${OUTPUT_DIR}/${RUN_DIR}"
if [[ ! -d "${RUN_DIR}" ]]; then
  echo "Creating directory ${OUTPUT_PATH_RUN}"
  mkdir -p $OUTPUT_PATH_RUN
fi
if [[ $N_ENTRIES -ge 2 ]]
then
  FIRST_IN_ARR_10MINS=( $(echo ${DIRS[0]} | grep -E -o "/raw/[0-9]{4}/" | grep -E -o "[0-9]{4}"))
  TMP=$((10#$FIRST_IN_ARR_10MINS))
  if [ $FIRST_IN_ARR_10MINS -eq "0000" ]; then
    for entry in ${!DIRS[@]}; do
      CURRENT_10MINS=( $(echo ${DIRS[$entry]} | grep -E -o "/raw/[0-9]{4}/" | grep -E -o "[0-9]{4}"))
      DIFF=$(( $((10#$CURRENT_10MINS))-$((10#$TMP)) ))
      if (( $DIFF>10 && $DIFF!=50 && $TMP%50!=0 )); then
        FIRST_INDEX=$entry
        SECOND_INDEX=$(( $FIRST_INDEX+1 ))
        if [[ CURRENT_10MINS -eq "2350" ]]; then
          SECOND_INDEX=0
        fi
        echo "INDEX ${FIRST_INDEX} ${SECOND_INDEX}"
      fi
      echo "Checking $DIFF ${CURRENT_10MINS} ${TMP}"
      TMP=$CURRENT_10MINS
    done
  fi
fi
FIRST_FILELIST=""


for entry in ${!DIRS[@]}; do
#  echo $entry
  DIR_PATH_GRID=${DIRS[$entry]}
  ENTRY_2D=( $(printf "%.2u" $entry))
  LIST_FILENAME="${INPUT_FILELIST}_${ENTRY_2D}.txt"
  # COMMAND_FILEPTHS="alien_find -l ${N_FILES} -f ${DIR_PATH_GRID} _ctf_ | sed 's/^/alien:\/\//'  > ${OUTPUT_PATH_RUN}/${LIST_FILENAME}"
  COMMAND_FILEPTHS="alien_ls -c ${DIR_PATH_GRID} | grep \"_ctf_\"| sed -n '10,110p' | sed 's/^/alien:\/\//'  > ${OUTPUT_PATH_RUN}/${LIST_FILENAME}"
  echo "Command for collecting file paths: ${COMMAND_FILEPTHS}"
  eval $COMMAND_FILEPTHS
done

for entry in ${!DIRS[@]}; do
  DIR_PATH_GRID=${DIRS[$entry]}
  ENTRY_2D=( $(printf "%.2u" $entry))
  LIST_FILENAME="${INPUT_FILELIST}_${ENTRY_2D}.txt"
  LOG_FILENAME="log${RUNNUM}_${ENTRY_2D}.log"
  OUTPUT_PATH_RUN="${OUTPUT_DIR}/${RUN_DIR}"
  COMMAND_CTF2DIGITS="o2-ctf-reader-workflow --ctf-input ${OUTPUT_PATH_RUN}/${LIST_FILENAME} --onlyDet FT0 --copy-cmd no-copy --ctf-dict ccdb -b"
  COMMAND_CTF2DIGITS+=" | o2-calibration-ft0-time-spectra-processor -b"
  COMMAND_CTF2DIGITS+=" | o2-calibration-ft0-time-offset-calib --tf-per-slot 53000 -b --configKeyValues \"FT0CalibParam.mRebinFactorPerChID[180]=4\""
  COMMAND_CTF2DIGITS+=" | o2-calibration-ccdb-populator-workflow -b --ccdb-path=\"file://${OUTPUT_PATH_RUN}\""
  if (( $entry == $FIRST_INDEX )); then
    FIRST_FILELIST="${LIST_FILENAME}"
    echo "First index ${entry}"
  fi
  if (( $entry == $SECOND_INDEX )); then
    echo "Second index ${entry}"
    eval "cat ${OUTPUT_PATH_RUN}/${FIRST_FILELIST} >> ${OUTPUT_PATH_RUN}/${LIST_FILENAME}"
  fi
  if (( $entry != $FIRST_INDEX && $N_ENTRIES>1 )) || (( $N_ENTRIES == 1 )); then
    echo "Other index ${entry}"
    echo "Command for CTF2Digit conversion: ${COMMAND_CTF2DIGITS}"
    eval "${COMMAND_CTF2DIGITS} > ${OUTPUT_PATH_RUN}/${LOG_FILENAME}"
    filenames_no_ext=( $(cat ${OUTPUT_PATH_RUN}/${LOG_FILENAME} | grep -E -o "Created local snapshot /.*.root"| grep -E -o "/.*[0-9]{1,}"))
    for entryFilename in ${!filenames_no_ext[@]}; do
      filename_old="${filenames_no_ext[$entryFilename]}.root"
      filename_new="${filenames_no_ext[$entryFilename]}_${ENTRY_2D}.root"
      mv $filename_old $filename_new
    done
  fi
done
