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
//  Purpose: GSM speech codec internal header file
//
*/

#ifndef UTIL_D_H
#define UTIL_D_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gsmamrapi.h"
#include "usc.h"

int text2rate(char *strRate, int *rate);
int read_frame(char **pLine, char **in_buff_cur, int *in_len_cur);
void Ref2Bits(const char *refBits, USC_Bitstream *in);
int getOutFrameSize(void);

extern USC_Fxns USC_GSMAMR_Fxns;
/*
   GetCodecByName() - quest codec functions table by codec name
*/
USC_Fxns* USC_GetCodecByName(void);

#endif /*UTIL_D_H*/
