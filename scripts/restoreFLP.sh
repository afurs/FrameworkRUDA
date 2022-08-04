#!/bin/bash

FLP_O2_SH_TMP="/etc/profile.d/o2.sh_tmp"
FLP_O2_SH="/etc/profile.d/o2.sh"

if [ -f "$FLP_O2_SH_TMP" ]; then
  echo "Restoring ${FLP_O2_SH}..."
  sudo mv $FLP_O2_SH_TMP $FLP_O2_SH
fi
source $FLP_O2_SH
FLP_LDCONFIG_TMP="/etc/ld.so.conf.d/o2-x86_64.conf_tmp"
FLP_LDCONFIG="/etc/ld.so.conf.d/o2-x86_64.conf"

if [ -f "$FLP_LDCONFIG_TMP" ]; then
  echo "Restoring ${FLP_LDCONFIG}..."
  sudo mv $FLP_LDCONFIG_TMP $FLP_LDCONFIG
fi
sudo ldconfig
