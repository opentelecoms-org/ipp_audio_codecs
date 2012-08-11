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
//  Purpose: G.728 codec API header file
//
*/

#if !defined( __CODECAPI_H__ )
#define __CODECAPI_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <ippsc.h>
#include "g728.h"
typedef enum{
    APIG728_StsBadCodecType     =   -5,
    APIG728_StsNotInitialized   =   -4,
    APIG728_StsBadArgErr        =   -3,
    APIG728_StsDeactivated      =   -2,
    APIG728_StsErr              =   -1,
    APIG728_StsNoErr            =    0
}APIG728_Status;


struct _G728Encoder_Obj;
struct _G728Decoder_Obj;
typedef struct _G728Encoder_Obj G728Encoder_Obj;
typedef struct _G728Decoder_Obj G728Decoder_Obj;


#define   G728_CODECAPI( type,name,arg )        extern type name arg;


G728_CODECAPI( APIG728_Status, apiG728Encoder_Alloc,(unsigned int* objSize))
G728_CODECAPI( APIG728_Status, apiG728Encoder_Init,(G728Encoder_Obj* encoderObj, G728_Rate rate))
G728_CODECAPI( APIG728_Status, apiG728Encoder_GetSize,
         (G728Encoder_Obj* encoderObj, unsigned int *pCodecSize))

G728_CODECAPI( APIG728_Status, apiG728Encode,
         (G728Encoder_Obj* encoderObj, short *src, unsigned char *dst))

G728_CODECAPI( APIG728_Status, apiG728Decoder_Alloc,(unsigned int* objSize))
G728_CODECAPI( APIG728_Status, apiG728Decoder_Init,
              (G728Decoder_Obj* decoderObj, G728_Type type, G728_Rate rate, int pst))
G728_CODECAPI( APIG728_Status, apiG728Decoder_GetSize,
         (G728Decoder_Obj* decoderObj, unsigned int *pCodecSize))

G728_CODECAPI( APIG728_Status, apiG728Decode, (G728Decoder_Obj* decoderObj,
              unsigned char *src, short *dst))

#ifdef __cplusplus
}
#endif


#endif /* __CODECAPI_H__ */
