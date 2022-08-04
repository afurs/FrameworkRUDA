#!/bin/bash
# function to get full path from middle of run
getFullPathMiddle() {
  FULL_PATH_TO_RAW=$PATH_TO_RAW
  local LIST_OF_SUBDIRS=$(alien_ls ${1} | grep -E -x '[0-9]{4}/')
  local N_DIRS=$(echo "${LIST_OF_SUBDIRS}" | wc -l )
  echo "Number of dirs: ${N_DIRS}"
  let EL=$N_DIRS/2
  local ARR_OF_SUBDIRS=($LIST_OF_SUBDIRS})
  FULL_PATH_TO_RAW+="/${ARR_OF_SUBDIRS[$EL]}"
}

MAIN_PATH="/alice/data"
YEAR=2022
N_FILES=50
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
#  getFullPathMiddle $PATH_TO_RAW
fi

INPUT_FILELIST="list${RUNNUM}.txt"
COMMAND_FILEPTHS="alien_find -l ${N_FILES} -f ${FULL_PATH_TO_RAW} _ctf_ | sed 's/^/alien:\/\//'  > ${INPUT_FILELIST}"
COMMAND_CTF2DIGITS="o2-ctf-reader-workflow --ctf-input ${INPUT_FILELIST} --onlyDet FT0 --copy-cmd no-copy --ctf-dict ccdb -b |
 o2-ft0-digits-writer-workflow --disable-mc -b --outfile o2_ft0digits_${RUNNUM}.root"

echo "Full path: ${FULL_PATH_TO_RAW}"
echo "Command for collecting file paths: ${COMMAND_FILEPTHS}"
echo "Command for CTF2Digit conversion: ${COMMAND_CTF2DIGITS}"
eval $COMMAND_FILEPTHS
eval $COMMAND_CTF2DIGITS