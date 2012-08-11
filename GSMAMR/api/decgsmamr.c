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
//  Purpose: GSM AMR-NB speech decoder API
//
*/

#include "owngsmamr.h"

/* filter coefficients (fc = 100 Hz) */

static const short b100[3] = {7699, -15398, 7699};      /* Q13 */
static const short a100[3] = {8192, 15836, -7667};      /* Q13 */


GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_GetSize,
         (GSMAMRDecoder_Obj* decoderObj, unsigned int *pCodecSize))
{
   if(NULL == decoderObj)
      return APIGSMAMR_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIGSMAMR_StsBadArgErr;
   if(decoderObj->objPrm.key != DEC_KEY)
      return APIGSMAMR_StsNotInitialized;

   *pCodecSize = decoderObj->objPrm.objSize;
   return APIGSMAMR_StsNoErr;
}


GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_Alloc,
         (const GSMAMRDec_Params *gsm_Params, unsigned int *pSizeTab))
{
   int fltSize;
   int allocSize=sizeof(GSMAMRDecoder_Obj);
   if(GSMAMR_CODEC != gsm_Params->codecType)
      return APIGSMAMR_StsBadCodecType;
   ippsHighPassFilterSize_G729(&fltSize);
   allocSize += fltSize; /* to provide memory for postprocessing high pass filter with upscaling */
   pSizeTab[0] =  allocSize;
   return APIGSMAMR_StsNoErr;
}

int ownDtxDecoderInit_GSMAMR(sDTXDecoderSt* dtxState)
{
   int i;

   dtxState->vLastSidFrame = 0;
   dtxState->vSidPeriodInv = (1 << 13);
   dtxState->vLogEnergy = 3500;
   dtxState->vLogEnergyOld = 3500;
   /* low level noise for better performance in  DTX handover cases*/

   dtxState->vPerfSeedDTX_long = PSEUDO_NOISE_SEED;

   /* Initialize state->a_LSP [] and state->a_LSP_Old [] */
   ippsCopy_16s(TableLSPInitData, &dtxState->a_LSP[0], LP_ORDER_SIZE);
   ippsCopy_16s(TableLSPInitData, &dtxState->a_LSP_Old[0], LP_ORDER_SIZE);
   dtxState->vLogMean = 0;
   dtxState->vLogEnergyHistory = 0;

   /* initialize decoder lsf history */
   ippsCopy_16s(TableMeanLSF_5, &dtxState->a_LSFHistory[0], LP_ORDER_SIZE);

   for (i = 1; i < DTX_HIST_SIZE; i++)
      ippsCopy_16s(&dtxState->a_LSFHistory[0], &dtxState->a_LSFHistory[LP_ORDER_SIZE*i], LP_ORDER_SIZE);
   
   ippsZero_16s(dtxState->a_LSFHistoryMean, LP_ORDER_SIZE*DTX_HIST_SIZE);

   /* initialize decoder log frame energy */
   ippsSet_16s(dtxState->vLogEnergy, dtxState->a_LogEnergyHistory, DTX_HIST_SIZE);
   dtxState->vLogEnergyCorrect = 0;
   dtxState->vDTXHangoverCt = DTX_HANG_PERIOD;
   dtxState->vDecExpireCt = 32767;
   dtxState->vFlagSidFrame = 0;
   dtxState->vFlagValidData = 0;
   dtxState->vDTXHangAdd = 0;
   dtxState->eDTXPrevState = DTX;
   dtxState->vFlagDataUpdate = 0;
   return 1;
}

int ownPhDispInit_GSMAMR(sPhaseDispSt* PhState)
{
   ippsZero_16s(PhState->a_GainMemory, LTP_GAIN_MEM_SIZE);
   PhState->vPrevState = 0;
   PhState->vPrevGain = 0;
   PhState->vFlagLockGain = 0;
   PhState->vOnSetGain = 0;          /* assume no onset in start */

   return 1;
}

int ownPostFilterInit_GSMAMR(sPostFilterSt *PostState)
{
   ippsZero_16s(PostState->a_MemSynPst, LP_ORDER_SIZE);
   ippsZero_16s(PostState->a_SynthBuf, LP_ORDER_SIZE);
   PostState->vPastGainScale = 4096;
   PostState->vMemPrevRes = 0;

  return 1;
}

