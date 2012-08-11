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
//  Purpose: G.722.1 encoder
//
*/

#include "owng722.h"

static const Ipp16u cnstDiffRegionPowerCodes_G722[REG_NUM][DIFF_REG_POW_LEVELS] = {
{ 
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
},
{ 
   8,38,18,10, 7, 6, 3, 2, 0, 1, 7, 6, 5, 4,11,78,158,318,1278,1279,2552,2553,2554,2555
},
{
   36, 8, 3, 5, 0, 1, 7, 6, 4, 3, 2, 5, 3, 4, 5,19,74,150,302,1213,1214,1215,2424,2425
},
{
   2582,644,160,41, 5,11, 7, 5, 4, 1, 0, 6, 4, 7, 3, 6, 4,21,81,323,1290,5167,10332,10333
},
{
   2940,366,181,180,47,46,27,10, 8, 5, 1, 0, 3, 7, 4, 9,12,26,44,182,734,2941,2942,2943
},
{
   3982,7967,994,249,63,26,19,18,14, 8, 6, 1, 0, 2, 5, 7,12,30,27,125,496,1990,15932,15933
},
{
   3254,1626,407,206,202,100,30,14, 3, 5, 3, 0, 2, 4, 2,13,24,31,102,207,812,6511,13020,13021
},
{
   1110,2216,1111,139,35, 9, 3,20,11, 4, 2, 1, 3, 3, 1, 0,21, 5,16,68,276,2217,2218,2219
},
{
   1013,1014,127,62,29, 6, 4,16, 0, 1, 3, 2, 3, 1, 5, 9,17, 5,28,30,252,1015,2024,2025
},
{
   381,380,372,191,94,44,16,10, 7, 3, 1, 0, 2, 6, 9,17,45,92,187,746,1494,2991,5980,5981
},
{
   3036,758,188,45,43,10, 4, 3, 6, 4, 2, 0, 3, 7,11,20,42,44,46,95,378,3037,3038,3039
},
{
   751,92,45,20,26, 4,12, 7, 4, 0, 4, 1, 3, 5, 5, 3,27,21,44,47,186,374,1500,1501
},
{
   45572U,5697,2849,1425,357,45,23, 6,10, 7, 2, 2, 3, 0, 4, 6, 7,88,179,713,11392,45573U,45574U,45575U
},
{
   2511,5016,5018,5017,312,79,38,36,30,14, 6, 0, 2, 1, 3, 5, 8,31,37,157,626,5019,5020,5021
}};


void EncodeFrame(Ipp16s bitFrameSize, Ipp16s* pMlt, Ipp16s scale, Ipp16s *pDst){
   Ipp16s nBitsAvailable, nBitsEnvelope;
   Ipp16s ctgCtrl;
   Ipp16s i, n;
   Ipp16s absRegPowerIndices[REG_NUM];
   Ipp16s powerCategories[REG_NUM];
   Ipp16s categoryBalances[CAT_CONTROL_MAX-1];
   Ipp16s drpNumBits[REG_NUM+1];
   Ipp16u drpCodeBits[REG_NUM+1];
   Ipp32s regMltBitCounts[REG_NUM];
   Ipp32u regMltBits[4*REG_NUM];
   Ipp16s offset;

   nBitsAvailable = bitFrameSize;
   for (i=0; i<REG_NUM; i++)
      regMltBitCounts[i] = 0;
   nBitsEnvelope = GetFrameRegionsPowers(pMlt, scale,
      drpNumBits, drpCodeBits, absRegPowerIndices);
   nBitsAvailable -= nBitsEnvelope + CAT_CONTROL_BITS;
   CategorizeFrame(nBitsAvailable, absRegPowerIndices,
      powerCategories, categoryBalances);
   offset = 2 * scale + REG_POW_TABLE_NUM_NEGATIVES;
   for (i=0; i<REG_NUM; i++) {
      absRegPowerIndices[i] += offset;
      n = absRegPowerIndices[i] - 39;
      n >>= 1;
      if (n > 0){
         ippsRShiftC_16s_I(n, &pMlt[i * REG_SIZE], REG_SIZE);
         absRegPowerIndices[i] -= 2*n;
      }
   }
   MltQquantization(nBitsAvailable, pMlt, absRegPowerIndices,
      powerCategories, categoryBalances, &ctgCtrl,
      regMltBitCounts, regMltBits);
   ExpandBitsToWords(regMltBits, regMltBitCounts, drpNumBits,
      drpCodeBits, pDst, ctgCtrl, bitFrameSize);
}


