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
//  Purpose: G.728 function for adapters
//
*/
#include "owng728.h"
#include "g728tables.h"

void WeightingFilterCoeffsCalc(Ipp16s *pTmpWghtFltrCoeffs, Ipp16s coeffsScale, 
                                     Ipp16s *pVecWghtFltrCoeffs) {
   Ipp32s dwProd, i;
   Ipp16s tmpCoeffs[10];
   Ipp16s *pNumerCoeffs = pVecWghtFltrCoeffs;
   Ipp16s *pDenomCoeffs = pVecWghtFltrCoeffs+LPCW;
   Ipp16s scale = 16 - coeffsScale;
   Ipp32s ovfPos = IPP_MAX_32S >> scale;
   Ipp32s ovfNeg = IPP_MIN_32S >> scale;

   /* Do the numerator coefficients */ 
   for(i=0; i<6; i++){
      dwProd = cnstZeroCtrlFactor[i+1] * pTmpWghtFltrCoeffs[i];
      /* if Overflow expected in ShiftL_32s Do not update */ 
      if(dwProd > ovfPos || dwProd < ovfNeg) return; 
      dwProd <<= scale;
      tmpCoeffs[i] = Cnvrt_NR_32s16s(dwProd);
   }

   for(i=6; i<LPCW; i++){
      dwProd = cnstZeroCtrlFactor[i+1] * pTmpWghtFltrCoeffs[i];
      dwProd <<= scale;
      tmpCoeffs[i] = Cnvrt_NR_32s16s(dwProd);
   }

   for(i=0; i<LPCW; i++)
      pNumerCoeffs[i] = tmpCoeffs[i];

   /* Do the denumerator coefficients */ 
   for(i=0; i<LPCW; i++){
      dwProd = cnstPoleCtrlFactor[i+1] * pTmpWghtFltrCoeffs[i];
      dwProd <<= scale;
      pDenomCoeffs[i] = Cnvrt_NR_32s16s(dwProd);
   }

   return;
}


void BandwidthExpansionModul(const Ipp16s* pConstBroadenVector, Ipp16s* pTmpCoeffs, 
                             Ipp16s coeffsScale, Ipp16s* pCoeffs, int len){
   Ipp32s i, dwProd;
   Ipp16s scale = 16 - coeffsScale;
   Ipp32s ovfPos = IPP_MAX_32S >> scale;
   Ipp32s ovfNeg = IPP_MIN_32S >> scale;

   for(i=0; i<len; i++) {
      dwProd = pConstBroadenVector[i] * pTmpCoeffs[i];
      /* if Overflow expected in ShiftL_32s Do not update */ 
      if(dwProd > ovfPos || dwProd < ovfNeg) return; 
      dwProd <<= scale;
      pTmpCoeffs[i] = Cnvrt_NR_32s16s(dwProd);
   }

   for(i=0; i<len; i++)
      pCoeffs[i] = pTmpCoeffs[i];

   return;
}

void BandwidthExpansionModulFE(const Ipp16s* pConstBroadenVector, Ipp16s* pTmpCoeffs, 
                     Ipp16s coeffsScale, Ipp16s* pCoeffs, int len, short countFE, short illCond){
   Ipp32s i, dwProd;
   Ipp16s scale = 16 - coeffsScale;
   Ipp32s ovfPos = IPP_MAX_32S >> scale;
   Ipp32s ovfNeg = IPP_MIN_32S >> scale;

   if((countFE==1)&&(illCond == G728_FALSE)) {
      for(i=0; i<LPC; i++) {
         dwProd = pConstBroadenVector[i] * pTmpCoeffs[i];
         if(dwProd > ovfPos || dwProd < ovfNeg) /* Do not update*/ 
            return;
         pCoeffs[i] = pTmpCoeffs[i] = Cnvrt_NR_32s16s(dwProd<<scale); /* I think atmp  doesn't need to be updated. (Igor S. Belyakov)*/ 
      }
   } else {
      for(i=0; i<LPC; i++) {
         dwProd = pConstBroadenVector[i] * pCoeffs[i];
         pCoeffs[i] = Cnvrt_NR_32s16s(dwProd<<2);
      }
   }
   return;
}


void LoggainLinearPrediction(Ipp16s *pVecLGPredictorCoeffs, Ipp16s *pLGPredictorState, Ipp32s *pGainLog){
   Ipp32s dwSum, i;
   Ipp16s tmp[LPCLG];

   /* predict log gain */ 
   dwSum = 0;
   for (i=0; i<LPCLG; i++) {
      dwSum = dwSum - pLGPredictorState[i]*pVecLGPredictorCoeffs[i];
      tmp[i] = pLGPredictorState[i];
   }
   *pGainLog = dwSum >> 14;
   
   for (i=1; i<LPCLG; i++)
      pLGPredictorState[i] = tmp[i-1];

   return;
}


void InverseLogarithmCalc(Ipp32s gainLog, Ipp16s *pLinearGain, Ipp16s *pScaleGain){
   Ipp32s dwVal0, dwVal1;
   Ipp32s x, z;
   Ipp16s tmp;

   /* add offset */ 
   z = gainLog + GOFF;

   /* Compute gain = 10**(z/20) */ 
   dwVal0 = 10 * z;
   dwVal1 = 2*20649 * z;
   dwVal1 = Cnvrt_NR_32s16s(dwVal1);
   dwVal0 += dwVal1;
   dwVal1 = dwVal0 >> 15;
   *pScaleGain = 14 - dwVal1;
   
   dwVal1 = ShiftL_32s(dwVal1, 15);
   x = Sub_32s(dwVal0, dwVal1);

   dwVal0 = 323 * x;  /* c_c4 */ 
   dwVal0 = ShiftL_32s(dwVal0, 1);
   dwVal0 = Add_32s(dwVal0, 1874<<16); /* + c_c3 */ 
   tmp = Cnvrt_NR_32s16s(dwVal0);
   
   dwVal0 = tmp * x;
   dwVal0 = ShiftL_32s(dwVal0, 1);
   dwVal0 = Add_32s(dwVal0, 7866<<16); /* + c_c2 */ 
   tmp = Cnvrt_NR_32s16s(dwVal0);

   dwVal0 = tmp * x;
   dwVal0 = ShiftL_32s(dwVal0, 1);
   dwVal0 = Add_32s(dwVal0, 22702<<16); /* + c_c1 */ 
   tmp = Cnvrt_NR_32s16s(dwVal0);

   dwVal0 = tmp * x;
   dwVal0 = Add_32s(dwVal0, 16384<<16); /* + c_c0 */ 
   *pLinearGain = Cnvrt_NR_32s16s(dwVal0);

   return;
}


Ipp16s LoggainAdderLimiter(Ipp32s gainLog, Ipp16s gainIdx, Ipp16s shapeIdx,
                           const Ipp16s *pCodebookGain) {
   Ipp32s dwVal;

   dwVal = (gainLog<<7) + (pCodebookGain[gainIdx]<<5) + (cnstShapeCodebookVectors[shapeIdx]<<5);
   dwVal = dwVal >> 7;
   if(dwVal < -16384) dwVal = -16384;

   return (Ipp16s) dwVal;
}




