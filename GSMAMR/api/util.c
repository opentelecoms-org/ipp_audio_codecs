/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives GSMAMR Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  GSMAMR is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: GSM AMR-NB speech codec: common utilities.
//
*/

#include "owngsmamr.h"

/****************************************************************************
 *  Function: ownVADPitchDetection_GSMAMR()
 ***************************************************************************/
void ownVADPitchDetection_GSMAMR(IppGSMAMRVad1State *st, short *pTimeVec, 
								 short *vLagCountOld, short *vLagOld)
{
   short lagcount, i;
   lagcount = 0;               
   
   for (i = 0; i < 2; i++) {
      if(abs(*vLagOld - pTimeVec[i]) < LOW_THRESHOLD) lagcount++;
      *vLagOld = pTimeVec[i];       
   }
   
   st->pitchFlag >>= 1;    
   if (*vLagCountOld + lagcount >= NUM_THRESHOLD)
      st->pitchFlag = st->pitchFlag | 0x4000;  

   *vLagCountOld = lagcount;     
}

/***************************************************************************
 *   Function: ownUpdateLTPFlag_GSMAMR()
 *************************************************************************/
void ownUpdateLTPFlag_GSMAMR(IppSpchBitRate rate, int L_Rmax, int L_R0, Ipp16s *vFlagLTP)
{
	short thresh;
	short hi1;
	short lo1;
	int Ltmp;

	if ((rate == IPP_SPCHBR_4750) || (rate == IPP_SPCHBR_5150))	thresh = 18022;		
	else if (rate == IPP_SPCHBR_10200) thresh = 19660;			
	else thresh = 21299;			

    hi1 = L_R0 >> 16;
    lo1 = (L_R0 >> 1) & 0x7fff;
	Ltmp = (hi1 * thresh +  ((lo1 * thresh) >> 15));
	if (L_Rmax > 2*Ltmp) *vFlagLTP = 1;					
	else                 *vFlagLTP = 0;

	return;
	/* End of ownUpdateLTPFlag_GSMAMR() */
}

static const short TableLog2[33] = {
     0,  1455,  2866,  4236,  5568,  6863,  8124,  9352, 
 10549, 11716, 12855, 13967, 15054, 16117, 17156, 18172, 
 19167, 20142, 21097, 22033, 22951, 23852, 24735, 25603, 
 26455, 27291, 28113, 28922, 29716, 30497, 31266, 32023, 
 32767 };

 /*************************************************************************
 * Function:   ownLog2_GSMAMR_norm
 *************************************************************************/
void ownLog2_GSMAMR_norm(int inVal, short exp, short *expPart, short *fracPart)
{
  short idx, a, tmp;
  int outVal;

  if( inVal <= 0 ) {
    *expPart = 0;
    *fracPart = 0;
    return;
  }

  *expPart = 30 - exp;
  idx = inVal >> 25;
  a = (inVal >> 10) & 0x7fff;
  idx   -=  32;

  outVal = TableLog2[idx] << 16;          
  tmp = TableLog2[idx] - TableLog2[idx+1];     
  outVal -= 2 * tmp * a;           

  *fracPart =  outVal >> 16;

  return;
  /* End of ownLog2_GSMAMR_norm() */
}
/*************************************************************************
 *  Function:  ownLog2_GSMAMR()
 *************************************************************************/
void ownLog2_GSMAMR(int inVal, short *expPart, short *fracPart)
{
  short exp;

  exp = Norm_32s_I(&inVal);
  ownLog2_GSMAMR_norm (inVal, exp, expPart, fracPart);

  return;
  /* End of ownLog2_GSMAMR() */
}

/*************************************************************************
 *  Function:  ownPow2_GSMAMR
 *************************************************************************/
static const short TablePow2[33] = {
 16384, 16743, 17109, 17484, 17867, 18258, 18658, 19066, 
 19484, 19911, 20347, 20792, 21247, 21713, 22188, 22674, 
 23170, 23678, 24196, 24726, 25268, 25821, 26386, 26964, 
 27554, 28158, 28774, 29405, 30048, 30706, 31379, 32066, 
 32767 };

int ownPow2_GSMAMR(short expPart, short fracPart)
{
  short exp, idx, a, tmp;
  int inVal, outVal, temp;

  exp = 30 - expPart;
  if (exp > 31) return 0;
  idx = fracPart >> 10;
  a   = (short)((fracPart << 5) & 0x7fff);

  inVal = TablePow2[idx] << 16;             
  tmp = TablePow2[idx] - TablePow2[idx+1];
  inVal = inVal - 2*tmp*a;

  if (exp >= 31) outVal = (inVal < 0) ? -1 : 0;
  else           outVal = inVal >> exp;
  
  temp = 1 << (exp-1);
  if(inVal & temp) outVal++;
  
  return(outVal);
  /* End of ownPow2_GSMAMR() */
}

/* table[i] = sqrt((i+16)*2^-6) * 2^15, i.e. sqrt(x) scaled Q15 */
static const short TableSqrt[49] =
{
 16384, 16888, 17378, 17854, 18318, 18770, 19212, 19644, 
 20066, 20480, 20886, 21283, 21674, 22058, 22435, 22806, 
 23170, 23530, 23884, 24232, 24576, 24915, 25249, 25580, 
 25905, 26227, 26545, 26859, 27170, 27477, 27780, 28081, 
 28378, 28672, 28963, 29251, 29537, 29819, 30099, 30377, 
 30652, 30924, 31194, 31462, 31727, 31991, 32252, 32511, 
 32767};

/********************************************************************************
 *  Function: ownSqrt_Exp_GSMAMR()
 ********************************************************************************/

int ownSqrt_Exp_GSMAMR (int inVal, short *exp)
{
    short idx, a, tmp;
    int outVal;

    if (inVal <= 0) { *exp = 0; return 0; }

    outVal = inVal;
    *exp = Norm_32s_I(&outVal) & 0xFFFE;    
    inVal <<= *exp;                       

	idx = inVal >> 25;
	a = (inVal >> 10) & 0x7fff;

    idx -= 16;                                                
    outVal = TableSqrt[idx] << 16;             
    tmp = TableSqrt[idx] - TableSqrt[idx+1];
    outVal = outVal - 2*tmp*a;

    return (outVal);
	/* End of ownSqrt_Exp_GSMAMR() */
}

