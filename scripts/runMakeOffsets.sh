#!/bin/bash

PATH_TO_SRC="hists5"
MASK_ROOT_FILES="hists[0-9]{6}.root"

ROOT_FILES=$(ls ${PATH_TO_SRC} | grep -E -x "${MASK_ROOT_FILES}")
for ROOT_FILE in $ROOT_FILES; do
  RUNNUM=$(echo $ROOT_FILE | grep -E -o '[0-9]{6}')
  COMMAND="root -l -b -q -x makeTimeOffsets.C+g\(${RUNNUM},\\\"${PATH_TO_SRC}/${ROOT_FILE}\\\"\)"
  echo $COMMAND
  eval $COMMAND
done