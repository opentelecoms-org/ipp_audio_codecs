/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives Speech Codec Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  Speech Codec is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: speech codec internal header file
//
*/
#ifndef UTIL_E_H
#define UTIL_E_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ipps.h>
#include "g729api.h"
#include "usc.h"

#define  G729_SPEECH_FRAME   80
#define G729_PURE_BITSTREAM_FILE_FRAME_SIZE 82
#define BITSTREAM_FILE_FRAME_SIZE (118+4)  /* Bits/frame + bfi+ number of speech bits
                                             + bit de mode + protection */
/* Max size of vector of analysis parameters in bytes */
#define G729ENC_PRM_MAX  /*18*/30
#define RATE_SID_OCTET    16     /* number of bits in Octet Transmission mode */
#define PRM_SIZE_D      10       /* Size of vector of analysis parameters.    */

#define BIT_0     (short)0x007f /* definition of zero-bit in bit-stream      */
#define BIT_1     (short)0x0081 /* definition of one-bit in bit-stream       */
#define SYNC_WORD (short)0x6b21 /* definition of frame erasure flag          */
#define SIZE_WORD (short)80     /* number of speech bits                     */
#define RATE_0           0      /* 0 bit/s rate                  */
#define RATE_SID        15      /* SID                           */
#define RATE_6400       64      /* Low  rate  (6400 bit/s)       */
#define RATE_8000       80      /* Full rate  (8000 bit/s)       */
#define RATE_11800      118     /* High rate (11800 bit/s)       */

#define USAGE          1
#define FOPEN_FAIL     2
#define MEMORY_FAIL    3
#define UNKNOWN_FORMAT 4
#define THREAD_FAIL    5
#define MT_SAFETY      6

static void encodedBitsUnPack_G729(
  const unsigned char *prm,  /* input : encoded parameters  (G729_PRM_SIZE parameters)  */
  short *bits,               /* output: serial bits (SERIAL_SIZE ) bits[0] = bfi
                                    bits[1] = 80 */
  int len
);
static void encodedBitsUnPack_G729AB(
  const unsigned char *prm,  /* input : encoded parameters  (G729_PRM_SIZE parameters)  */
  int frametype,
  short *bits                /* output: serial bits (SERIAL_SIZE ) bits[0] = bfi
                                    bits[1] = 80 */
);
static void encodedBitsUnPack_G729I(
  const unsigned char *prm,  /* input : encoded parameters  (G729_PRM_SIZE parameters)  */
  int frametype,
  short *bits                /* output: serial bits (SERIAL_SIZE ) bits[0] = bfi
                                    bits[1] = 80 || bits[1] = 118*/
);


extern USC_Fxns USC_G729FP_Fxns;

/*
   GetCodecByName() - quest codec functions table by codec name
*/
USC_Fxns* USC_GetCodecByName(void);
int text2rate(char *strRate, char **rat_buff, int *nRates, USC_CodecInfo *pInfo);
int checkVad(char *strVad, int *mode);
int getInFrameSize(void);
int getOutFrameSize(void);
int Bits2Ref(USC_PCMStream in, USC_Bitstream out, char *out_buff_cur);

#endif /*UTIL_E_H*/