/*************************************************************************
 *  Function: Reorder_lsf()
 *************************************************************************/
void ownReorderLSFVec_GSMAMR(short *lsf, short minDistance, short len)
{
    int i;
    short lsf_min;

    lsf_min = minDistance;         

    for (i = 0; i < len; i++)  {
        if(lsf[i] < lsf_min) lsf[i] = lsf_min;
        lsf_min = lsf[i] + minDistance;
    }
}
/*************************************************************************
 *  Function: ownGetMedianElements_GSMAMR() 
 *************************************************************************/
short ownGetMedianElements_GSMAMR (short *pPastGainVal, short num)
{
    short i, j, idx = 0;
    short max;
    short medianIndex;
    short tmp[MAX_MED_SIZE];
    short tmp2[MAX_MED_SIZE];

    ippsCopy_16s(pPastGainVal, tmp2, num);
    
	for (i = 0; i < num; i++) {
        max = -32767;                                          
        for (j = 0; j < num; j++)  {
            if (tmp2[j] >= max) {
                max = tmp2[j];                                
                idx = j;                                       
            }
        }
        tmp2[idx] = -32768;                                     
        tmp[i] = idx;                                           
    }
    medianIndex = tmp[num >> 1]; 

    return (pPastGainVal[medianIndex]);                                     
}
/**************************************************************************
*  Proc: ownCtrlDetectBackgroundNoise_GSMAMR()  - 
***************************************************************************/
short ownCtrlDetectBackgroundNoise_GSMAMR (short excitation[], short excEnergy, short exEnergyHist[], 
                                           short vVoiceHangover, short prevBFI, short carefulFlag)
{
   short exp;
   short testEnergy, scaleFactor, avgEnergy, prevEnergy;
   int t0;

   avgEnergy = ownGetMedianElements_GSMAMR(exEnergyHist, 9);                    
   prevEnergy = (exEnergyHist[7] + exEnergyHist[8]) >> 1;

   if( exEnergyHist[8] < prevEnergy) prevEnergy = exEnergyHist[8];

   if( excEnergy < avgEnergy && excEnergy > 5) {
      testEnergy = prevEnergy << 2;  

      if( vVoiceHangover < 7 || prevBFI != 0 ) testEnergy -= prevEnergy;  
      if( avgEnergy > testEnergy) avgEnergy = testEnergy;         
      
      exp = 0;
      for(; excEnergy < 0x4000; exp++) excEnergy <<= 1;
      excEnergy = (16383<<15)/excEnergy;
      t0 = avgEnergy * excEnergy;
      t0 >>= 19 - exp;  
      if(t0 > 32767)  t0 = 32767; 
      scaleFactor = (short)t0; 

      if((carefulFlag != 0) && (scaleFactor > 3072))  scaleFactor = 3072;              

      ippsMulC_16s_ISfs(scaleFactor, excitation, SUBFR_SIZE_GSMAMR, 10);
   }

   return 0;
}
/*************************************************************************
 *  Function: ownComputeCodebookGain_GSMAMR
 *************************************************************************/
short ownComputeCodebookGain_GSMAMR (short *pTargetVec, short *pFltVec)
{
    short i;
    short xy, yy, exp_xy, exp_yy, gain;
    IPP_ALIGNED_ARRAY(16, short, pFltVecScale, SUBFR_SIZE_GSMAMR);
    int s;

    ippsRShiftC_16s(pFltVec, 1, pFltVecScale, SUBFR_SIZE_GSMAMR);
    ippsDotProd_16s32s_Sfs(pTargetVec, pFltVecScale, SUBFR_SIZE_GSMAMR, &s, 0);

    if(s == 0) s = 1;
    s <<= 1;
    exp_xy = Norm_32s_I(&s);
    xy = (short)(s >> 16);
    if(xy <= 0) return ((short) 0);

    ippsDotProd_16s32s_Sfs(pFltVecScale, pFltVecScale, SUBFR_SIZE_GSMAMR, &s, -1);
    
	exp_yy = Norm_32s_I(&s);
    yy = (short)(s >> 16);
    xy >>= 1;  
    gain = (yy > 0)? (xy<<15)/yy : IPP_MAX_16S;

    i = exp_xy + 5;              
    i -= exp_yy;
    gain = (gain >> i) << 1;    
    
    return (gain);
}
/*************************************************************************
 *  Function: ownGainAdaptAlpha_GSMAMR
 *************************************************************************/
