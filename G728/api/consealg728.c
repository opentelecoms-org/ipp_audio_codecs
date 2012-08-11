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
//  Purpose: G.728 Frame or packed loss concealment function for G728 decoder
//           (Annex I)
//
*/
#include "owng728.h"
#include "g728tables.h"

#define VTH           7022         /* Voicing threshold for frame erasures*/
#define DIMINV       13107         /* Inverse of vector dimentiom (=1/5 = 0.2)*/
#define FEGAINMAX     1024         /* Maximum gain DB increase after frame erasure*/

/* LinToDb constants*/

static short c_L[6] = {
-3400, 15758, -10505, 
 9338, -9338,   9961};


#define FAC1     24660
#define GOFF_L  -16384
#define THRQTR   24576

/* On my opinion, these values can be divided into two for the best PESQ results (Igor S. Belyakov) */

static short voicedfegain[5] = {
   26214, 26214, 19661, 13107, 6554};

static short unvoicedfegain[6] = {
   32767, 32767, 26214, 19661, 13107, 6554};

static void LinToDB(Ipp16s x, Ipp16s nlsx, Ipp16s *ret);

static short Rand_G728_16s( short *seed ) /* G.723.1 */ 
{
   /* initial value 11111*/ 
   *seed = *seed * 31821 + 13849;
   return ((*seed>>9)+69);
}

static void LinToDB(Ipp16s x, Ipp16s nlsx, Ipp16s *ret) {
   Ipp32s aa0, aa1, p, i;

   aa0 = (x - THRQTR) << 1;
   aa1 = 0;
   for(i=5; i>0; i--) {
      aa1 = aa1 + c_L[i];
      p = aa1 * aa0;
      aa1 = p >> 16;
   }
   aa1 = aa1 + c_L[0];
   aa1 = aa1 >> 3;
   aa0 = 15 - nlsx;
   aa0 = aa0 << 10;
   aa1 = aa1 + aa0;
   aa1 = aa1 * FAC1;

   aa1 = aa1 >> 14;
   aa1 = aa1 - GOFF_L;
   *ret = aa1;

   return;
}

void Set_Flags_and_Scalin_Factor_for_Frame_Erasure(Ipp16s fecount, Ipp16s ptab, Ipp16s kp, Ipp16s* fedelay, Ipp16s* fescale, 
                                                   Ipp16s* nlsfescale, Ipp16s* voiced, Ipp16s* etpast, Ipp16s *avmag, Ipp16s *nlsavmag)
{
   Ipp16s n10msec, nls;
   Ipp32s aa0, i;

   n10msec = fecount >> 2;

   if(n10msec == 0) {
      if(ptab > VTH) {
         *fedelay = kp;
         *voiced = G728_TRUE;
      } else {
         *voiced = G728_FALSE;
         aa0 = 0;
         for(i = -40;i < 0; i++)
            aa0 = aa0 + Abs_32s(etpast[i]);
         VscaleOne_Range30_32s(&aa0, &aa0, &nls);
         *avmag = Cnvrt_NR_32s16s(aa0);
         *nlsavmag = nls - 16 + 3;
      }
   }
   if(*voiced == G728_TRUE) {
      if(n10msec < 5) *fescale = voicedfegain[n10msec];
      else *fescale = 0;
   } else {
      if(n10msec < 6) {
         *fescale = unvoicedfegain[n10msec];
         aa0 = (*fescale) * (*avmag);
         VscaleOne_Range30_32s(&aa0, &aa0, nlsfescale);
         *fescale = Cnvrt_NR_32s16s(aa0);
         *nlsfescale = (*nlsfescale)+(*nlsavmag) - 1;
      } else {
         *fescale = 0;
      }
   }

   return;
}

void ExcitationSignalExtrapolation(Ipp16s voiced, Ipp16s *fedelay, Ipp16s fescale, 
         Ipp16s nlsfescale, Ipp16s* etpast, Ipp16s *et, Ipp16s *nlset, Ipp16s *seed) {
   Ipp16s temp, nlstemp,den,nlsden,nls;
   Ipp32s i, aa0, aa1;

   if(voiced == G728_TRUE) {
      temp = fescale;
      nlstemp = 15;
      for(i=0; i<IDIM; i++)
         etpast[i] = etpast[i-(*fedelay)];
     /* nlstemp = 15;*/ 
   }
   if(voiced == G728_FALSE) {
      *fedelay = Rand_G728_16s(seed);
      aa1 = 0;
      for(i=0; i<IDIM; i++) {
         etpast[i] = etpast[i-(*fedelay)];
         aa1 = aa1 + Abs_32s(etpast[i]);
      }
      if((aa1==0)||(fescale==0)) {
         temp = 0;
         nlstemp = 15;
      } else {
         VscaleOne_Range30_32s(&aa1, &aa1, &nlsden);
         den = Cnvrt_NR_32s16s(aa1);
         nlsden = nlsden - 16;
         Divide_16s(fescale, nlsfescale, den, nlsden, &temp, &nlstemp);
      }
   }
   for(i=0; i<IDIM; i++) {
      aa0 = temp * etpast[i];
      aa0 = aa0 >> nlstemp;
      aa0 = Cnvrt_32s16s(aa0);
      etpast[i] = aa0;
   }
   VscaleFive_16s(etpast, et, 13, &nls);
   *nlset = nls + 2;

   return;
}

void UpdateExcitationSignal(Ipp16s *etpast, Ipp16s *et, Ipp16s nlset) {
   Ipp32s i, aa0;
   Ipp16s scale;

   for(i=-KPMAX; i<-IDIM; i++)
      etpast[i] = etpast[i+IDIM];

   scale = nlset - 2;

   if (scale >= 0) {
      for(i=0; i<IDIM; i++) {
         etpast[i-IDIM] = et[i] >> scale;
      }
   } else {
      scale = -scale;
      for(i=0; i<IDIM; i++) {
         aa0 = et[i];
         aa0 = aa0 << scale;
         etpast[i-IDIM] = Cnvrt_32s16s(aa0);
      }
   }

   return;
}


void UpdateLGPredictorState(Ipp16s *et, Ipp16s *pLGPredictorState) {
   Ipp32s aa0, k;
   Ipp16s nls;
   Ipp16s etrms;      /* Energy in dB of current vector*/ 
   Ipp16s nlsetrms;   /* NLS for etrms*/ 

   aa0 = 0;
   for(k=0; k<IDIM; k++)
      aa0 = aa0 + et[k]*et[k];

   VscaleOne_Range30_32s(&aa0, &aa0, &nls);
   etrms = aa0 >> 16;
   nlsetrms = (4 + nls) -  16;
   aa0 = etrms * DIMINV;

   VscaleOne_Range30_32s(&aa0, &aa0, &nls);
   etrms = aa0 >> 16;
   nlsetrms = nlsetrms + nls;
   if(nlsetrms > 14) {
      etrms = 16384;
      nlsetrms = 14;
   }

   LinToDB(etrms, nlsetrms, pLGPredictorState);

   return;
}

void Log_gain_Limiter_after_erasure(Ipp32s *pGainLog, Ipp32s ogaindb, Ipp16s afterfe)
{
   Ipp32s tmp;

   if(*pGainLog > 14336) *pGainLog = 14336;
   if(*pGainLog < -16384) *pGainLog = -16384;

   tmp = (*pGainLog) - ogaindb;
   if((afterfe > 0)&&(tmp > FEGAINMAX))
      *pGainLog = ogaindb + FEGAINMAX;

   return;
}



