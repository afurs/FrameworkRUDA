#!/bin/bash

LDCONFIG_PATH="/etc/ld.so.conf.d/"
O2_SW_PATH="/home/flp/alice/sw"
RUDA_PATH="/home/flp/.local/share/RUDA"
ADDITIONAL_LIB_LINKS="/home/flp/alice/lib"
O2_LIB_CONFIG="o2-x86_64.conf"
ENV_WITH_O2="PATH="
LDCONFIGS=$(ls $LDCONFIG_PATH | grep -v "$O2_LIB_CONFIG")
echo $LDCONFIGS
> ldconfig_tmp.conf
for CONFIG_FILE in $LDCONFIGS;
do
  CONFIG_FILEPATH=$LDCONFIG_PATH
  CONFIG_FILEPATH+=$CONFIG_FILE
  echo $CONFIG_FILEPATH >> ldconfig_tmp.conf
  echo $CONFIG_FILEPATH
done
sudo ldconfig -f ldconfig_tmp.conf
PATH_BIN_O2=$(env | grep "^${ENV_WITH_O2}"| grep -o "[^${ENV_WITH_O2}]*")
echo $PATH_BIN_O2
NEW_PATH_TMP=""
for PATHS_BIN in $(echo $PATH_BIN_O2 | tr ":" "\n");
do

if [[ $PATHS_BIN != *"/o2/"* ]]; then
  NEW_PATH_TMP+=":"
  NEW_PATH_TMP+=$PATHS_BIN
fi

done
NEW_PATH=$(echo $NEW_PATH_TMP | grep -o '[^:].*')
export PATH="${NEW_PATH}"

unset ROOT_DYN_PATH
eval `alienv printenv O2/latest-dev_fit-o2 -w ${O2_SW_PATH}`
echo "UPDATING..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$RUDA_PATH/lib
export ROOT_INCLUDE_PATH=$ROOT_INCLUDE_PATH:$RUDA_PATH/include