void ExpandBitsToWords(Ipp32u *pRegMltBits, Ipp32s *pRegMltBitCounts,
         Ipp16s *pDrpNumBits, Ipp16u *pDrpCodeBits, Ipp16s *pDst,
         Ipp16s ctgCtrl, Ipp16s bitFrameSize){
   Ipp16s j, regNum, regBitCount;
   Ipp16s word, slice, outIndex = 0;
   Ipp16s nLeftBits, nFreeBits = 16;
   Ipp32u *pSrcWord, curWord;

   word = 0;
   pDrpNumBits[REG_NUM] = CAT_CONTROL_BITS;
   pDrpCodeBits[REG_NUM] = (Ipp16u)ctgCtrl;
   for (regNum=0; regNum<=REG_NUM; regNum++) {
      nLeftBits = pDrpNumBits[regNum];
      curWord = (Ipp32u)pDrpCodeBits[regNum];
      j = nLeftBits - nFreeBits;
      if (j >= 0) {
         word += curWord >> j;
         pDst[outIndex++] = word;
         nFreeBits = 16 - j;
         word = (Ipp16s)(curWord << nFreeBits);
      } else {
         word += (curWord << (-j));
         nFreeBits -= nLeftBits;
      }
   }
   for (regNum=0; regNum<REG_NUM; regNum++){
      if((outIndex<<4) < bitFrameSize){
         pSrcWord = &pRegMltBits[regNum<<2];
         regBitCount = pRegMltBitCounts[regNum];
         if(32 > regBitCount)
            nLeftBits = regBitCount;
         else
            nLeftBits = 32;
         curWord = *pSrcWord++;
         while ((regBitCount>0) && ((outIndex<<4)<bitFrameSize)){
            if (nLeftBits >= nFreeBits){
               slice = curWord >> (32 - nFreeBits);
               word += slice;
               curWord <<= nFreeBits;
               nLeftBits -= nFreeBits;
               pDst[outIndex++] = word;
               word = 0;
               nFreeBits = 16;
            } else {
               slice = curWord >> (32 - nLeftBits);
               word += slice << (nFreeBits - nLeftBits);
               nFreeBits -= nLeftBits;
               nLeftBits = 0;
            }
            if (nLeftBits == 0){
               curWord = *pSrcWord++;
               regBitCount -= 32;
               if(32 > regBitCount)
                  nLeftBits = regBitCount;
               else
                  nLeftBits = 32;
            }
         }
      }
   }
   while ((outIndex<<4) < bitFrameSize) {
      curWord = 0x0000ffff;
      slice = (Ipp16u)(curWord >> (16 - nFreeBits));
      word += slice;
      pDst[outIndex++] = word;
      word = 0;
      nFreeBits = 16;
   }
}


Ipp16s GetFrameRegionsPowers(Ipp16s *pSrcMlt, Ipp16s scale,
         Ipp16s *pDrpNumBits, Ipp16u *pDrpCodeBits, Ipp16s *pRegPowerIndices){
   Ipp16s* pSrc;
   int regPower;
   Ipp16s powerScale;
   Ipp16s i, j;
   Ipp16s diffRegionPowerIdx[REG_NUM];
   Ipp16s nBits;
   pSrc = pSrcMlt;
   for (i=0; i<REG_NUM; i++, pSrc+=REG_SIZE){
      ippsDotProd_16s32s_Sfs(pSrc, pSrc, REG_SIZE, &regPower, 0);
      powerScale = 0;
      while (regPower > 32767) {
         regPower >>= 1;
         powerScale++;
      }
      while ((regPower <= 32767) && (powerScale >= -15)){
         regPower <<= 1;
         powerScale--;
      }
      regPower >>= 1;
      if (regPower >= 28963) powerScale++; /* 28963 is square root of 2 times REG_SIZE(20). */
      pRegPowerIndices[i] = powerScale - (scale<<1) + 35 - REG_POW_TABLE_NUM_NEGATIVES;
   }
   for (i = REG_NUM-2; i >= 0; i--){
      if (pRegPowerIndices[i] < pRegPowerIndices[i+1] - DRP_DIFF_MAX)
         pRegPowerIndices[i] = pRegPowerIndices[i+1] - DRP_DIFF_MAX;
   }
   /* Adjust the mlts */
   if (pRegPowerIndices[0] < 1-ESF_ADJUSTMENT_TO_RMS_INDEX)
      pRegPowerIndices[0] = 1-ESF_ADJUSTMENT_TO_RMS_INDEX;
   if (pRegPowerIndices[0] > 31-ESF_ADJUSTMENT_TO_RMS_INDEX)
      pRegPowerIndices[0] = 31-ESF_ADJUSTMENT_TO_RMS_INDEX;
   diffRegionPowerIdx[0] = pRegPowerIndices[0];
   nBits = 5;
   pDrpNumBits[0] = 5;
   pDrpCodeBits[0] = (Ipp16u)(pRegPowerIndices[0] + ESF_ADJUSTMENT_TO_RMS_INDEX);
   for (i=1; i<REG_NUM; i++){
      if (pRegPowerIndices[i] < -8 - ESF_ADJUSTMENT_TO_RMS_INDEX)
         pRegPowerIndices[i] = -8 - ESF_ADJUSTMENT_TO_RMS_INDEX;
      if (pRegPowerIndices[i] > 31 - ESF_ADJUSTMENT_TO_RMS_INDEX)
         pRegPowerIndices[i] = 31 - ESF_ADJUSTMENT_TO_RMS_INDEX;
   }
   for (i=1; i<REG_NUM; i++){
      j = pRegPowerIndices[i] - pRegPowerIndices[i-1] - DRP_DIFF_MIN;
      if (j < 0) j = 0;
      diffRegionPowerIdx[i] = j;
      pRegPowerIndices[i] = pRegPowerIndices[i-1] + diffRegionPowerIdx[i] + DRP_DIFF_MIN;
      nBits += cnstDiffRegionPowerBits_G722[i][j];
      pDrpNumBits[i] = cnstDiffRegionPowerBits_G722[i][j];
      pDrpCodeBits[i] = cnstDiffRegionPowerCodes_G722[i][j];
   }
   return (nBits);
}


