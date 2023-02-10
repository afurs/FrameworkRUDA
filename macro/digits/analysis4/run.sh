#!/bin/bash

NPARALLEL_JOBS=40
NCHUNCK_PER_RUN=1
PERIOD="LHC22o"

SRC_PATH_FDD="/data/work/run3/digits/production_all/d1/${PERIOD}"
SRC_PATH_FT0="/data/work/run3/digits/production_all/d2/${PERIOD}/"
SRC_PATH_FV0="/data/work/run3/digits/production_all/d3/${PERIOD}"

DST_PATH="histsFIT2/${PERIOD}"
MACRO_PATH="runAnalysisFullFIT.C"
root -b -l -x <<EOF
.L ${MACRO_PATH}+g
runAnalysisFull({"${SRC_PATH_FDD}","${SRC_PATH_FT0}","${SRC_PATH_FV0}"},"${DST_PATH}",${NPARALLEL_JOBS},${NCHUNCK_PER_RUN})
.q
EOF