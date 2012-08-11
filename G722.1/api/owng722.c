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
//  Purpose: G.722.1 auxiliaries
//
*/

#include "owng722.h"

const Ipp16s cnstDiffRegionPowerBits_G722[REG_NUM][DIFF_REG_POW_LEVELS] = {
{
  99,99,99,99,99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,99,99,99,99
},
{
   4, 6, 5, 5, 4, 4, 4, 4, 4, 4, 3, 3,
   3, 4, 5, 7, 8, 9,11,11,12,12,12,12
},
{
  10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 3, 3,
   4, 5, 7, 9,11,12,13,15,15,15,16,16
},
{
  12,10, 8, 6, 5, 4, 4, 4, 4, 4, 4, 3,
   3, 3, 4, 4, 5, 5, 7, 9,11,13,14,14},
{
  13,10, 9, 9, 7, 7, 5, 5, 4, 3, 3, 3,
   3, 3, 4, 4, 4, 5, 7, 9,11,13,13,13
},
{
  12,13,10, 8, 6, 6, 5, 5, 4, 4, 3, 3,
   3, 3, 3, 4, 5, 5, 6, 7, 9,11,14,14
},
{
  12,11, 9, 8, 8, 7, 5, 4, 4, 3, 3, 3,
   3, 3, 4, 4, 5, 5, 7, 8,10,13,14,14
},
{
  15,16,15,12,10, 8, 6, 5, 4, 3, 3, 3,
   2, 3, 4, 5, 5, 7, 9,11,13,16,16,16
},
{
  14,14,11,10, 9, 7, 7, 5, 5, 4, 3, 3,
   2, 3, 3, 4, 5, 7, 9, 9,12,14,15,15
},
{
   9, 9, 9, 8, 7, 6, 5, 4, 3, 3, 3, 3,
   3, 3, 4, 5, 6, 7, 8,10,11,12,13,13
},
{
  14,12,10, 8, 6, 6, 5, 4, 3, 3, 3, 3,
   3, 3, 4, 5, 6, 8, 8, 9,11,14,14,14
},
{
  13,10, 9, 8, 6, 6, 5, 4, 4, 4, 3, 3,
   2, 3, 4, 5, 6, 8, 9, 9,11,12,14,14
},
{
  16,13,12,11, 9, 6, 5, 5, 4, 4, 4, 3,
   2, 3, 3, 4, 5, 7, 8,10,14,16,16,16},
{
  13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3,
   2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14
}};
const Ipp16s cnstVectorDimentions_G722[NUM_CATEGORIES] =  { 2, 2, 2, 4, 4, 5, 5, 1};
const Ipp16s cnstNumberOfVectors_G722[NUM_CATEGORIES] = {10,10,10, 5, 5, 4, 4,20};
const Ipp16s cnstMaxBin_G722[NUM_CATEGORIES] = {13, 9, 6, 4, 3, 2, 1, 1};
const Ipp16s cnstMaxBinInverse_G722[NUM_CATEGORIES] = {
	2341,3277,4682,6554,8193,10923,16385,16385
};

Ipp16s const cnstExpectedBits_G722[NUM_CATEGORIES] = {52, 47, 43, 37, 29, 22, 16,   0};

/*F*
//  Name:      CategorizeFrame
//  Purpose:   Computes a series of categorizations
*F*/
void CategorizeFrame(Ipp16s nAccessibleBits, Ipp16s* pRmsIndices,
         Ipp16s* pPowerCtgs, Ipp16s* pCtgBalances){
   Ipp16s offset;
   /* Compensation accessible bits at higher bit rates */
   if (nAccessibleBits > 320){
      nAccessibleBits -= 320;
      nAccessibleBits *= 5;
      nAccessibleBits >>= 3;
      nAccessibleBits += 320;
   }
   /* calculate the offset */
   GetPowerCategories(pRmsIndices, nAccessibleBits, pPowerCtgs, &offset);
   /* Getting poweres and balancing categories */
   GetCtgPoweresBalances(pPowerCtgs, pCtgBalances, pRmsIndices,
      nAccessibleBits, offset);
}

