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
//  Purpose: GSM speech codec API
//
*/

#ifndef GSMAMR_API_H
#define GSMAMR_API_H

#include "gsmamr.h"
#include "gsmamr_types.h"
#ifdef __cplusplus
extern "C" {
/* for C++ */
#endif

typedef enum _GSMAMRCodec_Type{
   GSMAMR_CODEC=0
}GSMAMRCodec_Type;

typedef enum _GSMAMREncode_Mode{
   GSMAMREncode_DefaultMode=0,
   GSMAMREncode_VAD1_Enabled,
   GSMAMREncode_VAD2_Enabled
}GSMAMREncode_Mode;

typedef enum _GSMAMRDecode_Mode{
   GSMAMRDecode_DefaultMode=0
}GSMAMRDecode_Mode;

typedef struct _GSMAMREnc_Params{
   GSMAMRCodec_Type codecType;
   int mode;
}GSMAMREnc_Params;

typedef struct _GSMAMRDec_Params{
   GSMAMRCodec_Type codecType;
   int mode;
}GSMAMRDec_Params;

typedef enum{
    APIGSMAMR_StsBadCodecType     =   -5,
    APIGSMAMR_StsNotInitialized   =   -4,
    APIGSMAMR_StsBadArgErr        =   -3,
    APIGSMAMR_StsDeactivated      =   -2,
    APIGSMAMR_StsErr              =   -1,
    APIGSMAMR_StsNoErr            =    0
}APIGSMAMR_Status;

struct _GSMAMREncoder_Obj;
struct _GSMAMRDecoder_Obj;
typedef struct _GSMAMREncoder_Obj GSMAMREncoder_Obj;
typedef struct _GSMAMRDecoder_Obj GSMAMRDecoder_Obj;

#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
#else
  #define __STDCALL
  #define __CDECL
#endif

#define   GSMAMR_CODECAPI(type,name,arg)                extern type name arg;

/*
                   Functions declarations
*/
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_Alloc,
         (const GSMAMREnc_Params *gsm_Params, unsigned int *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_Alloc,
         (const GSMAMRDec_Params *gsm_Params, unsigned int *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_Init,
         (GSMAMREncoder_Obj* encoderObj, unsigned int mode))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_Init,
         (GSMAMRDecoder_Obj* decoderObj, unsigned int mode))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_GetSize,
         (GSMAMRDecoder_Obj* decoderObj, unsigned int *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_GetSize,
         (GSMAMREncoder_Obj* encoderObj, unsigned int *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncode,
         (GSMAMREncoder_Obj* encoderObj,const short* src, GSMAMR_Rate_t rate,
          unsigned char* dst, int *pVad  ))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecode,
         (GSMAMRDecoder_Obj* decoderObj,const unsigned char* src, GSMAMR_Rate_t rate,
          RXFrameType rx_type, short* dst))

#ifdef __cplusplus
}
#endif


#endif /* GSMAMR_API_H */
