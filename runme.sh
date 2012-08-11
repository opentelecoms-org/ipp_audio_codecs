#!/bin/bash

# This script will build each of the Asterisk codecs for you

# It also builds the modified command line utilities for encoding/decoding
# raw G729 files

ASTERISK_MODULES_DIR=/usr/lib/asterisk/modules

SRC_DIR=`pwd`/${SRC_DIR}

cd "${SRC_DIR}/G729-float"
./build.sh

cd "${SRC_DIR}/G723.1"
./build.sh

cd "${SRC_DIR}"

echo ""
echo "Unless you have manually edited the file G729-float/Makefile and"
echo "G723.1/Makefile, the codecs are optimized for Pentium II"
echo ""
echo "This code will not run on Pentium I or lower"
echo ""
echo "If you have a more powerful processor than Pentium II, you are"
echo "strongly encouraged to edit the Makefiles and select the "
echo "optimizations for your processors.  This can give a performance"
echo "boost of up to 30%"
echo ""
echo "To use the codecs, just copy codec_g729.so and codec_g723.so to"
echo "your Asterisk modules directory, usually /usr/lib/asterisk/modules"
echo "and restart Asterisk.  The Asterisk command \`show translation'"
echo "will show you whether the modules loaded successfully."
echo ""

mv "${SRC_DIR}/G729-float/bin/codec_g729.so" "${SRC_DIR}"
mv "${SRC_DIR}/G723.1/bin/codec_g723.so" "${SRC_DIR}"

if [ "X${1}" == "Xinstall" ];
then
  DEST_DIR="${2}"
  install -d "${DEST_DIR}${ASTERISK_MODULES_DIR}/"
  install "${SRC_DIR}/codec_g729.so" "${DEST_DIR}${ASTERISK_MODULES_DIR}/"
  install "${SRC_DIR}/codec_g723.so" "${DEST_DIR}${ASTERISK_MODULES_DIR}/"

fi