void MltQquantization(Ipp16s nBitsAvailable, Ipp16s* pMlt,Ipp16s* pRegPowerIndices,
         Ipp16s* pPowerCtgs, Ipp16s* pCatBalances, Ipp16s* pCtgCtrl,
         int* pRegMltBitCounts, Ipp32u* pRegMltBits){
   Ipp16s *pRegMlt;
   Ipp16s regNum, catNum;
   Ipp16s totalMltBits = 0;

   for (*pCtgCtrl = 0; *pCtgCtrl < (CAT_CONTROL_NUM/2)-1; (*pCtgCtrl)++){
      regNum = pCatBalances[*pCtgCtrl];
      pPowerCtgs[regNum]++;
   }
   for (regNum=0; regNum<REG_NUM; regNum++){
      catNum = pPowerCtgs[regNum];
      pRegMlt = &pMlt[regNum*REG_SIZE];
      if (catNum < NUM_CATEGORIES-1)
         ippsHuffmanEncode_G722_16s32u(catNum, pRegPowerIndices[regNum], pRegMlt,
            &pRegMltBits[regNum<<2], &pRegMltBitCounts[regNum]);
      else
         pRegMltBitCounts[regNum] = 0;
      totalMltBits += pRegMltBitCounts[regNum];
   }

   while ((totalMltBits<nBitsAvailable) && (*pCtgCtrl>0)){ /* few bits */
      (*pCtgCtrl)--;
      regNum = pCatBalances[*pCtgCtrl];
      pPowerCtgs[regNum]--;
      totalMltBits -= pRegMltBitCounts[regNum];
      catNum = pPowerCtgs[regNum];
      pRegMlt = &pMlt[regNum*REG_SIZE];
      if (catNum < NUM_CATEGORIES-1)
         ippsHuffmanEncode_G722_16s32u(catNum, pRegPowerIndices[regNum], pRegMlt,
            &pRegMltBits[regNum<<2], &pRegMltBitCounts[regNum]);
      else
         pRegMltBitCounts[regNum] = 0;
      totalMltBits += pRegMltBitCounts[regNum];
   }

   while ((totalMltBits>nBitsAvailable) && (*pCtgCtrl<CAT_CONTROL_NUM-1)){ /* many bits */
      regNum = pCatBalances[*pCtgCtrl];
      pPowerCtgs[regNum]++;
      totalMltBits -= pRegMltBitCounts[regNum];
      catNum = pPowerCtgs[regNum];
      pRegMlt = &pMlt[regNum*REG_SIZE];
      if (catNum < NUM_CATEGORIES-1)
         ippsHuffmanEncode_G722_16s32u(catNum, pRegPowerIndices[regNum], pRegMlt,
            &pRegMltBits[regNum<<2], &pRegMltBitCounts[regNum]);
      else
         pRegMltBitCounts[regNum] = 0;
      totalMltBits += pRegMltBitCounts[regNum];
      (*pCtgCtrl)++;
   }
}

