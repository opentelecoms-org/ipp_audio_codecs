/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives GSMAMR Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  GSMAMR is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//      Purpose          : Converts the encoder parameter vector into a
//                       : vector of serial bits
//
*/

#ifndef UTIL_E_H
#define UTIL_E_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ipps.h>
#include "gsmamrapi.h"
#include "usc.h"

#define USAGE          1
#define FOPEN_FAIL     2
#define MEMORY_FAIL    3
#define UNKNOWN_FORMAT 4
#define THREAD_FAIL    5
#define MT_SAFETY      6

int text2rate(char *strRate, char **rat_buff, int *nRates, const USC_CodecInfo *pInfo);
int checkVad(char *strVad, int *mode);
int Bits2Ref(USC_PCMStream in, USC_Bitstream out, char *out_buff_cur);
int getInFrameSize(void);
int getOutFrameSize(void);

extern USC_Fxns USC_GSMAMR_Fxns;
/*
   GetCodecByName() - quest codec functions table by codec name
*/
USC_Fxns* USC_GetCodecByName(void);

#endif /*UTIL_E_H*/
