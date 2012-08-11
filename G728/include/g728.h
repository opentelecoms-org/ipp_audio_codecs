/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.728 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.728 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.728 common definition
//
*/

#ifndef __G728_H_
#define __G728_H_

#define G728_VEC_PER_FRAME  4
#define G728_VECTOR  5
#define G728_FRAME  (G728_VEC_PER_FRAME * G728_VECTOR)

typedef enum _G728_Rate{
   G728_Rate_16000=0,
   G728_Rate_12800=1,
   G728_Rate_9600=2
}G728_Rate;

typedef enum _G728_Type{
   G728_Pure=0,
   G728_Annex_I=1
}G728_Type;

typedef enum {
  G728_PST_OFF = 0,
  G728_PST_ON
} G728_PST;

typedef enum {
  G728_PCM_MuLaw = 0,
  G728_PCM_ALaw,
  G728_PCM_Linear
} G728_PCM_Law;

#endif /* __G728_H_ */


