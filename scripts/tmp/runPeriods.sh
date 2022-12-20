#!/bin/bash

PERIODS=("LHC22o" "LHC22m" "LHC22c" "LHC22e" "LHC22f" "LHC22h" "LHC22j" "LHC22n" "LHC22p" "LHC22q" "LHC22r")

for entry in ${PERIODS[@]}; do
  PERIOD=$entry
  SRC_PATH="/data/work/run3/digits/production_all/d2/${PERIOD}"
  DST_PATH="hists_spectra/${PERIOD}"
  echo "${PERIOD} ${SRC_PATH} ${DST_PATH}"

root -b -l -x <<EOF
.L runAnalysisFull_Spectra.C+g
runAnalysisFull("${SRC_PATH}","${DST_PATH}")
.q
EOF

done