void ownGainAdaptAlpha_GSMAMR(short *vOnSetQntGain, short *vPrevAdaptOut, short *vPrevGainZero,       
                              short *a_LTPHistoryGain, short ltpg, short gainCode, short *alpha)
{
    short adapt;     
    short result;    
    short filt;      
    short tmp, i;
    int temp;
    
    if (ltpg <= LTP_GAIN_LOG10_1) adapt = 0;                            
    else if (ltpg <= LTP_GAIN_LOG10_2) adapt = 1;                        
    else adapt = 2;                        

    tmp = gainCode >> 1;
    if (gainCode & 1) tmp++;

    if (tmp > *vPrevGainZero && gainCode > 200) *vOnSetQntGain = 8;                            
    else if (*vOnSetQntGain != 0) *vOnSetQntGain -= 1;      

    if ((*vOnSetQntGain != 0) && (adapt < 2)) adapt += 1;

    a_LTPHistoryGain[0] = ltpg;                       
    filt = ownGetMedianElements_GSMAMR(a_LTPHistoryGain, 5);    

    if(adapt == 0) {
       if (filt > 5443)   result = 0; 
       else if (filt < 0)  result = 16384;   
       else {   
           temp = (24660 * filt)>>13;
           result = 16384 - temp;
       }
    } else result = 0; 

    if (*vPrevAdaptOut == 0) result >>= 1;
    *alpha = result;                           
    
    *vPrevAdaptOut = result;                   
    *vPrevGainZero = gainCode;                    

    for (i = NUM_MEM_LTPG-1; i > 0; i--)
        a_LTPHistoryGain[i] = a_LTPHistoryGain[i-1];   

}
/**************************************************************************
*  Function: ownCBGainAverage_GSMAMR()
**************************************************************************/
short ownCBGainAverage_GSMAMR(short *a_GainHistory, short *vHgAverageVar, short *vHgAverageCount,       
                              IppSpchBitRate rate, short gainCode, short *lsp, short *lspAver,         
                              short badFrame, short vPrevBadFr, short potDegBadFrame, short vPrevDegBadFr,    
                              short vBackgroundNoise, short vVoiceHangover)
{
   short i;
   short cbGainMix, diff, bgMix, cbGainMean;
   int L_sum;
   short tmp[LP_ORDER_SIZE], tmp1, tmp2, shift1, shift2, shift;

   cbGainMix = gainCode;             
   for (i = 0; i < (CBGAIN_HIST_SIZE-1); i++)
      a_GainHistory[i] = a_GainHistory[i+1];    

   a_GainHistory[CBGAIN_HIST_SIZE-1] = gainCode;       
   
   for (i = 0; i < LP_ORDER_SIZE; i++) {
      tmp1 = abs(lspAver[i] - lsp[i]);  
      shift1 = Exp_16s(tmp1) - 1;          
      tmp1 <<= shift1;               
      shift2 = Exp_16s(lspAver[i]);            
      tmp2 = lspAver[i] << shift2;         
	  tmp[i] = (tmp2>0)? (tmp1<<15)/tmp2 : IPP_MAX_16S;            
                                              
      shift = shift1 + 2 - shift2;
      if(shift >= 0) tmp[i] >>= shift;         
      else           tmp[i] <<= -shift; 
   }
   
   ippsSum_16s_Sfs(tmp, LP_ORDER_SIZE, &diff, 0);

   if(diff > 5325) {
      *vHgAverageVar = *vHgAverageVar + 1;
      if(*vHgAverageVar > 10) *vHgAverageCount = 0;  
   } else *vHgAverageVar = 0;     

   /* Compute mix constant (bgMix) */   
   bgMix = 8192;    /* 1 in Q13 */    
   if ((rate <= IPP_SPCHBR_6700) || (rate == IPP_SPCHBR_10200))  
   {
      /* if errors and presumed noise make smoothing probability stronger */
      if (((((potDegBadFrame != 0) && (vPrevDegBadFr != 0)) || (badFrame != 0) || (vPrevBadFr != 0)) &&
          (vVoiceHangover > 1) && (vBackgroundNoise != 0) && 
          ((rate == IPP_SPCHBR_4750) || (rate == IPP_SPCHBR_5150) || (rate == IPP_SPCHBR_5900)) ))
      {
         if (diff > 4506) {
            if (6554 < diff) bgMix = 8192;          
            else             bgMix = (diff - 4506) << 2;
         }
         else bgMix = 0;              
      } else {
         if (diff > 3277) {
            if(5325 < diff)  bgMix = 8192;           
            else             bgMix = (diff - 3277) << 2;
         }
         else bgMix = 0;               
      }

      if((*vHgAverageCount < 40) || (diff > 5325)) bgMix = 8192;  

      ippsSum_16s32s_Sfs(a_GainHistory+2, CBGAIN_HIST_SIZE-2, &L_sum, 0);
      L_sum*=6554;
      cbGainMean = (short)((L_sum + 0x4000) >> 15);    
      
      if (((badFrame != 0) || (vPrevBadFr != 0)) && (vBackgroundNoise != 0) &&
          ((rate == IPP_SPCHBR_4750) ||
           (rate == IPP_SPCHBR_5150) ||
           (rate == IPP_SPCHBR_5900)) )
      {
         ippsSum_16s32s_Sfs(a_GainHistory, CBGAIN_HIST_SIZE, &L_sum, 0);
         L_sum*=4681;
         cbGainMean = (short)((L_sum + 0x4000) >> 15); 
      }
      
      L_sum = bgMix*cbGainMix;              
      L_sum += 8192*cbGainMean;         
      L_sum -= bgMix*cbGainMean;
      cbGainMix = (short)((L_sum + 0x1000) >> 13);   
   }
   
   *vHgAverageCount = *vHgAverageCount + 1;
   return cbGainMix;
}
/***************************************************************************
 *  Function:  ownCheckLSPVec_GSMAMR()                                                     *
 ***************************************************************************/
short ownCheckLSPVec_GSMAMR(short *count, short *lsp)
{
   short i, dist, dist_min1, dist_min2, dist_thresh;
  
   dist_min1 = IPP_MAX_16S;                       
   for (i = 3; i < LP_ORDER_SIZE-2; i++) {
      dist = lsp[i] - lsp[i+1];
      if(dist < dist_min1) dist_min1 = dist;                   
   }

   dist_min2 = IPP_MAX_16S;                       
   for (i = 1; i < 3; i++) {
      dist = lsp[i] - lsp[i+1];
      if (dist < dist_min2) dist_min2 = dist;                   
   }

   if(lsp[1] > 32000) dist_thresh = 600;                         
   else if (lsp[1] > 30500) dist_thresh = 800;                         
   else dist_thresh = 1100;                        

   if (dist_min1 < 1500 || dist_min2 < dist_thresh) *count = *count + 1;
   else                                             *count = 0;                         

   if (*count >= 12) {
      *count = 12;                      
      return 1;
   } else return 0;
   /* End of ownCheckLSPVec_GSMAMR() */
}
/***************************************************************************
 *  Function:  ownSubframePostProc_GSMAMR()                                 
 ***************************************************************************/
