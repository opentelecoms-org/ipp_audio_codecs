/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.722.1 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.722.1 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.722.1 codec API header file
//
*/

#if !defined( __G7221API_H__ )
#define __G7221API_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    APIG722_StsBadCodecType     =   -5,
    APIG722_StsNotInitialized   =   -4,
    APIG722_StsBadArgErr        =   -3,
    APIG722_StsDeactivated      =   -2,
    APIG722_StsErr              =   -1,
    APIG722_StsNoErr            =    0
}APIG722_Status;


struct _G722Encoder_Obj;
struct _G722Decoder_Obj;
typedef struct _G722Encoder_Obj G722Encoder_Obj;
typedef struct _G722Decoder_Obj G722Decoder_Obj;


#define   G722_CODECAPI( type,name,arg )        extern type name arg;


G722_CODECAPI( APIG722_Status, apiG722Encoder_Alloc, (unsigned int* objSize))
G722_CODECAPI( APIG722_Status, apiG722Encoder_Init, (G722Encoder_Obj* pEncoderObj))
G722_CODECAPI( APIG722_Status, apiG722Encoder_Free, (G722Encoder_Obj* pEncoderObj))
G722_CODECAPI( APIG722_Status, apiG722Encode, (G722Encoder_Obj* pEncoderObj,
              int nBitsPerFrame, short *pSrc, short *pDst))

G722_CODECAPI( APIG722_Status, apiG722Decoder_Alloc, (unsigned int* objSize))
G722_CODECAPI( APIG722_Status, apiG722Decoder_Init, (G722Decoder_Obj* pDecoderObj))
G722_CODECAPI( APIG722_Status, apiG722Decoder_Free, (G722Decoder_Obj* pDecoderObj))
G722_CODECAPI( APIG722_Status, apiG722Decode, (G722Decoder_Obj* pDecoderObj,
              short errFlag, int nBitsPerFrame, short *pSrc, short *pDst))

G722_CODECAPI(APIG722_Status, apiG722GetObjSize, (void* pObj,
              unsigned int *pObjSize))

#ifdef __cplusplus
}
#endif


#endif /* __CODECAPI_H__ */
