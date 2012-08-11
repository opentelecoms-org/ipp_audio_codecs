/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives AMRWB Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  AMRWB is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: AMRWB speech codec: common utilities.
//
*/

#include "ownamrwb.h"

const IppSpchBitRate Mode2RateTbl[11] = {
   IPP_SPCHBR_6600,      /* 6.60 kbps */
   IPP_SPCHBR_8850,      /* 8.85 kbps */ 
   IPP_SPCHBR_12650,     /* 12.65 kbps */
   IPP_SPCHBR_14250,     /* 14.25 kbps */
   IPP_SPCHBR_15850,     /* 15.85 kbps */
   IPP_SPCHBR_18250,     /* 18.25 kbps */
   IPP_SPCHBR_19850,     /* 19.85 kbps */
   IPP_SPCHBR_23050,     /* 23.05 kbps */
   IPP_SPCHBR_23850,     /* 23.85 kbps */
   IPP_SPCHBR_DTX
};

/* Find the voicing factor (1=voice to -1=unvoiced) */
short ownVoiceFactor(short *pPitchExc, short valExcFmt, short valPitchGain,
                   short *pFixCdbkExc, short valCodeGain, short len)
{
    short valExpDiff, tmp, valExp, valEner, valExp1, valEner2, valExp2, valVoiceFac;
    int s;

    ippsDotProd_16s32s_Sfs( pPitchExc, pPitchExc, len, &s, -1);
    s = Add_32s(s, 1);

    valExp1 = (30 - Norm_32s_I(&s));
    valEner = s>>16;
    valExp1 -= (valExcFmt << 1);

    s = (valPitchGain * valPitchGain) << 1;
    valExp = Norm_32s_I(&s);
    tmp = (short)(s>>16);

    valEner = Mul_16s_Sfs(valEner, tmp, 15);
    valExp1 -= (valExp + 10);

    ippsDotProd_16s32s_Sfs( pFixCdbkExc, pFixCdbkExc,len, &s, -1);
    s = Add_32s(s, 1);

    valExp2 = (30 - Norm_32s_I(&s));
    valEner2 = s>>16;

    valExp = Exp_16s(valCodeGain);
    tmp = valCodeGain << valExp;
    tmp = Mul_16s_Sfs(tmp, tmp, 15);
    valEner2 = Mul_16s_Sfs(valEner2, tmp, 15);
    valExp2 -= (valExp << 1);

    valExpDiff = (valExp1 - valExp2);

    if (valExpDiff >= 0)
    {
        valEner >>= 1;
        valEner2 >>= (valExpDiff + 1);
    } else
    {
        if ((1 - valExpDiff) >= 15) 
            valEner = 0;
        else
            valEner >>= (1 - valExpDiff);
        valEner2 >>= 1;
    }

    valVoiceFac = valEner - valEner2;
    valEner += (valEner2 + 1);

    if (valVoiceFac >= 0)
        valVoiceFac = (valVoiceFac << 15) / valEner;
    else
        valVoiceFac = -(((-valVoiceFac) << 15) / valEner);

    return (valVoiceFac);
}

/* 2.0 - 6.4 kHz phase dispersion */
static short impPhaseDispLowTbl[SUBFRAME_SIZE] =
{
    20182,  9693,  3270, -3437,  2864, -5240,  1589, -1357,   600,  
     3893, -1497,  -698,  1203, -5249,  1199,  5371, -1488,  -705, 
    -2887,  1976,   898,   721, -3876,  4227, -5112,  6400, -1032, 
    -4725,  4093, -4352,  3205,  2130, -1996, -1835,  2648, -1786,
     -406,   573,  2484, -3608,  3139, -1363, -2566,  3808,  -639,
    -2051,  -541,  2376,  3932, -6262,  1432, -3601,  4889,   370,
      567, -1163, -2854,  1914,    39, -2418,  3454,  2975, -4021, 
     3431
};