int ownSubframePostProc_GSMAMR(short *pSpeechVec, IppSpchBitRate rate, short numSubfr,         
                               short gainPitch, short gainCode, short *pAQnt, short *pLocSynth,       
                               short *pTargetPitchVec, short *code, short *pFltAdaptExc, short *pFltVec,         
                               short *a_MemorySyn, short *a_MemoryErr, short *a_Memory_W0, short *pLTPExc,         
                               short *vFlagSharp)
{
   short i, j, k;
   short temp;
   int TmpVal;
   short tempShift;
   short kShift;
   short pitchFactor;

   if (rate != IPP_SPCHBR_12200) {
      tempShift = 1;                     
      kShift = 2;                        
      pitchFactor = gainPitch;              
   } else {
      tempShift = 2;                     
      kShift = 4;                        
      pitchFactor = gainPitch >> 1;
   }
      
   *vFlagSharp = gainPitch;                   
   if(*vFlagSharp > PITCH_SHARP_MAX) *vFlagSharp = PITCH_SHARP_MAX;                 

   for (i = 0; i < SUBFR_SIZE_GSMAMR; i++) {
      TmpVal = (pLTPExc[i + numSubfr]*pitchFactor) + (code[i]*gainCode);
      TmpVal <<= tempShift;
      pLTPExc[i + numSubfr] = (short)((TmpVal + 0x4000) >> 15);   
   }

   ippsSynthesisFilter_NR_16s_Sfs(pAQnt, &pLTPExc[numSubfr], &pLocSynth[numSubfr], SUBFR_SIZE_GSMAMR, 12, a_MemorySyn);
   ippsCopy_16s(&pLocSynth[numSubfr + SUBFR_SIZE_GSMAMR - LP_ORDER_SIZE], a_MemorySyn, LP_ORDER_SIZE);
   
   for (i = SUBFR_SIZE_GSMAMR - LP_ORDER_SIZE, j = 0; i < SUBFR_SIZE_GSMAMR; i++, j++) {
      a_MemoryErr[j] = pSpeechVec[numSubfr + i] - pLocSynth[numSubfr + i];         
      temp = (short)((pFltAdaptExc[i]*gainPitch) >> 14);
      k = (short)((pFltVec[i]*gainCode) >> (15 - kShift));
      a_Memory_W0[j] = pTargetPitchVec[i] - temp - k;          
   }

   return 0;
   /* End of ownSubframePostProc_GSMAMR() */
}

static const short TableInterN6[LEN_FIRFLT] =
{
    29443, 28346, 25207, 20449, 14701,  8693, 3143, -1352, -4402, -5865,
    -5850, -4673, -2783,  -672,  1211,  2536, 3130,  2991,  2259,  1170,
        0, -1001, -1652, -1868, -1666, -1147, -464,   218,   756,  1060,
     1099,   904,   550,   135,  -245,  -514, -634,  -602,  -451,  -231,
        0,   191,   308,   340,   296,   198,   78,   -36,  -120,  -163,
     -165,  -132,   -79,   -19,    34,    73,   91,    89,    70,    38, 
     0
};

/*************************************************************************
 *  Function:   ownPredExcMode3_6_GSMAMR()
 *************************************************************************/
void ownPredExcMode3_6_GSMAMR(short *pLTPExc, short T0, short frac, short lenSubfr, short flag3)
{
    short i, j, k;
    short *x0, *x1, *x2;
    const short *c1, *c2;
    int s;

    x0 = &pLTPExc[-T0];             

    frac = -frac;
    if(flag3 != 0) frac <<= 1;   

    if (frac < 0) {
        frac += MAX_UPSAMPLING;
        x0--;
    }

    for (j = 0; j < lenSubfr; j++) {
        x1 = x0++;              
        x2 = x0;                
        c1 = &TableInterN6[frac];
        c2 = &TableInterN6[MAX_UPSAMPLING - frac];

        s = 0;                  
        for (i = 0, k = 0; i < LEN_INTERPOL_10; i++, k += MAX_UPSAMPLING) {
            s += x1[-i]*c1[k];
            s += x2[i]*c2[k];
        }
        pLTPExc[j] = (short)((s + 0x4000) >> 15);   
    }

    return;
	/* End of ownPredExcMode3_6_GSMAMR() */
}

static const short TblPhImpLow_M795[] = 
{
  26777,    801,   2505,   -683,  -1382,    582,    604,  -1274,
   3511,  -5894,   4534,   -499,  -1940,   3011,  -5058,   5614,
  -1990,  -1061,  -1459,   4442,   -700,  -5335,   4609,    452,
   -589,  -3352,   2953,   1267,  -1212,  -2590,   1731,   3670,
  -4475,   -975,   4391,  -2537,    949,  -1363,   -979,   5734,
  26777,    801,   2505,   -683,  -1382,    582,    604,  -1274,
   3511,  -5894,   4534,   -499,  -1940,   3011,  -5058,   5614,
  -1990,  -1061,  -1459,   4442,   -700,  -5335,   4609,    452,
   -589,  -3352,   2953,   1267,  -1212,  -2590,   1731,   3670,
   -4475,   -975,   4391,  -2537,    949,  -1363,   -979,   5734
};

static const short TblPhImpLow[] =
{
  14690,  11518,   1268,  -2761,  -5671,   7514,    -35,  -2807,
  -3040,   4823,   2952,  -8424,   3785,   1455,   2179,  -8637,
   8051,  -2103,  -1454,    777,   1108,  -2385,   2254,   -363,
   -674,  -2103,   6046,  -5681,   1072,   3123,  -5058,   5312,
  -2329,  -3728,   6924,  -3889,    675,  -1775,     29,  10145,
  14690,  11518,   1268,  -2761,  -5671,   7514,    -35,  -2807,
  -3040,   4823,   2952,  -8424,   3785,   1455,   2179,  -8637,
   8051,  -2103,  -1454,    777,   1108,  -2385,   2254,   -363,
   -674,  -2103,   6046,  -5681,   1072,   3123,  -5058,   5312,
  -2329,  -3728,   6924,  -3889,    675,  -1775,     29,  10145
};
static const short TblPhImpMid[] =
{
  30274,   3831,  -4036,   2972,  -1048,  -1002,   2477,  -3043,
   2815,  -2231,   1753,  -1611,   1714,  -1775,   1543,  -1008,
    429,   -169,    472,  -1264,   2176,  -2706,   2523,  -1621,
    344,    826,  -1529,   1724,  -1657,   1701,  -2063,   2644,
  -3060,   2897,  -1978,    557,    780,  -1369,    842,    655,
  30274,   3831,  -4036,   2972,  -1048,  -1002,   2477,  -3043,
   2815,  -2231,   1753,  -1611,   1714,  -1775,   1543,  -1008,
    429,   -169,    472,  -1264,   2176,  -2706,   2523,  -1621,
    344,    826,  -1529,   1724,  -1657,   1701,  -2063,   2644,
  -3060,   2897,  -1978,    557,    780,  -1369,    842,    655
};

