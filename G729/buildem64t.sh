#! /bin/sh

#
# Usage: build.sh {gcc}
#

IPPENV=${IPPROOT}/tools/env/ippvarsem64t.sh

if [ ! -x "${IPPENV}" ]; then
  IPPENV=/opt/intel/ipp41/em64t/tools/env/ippvarsem64t.sh
fi

if [ ! -x "${IPPENV}" ]; then
  IPPENV=/opt/intel/ipp41_eval/em64t/tools/env/ippvarsem64t.sh
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
ARCH=lem64t

if [ "${ARG}" == "" ]; then

  CCFIND=/opt/intel_cce_80/bin/iccvars.sh
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
make CC=${CC} CXX=${CXX} ARCH=${ARCH}
