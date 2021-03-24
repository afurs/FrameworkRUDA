#!/bin/bash
for (( i=1; i <= 100; i++ ))
do
echo "Iteration $i"
echo "############STATISTICS############"
root -l -b -q $RUDA_FRAMEWORK_BUILD_PATH/macro/grid/gridPrint.C
echo "############FULL RESUBMIT############"
root -l -b -q $RUDA_FRAMEWORK_BUILD_PATH/macro/grid/gridFull.C
echo "Waiting... "
sleep 10m
done