int ownDecoderInit_GSMAMR(sDecoderState_GSMAMR* decState, GSMAMR_Rate_t rate)
{
   /* Initialize static pointer */
   decState->pExcVec = decState->a_ExcVecOld + PITCH_MAX_LAG + FLT_INTER_SIZE;

   /* Static vectors to zero */
   ippsZero_16s(decState->a_ExcVecOld, PITCH_MAX_LAG + FLT_INTER_SIZE);

   if (rate != GSMAMR_RATE_DTX)
     ippsZero_16s(decState->a_MemorySyn, LP_ORDER_SIZE);

   /* initialize pitch sharpening */
   decState->vFlagSharp = PITCH_SHARP_MIN;
   decState->vPrevPitchLag = 40;

   /* Initialize decState->lsp_old [] */

   if (rate != GSMAMR_RATE_DTX) {
      ippsCopy_16s(TableLSPInitData, &decState->a_LSP_Old[0], LP_ORDER_SIZE);
   }

   /* Initialize memories of bad frame handling */
   decState->vPrevBadFr = 0;
   decState->vPrevDegBadFr = 0;
   decState->vStateMachine = 0;

   decState->vLTPLag = 40;
   decState->vBackgroundNoise = 0;
   ippsZero_16s(decState->a_EnergyHistVector, ENERGY_HIST_SIZE);

   /* Initialize hangover handling */
   decState->vCountHangover = 0;
   decState->vVoiceHangover = 0;
   if (rate != GSMAMR_RATE_DTX) {
      ippsZero_16s(decState->a_EnergyHistSubFr, 9);
   }

   ippsZero_16s(decState->a_LTPGainHistory, 9);
   ippsZero_16s(decState->a_GainHistory, CBGAIN_HIST_SIZE);

   /* Initialize hangover handling */
   decState->vHgAverageVar = 0;
   decState->vHgAverageCount= 0;
   if (rate != GSMAMR_RATE_DTX)
     ippsCopy_16s(TableMeanLSF_5, &decState->a_LSPAveraged[0], LP_ORDER_SIZE);
   ippsZero_16s(decState->a_PastQntPredErr, LP_ORDER_SIZE);             /* Past quantized prediction error */

   /* Past dequantized lsfs */
   ippsCopy_16s(TableMeanLSF_5, &decState->a_PastLSFQnt[0], LP_ORDER_SIZE);
   ippsSet_16s(1640, decState->a_LSFBuffer, 5);
   decState->vPastGainZero = 0;
   decState->vPrevGainZero = 16384;
   ippsSet_16s(1, decState->a_GainBuffer, 5);
   decState->vPastGainCode = 0;
   decState->vPrevGainCode = 1;

   if (rate != GSMAMR_RATE_DTX) {
        ippsSet_16s(MIN_ENERGY, decState->a_PastQntEnergy, NUM_PRED_TAPS);
        ippsSet_16s(MIN_ENERGY_M122, decState->a_PastQntEnergy_M122, NUM_PRED_TAPS);
   }

   decState->vCNGen = 21845;
   ownPhDispInit_GSMAMR(&decState->stPhDispState);
   if (rate != GSMAMR_RATE_DTX)
     ownDtxDecoderInit_GSMAMR(&decState->dtxDecoderState);
   ownDecSidSyncReset_GSMAMR(&decState->dtxDecoderState);

   return 1;
}
/*************************************************************************
* apiGSMAMRDecoder_Init()
*   Initializes the decoder object
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecoder_Init, (GSMAMRDecoder_Obj* decoderObj, unsigned int mode))
{
   int fltSize;
   IPP_ALIGNED_ARRAY(16, Ipp16s, abDec, 6);
   int obj_size = sizeof(GSMAMRDecoder_Obj);
   ippsZero_16s((short*)decoderObj,sizeof(GSMAMRDecoder_Obj)>>1) ;   /* non-initialized data, bug in debug mode AK27.08.01 */
   decoderObj->objPrm.mode = mode;
   decoderObj->objPrm.key = DEC_KEY;
   decoderObj->postProc = (char*)decoderObj + sizeof(GSMAMRDecoder_Obj);
   ippsHighPassFilterSize_G729(&fltSize);
   decoderObj->objPrm.objSize = obj_size+fltSize;

   abDec[0] = a100[0];
   abDec[1] = a100[1];
   abDec[2] = a100[2];
   abDec[3] = b100[0];
   abDec[4] = b100[1];
   abDec[5] = b100[2];
   ownPostFilterInit_GSMAMR(&decoderObj->stPFiltState);
   ippsHighPassFilterInit_G729(abDec,decoderObj->postProc);
   ownDecoderInit_GSMAMR(&decoderObj->stDecState,decoderObj->rate);
   return APIGSMAMR_StsNoErr;
}

