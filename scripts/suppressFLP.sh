#!/bin/bash

FLP_O2_SH_TMP="/etc/profile.d/o2.sh_tmp"
FLP_O2_SH="/etc/profile.d/o2.sh"

if [ -f "$FLP_O2_SH" ]; then
  echo "Renaming ${FLP_O2_SH}..."
  sudo mv $FLP_O2_SH $FLP_O2_SH_TMP
fi

FLP_LDCONFIG_TMP="/etc/ld.so.conf.d/o2-x86_64.conf_tmp"
FLP_LDCONFIG="/etc/ld.so.conf.d/o2-x86_64.conf"

if [ -f "$FLP_LDCONFIG" ]; then
  echo "Renaming ${FLP_LDCONFIG}..."
  sudo mv $FLP_LDCONFIG $FLP_LDCONFIG_TMP
fi
sudo ldconfig
# Cleaning
cleaning() {
  env_param=$1
  phrase_to_clean=$2
  new_content=""
  for entry in $(echo $env_param | tr ":" "\n");
  do
    if [[ $entry != *"${phrase_to_clean}"* ]]; then
      new_content+=":"
      new_content+=$entry
    fi
  done
  result=$(echo $new_content | grep -o '[^:].*')
  echo "${result}"
}


export PYTHONPATH=$(cleaning $PYTHONPATH "/o2/")
export ROOT_DYN_PATH=$(cleaning $ROOT_DYN_PATH "/o2/")
export ROOT_INCLUDE_PATH=$(cleaning $ROOT_INCLUDE_PATH "/o2/")
export PATH=$(cleaning $PATH "/o2/")
#CHECK=$(cleaning $PYTHONPATH "/o2/")
#echo "Cleaned: ${CHECK}"

unset CONTROL_CORE_ROOT
unset OPENSSL_ROOT
unset RE2_ROOT
unset PROTOBUF_ROOT
unset GLOG_ROOT
unset C_ARES_ROOT
unset GRPC_ROOT
unset RAPIDJSON_ROOT
unset MESOS_ROOT
unset COCONUT_ROOT
unset DIM_ROOT
unset PYTHON_MODULES_ROOT
unset BOOST_ROOT
unset LIBINFOLOGGER_ROOT
unset MONITORING_ROOT
unset COMMON_O2_ROOT
unset FMT_ROOT
unset FAIRLOGGER_ROOT
unset ASIO_ROOT
unset ASIOFI_ROOT
unset DDS_ROOT
unset PPCONSUL_ROOT
unset CONFIGURATION_ROOT
unset UTF8PROC_ROOT
unset MS_GSL_ROOT
unset LIBJALIENO2_ROOT
unset ZEROMQ_ROOT
unset FAIRMQ_ROOT
unset CONTROL_OCCPLUGIN_ROOT
unset VC_ROOT
unset GSL_ROOT
unset FFTW3_ROOT
unset DEBUGGUI_ROOT
unset CLANG_ROOT
unset ARROW_ROOT
unset ROOT_ROOT
unset FAIRROOT_ROOT
unset O2_ROOT
unset QUALITYCONTROL_ROOT
unset PDA_ROOT
unset READOUTCARD_ROOT
unset LLA_ROOT
unset DIMRPCPARALLEL_ROOT
unset ALF_ROOT
unset READOUT_ROOT
unset DATADISTRIBUTION_ROOT
unset LIBUV_ROOT
unset TBB_ROOT
unset VMC_ROOT
unset UCX_ROOT

