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
//  Purpose: AMRWB speech encoder API
//
*/

#include "ownamrwb.h"

static short ownSynthesis(short *pLPCQuantvec, short *pExc, short valQNew,
                          const unsigned short *pSynthSignal, AMRWBEncoder_Obj *st);

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_GetSize, 
         (AMRWBEncoder_Obj* encoderObj, unsigned int *pCodecSize))
{
   if(NULL == encoderObj)
      return APIAMRWB_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIAMRWB_StsBadArgErr;
   if(encoderObj->objPrm.iKey != DEC_KEY)
      return APIAMRWB_StsNotInitialized;

   *pCodecSize = encoderObj->objPrm.iObjSize;
   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_Alloc,
         (const AMRWBEnc_Params *amrwb_Params, unsigned int *pCodecSize))
{
   int hpfltsize;
   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize);
   *pCodecSize=sizeof(AMRWBEncoder_Obj);
   *pCodecSize += (2*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize);
   *pCodecSize += (3*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(3, &hpfltsize);
   *pCodecSize += hpfltsize;
   ippsVADGetSize_AMRWB_16s(&hpfltsize);
   *pCodecSize += hpfltsize;

   return APIAMRWB_StsNoErr;
}

static void InitEncoder(AMRWBEncoder_Obj* encoderObj)
{
   ippsZero_16s(encoderObj->asiExcOld, PITCH_LAG_MAX + INTERPOL_LEN);
   ippsZero_16s(encoderObj->asiSynthesis, LP_ORDER);
   ippsZero_16s(encoderObj->asiIsfQuantPast, LP_ORDER);

   encoderObj->siSpeechWgt = 0;                 
   encoderObj->siTiltCode = 0;              
   encoderObj->siFrameFirst = 1;            

   encoderObj->asiGainPitchClip[0] = MAX_DISTANCE_ISF;                 
   encoderObj->asiGainPitchClip[1] = MIN_GAIN_PITCH;

   encoderObj->iNoiseEnhancerThres = 0;

   return;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_Init, 
         (AMRWBEncoder_Obj* encoderObj, int mode))
{
   int hpfltsize;

   InitEncoder(encoderObj);

   encoderObj->pSHPFIRState = (HighPassFIRState_AMRWB_16s_ISfs *)((char*)encoderObj + sizeof(AMRWBEncoder_Obj));
   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize);
   encoderObj->pSHPFIRState2 = (HighPassFIRState_AMRWB_16s_ISfs *)((char*)encoderObj->pSHPFIRState + hpfltsize);
   encoderObj->pSHPFiltStateSgnlIn = (IppsHighPassFilterState_AMRWB_16s *)((char*)encoderObj->pSHPFIRState2 + hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize);
   encoderObj->pSHPFiltStateSgnlOut = (IppsHighPassFilterState_AMRWB_16s *)((char*)encoderObj->pSHPFiltStateSgnlIn + hpfltsize);
   encoderObj->pSHPFiltStateSgnl400 = (IppsHighPassFilterState_AMRWB_16s *)((char*)encoderObj->pSHPFiltStateSgnlOut + hpfltsize);
   encoderObj->pSHPFiltStateWsp = (IppsHighPassFilterState_AMRWB_16s *)((char*)encoderObj->pSHPFiltStateSgnl400 + hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(3, &hpfltsize);
   encoderObj->pSVadState = (IppsVADState_AMRWB_16s *)((char*)encoderObj->pSHPFiltStateWsp + hpfltsize);

   /* Static vectors to zero */

   ippsZero_16s(encoderObj->asiSpeechOld, SPEECH_SIZE - FRAME_SIZE);
   ippsZero_16s(encoderObj->asiWspOld, (PITCH_LAG_MAX / OPL_DECIM));
   ippsZero_16s(encoderObj->asiWspDecimate, 3);

   /* routines initialization */

   ippsZero_16s(encoderObj->asiSpeechDecimate, 2 * NB_COEF_DOWN);
   ippsHighPassFilterInit_AMRWB_16s((short*)ACoeffHP50Tbl, (short*)BCoeffHP50Tbl, 2, encoderObj->pSHPFiltStateSgnlIn);
   ippsZero_16s(encoderObj->asiLevinsonMem, 32);
   ippsSet_16s(-14336, encoderObj->asiGainQuant, PRED_ORDER);
   ippsHighPassFilterInit_AMRWB_16s((short*)ACoeffTbl, (short*)BCoeffTbl, 3, encoderObj->pSHPFiltStateWsp);

   /* isp initialization */

   ippsCopy_16s(IspInitTbl, encoderObj->asiIspOld, LP_ORDER);
   ippsCopy_16s(IspInitTbl, encoderObj->asiIspQuantOld, LP_ORDER);

   /* variable initialization */

   encoderObj->siPreemph = 0;        
   encoderObj->siWsp = 0;            
   encoderObj->siScaleFactorOld = 15;             
   encoderObj->asiScaleFactorMax[0] = 15;          
   encoderObj->asiScaleFactorMax[1] = 15;          
   encoderObj->siWspOldMax = 0;        
   encoderObj->siWspOldShift = 0;      

   /* pitch ol initialization */

   encoderObj->siMedianOld = 40;        
   encoderObj->siOpenLoopGain = 0;            
   encoderObj->siAdaptiveParam = 0;              
   encoderObj->siWeightFlag = 0;        
   ippsSet_16s(40, encoderObj->asiPitchLagOld, 5);
   ippsZero_16s(encoderObj->asiHypassFiltWspOld, (FRAME_SIZE / 2) / OPL_DECIM + (PITCH_LAG_MAX / OPL_DECIM));

   ippsZero_16s(encoderObj->asiSynthHighFilt, LP_ORDER);
   ippsZero_16s((short*)encoderObj->aiSynthMem, LP_ORDER * 2);

   ippsHighPassFilterInit_AMRWB_16s((short*)ACoeffHP50Tbl, (short*)BCoeffHP50Tbl, 2, encoderObj->pSHPFiltStateSgnlOut);
   HighPassFIRInit_AMRWB_16s_ISfs((short*)Fir6k_7kTbl, 2, encoderObj->pSHPFIRState);
   ippsHighPassFilterInit_AMRWB_16s((short*)ACoeffHP400Tbl, (short*)BCoeffHP400Tbl, 2, encoderObj->pSHPFiltStateSgnl400);

   ippsCopy_16s(IsfInitTbl, encoderObj->asiIsfOld, LP_ORDER);

   encoderObj->siDeemph = 0;         

   encoderObj->siHFSeed = RND_INIT;          

   HighPassFIRInit_AMRWB_16s_ISfs((short*)Fir6k_7kTbl, 2, encoderObj->pSHPFIRState2);
   encoderObj->siAlphaGain = IPP_MAX_16S;     

   encoderObj->siVadHist = 0;
   encoderObj->iDtx = mode;

   ippsVADInit_AMRWB_16s(encoderObj->pSVadState);
   encoderObj->siToneFlag = 0;
   ownDTXEncReset(&encoderObj->dtxEncState, (short*)IsfInitTbl);
   encoderObj->siScaleExp = 0;

   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncode,
         (AMRWBEncoder_Obj* encoderObj, const unsigned short* src, 
          unsigned char* dst, AMRWB_Rate_t *rate, int Vad ))
{
    /* Coder states */
    AMRWBEncoder_Obj *st;

    /* Speech vector */
    IPP_ALIGNED_ARRAY(16, short, pSpeechOldvec, SPEECH_SIZE);
    short *pSpeechNew, *pSpeech, *pWindow;

    /* Weighted speech vector */
    IPP_ALIGNED_ARRAY(16, short, pWgtSpchOldvec, FRAME_SIZE + (PITCH_LAG_MAX / OPL_DECIM));
    short *pWgtSpch;

    /* Excitation vector */
    IPP_ALIGNED_ARRAY(16, short, pExcOldvec, (FRAME_SIZE + 1) + PITCH_LAG_MAX + INTERPOL_LEN);
    short *pExc;

    /* LPC coefficients */

    IPP_ALIGNED_ARRAY(16, int, pAutoCorrvec, LP_ORDER + 1);
    IPP_ALIGNED_ARRAY(16, short, pRCvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, short, pLPCvec, LP_ORDER + 1);
    IPP_ALIGNED_ARRAY(16, short, pIspvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, short, pIspQuantvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, short, pIsfvec, LP_ORDER);
    short *pLPCUnQuant, *pLPCQuant;
    IPP_ALIGNED_ARRAY(16, short, pLPCUnQuantvec, NUMBER_SUBFRAME*(LP_ORDER+1));
    IPP_ALIGNED_ARRAY(16, short, pLPCQuantvec, NUMBER_SUBFRAME*(LP_ORDER+1));

    /* Other vectors */

    IPP_ALIGNED_ARRAY(16, short, pAdptTgtPitch, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pAdptTgtCdbk, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pCorrvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pResidvec, SUBFRAME_SIZE);

    IPP_ALIGNED_ARRAY(16, short, pImpRespPitchvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pImpRespCdbkvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pFixCdbkvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pFixCdbkExcvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pFltAdptvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pFltAdpt2vec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pErrQuant, LP_ORDER + SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pSynthvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pExc2vec, FRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, short, pVadvec, FRAME_SIZE);

    /* Scalars */

    short valSubFrame, valSelect, valClipFlag, valVadFlag;
    short valPitchLag, valFracPitchLag;
    short pOpenLoopLag[2],pPitchLagBounds[2];
    short valPitchGain, valScaleCodeGain, pGainCoeff[4], pGainCoeff2[4];
    short tmp, valGainCoeff, valAdptGain, valExp, valQNew, valShift, max;
    short valVoiceFac;

    int i, z, valCodeGain, valMax;

    short valStabFac, valFac, valCodeGainLow;
    short pSrcCoeff[2];

    short valCorrGain;
    int valSize, subFrame;
    IppSpchBitRate irate = Mode2RateTbl[*rate];
    IppSpchBitRate valCodecMode = irate;
    short *pHPWgtSpch;
    IPP_ALIGNED_ARRAY(16, short, pPrmvec, MAX_PARAM_SIZE);
    short *pPrm = pPrmvec;

    st = encoderObj;

    pSpeechNew = pSpeechOldvec + SPEECH_SIZE - FRAME_SIZE - UP_SAMPL_FILT_DELAY;
    pSpeech = pSpeechOldvec + SPEECH_SIZE - FRAME_SIZE - SUBFRAME_SIZE;
    pWindow = pSpeechOldvec + SPEECH_SIZE - WINDOW_SIZE; 

    pExc = pExcOldvec + PITCH_LAG_MAX + INTERPOL_LEN;  
    pWgtSpch = pWgtSpchOldvec + (PITCH_LAG_MAX / OPL_DECIM); 

    /* copy coder memory state into working space (internal memory for DSP) */

    ippsCopy_16s(st->asiSpeechOld, pSpeechOldvec, SPEECH_SIZE - FRAME_SIZE);
    ippsCopy_16s(st->asiWspOld, pWgtSpchOldvec, PITCH_LAG_MAX / OPL_DECIM);
    ippsCopy_16s(st->asiExcOld, pExcOldvec, PITCH_LAG_MAX + INTERPOL_LEN);

    /* Down sampling signal from 16kHz to 12.8kHz */

    ownDecimation_AMRWB_16s((short*)src, L_FRAME16k, pSpeechNew, st->asiSpeechDecimate);

    /* last UP_SAMPL_FILT_DELAY samples for autocorrelation window */
    ippsCopy_16s(st->asiSpeechDecimate, pFixCdbkvec, 2*DOWN_SAMPL_FILT_DELAY);
    ippsZero_16s(pErrQuant, DOWN_SAMPL_FILT_DELAY);            /* set next sample to zero */
    ownDecimation_AMRWB_16s(pErrQuant, DOWN_SAMPL_FILT_DELAY, pSpeechNew + FRAME_SIZE, pFixCdbkvec);

    /* Perform 50Hz HP filtering of input signal */

    ippsHighPassFilter_AMRWB_16s_ISfs(pSpeechNew, FRAME_SIZE, st->pSHPFiltStateSgnlIn, 14);

    /* last UP_SAMPL_FILT_DELAY samples for autocorrelation window */
    ippsHighPassFilterGetSize_AMRWB_16s(2, &valSize);
    ippsCopy_8u((const Ipp8u*)st->pSHPFiltStateSgnlIn, (Ipp8u*)pFixCdbkvec, valSize);
    ippsHighPassFilter_AMRWB_16s_ISfs(pSpeechNew + FRAME_SIZE, UP_SAMPL_FILT_DELAY, (IppsHighPassFilterState_AMRWB_16s *)pFixCdbkvec, 14);

    /* get max of new preemphased samples (FRAME_SIZE+UP_SAMPL_FILT_DELAY) */

    z = pSpeechNew[0] << 15;
    z -= st->siPreemph * PREEMPH_FACTOR;
    valMax = Abs_32s(z);

    for (i = 1; i < FRAME_SIZE + UP_SAMPL_FILT_DELAY; i++)
    {
        z  = pSpeechNew[i] << 15;
        z -= pSpeechNew[i - 1] * PREEMPH_FACTOR;
        z  = Abs_32s(z);
        if (z > valMax) valMax = z;                 
    }

    /* get scaling factor for new and previous samples */
    tmp = (short)(valMax >> 16);
    
    if (tmp == 0)
    {
        valShift = MAX_SCALE_FACTOR;                     
    } else
    {
        valShift = Exp_16s(tmp) - 1;
        if (valShift < 0) valShift = 0;                     
        if (valShift > MAX_SCALE_FACTOR) valShift = MAX_SCALE_FACTOR;                 
    }
    valQNew = valShift;                         
    
    if (valQNew > st->asiScaleFactorMax[0]) valQNew = st->asiScaleFactorMax[0];              
    if (valQNew > st->asiScaleFactorMax[1]) valQNew = st->asiScaleFactorMax[1];              
    valExp = valQNew - st->siScaleFactorOld;
    st->siScaleFactorOld = valQNew;                     
    st->asiScaleFactorMax[1] = st->asiScaleFactorMax[0];           
    st->asiScaleFactorMax[0] = valShift;                  

    /* preemphasis with scaling (FRAME_SIZE+UP_SAMPL_FILT_DELAY) */

    tmp = pSpeechNew[FRAME_SIZE - 1];         

    ippsPreemphasize_AMRWB_16s_ISfs(PREEMPH_FACTOR, pSpeechNew, FRAME_SIZE + UP_SAMPL_FILT_DELAY, 15-valQNew, &st->siPreemph);

    st->siPreemph = tmp;                 

    /* scale previous samples and memory */

    ownScaleSignal_AMRWB_16s_ISfs(pSpeechOldvec, SPEECH_SIZE - FRAME_SIZE - UP_SAMPL_FILT_DELAY, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(pExcOldvec, PITCH_LAG_MAX + INTERPOL_LEN, valExp);

    ownScaleSignal_AMRWB_16s_ISfs(st->asiSynthesis, LP_ORDER, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(st->asiWspDecimate, 3, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(&(st->siWsp), 1, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(&(st->siSpeechWgt), 1, valExp);

     /*  Call VAD */

    ippsCopy_16s(pSpeechNew, pVadvec, FRAME_SIZE);
    ownScaleSignal_AMRWB_16s_ISfs(pVadvec, FRAME_SIZE, (1 - valQNew));
    ippsVAD_AMRWB_16s(pVadvec, st->pSVadState, &encoderObj->siToneFlag, &valVadFlag);
    if (valVadFlag == 0)
        st->siVadHist += 1;        
    else
        st->siVadHist = 0;              

    /* DTX processing */
    
    if (Vad != 0)
    {
       short foo;
       short dtxMode = ~IPP_SPCHBR_DTX;
       ippsEncDTXHandler_GSMAMR_16s(&st->dtxEncState.siHangoverCount, 
                  &st->dtxEncState.siAnaElapsedCount, &dtxMode, &foo, valVadFlag);
       if(dtxMode==IPP_SPCHBR_DTX) {
          *rate = AMRWB_RATE_DTX;
          irate = IPP_SPCHBR_DTX;
       }
    }
  
    if (*rate != AMRWB_RATE_DTX)
    {
        *(pPrm)++ = valVadFlag;
    }
     /* Perform LPC analysis */

    ownAutoCorr_AMRWB_16s32s(pWindow, LP_ORDER, pAutoCorrvec);
    ownLagWindow_AMRWB_32s_I(pAutoCorrvec);
    if(ippsLevinsonDurbin_G729_32s16s(pAutoCorrvec, LP_ORDER, pLPCUnQuantvec, pRCvec, &tmp) == ippStsOverflow){
         pLPCUnQuantvec[0] = 4096;
         ippsCopy_16s(st->asiLevinsonMem, &pLPCUnQuantvec[1], LP_ORDER);
         pRCvec[0] = st->asiLevinsonMem[LP_ORDER]; /* only two pRCvec coefficients are needed */
         pRCvec[1] = st->asiLevinsonMem[LP_ORDER+1];
    }else{
         ippsCopy_16s(pLPCUnQuantvec, st->asiLevinsonMem, LP_ORDER+1);
         st->asiLevinsonMem[LP_ORDER] = pRCvec[0]; 
         st->asiLevinsonMem[LP_ORDER+1] = pRCvec[1];
    }
    ippsLPCToISP_AMRWB_16s(pLPCUnQuantvec, pIspvec, st->asiIspOld);

    /* Find the interpolated ISPs and convert to pLPCUnQuantvec for all subframes */
    {
      IPP_ALIGNED_ARRAY(16, short, isp, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld, 18022, pIspvec,14746, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, pLPCUnQuantvec, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld,  6554, pIspvec,26214, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCUnQuantvec[LP_ORDER + 1], LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld,  1311, pIspvec,31457, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCUnQuantvec[2*(LP_ORDER + 1)], LP_ORDER);

      /* 4th subframe: pIspvec (frac=1.0) */

      ippsISPToLPC_AMRWB_16s(pIspvec, &pLPCUnQuantvec[3*(LP_ORDER + 1)], LP_ORDER);

      /* update asiIspOld[] for the next frame */
      ippsCopy_16s(pIspvec, st->asiIspOld, LP_ORDER);
    }

    /* Convert ISPs to frequency domain 0..6400 */
    ippsISPToISF_Norm_AMRWB_16s(pIspvec, pIsfvec, LP_ORDER);

    /* check resonance for pitch clipping algorithm */
    ownCheckGpClipIsf(pIsfvec, st->asiGainPitchClip);

     /* Perform PITCH_OL analysis */

    pLPCUnQuant = pLPCUnQuantvec;                               
    for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
    {
        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pLPCvec, LP_ORDER+1, 15);
        ippsResidualFilter_AMRWB_16s_Sfs(pLPCvec, LP_ORDER, &pSpeech[valSubFrame], &pWgtSpch[valSubFrame], SUBFRAME_SIZE,10);
        pLPCUnQuant += (LP_ORDER + 1);                    
    }
    ippsDeemphasize_AMRWB_NR_16s_I(TILT_FACTOR, pWgtSpch, FRAME_SIZE, &(st->siWsp));

    /* find maximum value on pWgtSpch[] for 12 bits scaling */
    ippsMinMax_16s(pWgtSpch, FRAME_SIZE, &tmp, &max);

    max = Abs_16s(max);
    tmp = Abs_16s(tmp);

    if(tmp > max) max = tmp;
    tmp = st->siWspOldMax;                 
   
    if (max > tmp) tmp = max;
    st->siWspOldMax = max;                 

    valShift = Exp_16s(tmp) - 3;
    if (valShift > 0) valShift = 0;               /* valShift = 0..-3 */

    /* decimation of pWgtSpch[] to search pitch in LF and to reduce complexity */
    ownLPDecim2(pWgtSpch, FRAME_SIZE, st->asiWspDecimate);

    /* scale pWgtSpch[] in 12 bits to avoid overflow */
    ownScaleSignal_AMRWB_16s_ISfs(pWgtSpch, FRAME_SIZE / OPL_DECIM, valShift);

    /* scale asiWspOld */
    valExp += (valShift - st->siWspOldShift);
    st->siWspOldShift = valShift;
    ownScaleSignal_AMRWB_16s_ISfs(pWgtSpchOldvec, PITCH_LAG_MAX / OPL_DECIM, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(st->asiHypassFiltWspOld, PITCH_LAG_MAX / OPL_DECIM, valExp);
    st->siScaleExp -= valExp;

    /* Find open loop pitch lag for whole speech frame */

    pHPWgtSpch = st->asiHypassFiltWspOld + PITCH_LAG_MAX / OPL_DECIM;

    if(irate == IPP_SPCHBR_6600) {
       ippsHighPassFilter_AMRWB_16s_Sfs(pWgtSpch, pHPWgtSpch, (FRAME_SIZE) / OPL_DECIM, st->pSHPFiltStateWsp,st->siScaleExp);
       ippsOpenLoopPitchSearch_AMRWB_16s(pWgtSpch, pHPWgtSpch, &st->siMedianOld, &st->siAdaptiveParam, &pOpenLoopLag[0],
                              &encoderObj->siToneFlag, &st->siOpenLoopGain,
                              st->asiPitchLagOld, &st->siWeightFlag, (FRAME_SIZE) / OPL_DECIM);
       ippsCopy_16s(&st->asiHypassFiltWspOld[(FRAME_SIZE) / OPL_DECIM],st->asiHypassFiltWspOld,PITCH_LAG_MAX / OPL_DECIM);
       pOpenLoopLag[1] = pOpenLoopLag[0];
    } else {
       ippsHighPassFilter_AMRWB_16s_Sfs(pWgtSpch, pHPWgtSpch, (FRAME_SIZE / 2) / OPL_DECIM, st->pSHPFiltStateWsp,st->siScaleExp);
       ippsOpenLoopPitchSearch_AMRWB_16s(pWgtSpch, pHPWgtSpch, &st->siMedianOld, &st->siAdaptiveParam, &pOpenLoopLag[0],
                              &encoderObj->siToneFlag, &st->siOpenLoopGain,
                              st->asiPitchLagOld, &st->siWeightFlag, (FRAME_SIZE / 2) / OPL_DECIM);
       ippsCopy_16s(&st->asiHypassFiltWspOld[(FRAME_SIZE / 2) / OPL_DECIM],st->asiHypassFiltWspOld,PITCH_LAG_MAX / OPL_DECIM);
       ippsHighPassFilter_AMRWB_16s_Sfs(pWgtSpch + ((FRAME_SIZE / 2) / OPL_DECIM), pHPWgtSpch, 
                              (FRAME_SIZE / 2) / OPL_DECIM, st->pSHPFiltStateWsp,st->siScaleExp);
       ippsOpenLoopPitchSearch_AMRWB_16s(pWgtSpch + ((FRAME_SIZE / 2) / OPL_DECIM), pHPWgtSpch, &st->siMedianOld, &st->siAdaptiveParam, &pOpenLoopLag[1],
                              &encoderObj->siToneFlag, &st->siOpenLoopGain,
                              st->asiPitchLagOld, &st->siWeightFlag, (FRAME_SIZE / 2) / OPL_DECIM);
       ippsCopy_16s(&st->asiHypassFiltWspOld[(FRAME_SIZE / 2) / OPL_DECIM],st->asiHypassFiltWspOld,PITCH_LAG_MAX / OPL_DECIM);
    }

    if (irate == IPP_SPCHBR_DTX)            /* CNG mode */
    {
        ippsResidualFilter_AMRWB_16s_Sfs(&pLPCUnQuantvec[3 * (LP_ORDER + 1)], LP_ORDER, pSpeech, pExc, FRAME_SIZE,10);
        ippsRShiftC_16s(pExc, valQNew, pExc2vec, FRAME_SIZE);
        ippsEncDTXBuffer_AMRWB_16s(pExc2vec, pIsfvec, &st->dtxEncState.siHistPtr, 
                  st->dtxEncState.asiIsfHistory, st->dtxEncState.siLogEnerHist, valCodecMode);
        ownDTXEnc(&st->dtxEncState, pIsfvec, pExc2vec, (unsigned short*)pPrm);

        /* Convert ISFs to the cosine domain */
        ippsISFToISP_AMRWB_16s(pIsfvec, pIspQuantvec, LP_ORDER);
        ippsISPToLPC_AMRWB_16s(pIspQuantvec, pLPCQuantvec, LP_ORDER);

        for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
        {
            valCorrGain = ownSynthesis(pLPCQuantvec, &pExc2vec[valSubFrame], 0, &src[valSubFrame * 5 / 4], st);
        }
        ippsCopy_16s(pIsfvec, st->asiIsfOld, LP_ORDER);
        InitEncoder(encoderObj);
        ownPrms2Bits(pPrmvec,dst,AMRWB_RATE_DTX);

        ippsCopy_16s(&pSpeechOldvec[FRAME_SIZE], st->asiSpeechOld, SPEECH_SIZE - FRAME_SIZE);
        ippsCopy_16s(&pWgtSpchOldvec[FRAME_SIZE / OPL_DECIM], st->asiWspOld, PITCH_LAG_MAX / OPL_DECIM);

        return APIAMRWB_StsNoErr;
    }

    ippsISFQuant_AMRWB_16s(pIsfvec, st->asiIsfQuantPast, pIsfvec, (short*)pPrm, irate);

    if (irate == IPP_SPCHBR_6600)
        pPrm += 5;
    else
        pPrm += 7;

    /* Check stability on pIsfvec : distance between old pIsfvec and current isf */
    if (irate == IPP_SPCHBR_23850) {
        valStabFac = ownChkStab(pIsfvec, st->asiIsfOld, LP_ORDER-1);
    }
    ippsCopy_16s(pIsfvec, st->asiIsfOld, LP_ORDER);

    /* Convert ISFs to the cosine domain */
    ippsISFToISP_AMRWB_16s(pIsfvec, pIspQuantvec, LP_ORDER);
    
    if (st->siFrameFirst != 0)
    {
        st->siFrameFirst = 0;               
        ippsCopy_16s(pIspQuantvec, st->asiIspQuantOld, LP_ORDER);
    }

    /* Find the interpolated ISPs and convert to pIspQuantvec[] for all subframes */

    {
      IPP_ALIGNED_ARRAY( 16, short, isp, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspQuantOld, 18022, pIspQuantvec,14746, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, pLPCQuantvec, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspQuantOld,  6554, pIspQuantvec,26214, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCQuantvec[LP_ORDER + 1], LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspQuantOld,  1311, pIspQuantvec,31457, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCQuantvec[2*(LP_ORDER + 1)], LP_ORDER);

      /* 4th subframe: pIspQuantvec (frac=1.0) */

      ippsISPToLPC_AMRWB_16s(pIspQuantvec, &pLPCQuantvec[3*(LP_ORDER + 1)], LP_ORDER);

      ippsCopy_16s(pIspQuantvec, st->asiIspQuantOld, LP_ORDER);
    }

    pLPCQuant = pLPCQuantvec;
    for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
    {
        ippsResidualFilter_AMRWB_16s_Sfs(pLPCQuant, LP_ORDER, &pSpeech[valSubFrame], &pExc[valSubFrame], SUBFRAME_SIZE,10);
        pLPCQuant += (LP_ORDER + 1);                   
    }

    /* Buffer isf's and energy for dtx on non-speech frame */
    
    if (valVadFlag == 0)
    {
        ippsRShiftC_16s(pExc, valQNew, pExc2vec, FRAME_SIZE);
        ippsEncDTXBuffer_AMRWB_16s(pExc2vec, pIsfvec, &st->dtxEncState.siHistPtr, 
                  st->dtxEncState.asiIsfHistory, st->dtxEncState.siLogEnerHist, valCodecMode);
    }

    /* Loop for every subframe in the analysis frame */

    pLPCUnQuant = pLPCUnQuantvec;                               
    pLPCQuant = pLPCQuantvec;                             

    for (valSubFrame = 0,subFrame=0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE, subFrame++)
    {
        ippsSub_16s(st->asiSynthesis, &pSpeech[valSubFrame - LP_ORDER], pErrQuant, LP_ORDER);
        ippsResidualFilter_AMRWB_16s_Sfs(pLPCQuant, LP_ORDER, &pSpeech[valSubFrame], &pExc[valSubFrame], SUBFRAME_SIZE,10);
        {
           short tmp;
           tmp = pLPCQuant[0];
           pLPCQuant[0] >>= 1;
           ippsSynthesisFilter_G729E_16s(pLPCQuant, LP_ORDER,  &pExc[valSubFrame], pErrQuant + LP_ORDER, SUBFRAME_SIZE, pErrQuant);
           pLPCQuant[0] = tmp;
        }

        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pLPCvec, LP_ORDER+1, 15);
        ippsResidualFilter_AMRWB_16s_Sfs(pLPCvec, LP_ORDER, pErrQuant + LP_ORDER, pAdptTgtPitch, SUBFRAME_SIZE,10);

        ippsDeemphasize_AMRWB_NR_16s_I(TILT_FACTOR, pAdptTgtPitch, SUBFRAME_SIZE, &(st->siSpeechWgt));

        ippsZero_16s(pFixCdbkvec, LP_ORDER);
        ippsCopy_16s(pAdptTgtPitch, pFixCdbkvec + LP_ORDER, SUBFRAME_SIZE / 2);

        tmp = 0;                           
        ippsPreemphasize_AMRWB_16s_ISfs (TILT_FACTOR, pFixCdbkvec + LP_ORDER, SUBFRAME_SIZE / 2, 13, &tmp);
        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pLPCvec, LP_ORDER+1, 15);
        {
           short tmp;
           tmp = pLPCvec[0];
           pLPCvec[0] >>= 1;
           ippsSynthesisFilter_G729E_16s_I(pLPCvec, LP_ORDER, pFixCdbkvec + LP_ORDER, SUBFRAME_SIZE / 2, pFixCdbkvec);
           pLPCvec[0] = tmp;
        }
        ippsResidualFilter_AMRWB_16s_Sfs(pLPCQuant, LP_ORDER, pFixCdbkvec + LP_ORDER, pResidvec, SUBFRAME_SIZE / 2,10);

        ippsCopy_16s(&pExc[valSubFrame + (SUBFRAME_SIZE / 2)], pResidvec + (SUBFRAME_SIZE / 2), SUBFRAME_SIZE / 2);

        ippsZero_16s(pErrQuant, LP_ORDER + SUBFRAME_SIZE);
        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pErrQuant + LP_ORDER, LP_ORDER+1, 15);
        {
           short tmp;
           tmp = pLPCQuant[0];
           pLPCQuant[0] = 16384;
           ippsSynthesisFilter_G729E_16s_I(pLPCQuant, LP_ORDER, pErrQuant + LP_ORDER, SUBFRAME_SIZE, pErrQuant);
           pLPCQuant[0] = tmp;
           ippsCopy_16s(pErrQuant + LP_ORDER,pImpRespPitchvec,SUBFRAME_SIZE);
        }
        tmp = 0;                           
        ippsDeemphasize_AMRWB_NR_16s_I(TILT_FACTOR, pImpRespPitchvec, SUBFRAME_SIZE, &tmp);

        ippsCopy_16s(pImpRespPitchvec, pImpRespCdbkvec, SUBFRAME_SIZE);
        ownScaleSignal_AMRWB_16s_ISfs(pImpRespCdbkvec, SUBFRAME_SIZE, -2);

        ownScaleSignal_AMRWB_16s_ISfs(pAdptTgtPitch, SUBFRAME_SIZE, valShift);
        ownScaleSignal_AMRWB_16s_ISfs(pImpRespPitchvec, SUBFRAME_SIZE, 1 + valShift);

        /* find closed loop fractional pitch  lag */
        ippsAdaptiveCodebookSearch_AMRWB_16s(pAdptTgtPitch, pImpRespPitchvec, pOpenLoopLag, &valPitchLag, pPitchLagBounds, &pExc[valSubFrame], &valFracPitchLag,
                                 (short*)pPrm, subFrame, irate);
        pPrm += 1;

        /* Gain clipping test to avoid unstable synthesis on frame erasure */
        valClipFlag = ownGpClip(st->asiGainPitchClip);
        
        if ((irate != IPP_SPCHBR_6600)&&(irate != IPP_SPCHBR_8850))
        {
            ippsConvPartial_NR_16s(&pExc[valSubFrame], pImpRespPitchvec, pFltAdptvec, SUBFRAME_SIZE);
            ippsAdaptiveCodebookGainCoeff_AMRWB_16s(pAdptTgtPitch, pFltAdptvec, pGainCoeff, &valGainCoeff, SUBFRAME_SIZE);
            if ((valClipFlag != 0) && (valGainCoeff > PITCH_GAIN_CLIP))
                valGainCoeff = PITCH_GAIN_CLIP;           
            ippsInterpolateC_G729_16s_Sfs(pAdptTgtPitch, 16384, pFltAdptvec,-valGainCoeff, pCorrvec, SUBFRAME_SIZE, 14); /* pCorrvec used temporary */
        } else
        {
            valGainCoeff = 0;                     
        }

        /* find pitch excitation with lp filter */
        pSrcCoeff[0] = 20972;
        pSrcCoeff[1] = -5898;
        ippsHighPassFilter_Direct_AMRWB_16s(pSrcCoeff, &pExc[valSubFrame], pFixCdbkvec, SUBFRAME_SIZE, 1);

        ippsConvPartial_NR_16s(pFixCdbkvec, pImpRespPitchvec, pFltAdpt2vec, SUBFRAME_SIZE);
        ippsAdaptiveCodebookGainCoeff_AMRWB_16s(pAdptTgtPitch, pFltAdpt2vec, pGainCoeff2, &valAdptGain, SUBFRAME_SIZE);

        if ((valClipFlag != 0) && (valAdptGain > PITCH_GAIN_CLIP))
            valAdptGain = PITCH_GAIN_CLIP;               
        ippsInterpolateC_G729_16s_Sfs(pAdptTgtPitch, 16384, pFltAdpt2vec,-valAdptGain, pAdptTgtCdbk, SUBFRAME_SIZE, 14);

        valSelect = 0;                        
        if ((irate != IPP_SPCHBR_6600)&&(irate != IPP_SPCHBR_8850))
        {
           int valCorrSum, valCdbkSum;
           ippsDotProd_16s32s_Sfs( pCorrvec, pCorrvec,SUBFRAME_SIZE, &valCorrSum, 0);
           ippsDotProd_16s32s_Sfs( pAdptTgtCdbk, pAdptTgtCdbk,SUBFRAME_SIZE, &valCdbkSum, 0);

           if (valCorrSum <= valCdbkSum) valSelect = 1;                
           *(pPrm)++ = valSelect;
        }
        
        if (valSelect == 0)
        {
            /* use the lp filter for pitch excitation prediction */
            valPitchGain = valAdptGain;              
            ippsCopy_16s(pFixCdbkvec, &pExc[valSubFrame], SUBFRAME_SIZE);
            ippsCopy_16s(pFltAdpt2vec, pFltAdptvec, SUBFRAME_SIZE);
            ippsCopy_16s(pGainCoeff2, pGainCoeff, 4);
        } else
        {
            /* no filter used for pitch excitation prediction */
            valPitchGain = valGainCoeff;              
            ippsCopy_16s(pCorrvec, pAdptTgtCdbk, SUBFRAME_SIZE);
        }

        ippsInterpolateC_G729_16s_Sfs(pResidvec, 16384, &pExc[valSubFrame],-valPitchGain, pResidvec, SUBFRAME_SIZE, 14);
        ownScaleSignal_AMRWB_16s_ISfs(pResidvec, SUBFRAME_SIZE, valShift);

        tmp = 0;                           
        ippsPreemphasize_AMRWB_16s_ISfs (st->siTiltCode, pImpRespCdbkvec, SUBFRAME_SIZE, 14, &tmp);
        
        if (valFracPitchLag > 2) valPitchLag += 1;

        if (valPitchLag < SUBFRAME_SIZE) {
            ippsHarmonicFilter_NR_16s(PITCH_SHARP_FACTOR, valPitchLag, &pImpRespCdbkvec[valPitchLag], &pImpRespCdbkvec[valPitchLag], SUBFRAME_SIZE-valPitchLag);
        }

        ippsAlgebraicCodebookSearch_AMRWB_16s(pAdptTgtCdbk, pResidvec, pImpRespCdbkvec, pFixCdbkvec, pFltAdpt2vec, irate, (short*)pPrm);

        if (irate == IPP_SPCHBR_6600)
        {
            pPrm += 1;
        } else if ((irate == IPP_SPCHBR_8850) || (irate == IPP_SPCHBR_12650) ||
            (irate == IPP_SPCHBR_14250) || (irate == IPP_SPCHBR_15850))
        {
            pPrm += 4;
        } else
        {
            pPrm += 8;
        }

        tmp = 0;                           
        ippsPreemphasize_AMRWB_16s_ISfs (st->siTiltCode, pFixCdbkvec, SUBFRAME_SIZE, 14,  &tmp);

        if (valPitchLag < SUBFRAME_SIZE) {
            ippsHarmonicFilter_NR_16s(PITCH_SHARP_FACTOR, valPitchLag, &pFixCdbkvec[valPitchLag], &pFixCdbkvec[valPitchLag], SUBFRAME_SIZE-valPitchLag);
        }

        ippsGainQuant_AMRWB_16s(pAdptTgtPitch, pFltAdptvec, (valQNew + valShift), pFixCdbkvec, pFltAdpt2vec, pGainCoeff, 
                  st->asiGainQuant, &valPitchGain, &valCodeGain, valClipFlag, (short*)pPrm, SUBFRAME_SIZE, irate);
        pPrm += 1;

        /* test quantized gain of pitch for pitch clipping algorithm */
        ownCheckGpClipPitchGain(valPitchGain, st->asiGainPitchClip);

        z = ShiftL_32s(valCodeGain, valQNew);
        valScaleCodeGain = Cnvrt_NR_32s16s(z);

        /* find voice factor (1=voiced, -1=unvoiced) */

        ippsCopy_16s(&pExc[valSubFrame], pExc2vec, SUBFRAME_SIZE);
        ownScaleSignal_AMRWB_16s_ISfs(pExc2vec, SUBFRAME_SIZE, valShift);

        valVoiceFac = ownVoiceFactor(pExc2vec, valShift, valPitchGain, pFixCdbkvec, valScaleCodeGain, SUBFRAME_SIZE);

        /* tilt of code for next subframe: 0.5=voiced, 0=unvoiced */

        st->siTiltCode = (valVoiceFac >> 2) + 8192;   

        z = valScaleCodeGain * pFltAdpt2vec[SUBFRAME_SIZE - 1];
        z <<= (5 + valShift);
        z = Negate_32s(z);
        z += pAdptTgtPitch[SUBFRAME_SIZE - 1] * 16384;
        z -= pFltAdptvec[SUBFRAME_SIZE - 1] * valPitchGain;
        z <<= (2 - valShift);
        st->siSpeechWgt = Cnvrt_NR_32s16s(z);         

        if (irate == IPP_SPCHBR_23850)
            ippsCopy_16s(&pExc[valSubFrame], pExc2vec, SUBFRAME_SIZE);

        ippsInterpolateC_NR_16s(pFixCdbkvec, valScaleCodeGain, 7,
            &pExc[valSubFrame], valPitchGain, 2, &pExc[valSubFrame], SUBFRAME_SIZE);

        {
           short tmp;
           tmp = pLPCQuant[0];
           pLPCQuant[0] >>= 1;
           ippsSynthesisFilter_G729E_16s(pLPCQuant, LP_ORDER,  &pExc[valSubFrame], pSynthvec, SUBFRAME_SIZE, st->asiSynthesis);
           ippsCopy_16s(&pSynthvec[SUBFRAME_SIZE-LP_ORDER],st->asiSynthesis,LP_ORDER);
           pLPCQuant[0] = tmp;
        }

        if (irate == IPP_SPCHBR_23850)
        {
            L_Extract(valCodeGain, &valScaleCodeGain, &valCodeGainLow);

            /* noise enhancer */
            tmp = 16384 - (valVoiceFac >> 1);        /* 1=unvoiced, 0=voiced */
            valFac = Mul_16s_Sfs(valStabFac, tmp, 15);

            z = valCodeGain;           
            
            if (z < st->iNoiseEnhancerThres)
            {
                z += Mpy_32_16(valScaleCodeGain, valCodeGainLow, 6226);
                if (z > st->iNoiseEnhancerThres) z = st->iNoiseEnhancerThres;
            } else
            {
                z = Mpy_32_16(valScaleCodeGain, valCodeGainLow, 27536);
                if (z < st->iNoiseEnhancerThres) z = st->iNoiseEnhancerThres;
            }
            st->iNoiseEnhancerThres = z;        

            valCodeGain = Mpy_32_16(valScaleCodeGain, valCodeGainLow, (IPP_MAX_16S - valFac));
            L_Extract(z, &valScaleCodeGain, &valCodeGainLow);
            valCodeGain += Mpy_32_16(valScaleCodeGain, valCodeGainLow, valFac);

            /* pitch enhancer */
            tmp = (valVoiceFac >> 3) + 4096; /* 0.25=voiced, 0=unvoiced */

            pSrcCoeff[0] = 1;
            pSrcCoeff[1] = tmp;
            ippsHighPassFilter_Direct_AMRWB_16s(pSrcCoeff, pFixCdbkvec, pFixCdbkExcvec, SUBFRAME_SIZE, 0);

            /* build excitation */

            valScaleCodeGain = Cnvrt_NR_32s16s(valCodeGain << valQNew);
            ippsInterpolateC_NR_16s(pFixCdbkExcvec, valScaleCodeGain, 7, 
                pExc2vec, valPitchGain, 2, pExc2vec,SUBFRAME_SIZE);
            valCorrGain = ownSynthesis(pLPCQuant, pExc2vec, valQNew, &src[valSubFrame * 5 / 4], st);
            *(pPrm)++ = valCorrGain;
        }
        pLPCUnQuant += (LP_ORDER + 1);
        pLPCQuant += (LP_ORDER + 1);
    }                                      /* end of subframe loop */

    ownPrms2Bits(pPrmvec,dst,*rate);

    ippsCopy_16s(&pSpeechOldvec[FRAME_SIZE], st->asiSpeechOld, SPEECH_SIZE - FRAME_SIZE);
    ippsCopy_16s(&pWgtSpchOldvec[FRAME_SIZE / OPL_DECIM], st->asiWspOld, PITCH_LAG_MAX / OPL_DECIM);
    ippsCopy_16s(&pExcOldvec[FRAME_SIZE], st->asiExcOld, PITCH_LAG_MAX + INTERPOL_LEN);

    return APIAMRWB_StsNoErr;
}

/* Synthesis of signal at 16kHz with pHighFreqvec extension */
static short ownSynthesis(short *pLPCQuantvec, short *pExc, short valQNew,
                          const unsigned short *pSynthSignal, AMRWBEncoder_Obj *st)
{
    short valFac, tmp, valExp;
    short valEner, valExpEner;
    int i, z;

    IPP_ALIGNED_ARRAY( 16, int, pSynthHighvec, LP_ORDER + SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY( 16, short, pSynthvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY( 16, short, pHighFreqvec, SUBFRAME_SIZE_16k);
    IPP_ALIGNED_ARRAY( 16, short, pLPCvec, LP_ORDER + 1);
    IPP_ALIGNED_ARRAY( 16, short, pHighFreqSpchvec, SUBFRAME_SIZE_16k);

    short valHPEstGain, valHPCalcGain, valHPCorrGain;
    short valDistMin, valDist;
    short valHPGainIndex = 0;
    short valGainCoeff, valAdptGain;
    short valWeight, valWeight2;

    /* speech synthesis */
    ippsCopy_16s((short*)st->aiSynthMem, (short*)pSynthHighvec, LP_ORDER*2);
    tmp = pLPCQuantvec[0];
    pLPCQuantvec[0] >>= (4 + valQNew);
    ippsSynthesisFilter_AMRWB_16s32s_I(pLPCQuantvec, LP_ORDER, pExc, pSynthHighvec + LP_ORDER, SUBFRAME_SIZE);
    pLPCQuantvec[0] = tmp;

    ippsCopy_16s((short*)(&pSynthHighvec[SUBFRAME_SIZE]), (short*)st->aiSynthMem, LP_ORDER*2);
    ippsDeemphasize_AMRWB_32s16s(PREEMPH_FACTOR>>1, pSynthHighvec + LP_ORDER, pSynthvec, SUBFRAME_SIZE, &(st->siDeemph));
    ippsHighPassFilter_AMRWB_16s_ISfs(pSynthvec, SUBFRAME_SIZE, st->pSHPFiltStateSgnlOut, 14);

    /* Original speech signal as reference for high band gain quantisation */
    ippsCopy_16s((short*)pSynthSignal,pHighFreqSpchvec,SUBFRAME_SIZE_16k);

    /* pHighFreqvec noise synthesis */

    for (i = 0; i < SUBFRAME_SIZE_16k; i++)
        pHighFreqvec[i] = (Random(&(st->siHFSeed)) >> 3);   

    /* energy of excitation */

    ownScaleSignal_AMRWB_16s_ISfs(pExc, SUBFRAME_SIZE, -3);
    valQNew -= 3;

    ippsDotProd_16s32s_Sfs( pExc, pExc,SUBFRAME_SIZE, &z, -1);
    z = Add_32s(z, 1);

    valExpEner = (30 - Norm_32s_I(&z));
    valEner = z >> 16;

    valExpEner -= (valQNew + valQNew);

    /* set energy of white noise to energy of excitation */

    ippsDotProd_16s32s_Sfs( pHighFreqvec, pHighFreqvec,SUBFRAME_SIZE_16k, &z, -1);
    z = Add_32s(z, 1);

    valExp = (30 - Norm_32s_I(&z));
    tmp = z >> 16;
    
    if (tmp > valEner)
    {
        tmp >>= 1;
        valExp += 1;
    }
    if (tmp == valEner) 
        z = (int)IPP_MAX_16S << 16;
    else
        z = (int)(((int)tmp << 15) / (int)valEner) << 16;
    valExp -= valExpEner ;
    ownInvSqrt_AMRWB_32s16s_I(&z, &valExp);
    if ((valExp+1) >= 0)
        z = ShiftL_32s(z, (valExp + 1));
    else
        z >>= (-(valExp + 1));
    tmp = (short)(z >> 16);

    ownMulC_16s_ISfs(tmp, pHighFreqvec, SUBFRAME_SIZE_16k, 15);

    /* find tilt of synthesis speech (tilt: 1=voiced, -1=unvoiced) */

    ippsHighPassFilter_AMRWB_16s_ISfs(pSynthvec, SUBFRAME_SIZE, st->pSHPFiltStateSgnl400, 15);
    ippsDotProd_16s32s_Sfs( pSynthvec, pSynthvec,SUBFRAME_SIZE, &z, -1);
    z = Add_32s(z, 1);

    valExp = Norm_32s_I(&z);
    valEner = z>>16;

    ippsDotProd_16s32s_Sfs( pSynthvec, &pSynthvec[1],SUBFRAME_SIZE-1, &z, -1);
    z = Add_32s(z, 1);

    tmp = ShiftL_32s(z, valExp) >> 16;
    if (tmp > 0)
    {
        if (tmp == valEner) 
            valFac = IPP_MAX_16S;
        else 
            valFac = (short)(((int)tmp << 15) / (int)valEner);
    } else
        valFac = 0;                           

    /* modify energy of white noise according to synthesis tilt */
    valGainCoeff = IPP_MAX_16S - valFac;
    valAdptGain = Cnvrt_32s16s((valGainCoeff * 5) >> 2);
    
    if (st->siVadHist > 0)
    {
        valWeight = 0;
        valWeight2 = IPP_MAX_16S;
    } else
    {
        valWeight = IPP_MAX_16S;
        valWeight2 = 0;
    }
    tmp = Mul_16s_Sfs(valWeight, valGainCoeff, 15);
    tmp += Mul_16s_Sfs(valWeight2, valAdptGain, 15);
    
    if (tmp != 0) tmp += 1;
    valHPEstGain = tmp;
    
    if (valHPEstGain < 3277) valHPEstGain = 3277;

    /* synthesis of noise: 4.8kHz..5.6kHz --> 6kHz..7kHz */
    ippsMulPowerC_NR_16s_Sfs(pLPCQuantvec, 19661, pLPCvec, LP_ORDER+1, 15); /* valFac=0.6 */
    {
      short tmp;
      tmp = pLPCvec[0];
      pLPCvec[0] >>= 1;
      ippsSynthesisFilter_G729E_16s_I(pLPCvec, LP_ORDER, pHighFreqvec, SUBFRAME_SIZE_16k, st->asiSynthHighFilt);
      ippsCopy_16s(&pHighFreqvec[SUBFRAME_SIZE_16k-LP_ORDER],st->asiSynthHighFilt,LP_ORDER);
      pLPCvec[0] = tmp;
    } 

    /* noise High Pass filtering (1ms of delay) */
    HighPassFIR_AMRWB_16s_ISfs(pHighFreqvec, SUBFRAME_SIZE_16k, st->pSHPFIRState);

    /* filtering of the original signal */
    HighPassFIR_AMRWB_16s_ISfs(pHighFreqSpchvec, SUBFRAME_SIZE_16k, st->pSHPFIRState2);

    /* check the gain difference */
    ownScaleSignal_AMRWB_16s_ISfs(pHighFreqSpchvec, SUBFRAME_SIZE_16k, -1);

    ippsDotProd_16s32s_Sfs( pHighFreqSpchvec, pHighFreqSpchvec,SUBFRAME_SIZE_16k, &z, -1);
    z = Add_32s(z, 1);

    valExpEner = (30 - Norm_32s_I(&z));
    valEner = z >> 16;

    /* set energy of white noise to energy of excitation */
    ippsDotProd_16s32s_Sfs( pHighFreqvec, pHighFreqvec,SUBFRAME_SIZE_16k, &z, -1);
    z = Add_32s(z, 1);

    valExp = (30 - Norm_32s_I(&z));
    tmp = z >> 16;
    
    if (tmp > valEner)
    {
        tmp >>= 1;
        valExp += 1;
    }
    if (tmp == valEner) 
        z = (int)IPP_MAX_16S << 16;
    else
        z = (int)(((int)tmp << 15) / (int)valEner) << 16;
    valExp -= valExpEner;
    ownInvSqrt_AMRWB_32s16s_I(&z, &valExp);
    if (valExp > 0)
        z = ShiftL_32s(z, valExp);
    else
        z >>= (-valExp);
    valHPCalcGain = (short)(z >> 16);

    z = st->dtxEncState.siHangoverCount * 4681;
    st->siAlphaGain = Mul_16s_Sfs(st->siAlphaGain, (short)z, 15);
    
    if (st->dtxEncState.siHangoverCount > 6)
        st->siAlphaGain = IPP_MAX_16S;
    valHPEstGain >>= 1;
    valHPCorrGain  = Mul_16s_Sfs(valHPCalcGain, st->siAlphaGain, 15);
    valHPCorrGain += Mul_16s_Sfs((IPP_MAX_16S - st->siAlphaGain), valHPEstGain, 15);

    /* Quantise the correction gain */
    valDistMin = IPP_MAX_16S;
    for (i = 0; i < 16; i++)
    {
        valDist = Mul_16s_Sfs((valHPCorrGain - HPGainTbl[i]), (valHPCorrGain - HPGainTbl[i]), 15);
        if (valDistMin > valDist)
        {
            valDistMin = valDist;
            valHPGainIndex = i;
        }
    }

    valHPCorrGain = HPGainTbl[valHPGainIndex];

    /* return the quantised gain index when using the highest mode, otherwise zero */
    return (valHPGainIndex);
}