/* 3.2 - 6.4 kHz phase dispersion */
static short impPhaseDispMidTbl[SUBFRAME_SIZE] =
{
    24098, 10460, -5263,  -763,  2048,  -927, 1753, -3323,  2212, 
      652, -2146,  2487, -3539,  4109, -2107,  -374, -626,  4270,
    -5485,  2235,  1858, -2769,   744,  1140,  -763, -1615, 4060,
    -4574,  2982, -1163,   731, -1098,   803,   167,  -714,  606,
     -560,   639,    43, -1766,  3228, -2782,   665,   763,  233,
    -2002,  1291,  1871, -3470,  1032,  2710, -4040,  3624,-4214,
     5292, -4270,  1563,   108,  -580,  1642, -2458,   957,  544,
     2540
};

void ownPhaseDispInit(short *pDispvec)
{
    ippsZero_16s(pDispvec, 8);
    return;
}

void ownPhaseDisp(short valCodeGain, short valPitchGain, short *pFixCdbkExc,
                  short valLevelDisp, short *pDispvec)
{
    short valState;
    short *pPrevPitchGain, *pPrevCodeGain, *pPrevState;
    int i, j;
    IPP_ALIGNED_ARRAY (16, short, pCodevec, SUBFRAME_SIZE*2);

    pPrevState = pDispvec;
    pPrevCodeGain = pDispvec + 1;
    pPrevPitchGain = pDispvec + 2;

    ippsZero_16s(pCodevec, SUBFRAME_SIZE*2);
    
    if (valPitchGain < MIN_GAIN_PITCH)
        valState = 0;
    else if (valPitchGain < THRESH_GAIN_PITCH)
        valState = 1;
    else
        valState = 2;
    
    ippsMove_16s(pPrevPitchGain, &pPrevPitchGain[1], 5);
    pPrevPitchGain[0] = valPitchGain;           

    if ((valCodeGain - *pPrevCodeGain) > (*pPrevCodeGain << 1)) { 
        if (valState < 2) valState += 1;
    } else {
        j = 0;
        for (i = 0; i < 6; i++)
            if (pPrevPitchGain[i] < MIN_GAIN_PITCH) j += 1;
        if (j > 2) valState = 0;                     
        if (valState > (*pPrevState + 1)) valState -= 1;
    }

    *pPrevCodeGain = valCodeGain;           
    *pPrevState = valState;                   

    valState += valLevelDisp;
    if (valState == 0)
    {
        for (i = 0; i < SUBFRAME_SIZE; i++)
        {
            if (pFixCdbkExc[i] != 0)
            {
                for (j = 0; j < SUBFRAME_SIZE; j++)
                {
                    pCodevec[i + j] += (pFixCdbkExc[i] * impPhaseDispLowTbl[j] + 0x00004000) >> 15;
                }
            }
        }
    } else if (valState == 1)
    {
        for (i = 0; i < SUBFRAME_SIZE; i++)
        {
            if (pFixCdbkExc[i] != 0)
            {
                for (j = 0; j < SUBFRAME_SIZE; j++)
                {
                    pCodevec[i + j] += (pFixCdbkExc[i] * impPhaseDispMidTbl[j] + 0x00004000) >> 15;
                }
            }
        }
    }
    if (valState < 2)
        ippsAdd_16s(pCodevec, &pCodevec[SUBFRAME_SIZE], pFixCdbkExc, SUBFRAME_SIZE);

    return;
}

/*-------------------------------------------------------------------*
 * Decimate a vector by 2 with 2nd order fir filter.                 *
 *-------------------------------------------------------------------*/

#define L_FIR  5
#define L_MEM  (L_FIR-2)
static short hFirTbl[L_FIR] = {4260, 7536, 9175, 7536, 4260};