/*************************************************************************
*  Proc:   ownPhaseDispersion_GSMAMR
**************************************************************************/
void ownPhaseDispersion_GSMAMR(sPhaseDispSt *state, IppSpchBitRate rate, short *pLTPExcSignal,
                               short cbGain, short ltpGain, short *innovVec, short pitchFactor,   
                               short tmpShift)
{
   short i, i1;
   short tmp1;
   short impNr;           

   IPP_ALIGNED_ARRAY(16, short, inno_sav, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, short, ps_poss, SUBFR_SIZE_GSMAMR);
   short nze, nPulse;
   const short *ph_imp;   

   for(i = LTP_GAIN_MEM_SIZE-1; i > 0; i--)
       state->a_GainMemory[i] = state->a_GainMemory[i-1];                   

   state->a_GainMemory[0] = ltpGain;                                    
   
   if(ltpGain < LTP_THRESH2) {    /* if (ltpGain < 0.9) */
       if(ltpGain > LTP_THRESH1) impNr = 1; 
       else                      impNr = 0; 
   } else impNr = 2; 
   
   tmp1 = ShiftL_16s(state->vPrevGain, 1);
   if(cbGain > tmp1) state->vOnSetGain = 2;                                   
   else {
      if (state->vOnSetGain > 0) state->vOnSetGain -= 1;                   
   }
   
   if(state->vOnSetGain == 0) {
       i1 = 0;                                                     
       for(i = 0; i < LTP_GAIN_MEM_SIZE; i++) {
           if(state->a_GainMemory[i] < LTP_THRESH1) i1 ++;
       }
       if(i1 > 2) impNr = 0;                                              
   }

   if((impNr > state->vPrevState + 1) && (state->vOnSetGain == 0)) impNr -= 1;
   
   if((impNr < 2) && (state->vOnSetGain > 0)) impNr += 1;
   
   if(cbGain < 10) impNr = 2;                                                 
   
   if(state->vFlagLockGain == 1) impNr = 0;                                                  

   state->vPrevState = impNr;                                       
   state->vPrevGain = cbGain;                                     
  
   if((rate != IPP_SPCHBR_12200) && (rate != IPP_SPCHBR_10200) && (rate != IPP_SPCHBR_7400) && (impNr < 2)) {
       nze = 0;                                                    
       for (i = 0; i < SUBFR_SIZE_GSMAMR; i++) {
           if (innovVec[i] != 0) {
               ps_poss[nze] = i;                                  
               nze++;
           }
           inno_sav[i] = innovVec[i];
           innovVec[i] = 0;
       }

       if(rate == IPP_SPCHBR_7950) {
          if(impNr == 0) ph_imp = TblPhImpLow_M795;                            
          else           ph_imp = TblPhImpMid;                           
       } else {
          if(impNr == 0) ph_imp = TblPhImpLow;                                  
          else           ph_imp = TblPhImpMid;                                  
       }
       
       for (nPulse = 0; nPulse < nze; nPulse++) {
           short amp = inno_sav[ps_poss[nPulse]];
           const short *posImp = &ph_imp[SUBFR_SIZE_GSMAMR-ps_poss[nPulse]]; 
        
           /*ippsMulC_16s_Sfs(&ph_imp[SUBFR_SIZE_GSMAMR-ps_poss[nPulse]],inno_sav[ps_poss[nPulse]],tmpV,L_SUBFR,15);
           ippsAdd_16s_I(tmpV,innovVec,SUBFR_SIZE_GSMAMR);*/
           for(i = 0; i < SUBFR_SIZE_GSMAMR; i++)
              innovVec[i] += (amp * posImp[i]) >> 15;
       }
   }
       
   ippsInterpolateC_NR_G729_16s_Sfs(innovVec, cbGain, pLTPExcSignal, pitchFactor, 
	                                pLTPExcSignal, SUBFR_SIZE_GSMAMR, (15-tmpShift));
   return;
   /* End of ownPhaseDispersion_GSMAMR() */
}
/*************************************************************************
*  Function    : ownSourceChDetectorUpdate_GSMAMR
**************************************************************************/
short ownSourceChDetectorUpdate_GSMAMR(short *a_EnergyHistVector, short *vCountHangover,  
                                       short *ltpGainHist, short *pSpeechVec, short *vVoiceHangover)
{
   short i;
   short prevVoiced, inbgNoise;
   short temp;
   short ltpLimit, frameEnergyMin;
   short currEnergy, noiseFloor, maxEnergy, maxEnergyLastPart;
   int s;
   
   currEnergy = 0;
   s =  0;

   ippsDotProd_16s32s_Sfs(pSpeechVec, pSpeechVec, FRAME_SIZE_GSMAMR, &s, 0);
   currEnergy = (short)(s >> 13);
   frameEnergyMin = 32767;

   for(i = 0; i < ENERGY_HIST_SIZE; i++) {
      if(a_EnergyHistVector[i] < frameEnergyMin)
         frameEnergyMin = a_EnergyHistVector[i];
   }

   noiseFloor = (short)Mul16_32s(frameEnergyMin); 
   maxEnergy = a_EnergyHistVector[0];

   for(i = 1; i < ENERGY_HIST_SIZE-4; i++) {
      if(maxEnergy < a_EnergyHistVector[i])
         maxEnergy = a_EnergyHistVector[i];
   }
   
   maxEnergyLastPart = a_EnergyHistVector[2*ENERGY_HIST_SIZE/3];
   for(i = 2*ENERGY_HIST_SIZE/3+1; i < ENERGY_HIST_SIZE; i++) {
      if(maxEnergyLastPart < a_EnergyHistVector[i])
         maxEnergyLastPart = a_EnergyHistVector[i];  
   }

   inbgNoise = 0;               
   if((maxEnergy > LOW_NOISE_LIMIT) &&
      (currEnergy < THRESH_ENERGY_LIMIT) &&
      (currEnergy > LOW_NOISE_LIMIT) &&
      ((currEnergy < noiseFloor) || (maxEnergyLastPart < UPP_NOISE_LIMIT)))
   {
      if (*vCountHangover + 1 > 30) *vCountHangover = 30;
      else                          *vCountHangover = *vCountHangover + 1;
   } else {
	  *vCountHangover = 0;    
   }

   if(*vCountHangover > 1) inbgNoise = 1;       

   for (i = 0; i < ENERGY_HIST_SIZE-1; i++)
      a_EnergyHistVector[i] = a_EnergyHistVector[i+1];

   a_EnergyHistVector[ENERGY_HIST_SIZE-1] = currEnergy;
   
   ltpLimit = 13926;             
   if(*vCountHangover > 8)  ltpLimit = 15565;          
   if(*vCountHangover > 15) ltpLimit = 16383;         

   prevVoiced = 0;       
   if(ownGetMedianElements_GSMAMR(&ltpGainHist[4], 5) > ltpLimit) prevVoiced = 1;      
   if(*vCountHangover > 20) {
      if(ownGetMedianElements_GSMAMR(ltpGainHist, 9) > ltpLimit)  prevVoiced = 1;  
      else                                                        prevVoiced = 0; 
   }
   
   if(prevVoiced) *vVoiceHangover = 0;                      
   else {
      temp = *vVoiceHangover + 1;
      if(temp > 10) *vVoiceHangover = 10;                   
      else          *vVoiceHangover = temp;                
   }

   return inbgNoise;
   /* End of ownSourceChDetectorUpdate_GSMAMR() */
}
/*************************************************************************
 Function: ownCalcUnFiltEnergy_GSMAMR
 *************************************************************************/
