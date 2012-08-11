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
//  Purpose: G.728 postfilter function
//
*/
#include "owng728.h"
#include "g728tables.h"

void AbsSum_G728_16s32s(Ipp16s* pSrc, Ipp32s* pAbsSum){
   Ipp32s k, sum;

   for(k=0,sum=0; k<IDIM; k++) {
      sum += Abs_32s(pSrc[k]);
   }
   *pAbsSum = sum;

   return;
}

void ScaleFactorCalc(Ipp32s sumunfil, Ipp32s sumfil, Ipp16s *pRatio, Ipp16s *pScaleRatio)
{
   Ipp32s dwDen, dwNum;
   Ipp16s den, num;
   Ipp16s scaleDen, scaleNum;

   if(sumfil > 4) {
      VscaleOne_Range30_32s(&sumfil, &dwDen, &scaleDen);
      den = Cnvrt_NR_32s16s(dwDen);

      VscaleOne_Range30_32s(&sumunfil, &dwNum, &scaleNum);
      num = Cnvrt_NR_32s16s(dwNum);

      Divide_16s(num, scaleNum, den, scaleDen, pRatio, pScaleRatio);
   } else {
      *pRatio = 16384;
      *pScaleRatio = 14;
   }

   return;
}

void FirstOrderLowpassFilter_OuputGainScaling(Ipp16s scale, Ipp16s nlsscale, 
         Ipp16s *scalefil, Ipp16s *temp, Ipp16s *spf) {
   
   Ipp32s val1, val2, k;

   val2 = AGCFAC1 * scale;
   val2 = val2 >> (nlsscale - 7);

   for(k=0; k<IDIM; k++) {
      val1 = val2 + AGCFAC*(*scalefil);
      val1 = val1 << 2;
      *scalefil = Cnvrt_NR_32s16s(val1);

      val1 = (*scalefil) * temp[k];
      val1 = val1 << 2;
      spf[k] = Cnvrt_NR_32s16s(val1);
   }
   return;
}

void STPCoeffsCalc(Ipp16s *pLPCFltrCoeffs, Ipp16s scaleLPCFltrCoeffs, Ipp16s *pstA, 
                   Ipp16s rc1, Ipp16s *tiltz)
{
   Ipp32s dwVal, i, j;
   Ipp16s ws[3];
   Ipp16s *az = pstA;
   Ipp16s *ap = pstA+10;
   Ipp16s scale = 16 - scaleLPCFltrCoeffs;
   Ipp32s ovfPos = IPP_MAX_32S >> scale;
   Ipp32s ovfNeg = IPP_MIN_32S >> scale;

   for(i=0; i<2; i++) {
      dwVal = cnstSTPostFltrPoleVector[i] * pLPCFltrCoeffs[i];
      if(dwVal > ovfPos || dwVal < ovfNeg){  /* if  Overflow in ShiftL_32s*/ 
         if(scale==2) {
            for(j=0; j<10; j++) {
               dwVal = pLPCFltrCoeffs[j] << 15;
               pLPCFltrCoeffs[j] = Cnvrt_NR_32s16s(dwVal);
            }
         }
         if(scale==1) {
            for(j=0; j<10; j++) {
               dwVal = pLPCFltrCoeffs[j] << 14;
               pLPCFltrCoeffs[j] = Cnvrt_NR_32s16s(dwVal);
            }
         }
         return;
      }
      dwVal = ShiftL_32s(dwVal, scale);
      ws[i] = Cnvrt_NR_32s16s(dwVal);
   }
   for(i=0; i<2; i++)
      ap[i] = ws[i];

   for(i=2; i<10; i++) {
      dwVal = cnstSTPostFltrPoleVector[i] * pLPCFltrCoeffs[i];
      dwVal = ShiftL_32s(dwVal, scale);
      ap[i] = Cnvrt_NR_32s16s(dwVal);
   }

   for(i=0;i<10;i++) {
      dwVal = cnstSTPostFltrZeroVector[i] * pLPCFltrCoeffs[i];
      dwVal = ShiftL_32s(dwVal, scale);
      az[i] = Cnvrt_NR_32s16s(dwVal);
   }

   dwVal = TILTF * rc1;
   *tiltz = Cnvrt_NR_32s16s(dwVal);

   if(scale==2) {
      for(j=0; j<10; j++) {
         dwVal = pLPCFltrCoeffs[j] << 15;
         pLPCFltrCoeffs[j] = Cnvrt_NR_32s16s(dwVal);
      }
   }
   if(scale==1) {
      for(j=0; j<10; j++) {
         dwVal = pLPCFltrCoeffs[j] << 14;
         pLPCFltrCoeffs[j] = Cnvrt_NR_32s16s(dwVal);
      }
   }

   return;
}

