#!/bin/bash

#RUNNUM=520296
#RUNNUM=528231
RUNNUM=529418

#NPARALLEL_JOBS=8
NPARALLEL_JOBS=7

#NCHUNCK_PER_RUN=15
#NCHUNCK_PER_RUN=2
NCHUNCK_PER_RUN=1


#SRC_PATH="/tzero/deliner/run3/digits/prod_test/run${RUNNUM}"
SRC_PATH1="/tzero/deliner/run3/digits/production_all/d2/LHC22m"
SRC_PATH2="/tzero/deliner/run3/digits/production_all/d2/LHC22j"
SRC_PATH3="/tzero/deliner/run3/digits/production_all/d2/LHC22e"


#DST_PATH="hists${RUNNUM}"
DST_PATH="hists"

MACRO_PATH="runAnalysisFull_Spectra.C"
root -b -l -x <<EOF
.L ${MACRO_PATH}+g
runAnalysisFull({"${SRC_PATH1}","${SRC_PATH2}","${SRC_PATH3}"},"${DST_PATH}",${NPARALLEL_JOBS},${NCHUNCK_PER_RUN},{519502,520543,520544,523559})
.q
EOF