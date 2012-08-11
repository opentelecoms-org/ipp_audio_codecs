/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives AMRWB Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  AMRWB is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: AMR WB speech codec API
//
*/

#ifndef AMRWB_API_H
#define AMRWB_API_H

#include <ipps.h>
#include <ippsc.h>
#include "amrwb_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _AMRWBCodec_Type{
   AMRWB_CODEC=0
}AMRWBCodec_Type;

typedef enum _AMRWBEncode_Mode{
   AMRWBEncode_DefaultMode=0,
   AMRWBEncode_VAD_Enabled
}AMRWBEncode_Mode;

typedef enum _AMRWBDecode_Mode{
   AMRWBDecode_DefaultMode=0
}AMRWBDecode_Mode;

typedef struct _AMRWBEnc_Params{
   AMRWBCodec_Type codecType;
   int mode;
}AMRWBEnc_Params;

typedef struct _AMRWBDec_Params{
   AMRWBCodec_Type codecType;
   int mode;
}AMRWBDec_Params;

typedef enum{
    APIAMRWB_StsBadCodecType     =   -5,
    APIAMRWB_StsNotInitialized   =   -4,
    APIAMRWB_StsBadArgErr        =   -3,
    APIAMRWB_StsDeactivated      =   -2,
    APIAMRWB_StsErr              =   -1,
    APIAMRWB_StsNoErr            =    0
}APIAMRWB_Status;

struct _AMRWBEncoder_Obj;
struct _AMRWBDecoder_Obj;
typedef struct _AMRWBEncoder_Obj AMRWBEncoder_Obj;
typedef struct _AMRWBDecoder_Obj AMRWBDecoder_Obj;

#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
#else
  #define __STDCALL
  #define __CDECL
#endif

#define   AMRWB_CODECAPI(type,name,arg)                extern type name arg;

/*
                   Functions declarations
*/
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_Alloc,
         (const AMRWBEnc_Params *amrwb_Params, unsigned int *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecoder_Alloc,
         (const AMRWBDec_Params *amrwb_Params, unsigned int *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_Init,
         (AMRWBEncoder_Obj* encoderObj, int mode))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecoder_Init,
         (AMRWBDecoder_Obj* decoderObj))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecoder_GetSize,
         (AMRWBDecoder_Obj* decoderObj, unsigned int *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_GetSize,
         (AMRWBEncoder_Obj* encoderObj, unsigned int *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncode,
         (AMRWBEncoder_Obj* encoderObj, const unsigned short* src, unsigned char* dst, 
         AMRWB_Rate_t *rate, int Vad ))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecode,
         (AMRWBDecoder_Obj* decoderObj,const unsigned char* src, AMRWB_Rate_t rate,
          RXFrameType rx_type, unsigned short* dst))

#ifdef __cplusplus
}
#endif


#endif /* AMRWB_API_H */
