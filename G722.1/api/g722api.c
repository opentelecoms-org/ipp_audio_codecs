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
//  Purpose: G.722.1 decode API
//
*/

#include "owng722.h"
#include "g722api.h"

G722_CODECFUN( APIG722_Status, apiG722Encoder_Alloc,(unsigned int* objSize)){
   *objSize = sizeof(G722Encoder_Obj);
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Encoder_Free,(G722Encoder_Obj* pEncoderObj)){
   if(pEncoderObj == NULL) return APIG722_StsBadArgErr;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Encoder_Init,(G722Encoder_Obj* pEncoderObj)){
   int i;
   for (i=0;i<FRAMESIZE;i++) pEncoderObj->history[i] = 0;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Encode,
         (G722Encoder_Obj* pEncoderObj, int bitFrameSize, Ipp16s *pSrc, Ipp16s *pDst)){
   IPP_ALIGNED_ARRAY(16, short, vecMlt, FRAMESIZE);
   IPP_ALIGNED_ARRAY(16, short, vecWndData, DCT_LENGTH);
   Ipp16s scale;
   /* Convert input samples to rmlt coefs */
   ippsDecomposeMLTToDCT_G722_16s(pSrc, pEncoderObj->history, vecWndData);
   NormalizeWndData(vecWndData, &scale);
   ippsDCTFwd_G722_16s(vecWndData, vecMlt);
   /* Encode the mlt coefs */
   EncodeFrame((short)bitFrameSize, vecMlt, scale, pDst);
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decoder_Alloc,(unsigned int* objSize)){
   *objSize = sizeof(G722Decoder_Obj);
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decoder_Free,(G722Decoder_Obj* pDecoderObj)){
   if(pDecoderObj == NULL) return APIG722_StsBadArgErr;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decoder_Init,(G722Decoder_Obj* pDecoderObj)){
   int i;
   /* initialize the coefs history */
   for (i=0; i<DCT_LENGTH; i++)
      pDecoderObj->vecOldMlt[i] = 0;
   for (i=0; i<(DCT_LENGTH >> 1); i++)
      pDecoderObj->vecOldSmpls[i] = 0;
   pDecoderObj->prevScale= 0;
   /* initialize the random number generator */
   pDecoderObj->randVec[0] = 1;
   pDecoderObj->randVec[1] = 1;
   pDecoderObj->randVec[2] = 1;
   pDecoderObj->randVec[3] = 1;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decode, (G722Decoder_Obj* pDecoderObj,
              short errFlag, int nBitsPerFrame, Ipp16s *pSrc, Ipp16s *pDst)){
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecNewSmpls, DCT_LENGTH);
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecMlt, DCT_LENGTH);
   short scale;
   /* reinit the current word to point to the start of the buffer */
   pDecoderObj->bitObj.pBitstream = pSrc;
   pDecoderObj->bitObj.curWord = *pSrc;
   pDecoderObj->bitObj.bitCount = 0;
   pDecoderObj->bitObj.curBitsNumber = nBitsPerFrame;
   /* process the input samples into decoder Mlt coefs */
   DecodeFrame(&pDecoderObj->bitObj, pDecoderObj->randVec, vecMlt, &scale,
      &pDecoderObj->prevScale, pDecoderObj->vecOldMlt, errFlag);
   /* Convert the decoder mlt coefs  to samples (inverse DCT)*/
   ippsDCTInv_G722_16s(vecMlt, vecNewSmpls);
   if (scale > 0)
      ippsRShiftC_16s_I(scale, vecNewSmpls, DCT_LENGTH);
   else if (scale < 0)
      ippsLShiftC_16s_I(-scale, vecNewSmpls, DCT_LENGTH);
   ippsDecomposeDCTToMLT_G722_16s(vecNewSmpls, pDecoderObj->vecOldSmpls, pDst);
   return APIG722_StsNoErr;
}

G722_CODECFUN(APIG722_Status, apiG722GetObjSize, (void* pObj, unsigned int *pObjSize)){
   own_G722_Obj_t *pG722CodecObj = (own_G722_Obj_t*)pObj;
   if (pG722CodecObj == NULL) return APIG722_StsBadArgErr;
   *pObjSize = pG722CodecObj->objSize;
   return APIG722_StsNoErr;
}
