/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives G.729I Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  G729I is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.729I speech codec internal header file
//
*/
#ifndef UTIL_D_H
#define UTIL_D_H

#include <stdio.h>
#include <string.h>
#include "g729api.h"
#include "usc.h"

/* Max size of vector of analysis parameters in bytes */
#define  G729_SPEECH_FRAME   80
#define  G729E_PRM_SIZE_fwd  18    /* Size of vector of analysis parameters.    */
#define G729DEC_PRM_MAX  ((int)( (G729E_PRM_SIZE_fwd+1)*sizeof(short)))
#define BITSTREAM_FILE_FRAME_SIZE 82
#define RATE_SID_OCTET    16     /* number of bits in Octet Transmission mode */

#define BIT_0     (short)0x007f /* definition of zero-bit in bit-stream      */
#define BIT_1     (short)0x0081 /* definition of one-bit in bit-stream       */
#define SYNC_WORD (short)0x6b21 /* definition of frame erasure flag          */
#define SIZE_WORD (short)80     /* number of speech bits                     */

#define RATE_0           0      /* 0 bit/s rate                  */
#define RATE_SID        15      /* SID                           */
#define RATE_6400       64      /* Low  rate  (6400 bit/s)       */
#define RATE_8000       80      /* Full rate  (8000 bit/s)       */
#define RATE_11800      118     /* High rate (11800 bit/s)       */
#define PRM_SIZE_D      10       /* Size of vector of analysis parameters.    */

int text2rate(char *strRate, int *rate);
int read_frame(char **buffer, char **in_buff_cur, int *in_len_cur);
void Ref2Bits(const char *refBits, USC_Bitstream *in);
int getOutFrameSize(void);

extern USC_Fxns USC_G729FP_Fxns;
/*
   GetCodecByName() - quest codec functions table by codec name
*/
USC_Fxns* USC_GetCodecByName(void);

#endif /*UTIL_D_H*/