static int ownDecode_GSMAMR(sDecoderState_GSMAMR *decState,  GSMAMR_Rate_t rate, short *pSynthParm,    
    enum enDTXStateType newDTXState, RXFrameType frameType, short *pSynthSpeech, short *pA_LP);

/*************************************************************************
* apiGSMAMRDecode()
*   Decodes one frame: bitstream -> pcm audio
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMRDecode,
         (GSMAMRDecoder_Obj* decoderObj,const unsigned char* src, GSMAMR_Rate_t rate,
          RXFrameType rx_type, short* dst))
{
   IPP_ALIGNED_ARRAY(16, short, Az_dec, LP_ALL_FRAME);         /* Decoded Az for post-filter          */
   IPP_ALIGNED_ARRAY(16, short, prm_buf, MAX_NUM_PRM);
   short *prm = prm_buf;
   IPP_ALIGNED_ARRAY(16, short, tmp_synth, 160);
   enum enDTXStateType newDTXState;

   if(NULL==decoderObj || NULL==src || NULL ==dst)
      return APIGSMAMR_StsBadArgErr;
   if(0 >= decoderObj->objPrm.objSize)
      return APIGSMAMR_StsNotInitialized;
   if(rate > GSMAMR_RATE_DTX  || rate < 0)
      return APIGSMAMR_StsBadArgErr;
   if(DEC_KEY != decoderObj->objPrm.key)
      return APIGSMAMR_StsBadCodecType;
   decoderObj->rate = rate;
   newDTXState = ownRX_DTX_Handler_GSMAMR(decoderObj, rx_type);

   /* Synthesis */
   if( rx_type == RX_SID_BAD || rx_type == RX_SID_UPDATE) {
      ownBits2Prm_GSMAMR(src,prm,GSMAMR_RATE_DTX);
   }else{
      ownBits2Prm_GSMAMR(src,prm,decoderObj->rate);
   }
   ownDecode_GSMAMR(&decoderObj->stDecState, decoderObj->rate, prm, 
          newDTXState, rx_type, (short*)dst, Az_dec);
   /* Post-filter */
   ippsPostFilter_GSMAMR_16s(Az_dec, (short*)dst, &decoderObj->stPFiltState.vMemPrevRes,
                             &decoderObj->stPFiltState.vPastGainScale,
                             decoderObj->stPFiltState.a_SynthBuf,
                             decoderObj->stPFiltState.a_MemSynPst,
                             tmp_synth, mode2rates[rate]);
   ippsCopy_16s(tmp_synth, (short*)dst, 160);
   /* post HP filter, and 15->16 bits */
   ippsHighPassFilter_G729_16s_ISfs((short*)dst, FRAME_SIZE_GSMAMR, 13, decoderObj->postProc);
   return APIGSMAMR_StsNoErr;
}

/***************************************************************************
*  Function: ownDecode_GSMAMR     Speech decoder routine.
***************************************************************************/
/*
*  decState  [in/out] -  State variables
*  rate          [in] - GSMAMR rate 
*  pSynthParm    [in] - Vector of synthesis parameters (length of vector - PRM_SIZE)
*  newDTXState   [in] - State of DTX
*  frameType    [in] - Received frame type    
*  pSynthSpeech [out] - Synthesis speech (length of vector - FRAME_SIZE_GSMAMR)
*  pA_LP        [out] - Decoded LP filter in 4 subframes (length of vector - LP_ALL_FRAME)                                    
*/