void ownLPDecim2(short *pSignal, short len, short *pMem)
{
    short *pSignalPtr;
    int i, j, s;
    IPP_ALIGNED_ARRAY (16, short, pTmpvec, FRAME_SIZE + L_MEM);

    ippsCopy_16s(pMem, pTmpvec, L_MEM);
    ippsCopy_16s(pSignal, &pTmpvec[L_MEM], len);
    ippsCopy_16s(&pSignal[len-L_MEM], pMem, L_MEM);

    for (i = 0, j = 0; i < len; i += 2, j++)
    {
        pSignalPtr = &pTmpvec[i];                   
        ippsDotProd_16s32s_Sfs(pSignalPtr, hFirTbl, L_FIR, &s, -1);
        pSignal[j] = Cnvrt_NR_32s16s(s);               
    }
    return;
}

/* Conversion of 16th-order 12.8kHz ISF vector into 20th-order 16kHz ISF vector */
#define INV_LENGTH 2731

void ownIsfExtrapolation(short *pHfIsf)
{
    IPP_ALIGNED_ARRAY (16, short, pIsfDiffvec, LP_ORDER - 2);
    int i, s, pIsfCorrvec[3];
    short valCoeff, valMean, valExp, valExp2, valHigh, valLow, valMaxCorr;
    short tmp, tmp2, tmp3;

    pHfIsf[LP_ORDER_16K - 1] = pHfIsf[LP_ORDER - 1];        
    ippsSub_16s(pHfIsf, &pHfIsf[1], pIsfDiffvec, (LP_ORDER - 2));

    s = 0;                             
    for (i = 3; i < (LP_ORDER - 1); i++)
        s += pIsfDiffvec[i - 1] * INV_LENGTH;
    valMean = Cnvrt_NR_32s16s(s<<1);

    pIsfCorrvec[0] = 0;                        
    pIsfCorrvec[1] = 0;                        
    pIsfCorrvec[2] = 0;                        

    ippsMax_16s(pIsfDiffvec, (LP_ORDER - 2), &tmp);
    valExp = Exp_16s(tmp);
    ippsLShiftC_16s_I(valExp, pIsfDiffvec, (LP_ORDER - 2));
    valMean <<= valExp;
    for (i = 7; i < (LP_ORDER - 2); i++)
    {
        tmp2 = pIsfDiffvec[i] - valMean;
        tmp3 = pIsfDiffvec[i - 2] - valMean;
        s = tmp2 * tmp3;
        valHigh = s >> 15;
        valLow = s & 0x7fff;
        s = (valHigh*valHigh + ((valHigh*valLow + valLow*valHigh)>>15)) << 1;
        pIsfCorrvec[0] += s;  
    }
    for (i = 7; i < (LP_ORDER - 2); i++)
    {
        tmp2 = pIsfDiffvec[i] - valMean;
        tmp3 = pIsfDiffvec[i - 3] - valMean;
        s = tmp2 * tmp3;
        valHigh = s >> 15;
        valLow = s & 0x7fff;
        s = (valHigh*valHigh + ((valHigh*valLow + valLow*valHigh)>>15)) << 1;
        pIsfCorrvec[1] += s;  
    }
    for (i = 7; i < (LP_ORDER - 2); i++)
    {
        tmp2 = pIsfDiffvec[i] - valMean;
        tmp3 = pIsfDiffvec[i - 4] - valMean;
        s = tmp2 * tmp3;
        valHigh = s >> 15;
        valLow = s & 0x7fff;
        s = (valHigh*valHigh + ((valHigh*valLow + valLow*valHigh)>>15) ) << 1;
        pIsfCorrvec[2] += s;  
    }
    
    if (pIsfCorrvec[0] > pIsfCorrvec[1])
        valMaxCorr = 0;                       
    else
        valMaxCorr = 1;                       
    
    if (pIsfCorrvec[2] > pIsfCorrvec[valMaxCorr]) valMaxCorr = 2;                       
    valMaxCorr += 1;

    for (i = LP_ORDER - 1; i < (LP_ORDER_16K - 1); i++)
    {
        tmp = (pHfIsf[i - 1 - valMaxCorr] - pHfIsf[i - 2 - valMaxCorr]);
        pHfIsf[i] = (pHfIsf[i - 1] + tmp); 
    }

    tmp = (pHfIsf[4] + pHfIsf[3]);
    tmp = (pHfIsf[2] - tmp);
    tmp = Mul_16s_Sfs(tmp, 5461, 15);
    tmp += 20390;
    if (tmp > 19456) tmp = 19456;                       
    tmp -= pHfIsf[LP_ORDER - 2];
    tmp2 = (pHfIsf[LP_ORDER_16K - 2] - pHfIsf[LP_ORDER - 2]);

    valExp2 = Exp_16s(tmp2);
    valExp = Exp_16s(tmp);
    valExp -= 1;
    tmp <<= valExp;
    tmp2 <<= valExp2;
    valCoeff = (tmp << 15) / tmp2;
    valExp = (valExp2 - valExp);

    for (i = LP_ORDER - 1; i < (LP_ORDER_16K - 1); i++)
    {
        tmp = ((pHfIsf[i] - pHfIsf[i - 1]) * valCoeff) >> 15;
        if (valExp > 0)
            pIsfDiffvec[i - (LP_ORDER - 1)] = (tmp << valExp);
        else
            pIsfDiffvec[i - (LP_ORDER - 1)] = (tmp >> (-valExp));
    }

    for (i = LP_ORDER; i < (LP_ORDER_16K - 1); i++)
    {
        tmp = (pIsfDiffvec[i - (LP_ORDER - 1)] + pIsfDiffvec[i - LP_ORDER]) - 1280;
        if (tmp < 0)
        {
            if (pIsfDiffvec[i - (LP_ORDER - 1)] > pIsfDiffvec[i - LP_ORDER])
                pIsfDiffvec[i - LP_ORDER] = 1280 - pIsfDiffvec[i - (LP_ORDER - 1)];       
            else
                pIsfDiffvec[i - (LP_ORDER - 1)] = 1280 - pIsfDiffvec[i - LP_ORDER];       
        }
    }
    ippsAdd_16s(&pHfIsf[LP_ORDER - 2], pIsfDiffvec, &pHfIsf[LP_ORDER - 1], (LP_ORDER_16K -LP_ORDER));
    ownMulC_16s_ISfs(26214, pHfIsf, LP_ORDER_16K-1, 15);

    ippsISFToISP_AMRWB_16s(pHfIsf, pHfIsf, LP_ORDER_16K);

    return;
}

