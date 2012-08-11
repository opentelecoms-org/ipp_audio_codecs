/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2001-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.729 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.729/A/B/D/E are international standards promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.729/A/B/D/E speech codec API header file
//
*/

#if !defined( __G729IAPI_H__ )
#define __G729IAPI_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__ICL ) || defined ( __ECL )
/*  Intel C/C++ compiler bug for __declspec(align(8)) !!! */
  #define __ALIGN(n) __declspec(align(16))
  #define __ALIGN32 __declspec(align(32))
#else
  #define __ALIGN(n)
  #define __ALIGN32
#endif

typedef enum _G729Codec_Type{
   G729_CODEC=0,
   G729A_CODEC=1,
   G729D_CODEC=2,
   G729E_CODEC=3,
   G729I_CODEC=4
}G729Codec_Type;

typedef enum _G729Encode_Mode{
   G729Encode_VAD_Enabled=1,
   G729Encode_VAD_Disabled=0
}G729Encode_Mode;

typedef enum _G729Decode_Mode{
   G729Decode_DefaultMode=0
}G729Decode_Mode;

typedef enum{
    APIG729_StsBadCodecType     =   -5,
    APIG729_StsNotInitialized   =   -4,
    APIG729_StsBadArgErr        =   -3,
    APIG729_StsDeactivated      =   -2,
    APIG729_StsErr              =   -1,
    APIG729_StsNoErr            =    0
}APIG729_Status;

struct _G729Encoder_Obj;
struct _G729Decoder_Obj;
typedef struct _G729Encoder_Obj G729Encoder_Obj;
typedef struct _G729Decoder_Obj G729Decoder_Obj;

#if !defined (NO_SCRATCH_MEMORY_USED)
struct _ScratchMem_Obj;
typedef struct _ScratchMem_Obj ScratchMem_Obj;
#endif

#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
#else
  #define __STDCALL
  #define __CDECL
#endif

#define   G729_CODECAPI(type,name,arg)                extern type name arg;

/*
                   Functions declarations
*/
G729_CODECAPI( APIG729_Status, apiG729Encoder_Alloc,
         (G729Codec_Type codecType, int *pCodecSize))
G729_CODECAPI( APIG729_Status, apiG729Decoder_Alloc,
         (G729Codec_Type codecType, int *pCodecSize))
G729_CODECAPI( APIG729_Status, apiG729Codec_ScratchMemoryAlloc,
         (int *pCodecSize))
G729_CODECAPI( APIG729_Status, apiG729Encoder_Init,
         (G729Encoder_Obj* encoderObj, G729Codec_Type codecType, G729Encode_Mode mode))
G729_CODECAPI( APIG729_Status, apiG729Decoder_Init,
         (G729Decoder_Obj* decoderObj, G729Codec_Type codecType))
G729_CODECAPI( APIG729_Status, apiG729Encoder_InitBuff,
         (G729Encoder_Obj* encoderObj, char *buff))
G729_CODECAPI( APIG729_Status, apiG729Decoder_InitBuff,
         (G729Decoder_Obj* decoderObj, char *buff))
G729_CODECAPI( APIG729_Status, apiG729Encode,
         (G729Encoder_Obj* encoderObj,const short* src, unsigned char* dst, G729Codec_Type codecType, int *frametype))
G729_CODECAPI(  APIG729_Status, apiG729EncodeVAD,
         (G729Encoder_Obj* encoderObj,const short* src, short* dst, G729Codec_Type codecType, int *pVAD ))
G729_CODECAPI( APIG729_Status, apiG729Decode,
         (G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst))
G729_CODECAPI( APIG729_Status, apiG729Encoder_Mode,
         (G729Encoder_Obj* encoderObj, G729Encode_Mode mode))

#ifdef __cplusplus
}
#endif


#endif /* __CODECAPI_H__ */
