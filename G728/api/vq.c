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
//  Purpose: G.728 VQ functions
//
*/
#include "owng728.h"
#include "g728tables.h"

void VQ_target_vector_calc(Ipp16s *sw, Ipp16s *zir, Ipp16s *target) {
   Ipp32s k, aa0;

   for(k=0; k<IDIM; k++) {
      aa0 = sw[k] - zir[k];
      /* Clip if necessary*/ 
      target[k] = Cnvrt_32s16s(aa0);
   }
   return;
}

void Impulse_response_vec_calc(Ipp16s *a, Ipp16s *pWghtFltrCoeffs, Ipp16s *h) {
   Ipp16s temp[IDIM];
   Ipp16s ws[IDIM];
   Ipp32s k, i, aa0, aa1;
   Ipp16s *awz = pWghtFltrCoeffs;
   Ipp16s *awp = pWghtFltrCoeffs+LPCW;

   /* TEMP = synthesis filter memory */ 
   /* WS = W(z) all-pole part memory */ 

   temp[0] = 8192;
   ws[0] = 8192;

   for(k=1; k<IDIM; k++) {
      aa0 = 0;
      aa1 = 0;
      for (i=k; i>0; i--) {
         temp[i] = temp[i-1];
         ws[i] = ws[i-1];
         /* Filtering */ 
         aa0 = aa0 - a[i-1]*temp[i];
         aa1 = aa1 + awz[i-1]*temp[i];
         aa1 = aa1 - awp[i-1]*ws[i];
      }
      aa1 = aa0 + aa1;

      /* Because A[], AWZ[], AWP[] were in Q14 format */ 
      aa0 = aa0 >> 14;
      aa1 = aa1 >> 14;
      temp[i] = aa0;
      ws[i] = aa1;
      
   }
   /*Obtain H[n] by reversing the order of the memory of all-pole section of W[z]*/ 
   for(k=0; k<IDIM; k++)
      h[k] = ws[IDIM - 1 - k];
   return;
}

void Time_reversed_conv(Ipp16s *h, Ipp16s *target, Ipp16s nlstarget, Ipp16s *pn)
{
   Ipp32s k, j, aa0;

   for(k=0; k<IDIM; k++) {
      aa0 = 0;
      for(j=k; j<IDIM; j++) {
         aa0 = aa0 + target[j]*h[j-k];
      }
      aa0 = aa0 >> (13 + (nlstarget - 7));
      pn[k] = Cnvrt_32s16s(aa0);
   }
   return;
}


void VQ_target_vec_norm(Ipp16s linearGain, Ipp16s scaleGain, Ipp16s* target, Ipp16s* nlstarget){
   Ipp16s tmp, nlstmp, nls;
   Ipp32s k, aa0;

   Divide_16s(16384, 14, linearGain, scaleGain, &tmp, &nlstmp);
   for(k=0; k<IDIM; k++) {
      aa0 = tmp * target[k];
      target[k] = aa0 >> 15;
   }
   *nlstarget = 2 + (nlstmp - 15);
   VscaleFive_16s(target, target, 14, &nls);
   *nlstarget = (*nlstarget) + nls;
   return;
}


void Excitation_VQ_and_gain_scaling(Ipp16s linearGain, const Ipp16s* tbl, Ipp16s* et) {
   Ipp32s aa0, k;

   for(k=0; k<IDIM; k++) {
      aa0 = linearGain * tbl[k];
      et[k] = Cnvrt_NR_32s16s(aa0);
   }
   return;
}

