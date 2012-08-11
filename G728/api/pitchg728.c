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
//  Purpose: G.728 pitch search function
//
*/
#include "owng728.h"

void LTPCoeffsCalc(const Ipp16s *sst, Ipp32s kp, Ipp16s *gl, Ipp16s *glb, Ipp16s *pTab){
   Ipp32s aa0, aa1;
   Ipp16s den, num;
   Ipp16s nlsden, nlsnum, nlsptab, nls, b;

   /* Pitch_predictor_tab_calc*/ 
   ippsDotProd_16s32s_Sfs(sst-NPWSZ-kp, sst-NPWSZ-kp, NPWSZ,&aa0, 0);
   ippsDotProd_16s32s_Sfs(sst-NPWSZ, sst-NPWSZ-kp, NPWSZ,&aa1, 0);
   if(aa0 == 0) {
      *pTab = 0;
   } else if(aa1 <= 0) {
      *pTab = 0;
   } else {
      if(aa1 >= aa0) *pTab = 16384;
      else {
         VscaleOne_Range30_32s(&aa0, &aa0, &nlsden);
         VscaleOne_Range30_32s(&aa1, &aa1, &nlsnum);
         num = Cnvrt_NR_32s16s(aa1);
         den = Cnvrt_NR_32s16s(aa0);
         Divide_16s(num, nlsnum, den, nlsden, pTab, &nlsptab);
         *pTab = (*pTab) >> (nlsptab - 14);
      }
   }
   /* Update long-term postfilter coefficients*/ 
   if(*pTab < PPFTH) aa0 = 0;
   else              aa0 = PPFZCF * (*pTab);
   b = aa0 >> 14;
   aa0 = aa0 >> 16;
   aa0 = aa0 + 16384;
   den = aa0;
   Divide_16s(16384, 14, den, 14, gl, &nls);
   aa0 = *gl * b;
   *glb = aa0 >> nls;
   if(nls > 14) *gl = (*gl) >> (nls-14);
   return;
}