void ownCalcUnFiltEnergy_GSMAMR(short *pLPResVec, short *pLTPExc, short *code, short gainPitch,    
                                short lenSubfr, short *fracEnergyCoeff, short *expEnergyCoeff, short *ltpg)
{
    int s, TmpVal;
    short i, exp, tmp;
    short ltp_res_en, pred_gain;
    short ltpg_exp, ltpg_frac;

    ippsDotProd_16s32s_Sfs(pLPResVec, pLPResVec, lenSubfr, &s, -1);

    if (s < 400) {
        fracEnergyCoeff[0] = 0;                      
        expEnergyCoeff[0] = -15;                     
    } else {
        exp = Norm_32s_I(&s);
        fracEnergyCoeff[0] = (short)(s >> 16);  
        expEnergyCoeff[0] = 15 - exp;               
    }
    
    ippsDotProd_16s32s_Sfs(pLTPExc, pLTPExc, lenSubfr, &s, -1);
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[1] = (short)(s >> 16);  
    expEnergyCoeff[1] = 15 - exp;                

    ippsDotProd_16s32s_Sfs(pLTPExc, code, lenSubfr, &s, -1);
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[2] = (short)(s >> 16);  
    expEnergyCoeff[2] = 2 - exp;             

    s = 0;                                  
    for (i = 0; i < lenSubfr; i++) {
        TmpVal = pLTPExc[i] * gainPitch;
        tmp = pLPResVec[i] - (short)((TmpVal + 0x2000) >> 14);           /* LTP residual, Q0 */
        s += tmp * tmp;
    }
    exp = Norm_32s_I(&s);
    ltp_res_en = (short)(s >> 16);  
    exp = 16 - exp;

    fracEnergyCoeff[3] = ltp_res_en;                 
    expEnergyCoeff[3] = exp;                         
    
    if(ltp_res_en > 0 && fracEnergyCoeff[0] != 0) {
        pred_gain = ((fracEnergyCoeff[0] >> 1) << 15) / ltp_res_en;
        exp -= expEnergyCoeff[0];
        TmpVal = pred_gain << 16;

        if(exp < -3) ShiftL_32s(TmpVal, -(exp + 3));
        else         TmpVal >>= (exp + 3);

        ownLog2_GSMAMR(TmpVal, &ltpg_exp, &ltpg_frac);
        TmpVal = ((ltpg_exp - 27) << 15) + ltpg_frac;
        *ltpg = Cnvrt_NR_32s16s(TmpVal << 14);  
    }
    else *ltpg = 0;                     
   
	return;
	/* End of ownCalcUnFiltEnergy_GSMAMR() */
}

/*************************************************************************
 * Function: ownCalcFiltEnergy_GSMAMR
 *************************************************************************/
