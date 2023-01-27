#!/bin/bash

RUNNUM=528231

LIST_SUFFIX_ROOT_FILES=("AmpA_AfterCollBC" "AmpC_AfterCollBC" "AmpA_InCollBC" "AmpC_InCollBC"
 "TimeA_AfterCollBC" "TimeC_AfterCollBC" "TimeA_InCollBC" "TimeC_InCollBC"
 "AmpA_Corr_Afterpulse" "AmpC_Corr_Afterpulse" "AmpA_Corr_Reflection" "AmpC_Corr_Reflection"
 "AmpVsTime_sideA" "AmpVsTime_sideC" "LowAmpVsTime_sideA" "LowAmpVsTime_sideC"
 "AmpA_BC" "AmpC_BC" "TimeA_BC" "TimeC_BC"
 "LowAmpVsTimeSingleMCP_sideA" "LowAmpVsTimeSingleMCP_sideC"
 "AmpLowA_AfterCollBC_inTrgGate" "AmpLowA_AfterCollBC_inTrgGate"
 "Triggers" "Spectra" "SpectraPerBC" "Events" )

SRC_PREFIX="hist_run${RUNNUM}_*_"
DST_PREFIX="hist${RUNNUM}_"
FILE_EXT=".root"
COMMAND_BASE="hadd"

for index in ${!LIST_SUFFIX_ROOT_FILES[@]}; do
  suffix=${LIST_SUFFIX_ROOT_FILES[$index]}
  SRC_FILENAME="${SRC_PREFIX}${suffix}${FILE_EXT}"
  DST_FILENAME="${DST_PREFIX}${suffix}${FILE_EXT}"
  FIND_SRC=$(find . -name "${SRC_FILENAME}")
  if test -n "${FIND_SRC}"
  then
    echo "Exist: ${SRC_FILENAME}"
    COMMAND="${COMMAND_BASE} ${DST_FILENAME} ${SRC_FILENAME}"
    echo "${COMMAND}"
    eval $COMMAND
  fi
done