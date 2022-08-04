#!/bin/bash

PATH_TO_SRC="production"
PATH_TO_RESULT="hists"
MASK_ROOT_FILES="o2_ft0digits_[0-9]{6}.root"
# ROOT_FILES=$(ls ${PATH_TO_SRC} | grep -E -x '${MASK_ROOT_FILES}')
ROOT_FILES=$(ls ${PATH_TO_SRC} | grep -E -x 'o2_ft0digits_[0-9]{6}.root')
for ROOT_FILE in $ROOT_FILES; do
  RUNNUM=$(echo $ROOT_FILE | grep -E -o '[0-9]{6}')
  if [[ $RUNNUM -gt 520099 ]]; then
    COMMAND="root -l -b -q -x processDigits.C+g\(${RUNNUM},\\\"${PATH_TO_SRC}/${ROOT_FILE}\\\",\\\"hists${RUNNUM}.root\\\"\)"
  #  echo $ROOT_FILE
    echo $COMMAND
    eval $COMMAND
  fi
done