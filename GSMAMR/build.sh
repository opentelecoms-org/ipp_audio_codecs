#! /bin/sh

#
# Usage: build.sh {gcc}
#

IPPENV=${IPPROOT}/tools/env/ippvars32.sh

if [ ! -x "${IPPENV}" ]; then
  IPPENV=/opt/intel/ipp41/ia32_itanium/tools/env/ippvars32.sh
fi

if [ ! -x "${IPPENV}" ]; then
  IPPENV=/opt/intel/ipp41_eval/ia32_itanium/tools/env/ippvars32.sh
fi

if [ ! -x "${IPPENV}" ]; then
  @echo -e "*************************************************************************"
  @echo -e "Intel(R) IPP is not found!"
  @echo -e "Please install Intel(R) IPP or set IPPROOT environment variable correctly."
  @echo -e "*************************************************************************"
  exit
fi

ARG=$1

CC=gcc
CXX=g++

if [ "${ARG}" == "" ]; then

  CCFIND=/opt/intel/compiler60/ia32/bin/iccvars.sh
  if [ -x "${CCFIND}" ]; then
    CCENV=${CCFIND}
  fi

  CCFIND=/opt/intel/compiler70/ia32/bin/iccvars.sh
  if [ -x "${CCFIND}" ]; then
    CCENV=${CCFIND}
  fi

  CCFIND=/opt/intel/compiler80/ia32/bin/iccvars.sh
  if [ -x "${CCFIND}" ]; then
    CCENV=${CCFIND}
  fi

  CCFIND=/opt/intel_cc_80/bin/iccvars.sh
  if [ -x "${CCFIND}" ]; then
    CCENV=${CCFIND}
  fi
  
  if [ "${CCENV}" ]; then
    . ${CCENV}
    CC=icc
    CXX=icc
  fi

fi

. ${IPPENV}

make clean
make CC=${CC} CXX=${CXX}
