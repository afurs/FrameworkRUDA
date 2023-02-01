#!/bin/bash

#RUNNUM=520296
RUNNUM=528231
#RUNNUM=529418

NPARALLEL_JOBS=8
#NPARALLEL_JOBS=7
NCHUNCK_PER_RUN=15
#NCHUNCK_PER_RUN=2

SRC_PATH="/tzero/deliner/run3/digits/prod_test/run${RUNNUM}"
DST_PATH="hists${RUNNUM}"
MACRO_PATH="runAnalysisFull_Spectra.C"
root -b -l -x <<EOF
.L ${MACRO_PATH}+g
runAnalysisFull({"${SRC_PATH}"},"${DST_PATH}",${NPARALLEL_JOBS},${NCHUNCK_PER_RUN})
.q
EOF