short ownGpClip(short *pMem)
{
    short clip;

    clip = 0;
    if ((pMem[0] < THRESH_DISTANCE_ISF) && (pMem[1] > THRESH_GAIN_PITCH))
        clip = 1;                          

    return (clip);
}

void ownCheckGpClipIsf(short *pIsfvec, short *pMem)
{
    short valDist, valDistMin;
    int i;

    valDistMin = (pIsfvec[1] - pIsfvec[0]);
    for (i = 2; i < LP_ORDER - 1; i++)
    {
        valDist = (pIsfvec[i] - pIsfvec[i - 1]);
        if (valDist < valDistMin) valDistMin = valDist;               
    }

    valDist = (short)((26214 * pMem[0] + 6554 * valDistMin) >> 15);
    if (valDist > MAX_DISTANCE_ISF) valDist = MAX_DISTANCE_ISF;               
    pMem[0] = valDist;                         

    return;
}

void ownCheckGpClipPitchGain(short valPitchGain, short *pMem)
{
    short valGain;
    int s;

    s = 29491 * pMem[1];
    s += 3277 * valPitchGain;
    valGain = (short)(s>>15);

    if (valGain < MIN_GAIN_PITCH) valGain = MIN_GAIN_PITCH;               
    pMem[1] = valGain;                         

    return;
}
