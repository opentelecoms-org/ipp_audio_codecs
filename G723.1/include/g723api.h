/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.723.1 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.723.1 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.723.1 coder API header
//
*/

#ifndef  __G723API_H__ 
#define __G723API_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>

#define  G723_FrameSize       240
#define  G723_BitStreamFrameSizeMax 24


#define G723_ENC_KEY 0xecd723
#define G723_DEC_KEY 0xdec723

typedef struct {
   int                 objSize;
   int                 key;
   unsigned int        mode;       /* mode's */
   unsigned int        rat: 1;     /* encode rate: 0-63, 1-53, other bits reserved.*/
}G723_Obj_t;

typedef enum _G723Encode_Mode{
   G723Encode_DefaultMode=0, /* r63 ; HF, VAD disabled; */
   G723Encode_VAD_Enabled=1,
   G723Encode_HF_Enabled =2
}G723Encode_Mode;

typedef enum _G723Decode_Mode{
   G723Decode_DefaultMode=0, /* r63 ; PF disabled  */
   G723Decode_PF_Enabled =1
}G723Decode_Mode;

typedef enum{
    APIG723_StsBadCodecType     =   -5,
    APIG723_StsNotInitialized   =   -4,
    APIG723_StsBadArgErr        =   -3,
    APIG723_StsDeactivated      =   -2,
    APIG723_StsErr              =   -1,
    APIG723_StsNoErr            =    0
}APIG723_Status;

struct _G723Encoder_Obj;
struct _G723Decoder_Obj;
#if !defined (NO_SCRATCH_MEMORY_USED)
struct _ScratchMem_Obj;
#endif
typedef struct _G723Encoder_Obj G723Encoder_Obj;
typedef struct _G723Decoder_Obj G723Decoder_Obj;
#if !defined (NO_SCRATCH_MEMORY_USED)
typedef struct _ScratchMem_Obj ScratchMem_Obj;
#endif


#define   G723_CODECAPI(type,name,arg)                extern type name arg;

/* 
                   Functions declarations
*/
G723_CODECAPI( APIG723_Status, apiG723Encoder_Alloc, (int *pCodecSize))
G723_CODECAPI( APIG723_Status, apiG723Decoder_Alloc, (int *pCodecSize))
G723_CODECAPI( APIG723_Status, apiG723Encoder_Init, 
         (G723Encoder_Obj* encoderObj, unsigned int mode))
G723_CODECAPI( APIG723_Status, apiG723Decoder_Init, 
         (G723Decoder_Obj* decoderObj, unsigned int mode))
G723_CODECAPI( APIG723_Status, apiG723Encoder_InitBuff, 
         (G723Encoder_Obj* encoderObj, char *buff))
G723_CODECAPI( APIG723_Status, apiG723Decoder_InitBuff, 
         (G723Decoder_Obj* decoderObj, char *buff))
G723_CODECAPI( APIG723_Status, apiG723Encode,
         (G723Encoder_Obj* encoderObj, const short* src, short rat, char* dst ))
G723_CODECAPI( APIG723_Status, apiG723Decode, 
         (G723Decoder_Obj* decoderObj, const char* src, short badFrameIndicator, short* dst))
G723_CODECAPI( APIG723_Status, apiG723Encoder_Mode,
         (G723Encoder_Obj* encoderObj, unsigned int mode))

#define G723_ENCODER_SCRATCH_MEMORY_SIZE        (5120+40)

#ifdef __cplusplus
}
#endif


#endif /* __G723API_H__ */