/*F*
//  Name:      GetCtgPoweresBalances
//  Purpose:   Computes the poweres balances of categories.
*F*/
void GetCtgPoweresBalances(Ipp16s *pPowerCtgs, Ipp16s *pCtgBalances,
         Ipp16s* pRmsIndices, Ipp16s nBitsAvailable, Ipp16s offset){
   Ipp16s nCodeBits;
   Ipp16s regNum, j;
   Ipp16s ctgMaxRate[REG_NUM], ctgMinRate[REG_NUM];
   Ipp16s tmpCtgBalances[2*CAT_CONTROL_MAX];
   Ipp16s rawMax, rawMin;
   Ipp16s rawMaxIndx=0, rawMinIndx=0;
   Ipp16s maxRateIndx, minRateIndx;
   Ipp16s max, min;
   Ipp16s tmp0, tmp1;

   nCodeBits = 0;
   for (regNum=0; regNum<REG_NUM; regNum++)
      nCodeBits += cnstExpectedBits_G722[pPowerCtgs[regNum]];

   for (regNum=0; regNum<REG_NUM; regNum++){
      ctgMaxRate[regNum] = pPowerCtgs[regNum];
      ctgMinRate[regNum] = pPowerCtgs[regNum];
   }

   max = nCodeBits;
   min = nCodeBits;
   maxRateIndx = CAT_CONTROL_NUM;
   minRateIndx = CAT_CONTROL_NUM;

   for (j=0; j<CAT_CONTROL_NUM-1; j++) {
      if ((max + min) <= (nBitsAvailable << 1)) {
         rawMin = 99;
         /* Search for best region for high bit rate */
         for (regNum=0; regNum<REG_NUM; regNum++) {
            if (ctgMaxRate[regNum] > 0) {
               tmp0 = ctgMaxRate[regNum] << 1;
               tmp1 = offset - pRmsIndices[regNum];
               tmp0 = tmp1 - tmp0;
               if (tmp0 < rawMin) {
                  rawMin = tmp0;
                  rawMinIndx = regNum;
               }
            }
         }
         maxRateIndx--;
         tmpCtgBalances[maxRateIndx] = rawMinIndx;
        	max -= cnstExpectedBits_G722[ctgMaxRate[rawMinIndx]];
         ctgMaxRate[rawMinIndx]--;
         max += cnstExpectedBits_G722[ctgMaxRate[rawMinIndx]];
      } else {
         rawMax = -99;
         /* Search for best region for low bit rate */
         for (regNum= REG_NUM-1; regNum >= 0; regNum--) {
            if (ctgMinRate[regNum] < NUM_CATEGORIES-1){
               tmp0 = ctgMinRate[regNum]<<1;
               tmp1 = offset - pRmsIndices[regNum];
               tmp0 = tmp1 - tmp0;
               if (tmp0 > rawMax) {
                  rawMax = tmp0;
                  rawMaxIndx = regNum;
               }
            }
         }
         tmpCtgBalances[minRateIndx] = rawMaxIndx;
         minRateIndx++;
         min -= cnstExpectedBits_G722[ctgMinRate[rawMaxIndx]];
         ctgMinRate[rawMaxIndx]++;
         min += cnstExpectedBits_G722[ctgMinRate[rawMaxIndx]];
      }
   }
   for (regNum=0; regNum<REG_NUM; regNum++)
      pPowerCtgs[regNum] = ctgMaxRate[regNum];
   ippsCopy_16s(&tmpCtgBalances[maxRateIndx], pCtgBalances,
      CAT_CONTROL_NUM);
}

/*F*
//  Name:      GetPowerCategories
//  Purpose:   Calculates the category offset.  
*F*/
void GetPowerCategories(Ipp16s* pRmsIndices, Ipp16s nAccessibleBits,
         Ipp16s* pPowerCtgs, Ipp16s* pOffset){
   Ipp16s step, testOffset;
   Ipp16s i;
   Ipp16s powerCat;
   Ipp16s nBits;

   *pOffset = -32;
   step = 32;
   do {
      testOffset = (*pOffset) + step;
      nBits = 0;
      for (i=0; i<REG_NUM; i++){
         powerCat = (testOffset - pRmsIndices[i])>>1;
         powerCat = IPP_MAX(powerCat, 0);
         powerCat = IPP_MIN(powerCat, (NUM_CATEGORIES-1));
         nBits += cnstExpectedBits_G722[powerCat];
      }
      if (nBits >= nAccessibleBits - 32)
         *pOffset = testOffset;
      step >>= 1;
   } while (step > 0);

   for (i=0; i<REG_NUM; i++) {
      pPowerCtgs[i] = ((*pOffset) - pRmsIndices[i])>>1;
      pPowerCtgs[i] = IPP_MAX(pPowerCtgs[i], 0);
      pPowerCtgs[i] = IPP_MIN(pPowerCtgs[i], (NUM_CATEGORIES-1));
   }
}

/*F*
//  Name:      NormalizeWndData
//  Purpose:   Normalize input for DCT
*F*/
void NormalizeWndData(Ipp16s* pWndData, Ipp16s* pScale){
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecAbsWnd, DCT_LENGTH);
   Ipp32s enrgL1;
   Ipp16s maxWndData, tmp, scale;

   ippsAbs_16s(pWndData, vecAbsWnd, DCT_LENGTH);
   ippsMax_16s(vecAbsWnd, DCT_LENGTH, &maxWndData);
   *pScale = 0;
   scale = 0;
   if (maxWndData < 14000) { /* low amplitude input shall be shifted up*/
      tmp = maxWndData;
      if(maxWndData < 438) tmp++;
      tmp = (Ipp16s)((tmp * 9587) >> 19);
      scale = 9;
      if (tmp > 0)
         scale = Exp_16s_Pos(tmp) - 6;
   }
   ippsSum_16s32s_Sfs(vecAbsWnd, DCT_LENGTH, &enrgL1, 0);
   enrgL1 >>= 7;  /* L1 energy * 1/128 */
   if (maxWndData < enrgL1) scale--;
   if (scale > 0)
      ippsLShiftC_16s_I(scale, pWndData, DCT_LENGTH);
   else if (scale < 0)
      ippsRShiftC_16s_I(-scale, pWndData, DCT_LENGTH);
   *pScale = scale;
}