void ownCalcFiltEnergy_GSMAMR(IppSpchBitRate rate, short *pTargetPitchVec, short *pTargetVec,         
                              short *pFltAdaptExc, short *pFltVec, short *fracEnergyCoeff,    
                              short *expEnergyCoeff, short *optFracCodeGain, short *optExpCodeGain)
{
    short g_coeff[4];          
    int s, ener_init=1;
    short exp, frac;
    short pFltVecTmp[SUBFR_SIZE_GSMAMR];
    short gain;

    if(rate == IPP_SPCHBR_7950 || rate == IPP_SPCHBR_4750) ener_init = 0; 

    ippsRShiftC_16s(pFltVec, 3, pFltVecTmp, SUBFR_SIZE_GSMAMR);
    ippsAdaptiveCodebookGainCoeffs_GSMAMR_16s(pTargetPitchVec, pFltAdaptExc, &gain, g_coeff);

    fracEnergyCoeff[0] = g_coeff[0];          
    expEnergyCoeff[0]  = g_coeff[1];           
    fracEnergyCoeff[1] = (g_coeff[2] == IPP_MIN_16S) ? IPP_MAX_16S : -g_coeff[2];/* coeff[1] = -2 xn y1 */
    expEnergyCoeff[1]  = g_coeff[3] + 1;   

    ippsDotProd_16s32s_Sfs(pFltVecTmp, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
    s += ener_init;
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[2] = (short)(s >> 16);
    expEnergyCoeff[2] = -3 - exp;

    ippsDotProd_16s32s_Sfs(pTargetPitchVec, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
    s += ener_init;
    exp = Norm_32s_I(&s);
    s >>= 16;
    fracEnergyCoeff[3] = (s != IPP_MIN_16S)? -s : IPP_MAX_16S;
    expEnergyCoeff[3] = 7 - exp;

    ippsDotProd_16s32s_Sfs(pFltAdaptExc, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
    s += ener_init;
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[4] = (short)(s >> 16);
    expEnergyCoeff[4] = 7 - exp;

    if(rate == IPP_SPCHBR_4750 || rate == IPP_SPCHBR_7950) {
        ippsDotProd_16s32s_Sfs( pTargetVec, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
        s += ener_init;
        exp = Norm_32s_I(&s);
        frac = (short)(s >> 16);
        exp = 6 - exp;
        
        if (frac <= 0) {
            *optFracCodeGain = 0; 
            *optExpCodeGain = 0;  
        } else {
            *optFracCodeGain = ((frac >> 1) << 15) / fracEnergyCoeff[2];
            *optExpCodeGain = exp - expEnergyCoeff[2] - 14;
        }
    }
	return;
	/* End of ownCalcFiltEnergy_GSMAMR() */
}

/*************************************************************************
 * Function: ownCalcTargetEnergy_GSMAMR
 *************************************************************************/
void ownCalcTargetEnergy_GSMAMR(short *pTargetPitchVec, short *optExpCodeGain, short *optFracCodeGain)
{
    int s;
    short exp;

    ippsDotProd_16s32s_Sfs(pTargetPitchVec, pTargetPitchVec, SUBFR_SIZE_GSMAMR, &s, -1);
    exp = Norm_32s_I(&s);
    *optFracCodeGain = (short)(s >> 16);  
    *optExpCodeGain = 16 - exp;

	return;
	/* End of ownCalcTargetEnergy_GSMAMR() */
}
/**************************************************************************
*  Function:    : A_Refl
*  Convert from direct form coefficients to reflection coefficients
**************************************************************************/ 
void ownConvertDirectCoeffToReflCoeff_GSMAMR(short *pDirectCoeff, short *pReflCoeff)
{

   short i,j;
   short aState[LP_ORDER_SIZE];
   short bState[LP_ORDER_SIZE];
   short normShift, normProd;
   short scale, temp, mult;
   int const1;
   int TmpVal, TmpVal1, TmpVal2;

   ippsCopy_16s(pDirectCoeff, aState, LP_ORDER_SIZE);
   for (i = LP_ORDER_SIZE-1; i >= 0; i--) {
      if(Abs_16s(aState[i]) >= 4096) goto ExitRefl;

      pReflCoeff[i] = aState[i] << 3;   
      TmpVal1 = 2 * pReflCoeff[i] * pReflCoeff[i];
      TmpVal = IPP_MAX_32S - TmpVal1;
      normShift = Norm_32s_Pos_I(&TmpVal); 
      scale = 14 - normShift; 
      normProd = Cnvrt_NR_32s16s(TmpVal);
      mult = (16384 << 15) / normProd;
      const1 = 1<<(scale - 1);

      for (j = 0; j < i; j++) {
         TmpVal = (aState[j] << 15) - (pReflCoeff[i] * aState[i-j-1]);         
         temp = (short)((TmpVal + 0x4000) >> 15);
         TmpVal1 = mult * temp;
         TmpVal2 = (TmpVal1 + const1) >> scale;

         if(TmpVal2 > 32767) goto ExitRefl;
         bState[j] = (short)TmpVal2;
      }

      ippsCopy_16s(bState, aState, i);
   }
   return;

ExitRefl:
   ippsZero_16s(pReflCoeff, LP_ORDER_SIZE);
}
/**************************************************************************
*  Function    : ownScaleExcitation_GSMAMR 
***************************************************************************/
void ownScaleExcitation_GSMAMR(short *pInputSignal, short *pOutputSignal)
{
    short exp;
    short gain_in, gain_out, g0;
    int i, s;
    short temp[SUBFR_SIZE_GSMAMR];
    
    ippsDotProd_16s32s_Sfs(pOutputSignal, pOutputSignal, SUBFR_SIZE_GSMAMR, &s, 0);
    if (s >= IPP_MAX_32S/2) {
        ippsRShiftC_16s(pOutputSignal, 2, temp, SUBFR_SIZE_GSMAMR);
        ippsDotProd_16s32s_Sfs(temp, temp, SUBFR_SIZE_GSMAMR, &s, -1);
    } else s >>= 3;
    
    if(s == 0) return;
    
    exp = Exp_32s_Pos(s) - 1;
    gain_out = Cnvrt_NR_32s16s(s << exp);

    ippsDotProd_16s32s_Sfs(pInputSignal, pInputSignal, SUBFR_SIZE_GSMAMR, &s, 0);
    if (s >= IPP_MAX_32S/2) {
        ippsRShiftC_16s(pInputSignal, 2, temp, SUBFR_SIZE_GSMAMR);
        ippsDotProd_16s32s_Sfs(temp, temp, SUBFR_SIZE_GSMAMR, &s, -1);
    } else s >>= 3;
    
    if(s == 0) g0 = 0;
    else {
        i = Norm_32s_I(&s);
        gain_in = Cnvrt_NR_32s16s(s);
        exp -= i;
        s = ((gain_out << 15) / gain_in);
        s = ShiftL_32s(s, 7);       
        if (exp < 0) s = ShiftL_32s(s, (unsigned short)(-exp));
        else         s >>= exp;  
        ippsInvSqrt_32s_I(&s, 1);
        g0 = Cnvrt_NR_32s16s(ShiftL_32s(s, 9));
    }

    for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
        pOutputSignal[i] = (short)(Mul16_32s(pOutputSignal[i] * g0) >> 16);

    return;
	/* End of ownScaleExcitation_GSMAMR() */
}

static const short TablePredCoeff[NUM_PRED_TAPS] = {5571, 4751, 2785, 1556};
/* MEAN_ENER  = 36.0/constant, constant = 20*Log10(2)       */
#define MEAN_ENER_MR122  783741  
static const short TablePredCoeff_M122[NUM_PRED_TAPS] = {44, 37, 22, 12};

/*************************************************************************
 * Function:  ownPredEnergyMA_GSMAMR()
 *************************************************************************/
void ownPredEnergyMA_GSMAMR(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, IppSpchBitRate rate,    
                            short *code, short *expGainCodeCB, short *fracGainCodeCB, short *expEnergyCoeff, 
                            short *fracEnergyCoeff)
{
    int ener_code, ener;
    short exp, frac;
    ippsDotProd_16s32s_Sfs( code, code, SUBFR_SIZE_GSMAMR, &ener_code, -1);
                                     
    if (rate == IPP_SPCHBR_12200) {
        ener_code = 2 *((ener_code + 0x8000) >> 16) * 26214;   
        ownLog2_GSMAMR(ener_code, &exp, &frac);
        ener_code = ((exp - 30) << 16) + (frac << 1); 
        ippsDotProd_16s32s_Sfs(a_PastQntEnergy_M122, TablePredCoeff_M122, NUM_PRED_TAPS, &ener, -1);
        ener += MEAN_ENER_MR122; 
        ener = (ener - ener_code) >> 1;                /* Q16 */
        L_Extract(ener, expGainCodeCB, fracGainCodeCB);
    } else {
        int TmpVal;
        short exp_code, predGainCB;

        exp_code = Norm_32s_I(&ener_code);
        ownLog2_GSMAMR_norm (ener_code, exp_code, &exp, &frac);
        TmpVal = Mul16s_32s(exp, frac, -24660); 
        if(rate == IPP_SPCHBR_10200) TmpVal += 2 * 16678 * 64;     
        else if (rate == IPP_SPCHBR_7950) {
           *fracEnergyCoeff = (short) (ener_code>>16); 
           *expEnergyCoeff = -11 - exp_code;    
           TmpVal += 2 * 17062 * 64;     
        } else if (rate == IPP_SPCHBR_7400) {
            TmpVal += 2 * 32588 * 32;     
        } else if (rate == IPP_SPCHBR_6700) {
            TmpVal += 2 * 32268 * 32;    
        } else { /* MR59, MR515, MR475 */
            TmpVal += 2 * 16678 * 64;     
        }

        TmpVal <<= 10;               
        ippsDotProd_16s32s_Sfs(a_PastQntEnergy, TablePredCoeff, NUM_PRED_TAPS, &ener, -1); 
        TmpVal += ener;
        predGainCB = (short)(TmpVal>>16);               
        if(rate == IPP_SPCHBR_7400) TmpVal = predGainCB * 5439;  
        else                        TmpVal = predGainCB * 5443;  
        TmpVal >>= 7;                   
        L_Extract(TmpVal, expGainCodeCB, fracGainCodeCB); 
    }
	return;
	/* End of ownPredEnergyMA_GSMAMR() */
}
/*************************************************************************
*  Function:   ownCloseLoopFracPitchSearch_GSMAMR
***************************************************************************/
int ownCloseLoopFracPitchSearch_GSMAMR(short *vTimePrevSubframe, short *a_GainHistory, IppSpchBitRate rate,     
                                       short frameOffset, short *pLoopPitchLags, short *pImpResVec,      
                                       short *pLTPExc, short *pPredRes, short *pTargetPitchVec, short lspFlag,          
                                       short *pTargetVec, short *pFltAdaptExc, short *pExpPitchDel,    
                                       short *pFracPitchDel, short *gainPitch, short **ppAnalysisParam,
                                       short *gainPitchLimit)
{
    short i;
    short index;
    int TmpVal;     
    short subfrNum;
    short sum = 0;
    
    subfrNum = frameOffset / SUBFR_SIZE_GSMAMR;
    ippsAdaptiveCodebookSearch_GSMAMR_16s(pTargetPitchVec, pImpResVec, pLoopPitchLags, 
		                                  vTimePrevSubframe, (pLTPExc - MAX_OFFSET), 
										  pFracPitchDel, &index, pFltAdaptExc, subfrNum, rate);
    
    ippsConvPartial_16s_Sfs(pLTPExc, pImpResVec, pFltAdaptExc, SUBFR_SIZE_GSMAMR, 12);
   
    *pExpPitchDel = *vTimePrevSubframe;
    *(*ppAnalysisParam)++ = index;

    ippsAdaptiveCodebookGain_GSMAMR_16s(pTargetPitchVec, pFltAdaptExc, gainPitch);

    if(rate == IPP_SPCHBR_12200) *gainPitch &= 0xfffC;           

    *gainPitchLimit = IPP_MAX_16S;                                
    if((lspFlag != 0) && (*gainPitch > PITCH_GAIN_CLIP)) {
       ippsSum_16s_Sfs(a_GainHistory, PG_NUM_FRAME, &sum, 0);
       sum += *gainPitch >> 3;          /* Division by 8 */
       if(sum > PITCH_GAIN_CLIP) *gainPitchLimit = PITCH_GAIN_CLIP;                     
    }

   if ((rate == IPP_SPCHBR_4750) || (rate == IPP_SPCHBR_5150)) {
      if(*gainPitch > 13926) *gainPitch = 13926;     
   } else {
       if(sum > PITCH_GAIN_CLIP) *gainPitch = PITCH_GAIN_CLIP;                    

       if(rate == IPP_SPCHBR_12200) {
           index = ownQntGainPitch_M122_GSMAMR(*gainPitchLimit, *gainPitch);
           *gainPitch = TableQuantGainPitch[index] & 0xFFFC;
           *(*ppAnalysisParam)++ = index;
       }
   }

   /* update target vector und evaluate LTP residual 

      Algorithmically is equivalent to :

      ippsMulC_16s_Sfs(pFltAdaptExc,*gainPitch,y1Tmp,SUBFR_SIZE_GSMAMR,14);
      ippsMulC_16s_Sfs(pLTPExc,*gainPitch,excTmp,SUBFR_SIZE_GSMAMR,14);
      ippsSub_16s(y1Tmp,pTargetPitchVec,pTargetVec,SUBFR_SIZE_GSMAMR);
      ippsSub_16s_I(excTmp,pPredRes,SUBFR_SIZE_GSMAMR);

      But !!! could not be used due to bitexactness. 
   */
   
   for (i = 0; i < SUBFR_SIZE_GSMAMR; i++) {
       TmpVal = pFltAdaptExc[i] * *gainPitch;
       pTargetVec[i] = pTargetPitchVec[i] - (short)(TmpVal >> 14);

       TmpVal = pLTPExc[i] * *gainPitch;
       pPredRes[i] -= (short)(TmpVal >> 14);
   }

   return 0;
   /* End of ownCloseLoopFracPitchSearch_GSMAMR() */
}