static int ownDecode_GSMAMR(sDecoderState_GSMAMR *decState, GSMAMR_Rate_t rate, short *pSynthParm,                
                            enum enDTXStateType newDTXState, RXFrameType frameType,     
                            short *pSynthSpeech, short *pA_LP)
{
    /* LPC coefficients */
    short *Az;                /* Pointer on pA_LP */
    /* LSPs */
    IPP_ALIGNED_ARRAY(16, short, LspInter, SUBFR_SIZE_GSMAMR);
    /* LSFs */
    IPP_ALIGNED_ARRAY(16, short, prev_lsf, LP_ORDER_SIZE);
    IPP_ALIGNED_ARRAY(16, short, lsf_i, LP_ORDER_SIZE);
    /* Algebraic codevector */
    IPP_ALIGNED_ARRAY(16, short, code, SUBFR_SIZE_GSMAMR);
    /* excitation */
    IPP_ALIGNED_ARRAY(16, short, excp, SUBFR_SIZE_GSMAMR);
    IPP_ALIGNED_ARRAY(16, short, exc_enhanced, SUBFR_SIZE_GSMAMR);
    /* Scalars */
    short i, i_subfr;
    short T0, index, index_mr475 = 0;
    short gain_pit, gain_code, gain_code_mix, pit_sharp, pitch_fac;
    short tmp_shift, temp;
    short carefulFlag, excEnergy, subfrNr;
	int tmpRes;
    short evenSubfr = 0;
    short bfi = 0;   /* bad frame indication flag                          */
    short pdfi = 0;  /* potential degraded bad frame flag                  */
    IPP_ALIGNED_ARRAY(16, short, pDstAdptVector, SUBFR_SIZE_GSMAMR);
    sDTXDecoderSt *DTXState = &decState->dtxDecoderState;
    IppSpchBitRate irate = mode2rates[rate];

    /* find the new  DTX state  SPEECH OR DTX */
    /* function result */
    /* DTX actions */
    if (newDTXState != SPEECH)
    {
       ownDecoderInit_GSMAMR(decState,GSMAMR_RATE_DTX);
       ownDTXDecoder_GSMAMR(&decState->dtxDecoderState, decState->a_MemorySyn, decState->a_PastQntPredErr,
                            decState->a_PastLSFQnt, decState->a_PastQntEnergy, decState->a_PastQntEnergy_M122,
                            &decState->vHgAverageVar, &decState->vHgAverageCount, newDTXState, rate,
                            pSynthParm, pSynthSpeech, pA_LP);
       /* update average lsp */
       ippsLSFToLSP_GSMAMR_16s(decState->a_PastLSFQnt, decState->a_LSP_Old);
       ippsInterpolateC_NR_G729_16s_Sfs(decState->a_PastLSFQnt,EXP_CONST_016,
                                        decState->a_LSPAveraged,EXP_CONST_084,
                                        decState->a_LSPAveraged,LP_ORDER_SIZE,15);
       DTXState->eDTXPrevState = newDTXState;
       return 0;
    }
    /* SPEECH action state machine  */
    if((frameType == RX_SPEECH_BAD) || (frameType == RX_NO_DATA) || (frameType == RX_ONSET)) 
    {
       bfi = 1;
       if((frameType == RX_NO_DATA) || (frameType == RX_ONSET))
	     ownBuildCNParam_GSMAMR(&decState->vCNGen, TableParamPerModes[rate],TableBitAllModes[rate],pSynthParm);       
    }
    else if(frameType == RX_SPEECH_DEGRADED) pdfi = 1;
   
    if(bfi != 0)            decState->vStateMachine++;
    else if(decState->vStateMachine == 6) decState->vStateMachine = 5;
    else                     decState->vStateMachine = 0;

    if(decState->vStateMachine > 6) decState->vStateMachine = 6;
    if(DTXState->eDTXPrevState == DTX) {
       decState->vStateMachine = 5;  
	   decState->vPrevBadFr = 0;  
    } else if (DTXState->eDTXPrevState == DTX_MUTE) {
       decState->vStateMachine = 5; 
       decState->vPrevBadFr = 1;
    }

    /* save old LSFs for CB gain smoothing */
    ippsCopy_16s (decState->a_PastLSFQnt, prev_lsf, LP_ORDER_SIZE);
    /* decode LSF and generate interpolated lpc coefficients for the 4 subframes */
    ippsQuantLSPDecode_GSMAMR_16s(pSynthParm, decState->a_PastQntPredErr, decState->a_PastLSFQnt,
                                  decState->a_LSP_Old, LspInter, bfi, irate);
    ippsLSPToLPC_GSMAMR_16s(&(LspInter[0]), &(pA_LP[0]));  /* Subframe 1 */
    ippsLSPToLPC_GSMAMR_16s(&(LspInter[LP_ORDER_SIZE]), &(pA_LP[LP1_ORDER_SIZE]));  /* Subframe 2 */
    ippsLSPToLPC_GSMAMR_16s(&(LspInter[LP_ORDER_SIZE*2]), &(pA_LP[LP1_ORDER_SIZE*2]));  /* Subframe 3 */
    ippsLSPToLPC_GSMAMR_16s(&(LspInter[LP_ORDER_SIZE*3]), &(pA_LP[LP1_ORDER_SIZE*3]));  /* Subframe 4 */

    if(irate == IPP_SPCHBR_12200) pSynthParm += 5;
    else pSynthParm += 3;
    /*------------------------------------------------------------------------*
     *          Loop for every subframe in the analysis frame                 *
     *------------------------------------------------------------------------*/

    Az = pA_LP;      /* pointer to interpolated LPC parameters */
    evenSubfr = 0;
    subfrNr = -1;
    for (i_subfr = 0; i_subfr < FRAME_SIZE_GSMAMR; i_subfr += SUBFR_SIZE_GSMAMR)
    {
       subfrNr++;
       evenSubfr = 1 - evenSubfr;
       /* pitch index */
       index = *pSynthParm++;

       ippsAdaptiveCodebookDecode_GSMAMR_16s(
            index, &decState->vPrevPitchLag, &decState->vLTPLag, (decState->pExcVec - MAX_OFFSET),
            &T0, pDstAdptVector, subfrNr, bfi,
            decState->vBackgroundNoise, decState->vVoiceHangover, irate);
       
        if (irate == IPP_SPCHBR_12200)
        { /* MR122 */
            index = *pSynthParm++;
            if (bfi != 0)
                ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
            else
                gain_pit = (TableQuantGainPitch[index] >> 2) << 2;
            ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi,
                                  decState->vPrevBadFr, &gain_pit);
        }
        ippsFixedCodebookDecode_GSMAMR_16s(pSynthParm, code, subfrNr, irate);
        
		if (irate == IPP_SPCHBR_10200) { /* MR102 */
            pSynthParm+=7;
            pit_sharp = Cnvrt_32s16s(decState->vFlagSharp << 1);
        } else if (irate == IPP_SPCHBR_12200) { /* MR122 */
            pSynthParm+=10;
            pit_sharp = Cnvrt_32s16s(gain_pit << 1);
        } else {
            pSynthParm+=2;
            pit_sharp = Cnvrt_32s16s(decState->vFlagSharp << 1);
        }

        if(T0 < SUBFR_SIZE_GSMAMR)
           ippsHarmonicFilter_16s_I(pit_sharp,T0,code+T0, (SUBFR_SIZE_GSMAMR - T0));
        
        if (irate == IPP_SPCHBR_4750) {
           /* read and decode pitch and code gain */
           if(evenSubfr != 0)
              index_mr475 = *pSynthParm++;        /* index of gain(s) */
           if (bfi == 0) {
              ownDecodeCodebookGains_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index_mr475, code,
                       evenSubfr, &gain_pit, &gain_code);
           } else {
              ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
              ownConcealCodebookGain_GSMAMR(decState->a_GainBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                            decState->a_PastQntEnergy_M122, decState->vStateMachine,&gain_code);
           }
           ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi, decState->vPrevBadFr,
                                 &gain_pit);
           ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                &gain_code);
           pit_sharp = gain_pit;
           if(pit_sharp > PITCH_SHARP_MAX) pit_sharp = PITCH_SHARP_MAX;
        }
        else if ((irate <= IPP_SPCHBR_7400) || (irate == IPP_SPCHBR_10200))
        {
            /* read and decode pitch and code gain */
            index = *pSynthParm++;                /* index of gain(s) */
            if(bfi == 0) {
               ownDecodeCodebookGains_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index, code,
                        evenSubfr, &gain_pit, &gain_code);
            } else {
               ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
               ownConcealCodebookGain_GSMAMR(decState->a_LSFBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                             decState->a_PastQntEnergy_M122, decState->vStateMachine,&gain_code);
            }
            ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi, decState->vPrevBadFr,
                                  &gain_pit);
            ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                 &gain_code);
            pit_sharp = gain_pit;
            if (pit_sharp > PITCH_SHARP_MAX)  pit_sharp = PITCH_SHARP_MAX;
            if (irate == IPP_SPCHBR_10200) {
               if (decState->vPrevPitchLag > (SUBFR_SIZE_GSMAMR + 5)) pit_sharp >>= 2;
               
            }
        }
        else
        {
           /* read and decode pitch gain */
           index = *pSynthParm++;                 /* index of gain(s) */
           if (irate == IPP_SPCHBR_7950)
           {
              /* decode pitch gain */
              if (bfi != 0)
                 ownConcealGainPitch_GSMAMR(decState->a_LSFBuffer,decState->vPastGainZero, decState->vStateMachine, &gain_pit);
              else
                 gain_pit = TableQuantGainPitch[index];
              
              ownConcealGainPitchUpdate_GSMAMR(decState->a_LSFBuffer, &decState->vPastGainZero,&decState->vPrevGainZero, bfi, decState->vPrevBadFr,
                                    &gain_pit);
              /* read and decode code gain */
              index = *pSynthParm++;
              if (bfi == 0) {
                 ownDecodeFixedCodebookGain_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index, code, &gain_code);
              } else {
                 ownConcealCodebookGain_GSMAMR(decState->a_GainBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                               decState->a_PastQntEnergy_M122, decState->vStateMachine,
                               &gain_code);
              }
              ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                   &gain_code);
              pit_sharp = gain_pit;
              if(pit_sharp > PITCH_SHARP_MAX) pit_sharp = PITCH_SHARP_MAX;
           }
           else
           { /* MR122 */
              if (bfi == 0) {
                 ownDecodeFixedCodebookGain_GSMAMR(decState->a_PastQntEnergy,decState->a_PastQntEnergy_M122, irate, index, code, &gain_code);
              } else {
                 ownConcealCodebookGain_GSMAMR(decState->a_GainBuffer,decState->vPastGainCode, decState->a_PastQntEnergy,
                               decState->a_PastQntEnergy_M122, decState->vStateMachine,&gain_code);
              }
              ownConcealCodebookGainUpdate_GSMAMR(decState->a_GainBuffer,&decState->vPastGainCode,&decState->vPrevGainCode, bfi, decState->vPrevBadFr,
                                   &gain_code);
              pit_sharp = gain_pit;
           }
        }

        /* store pitch sharpening for next subframe          */
        if (irate != IPP_SPCHBR_4750 || evenSubfr == 0) {
            decState->vFlagSharp = gain_pit;
            if (decState->vFlagSharp > PITCH_SHARP_MAX)  decState->vFlagSharp = PITCH_SHARP_MAX;
            
        }
        pit_sharp = Cnvrt_32s16s(pit_sharp << 1);
        if (pit_sharp > 16384)
        {
           /* Bith are not bit-exact due to other rounding:
              ippsMulC_16s_Sfs(decState->pExcVec,pit_sharp,excp,SUBFR_SIZE_GSMAMR,15);
              ippsMulC_NR_16s_Sfs(decState->pExcVec,pit_sharp,excp,SUBFR_SIZE_GSMAMR,15);
           */
           for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
               excp[i] = (short)((decState->pExcVec[i] * pit_sharp) >> 15);
           
           for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)  {
               tmpRes = excp[i] * gain_pit;
               /* */
               if (irate != IPP_SPCHBR_12200) tmpRes = Mul2_32s(tmpRes);
               excp[i] = Cnvrt_NR_32s16s(tmpRes);
            }
        }

        if( bfi == 0 ) {
           ippsCopy_16s(decState->a_LTPGainHistory+1, decState->a_LTPGainHistory, 8);
           decState->a_LTPGainHistory[8] = gain_pit;
        }

        if ( (decState->vPrevBadFr != 0 || bfi != 0) && decState->vBackgroundNoise != 0 &&
             ((irate == IPP_SPCHBR_4750) ||
              (irate == IPP_SPCHBR_5150) ||
              (irate == IPP_SPCHBR_5900))
             )
        {
           if(gain_pit > 12288)                  /* if (gain_pit > 0.75) in Q14*/
              gain_pit = ((gain_pit - 12288) >> 1) + 12288;
                                                 /* gain_pit = (gain_pit-0.75)/2.0 + 0.75; */
           if(gain_pit > 14745) gain_pit = 14745;/* if (gain_pit > 0.90) in Q14*/
        }
        switch(i_subfr){
        case 0:
           ippsInterpolate_GSMAMR_16s(decState->a_PastLSFQnt,prev_lsf,lsf_i,LP_ORDER_SIZE);
           break;
        case SUBFR_SIZE_GSMAMR:
           ippsInterpolate_G729_16s(prev_lsf,decState->a_PastLSFQnt,lsf_i,LP_ORDER_SIZE);
           break;
        case 2*SUBFR_SIZE_GSMAMR:
           ippsInterpolate_GSMAMR_16s(prev_lsf,decState->a_PastLSFQnt,lsf_i,LP_ORDER_SIZE);
           break;
        default: /* 3*SUBFR_SIZE_GSMAMR:*/
           ippsCopy_16s(decState->a_PastLSFQnt,lsf_i,LP_ORDER_SIZE);
        }

        gain_code_mix = ownCBGainAverage_GSMAMR(decState->a_GainHistory,&decState->vHgAverageVar,&decState->vHgAverageCount, 
			                                    irate, gain_code, lsf_i, decState->a_LSPAveraged, bfi,
                                                decState->vPrevBadFr, pdfi, decState->vPrevDegBadFr, 
										        decState->vBackgroundNoise, decState->vVoiceHangover);
        /* make sure that MR74, MR795, MR122 have original code_gain*/
        if((irate > IPP_SPCHBR_6700) && (irate != IPP_SPCHBR_10200) )  /* MR74, MR795, MR122 */
           gain_code_mix = gain_code;
        
        /*-------------------------------------------------------*
         * - Find the total excitation.                          *
         * - Find synthesis speech corresponding to decState->pExcVec[].   *
         *-------------------------------------------------------*/
	    if (irate <= IPP_SPCHBR_10200)  { /* MR475, MR515, MR59, MR67, MR74, MR795, MR102*/  
           pitch_fac = gain_pit;
           tmp_shift = 1;
        } else {      /* MR122 */
           pitch_fac = gain_pit >> 1;
           tmp_shift = 2;
        }

        ippsCopy_16s(decState->pExcVec, exc_enhanced, SUBFR_SIZE_GSMAMR);
        ippsInterpolateC_NR_G729_16s_Sfs(code,gain_code,decState->pExcVec,pitch_fac,
                                         decState->pExcVec,SUBFR_SIZE_GSMAMR,15-tmp_shift);

        if ( ((irate == IPP_SPCHBR_4750) ||
              (irate == IPP_SPCHBR_5150) ||
              (irate == IPP_SPCHBR_5900))&&
             decState->vVoiceHangover > 3     &&
             decState->vBackgroundNoise != 0 &&
             bfi != 0 )
        {
            decState->stPhDispState.vFlagLockGain = 1; /* Always Use full Phase Disp. */
        } else { /* if error in bg noise       */
            decState->stPhDispState.vFlagLockGain = 0;    /* free phase dispersion adaption */
        }
        /* apply phase dispersion to innovation (if enabled) and
           compute total excitation for synthesis part           */
        ownPhaseDispersion_GSMAMR(&decState->stPhDispState, irate, exc_enhanced, 
			                      gain_code_mix, gain_pit, code, pitch_fac, tmp_shift);

        ippsDotProd_16s32s_Sfs((const Ipp16s*) exc_enhanced, (const Ipp16s*)exc_enhanced,
            SUBFR_SIZE_GSMAMR, &tmpRes, 0);
        tmpRes = ownSqrt_Exp_GSMAMR(tmpRes, &temp); /* function result */
        tmpRes >>= (temp >> 1) + 17;
        excEnergy = (short)tmpRes;   /* scaling in ownCtrlDetectBackgroundNoise_GSMAMR()     */
        if ( ((irate == IPP_SPCHBR_4750) ||
              (irate == IPP_SPCHBR_5150) ||
              (irate == IPP_SPCHBR_5900))  &&
             decState->vVoiceHangover > 5 &&
             decState->vBackgroundNoise != 0 &&
             decState->vStateMachine < 4 &&
             ( (pdfi != 0 && decState->vPrevDegBadFr != 0) ||
                bfi != 0 ||
                decState->vPrevBadFr != 0) )
        {
           carefulFlag = 0;
           if((pdfi != 0) && (bfi == 0)) carefulFlag = 1;
           
           ownCtrlDetectBackgroundNoise_GSMAMR(exc_enhanced, excEnergy, decState->a_EnergyHistSubFr,
                                               decState->vVoiceHangover, decState->vPrevBadFr, carefulFlag);
        }
        if((decState->vBackgroundNoise != 0) &&
           ( bfi != 0 || decState->vPrevBadFr != 0 ) &&
           (decState->vStateMachine < 4)) {
           ; /* do nothing! */
        } else {
           /* Update energy history for all rates */
           ippsCopy_16s(decState->a_EnergyHistSubFr+1, decState->a_EnergyHistSubFr, 8);
           decState->a_EnergyHistSubFr[8] = excEnergy;
        }

        if (pit_sharp > 16384) {
           ippsAdd_16s_I(exc_enhanced, excp, SUBFR_SIZE_GSMAMR);
           ownScaleExcitation_GSMAMR(exc_enhanced, excp);
           ippsSynthesisFilter_NR_16s_Sfs(Az,excp,&pSynthSpeech[i_subfr],SUBFR_SIZE_GSMAMR,12,decState->a_MemorySyn);
        } else {
           ippsSynthesisFilter_NR_16s_Sfs(Az,exc_enhanced,&pSynthSpeech[i_subfr],SUBFR_SIZE_GSMAMR,12,decState->a_MemorySyn);
        }
        ippsCopy_16s(&pSynthSpeech[i_subfr+SUBFR_SIZE_GSMAMR-LP_ORDER_SIZE], decState->a_MemorySyn, LP_ORDER_SIZE);
        /*--------------------------------------------------*
         * Update signal for next frame.                    *
         * -> shift to the left by SUBFR_SIZE_GSMAMR  decState->pExcVec[]       *
         *--------------------------------------------------*/
        ippsCopy_16s (&decState->a_ExcVecOld[SUBFR_SIZE_GSMAMR], &decState->a_ExcVecOld[0], (PITCH_MAX_LAG + FLT_INTER_SIZE));
        /* interpolated LPC parameters for next subframe */
        Az += LP1_ORDER_SIZE;
        /* store T0 for next subframe */
        decState->vPrevPitchLag = T0;
    }
    /*-------------------------------------------------------*
     * Call the Source Characteristic Detector which updates *
     * decState->vBackgroundNoise and decState->vVoiceHangover.         *
     *-------------------------------------------------------*/
    decState->vBackgroundNoise = ownSourceChDetectorUpdate_GSMAMR(
		                          decState->a_EnergyHistVector, &(decState->vCountHangover),
                                  &(decState->a_LTPGainHistory[0]), &(pSynthSpeech[0]),
                                  &(decState->vVoiceHangover));

    ippsDecDTXBuffer_GSMAMR_16s(pSynthSpeech, decState->a_PastLSFQnt, &decState->dtxDecoderState.vLogEnergyHistory,
                                decState->dtxDecoderState.a_LSFHistory, decState->dtxDecoderState.a_LogEnergyHistory);
    /* store bfi for next subframe */
    decState->vPrevBadFr = bfi;
    decState->vPrevDegBadFr = pdfi;
    ippsInterpolateC_NR_G729_16s_Sfs(decState->a_PastLSFQnt,EXP_CONST_016,
                                     decState->a_LSPAveraged,EXP_CONST_084,decState->a_LSPAveraged,LP_ORDER_SIZE,15);
    DTXState->eDTXPrevState = newDTXState;
    return 0;
}
