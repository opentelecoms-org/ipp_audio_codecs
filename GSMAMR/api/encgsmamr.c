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
//  Purpose: GSM AMR-NB speech encoder API
//
*/

#include "owngsmamr.h"

/* filter coefficients (fc = 140 Hz, coeff. b[] is divided by 2) */
static const short pTblCoeff_b140[3] = {1899, -3798, 1899};      /* 1/2 in Q12 */
static const short pTblCoeff_a140[3] = {4096, 7807, -3733};      /* Q12 */

/* Spectral expansion factors */

static const short pTableGamma1[4*LP_ORDER_SIZE+4] =
{
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650,
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650,
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650,
   32767, 30802, 28954, 27217, 25584, 24049, 22606, 21250, 19975, 18777, 17650
};

/* pTableGamma1 differs for the 12k2 coder */
static const short pTableGamma1_M122[4*LP_ORDER_SIZE+4] =
{
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425,
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425,
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425,
    32767, 29491, 26542, 23888, 21499, 19349, 17414, 15672, 14105, 12694, 11425
};

static const short pTableGamma2[4*LP_ORDER_SIZE+4] =
{
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198,
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198,
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198,
   32767, 19661, 11797, 7078, 4247, 2548, 1529, 917, 550, 330, 198
};

GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_GetSize, 
         (GSMAMREncoder_Obj* encoderObj, unsigned int *pCodecSize))
{
   if(NULL == encoderObj)
      return APIGSMAMR_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIGSMAMR_StsBadArgErr;
   if(encoderObj->objPrm.key != DEC_KEY)
      return APIGSMAMR_StsNotInitialized;

   *pCodecSize = encoderObj->objPrm.objSize;
   return APIGSMAMR_StsNoErr;
}


/*************************************************************************
*  Function:   apiGSMAMREncoder_Alloc
*     Enquire a size of the coder state memory
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_Alloc,
         (const GSMAMREnc_Params *gsm_Params, unsigned int *pSizeTab))
{
   int fltSize;
   int allocSize=sizeof(GSMAMREncoder_Obj);
   ippsHighPassFilterSize_G729(&fltSize); 
   allocSize += fltSize; 
   if(GSMAMR_CODEC != gsm_Params->codecType)
      return APIGSMAMR_StsBadCodecType;
   ownEncDetectSize_GSMAMR(gsm_Params->mode,&allocSize);
   pSizeTab[0] =  allocSize;
   return APIGSMAMR_StsNoErr;
}

int ownDtxEncoderInit_GSMAMR(sDTXEncoderSt* dtxSt)
{
   short i;

   dtxSt->vHistoryPtr = 0;
   dtxSt->vLogEnergyIndex = 0;
   dtxSt->vLSFQntIndex = 0;
   dtxSt->a_LSPIndex[0] = 0;
   dtxSt->a_LSPIndex[1] = 0;
   dtxSt->a_LSPIndex[2] = 0;
 
   for(i = 0; i < DTX_HIST_SIZE; i++)
     ippsCopy_16s(TableLSPInitData, &dtxSt->a_LSPHistory[i * LP_ORDER_SIZE], LP_ORDER_SIZE);

   ippsZero_16s(dtxSt->a_LogEnergyHistory, DTX_HIST_SIZE);
   dtxSt->vDTXHangoverCt = DTX_HANG_PERIOD;
   dtxSt->vDecExpireCt = 32767; 

   return 1;
}

int ownGainQuantInit_GSMAMR(sGainQuantSt *state)
{
   int i;

   state->vExpPredCBGain = 0;
   state->vFracPredCBGain = 0;
   state->vExpTargetEnergy = 0;
   state->vFracTargetEnergy = 0;
  
   ippsZero_16s(state->a_ExpEnCoeff, 5);
   ippsZero_16s(state->a_FracEnCoeff, 5);
   state->pGainPtr = NULL;
   ippsSet_16s(MIN_ENERGY, state->a_PastQntEnergy, NUM_PRED_TAPS);
   ippsSet_16s(MIN_ENERGY_M122, state->a_PastQntEnergy_M122, NUM_PRED_TAPS);
   ippsSet_16s(MIN_ENERGY, state->a_PastUnQntEnergy, NUM_PRED_TAPS);
   ippsSet_16s(MIN_ENERGY_M122, state->a_PastUnQntEnergy_M122, NUM_PRED_TAPS);
   state->vOnSetQntGain = 0;
   state->vPrevAdaptOut = 0;
   state->vPrevGainCode = 0;
   ippsZero_16s(state->a_LTPHistoryGain,NUM_MEM_LTPG);
  
   return 1;
}

int ownVAD1Init_GSMAMR(IppGSMAMRVad1State *state)
{
   short i;

   state->pitchFlag = 0;
   state->complexHigh = 0;            
   state->complexLow = 0;            
   state->complexHangTimer = 0;
   state->vadReg = 0;         
   state->statCount = 0;    
   state->burstCount = 0;    
   state->hangCount = 0;     
   state->complexHangCount = 0;     
   
   ippsZero_16s( state->pFifthFltState, 6 );
   ippsZero_16s( state->pThirdFltState, 5 );
   
   ippsSet_16s(INIT_BACKGROUND_NOISE, state->pBkgNoiseEstimate, NUM_SUBBANDS_VAD);
   ippsSet_16s(INIT_BACKGROUND_NOISE, state->pPrevSignalLevel, NUM_SUBBANDS_VAD);
   ippsSet_16s(INIT_BACKGROUND_NOISE, state->pPrevAverageLevel, NUM_SUBBANDS_VAD);
   ippsZero_16s( state->pPrevSignalSublevel, NUM_SUBBANDS_VAD );
   
   state->corrHp = INIT_LOWPOW_SEGMENT; 
   state->complexWarning = 0;
 
   return 1;
}

int ownVAD2Init_GSMAMR(IppGSMAMRVad2State *state)
{
    ippsZero_16s((Ipp16s *)state, sizeof(IppGSMAMRVad2State)/2);
	return 1;
}

int ownEncDetectSize_GSMAMR(int mode, int* pEncSize)
{
   if (mode == GSMAMREncode_VAD1_Enabled) 
      *pEncSize += sizeof(IppGSMAMRVad1State);
   if (mode == GSMAMREncode_VAD2_Enabled)
      *pEncSize += sizeof(IppGSMAMRVad2State);

   return 1;
}

int ownEncoderInit_GSMAMR(GSMAMREncoder_Obj* pEnc)
{
   int i;
   sEncoderState_GSMAMR *stEnc = &pEnc->stEncState;
   stEnc->vFlagDTX = pEnc->objPrm.mode;
      
   stEnc->pSpeechPtrNew = stEnc->a_SpeechVecOld + SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR;   /* New speech     */   
   stEnc->pSpeechPtr = stEnc->pSpeechPtrNew - SUBFR_SIZE_GSMAMR;                  /* Present frame  */
   stEnc->pWindowPtr = stEnc->a_SpeechVecOld + SPEECH_BUF_SIZE - LP_WINDOW_SIZE;    /* For LPC window */
   stEnc->pWindowPtr_M122 = stEnc->pWindowPtr - SUBFR_SIZE_GSMAMR; /* EFR LPC window: no lookahead */
      
   stEnc->pWeightSpeechVec = stEnc->a_WeightSpeechVecOld + PITCH_MAX_LAG;
   stEnc->pExcVec = stEnc->a_ExcVecOld + PITCH_MAX_LAG + FLT_INTER_SIZE;
   stEnc->pZeroVec = stEnc->a_ZeroVec + LP1_ORDER_SIZE;
   stEnc->pErrorPtr = stEnc->a_MemoryErr + LP_ORDER_SIZE;
   stEnc->pImpResVec = &stEnc->a_ImpResVec[0];
      
   ippsZero_16s(stEnc->a_SpeechVecOld, SPEECH_BUF_SIZE);
   ippsZero_16s(stEnc->a_ExcVecOld,PITCH_MAX_LAG + FLT_INTER_SIZE);
   ippsZero_16s(stEnc->a_WeightSpeechVecOld,PITCH_MAX_LAG);
   ippsZero_16s(stEnc->a_MemorySyn,LP_ORDER_SIZE);
   //ippsZero_16s(stEnc->a_Memory_W1,LP_ORDER_SIZE);
   ippsZero_16s(stEnc->a_Memory_W0,LP_ORDER_SIZE);
   ippsZero_16s(stEnc->a_MemoryErr,LP_ORDER_SIZE);
   ippsZero_16s(stEnc->pZeroVec,SUBFR_SIZE_GSMAMR);

   ippsSet_16s(40, stEnc->a_LTPStateOld, 5);
   
   ippsZero_16s(stEnc->a_SubState, (LP_ORDER_SIZE + 1));
   stEnc->a_SubState[0] = 4096;
   
   ippsCopy_16s(TableLSPInitData, &stEnc->a_LSP_Old[0], LP_ORDER_SIZE);
   ippsCopy_16s(stEnc->a_LSP_Old, stEnc->a_LSPQnt_Old, LP_ORDER_SIZE);
   ippsZero_16s(stEnc->a_PastQntPredErr,LP_ORDER_SIZE);
    
   stEnc->vTimePrevSubframe = 0;
   
   ownGainQuantInit_GSMAMR(&stEnc->stGainQntSt);

   stEnc->vTimeMedOld = 40;
   stEnc->vFlagVADState = 0;
   //stEnc->wght_flg = 0;

   stEnc->vCount = 0;
   ippsZero_16s(stEnc->a_GainHistory, PG_NUM_FRAME);    /* Init Gp_Clipping */

   if (stEnc->vFlagDTX == GSMAMREncode_VAD1_Enabled) 
      ownVAD1Init_GSMAMR((IppGSMAMRVad1State*)stEnc->pVADSt);
   if (stEnc->vFlagDTX == GSMAMREncode_VAD2_Enabled)
      ownVAD2Init_GSMAMR((IppGSMAMRVad2State*)stEnc->pVADSt);
   ownDtxEncoderInit_GSMAMR(&stEnc->stDTXEncState);

   stEnc->vFlagSharp = PITCH_SHARP_MIN;
   stEnc->vLagCountOld = 0;
   stEnc->vLagOld = 0;         
   stEnc->vFlagTone = 0;            
   stEnc->vBestHpCorr = INIT_LOWPOW_SEGMENT;
 
   return 1;
}
/*************************************************************************
*  Function:   apiGSMAMREncoder_Init
*     Initializes coder state memory
**************************************************************************/
GSMAMR_CODECFUN( APIGSMAMR_Status, apiGSMAMREncoder_Init, 
                (GSMAMREncoder_Obj* encoderObj, unsigned int mode))
{
   int fltSize;
   IPP_ALIGNED_ARRAY(16, Ipp16s, abEnc, 6);
   int obj_size = sizeof(GSMAMREncoder_Obj);
   ippsZero_16s((short*)encoderObj,sizeof(GSMAMREncoder_Obj)>>1) ; 
   encoderObj->objPrm.mode = mode;
   encoderObj->objPrm.key = ENC_KEY;
   encoderObj->preProc = (char*)encoderObj + sizeof(GSMAMREncoder_Obj);
   ippsHighPassFilterSize_G729(&fltSize); 
   obj_size += fltSize;
   ownEncDetectSize_GSMAMR(mode,&obj_size);
   encoderObj->stEncState.pVADSt = (char*)encoderObj->preProc + fltSize;
   encoderObj->objPrm.objSize = obj_size;

   abEnc[0] = pTblCoeff_a140[0];
   abEnc[1] = pTblCoeff_a140[1];
   abEnc[2] = pTblCoeff_a140[2];
   abEnc[3] = pTblCoeff_b140[0];
   abEnc[4] = pTblCoeff_b140[1];
   abEnc[5] = pTblCoeff_b140[2];
   ownEncoderInit_GSMAMR(encoderObj);
   ippsHighPassFilterInit_G729(abEnc, encoderObj->preProc);
  
   return APIGSMAMR_StsNoErr;
}
/*************************************************************************
*  Function:   apiGSMAMREncode
**************************************************************************/

GSMAMR_CODECFUN(  APIGSMAMR_Status, apiGSMAMREncode, 
         (GSMAMREncoder_Obj* encoderObj,const short* src,  GSMAMR_Rate_t rate, 
          unsigned char* dst, int *pVad ))
{
   IPP_ALIGNED_ARRAY(16, short, pSynthVec, FRAME_SIZE_GSMAMR);        /* Buffer for synthesis speech           */
   IPP_ALIGNED_ARRAY(16, short, pParamVec, MAX_NUM_PRM);
   short *pParamPtr = pParamVec;
   short *pNewSpeech = encoderObj->stEncState.pSpeechPtrNew;

   if(NULL==encoderObj || NULL==src || NULL ==dst )
      return APIGSMAMR_StsBadArgErr;
   if(0 >= encoderObj->objPrm.objSize)
      return APIGSMAMR_StsNotInitialized;
   if(rate > GSMAMR_RATE_DTX  || rate < 0)
      return APIGSMAMR_StsBadArgErr;
   if(ENC_KEY != encoderObj->objPrm.key)
      return APIGSMAMR_StsBadCodecType;
   encoderObj->rate = rate;

   /* filter + downscaling */
   ippsCopy_16s(src, pNewSpeech, FRAME_SIZE_GSMAMR);
   ippsHighPassFilter_G729_16s_ISfs(pNewSpeech, FRAME_SIZE_GSMAMR, 12, encoderObj->preProc);  
 
   /* Call the speech encoder */
   ownEncode_GSMAMR(&encoderObj->stEncState, encoderObj->rate, pParamPtr, pVad, pSynthVec);
  
   if(!*pVad) rate = GSMAMR_RATE_DTX;
   ownPrm2Bits_GSMAMR(pParamVec, dst, rate);

   return APIGSMAMR_StsNoErr;
}

/***************************************************************************
 * Function: ownEncode_GSMAMR
 ***************************************************************************/
int ownEncode_GSMAMR(sEncoderState_GSMAMR *encSt,GSMAMR_Rate_t rate, short *pAnaParam,             
                     int *pVad, short *pSynthVec)
{
   /* LPC coefficients */
   IPP_ALIGNED_ARRAY(16, short, A_t, (LP1_ORDER_SIZE) * 4);     
   IPP_ALIGNED_ARRAY(16, short, Aq_t, (LP1_ORDER_SIZE) * 4);    
   short *A, *Aq;            
   IPP_ALIGNED_ARRAY(16, short, pNewLSP, 2*LP_ORDER_SIZE);
   
   IPP_ALIGNED_ARRAY(16, short, pPitchSrch, SUBFR_SIZE_GSMAMR);        
   IPP_ALIGNED_ARRAY(16, short, pCodebookSrch, SUBFR_SIZE_GSMAMR);     
   IPP_ALIGNED_ARRAY(16, short, pFixCodebookExc, SUBFR_SIZE_GSMAMR);   
   IPP_ALIGNED_ARRAY(16, short, pFltAdaptExc, SUBFR_SIZE_GSMAMR);      
   IPP_ALIGNED_ARRAY(16, short, pFltFixExc, SUBFR_SIZE_GSMAMR);        
   IPP_ALIGNED_ARRAY(16, short, pLPCPredRes, SUBFR_SIZE_GSMAMR);       
   IPP_ALIGNED_ARRAY(16, short, pLPCPredRes2, SUBFR_SIZE_GSMAMR);      

   IPP_ALIGNED_ARRAY(16, short, pPitchSrchM475, SUBFR_SIZE_GSMAMR);      
   IPP_ALIGNED_ARRAY(16, short, pFltCodebookM475, SUBFR_SIZE_GSMAMR);    
   IPP_ALIGNED_ARRAY(16, short, pFixCodebookExcM475, SUBFR_SIZE_GSMAMR); 
   IPP_ALIGNED_ARRAY(16, short, pImpResM475, SUBFR_SIZE_GSMAMR);         
   IPP_ALIGNED_ARRAY(16, short, pMemSynSave, LP_ORDER_SIZE);             
   IPP_ALIGNED_ARRAY(16, short, pMemW0Save, LP_ORDER_SIZE);              
   IPP_ALIGNED_ARRAY(16, short, pMemErrSave, LP_ORDER_SIZE);             
   IPP_ALIGNED_ARRAY(16, short, pSrcWgtLpc1, 44);
   IPP_ALIGNED_ARRAY(16, short, pSrcWgtLpc2, 44);
   IPP_ALIGNED_ARRAY(16, short, pNewLSP1, 2*LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, short, pNewQntLSP1, 2*LP_ORDER_SIZE);
   short sharp_save;          
   short evenSubfr;           
   short T0_sf0 = 0;            
   short T0_frac_sf0 = 0;      
   short i_subfr_sf0 = 0;     
   short gain_pit_sf0;        
   short gain_code_sf0;       
    
   short i_subfr, subfrNr;
   short T_op[2];
   short T0, T0_frac;
   short gain_pit, gain_code;
   IppSpchBitRate irate = mode2rates[rate];
   IppSpchBitRate usedRate = irate;
   
   /* Flags */
   short lsp_flag = 0;        
   short gainPitchLimit;            
   short vad_flag=0;          
   short compute_sid_flag=0;  

   const short *g1;           
   int L_Rmax=0;
   int L_R0=0;
   int i;

   {
      IPP_ALIGNED_ARRAY(16, int, r_auto, LP1_ORDER_SIZE*2);    
      if ( IPP_SPCHBR_12200 == irate ) {
        ippsAutoCorr_GSMAMR_16s32s(encSt->pWindowPtr_M122, r_auto, irate);
        ippsLevinsonDurbin_GSMAMR_32s16s(&r_auto[0], &A_t[LP1_ORDER_SIZE]); 
        ippsLevinsonDurbin_GSMAMR_32s16s(&r_auto[LP1_ORDER_SIZE], &A_t[LP1_ORDER_SIZE*3]); 
      } else {
        ippsAutoCorr_GSMAMR_16s32s(encSt->pWindowPtr, r_auto,  irate);
        ippsLevinsonDurbin_GSMAMR_32s16s(r_auto, &A_t[LP1_ORDER_SIZE * 3]);
      }   
   }
   *pVad = 1;

   if (encSt->vFlagDTX) {  
      Ipp16s irate = -1;  

      if (encSt->vFlagDTX == GSMAMREncode_VAD2_Enabled) 
         ippsVAD2_GSMAMR_16s(encSt->pSpeechPtrNew, (IppGSMAMRVad2State*)encSt->pVADSt,
                             &vad_flag, encSt->vFlagLTP);
      else
         ippsVAD1_GSMAMR_16s(encSt->pSpeechPtr, (IppGSMAMRVad1State*)encSt->pVADSt, 
                             &vad_flag, encSt->vBestHpCorr, encSt->vFlagTone);  

      ippsEncDTXHandler_GSMAMR_16s(&encSt->stDTXEncState.vDTXHangoverCt, 
                                   &encSt->stDTXEncState.vDecExpireCt,
                                   &irate, &compute_sid_flag, vad_flag);
      if(irate == IPP_SPCHBR_DTX) { 
         usedRate = IPP_SPCHBR_DTX;
         *pVad = 0;  /* SID frame */
      }
   }

   /* From A(z) to lsp. LSP quantization and interpolation */
   {
      IPP_ALIGNED_ARRAY(16, short, lsp_new_q, LP_ORDER_SIZE*2);

      if( IPP_SPCHBR_12200 == irate) {
         IPP_ALIGNED_ARRAY(16, short, lsp, LP_ORDER_SIZE);

         ippsLPCToLSP_GSMAMR_16s(&A_t[LP1_ORDER_SIZE], encSt->a_LSP_Old, &(pNewLSP[LP_ORDER_SIZE]));
         ippsLPCToLSP_GSMAMR_16s(&A_t[LP1_ORDER_SIZE * 3], &(pNewLSP[LP_ORDER_SIZE]), &(pNewLSP[0]));

         ippsInterpolate_G729_16s(&pNewLSP[LP_ORDER_SIZE],encSt->a_LSP_Old,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, A_t);                    
         ippsInterpolate_G729_16s(&pNewLSP[LP_ORDER_SIZE],pNewLSP,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, &A_t[2*LP1_ORDER_SIZE]);              

         if ( IPP_SPCHBR_DTX != usedRate) {

             ippsCopy_16s(&pNewLSP[LP_ORDER_SIZE], pNewLSP1, LP_ORDER_SIZE);
             ippsCopy_16s(pNewLSP, &pNewLSP1[LP_ORDER_SIZE], LP_ORDER_SIZE);
             ippsLSPQuant_GSMAMR_16s(pNewLSP1, encSt->a_PastQntPredErr, pNewQntLSP1, pAnaParam, irate); 
             ippsCopy_16s(&pNewQntLSP1[LP_ORDER_SIZE], lsp_new_q, LP_ORDER_SIZE);
             ippsCopy_16s(pNewQntLSP1, &lsp_new_q[LP_ORDER_SIZE], LP_ORDER_SIZE);
             
             ippsInterpolate_G729_16s(&lsp_new_q[LP_ORDER_SIZE],encSt->a_LSPQnt_Old,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, Aq_t);                     
             ippsLSPToLPC_GSMAMR_16s(&lsp_new_q[LP_ORDER_SIZE], &Aq_t[LP1_ORDER_SIZE]);  
             ippsInterpolate_G729_16s(&lsp_new_q[LP_ORDER_SIZE],lsp_new_q,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, &Aq_t[2*LP1_ORDER_SIZE]);             
             ippsLSPToLPC_GSMAMR_16s(lsp_new_q, &Aq_t[3*LP1_ORDER_SIZE]);       
             pAnaParam += 5; 
         }	 
      }
      else
      {
         IPP_ALIGNED_ARRAY(16, short, lsp, LP_ORDER_SIZE);
         ippsLPCToLSP_GSMAMR_16s(&A_t[LP1_ORDER_SIZE * 3], encSt->a_LSP_Old, pNewLSP);
          
         ippsInterpolate_GSMAMR_16s(pNewLSP,encSt->a_LSP_Old,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, A_t);                      
         ippsInterpolate_G729_16s(encSt->a_LSP_Old,pNewLSP,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, &A_t[LP1_ORDER_SIZE]);                   
         ippsInterpolate_GSMAMR_16s(encSt->a_LSP_Old,pNewLSP,lsp,LP_ORDER_SIZE);
         ippsLSPToLPC_GSMAMR_16s(lsp, &A_t[2*LP1_ORDER_SIZE]);                 
         /*  subframe 4 is not processed (available) */

         if ( IPP_SPCHBR_DTX != usedRate)
         {

             ippsLSPQuant_GSMAMR_16s(pNewLSP, encSt->a_PastQntPredErr, lsp_new_q, pAnaParam, irate);

             ippsInterpolate_GSMAMR_16s(lsp_new_q,encSt->a_LSPQnt_Old,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, Aq_t);                       
             ippsInterpolate_G729_16s(encSt->a_LSPQnt_Old,lsp_new_q,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, &Aq_t[LP1_ORDER_SIZE]);                   
             ippsInterpolate_GSMAMR_16s(encSt->a_LSPQnt_Old,lsp_new_q,lsp,LP_ORDER_SIZE);
             ippsLSPToLPC_GSMAMR_16s(lsp, &Aq_t[2*LP1_ORDER_SIZE]);                 

             ippsLSPToLPC_GSMAMR_16s(lsp_new_q, &Aq_t[3*LP1_ORDER_SIZE]);            
             pAnaParam += 3; 
         }
      }
          
      ippsCopy_16s(&(pNewLSP[0]), encSt->a_LSP_Old, LP_ORDER_SIZE);
      ippsCopy_16s(&(lsp_new_q[0]), encSt->a_LSPQnt_Old, LP_ORDER_SIZE);
   }
   
   ippsEncDTXBuffer_GSMAMR_16s(encSt->pSpeechPtrNew, pNewLSP, &encSt->stDTXEncState.vHistoryPtr,
                               encSt->stDTXEncState.a_LSPHistory, encSt->stDTXEncState.a_LogEnergyHistory);

   if (IPP_SPCHBR_DTX == usedRate)
   {
      ippsEncDTXSID_GSMAMR_16s(encSt->stDTXEncState.a_LSPHistory, encSt->stDTXEncState.a_LogEnergyHistory,
                               &encSt->stDTXEncState.vLogEnergyIndex,
                               &encSt->stDTXEncState.vLSFQntIndex, encSt->stDTXEncState.a_LSPIndex,
                               encSt->stGainQntSt.a_PastQntEnergy, 
                               encSt->stGainQntSt.a_PastQntEnergy_M122, compute_sid_flag);
      *(pAnaParam)++ = encSt->stDTXEncState.vLSFQntIndex;     
      *(pAnaParam)++ = encSt->stDTXEncState.a_LSPIndex[0];          
      *(pAnaParam)++ = encSt->stDTXEncState.a_LSPIndex[1];          
      *(pAnaParam)++ = encSt->stDTXEncState.a_LSPIndex[2];          
      *(pAnaParam)++ = encSt->stDTXEncState.vLogEnergyIndex;      
      
      ippsZero_16s(encSt->a_ExcVecOld,    PITCH_MAX_LAG + FLT_INTER_SIZE);
      ippsZero_16s(encSt->a_Memory_W0,     LP_ORDER_SIZE);
      ippsZero_16s(encSt->a_MemoryErr,    LP_ORDER_SIZE);
      ippsZero_16s(encSt->pZeroVec,       SUBFR_SIZE_GSMAMR);

      ippsCopy_16s(TableLSPInitData, &encSt->a_LSP_Old[0], LP_ORDER_SIZE);
      ippsCopy_16s(encSt->a_LSP_Old, encSt->a_LSPQnt_Old, LP_ORDER_SIZE);
      ippsZero_16s(encSt->a_PastQntPredErr,LP_ORDER_SIZE);
      ippsCopy_16s(pNewLSP, encSt->a_LSP_Old, LP_ORDER_SIZE);
      ippsCopy_16s(pNewLSP, encSt->a_LSPQnt_Old, LP_ORDER_SIZE);
      encSt->vTimePrevSubframe = 0;
      encSt->vFlagSharp = PITCH_SHARP_MIN;      
   
   } else lsp_flag = ownCheckLSPVec_GSMAMR(&encSt->vCount, encSt->a_LSP_Old);
   
   
   if(irate <= IPP_SPCHBR_7950) g1 = pTableGamma1;                          
   else                         g1 = pTableGamma1_M122;

   ippsMul_NR_16s_Sfs(A_t,g1,pSrcWgtLpc1,4*LP_ORDER_SIZE+4,15); /* four frames at a time*/
   ippsMul_NR_16s_Sfs(A_t,pTableGamma2,pSrcWgtLpc2,4*LP_ORDER_SIZE+4,15);

   if(encSt->vFlagDTX == GSMAMREncode_DefaultMode) {
       
       ippsOpenLoopPitchSearchNonDTX_GSMAMR_16s(pSrcWgtLpc1,pSrcWgtLpc2,(encSt->pSpeechPtr-LP_ORDER_SIZE),
                                                &encSt->vTimeMedOld,&encSt->vFlagVADState,encSt->a_LTPStateOld,
                                                encSt->a_WeightSpeechVecOld,T_op,encSt->a_GainFlg,irate);
   }
   else if(encSt->vFlagDTX == GSMAMREncode_VAD1_Enabled){

         ippsOpenLoopPitchSearchDTXVAD1_GSMAMR_16s(pSrcWgtLpc1, pSrcWgtLpc2,(encSt->pSpeechPtr-LP_ORDER_SIZE),
             &encSt->vFlagTone, &encSt->vTimeMedOld,&encSt->vFlagVADState,encSt->a_LTPStateOld,encSt->a_WeightSpeechVecOld,
             &encSt->vBestHpCorr,T_op,encSt->a_GainFlg,irate);

         ownVADPitchDetection_GSMAMR((IppGSMAMRVad1State*)encSt->pVADSt, T_op, &encSt->vLagCountOld, &encSt->vLagOld);
   } else if(encSt->vFlagDTX == GSMAMREncode_VAD2_Enabled){
         ippsOpenLoopPitchSearchDTXVAD2_GSMAMR_16s32s(pSrcWgtLpc1,
         pSrcWgtLpc2,(encSt->pSpeechPtr-LP_ORDER_SIZE),&encSt->vTimeMedOld,&encSt->vFlagVADState,encSt->a_LTPStateOld,encSt->a_WeightSpeechVecOld,
         &L_Rmax, &L_R0, T_op,encSt->a_GainFlg,irate);

         ownUpdateLTPFlag_GSMAMR(irate, L_Rmax, L_R0, &encSt->vFlagLTP);
      }

   if (usedRate == IPP_SPCHBR_DTX)  {
      ippsCopy_16s(&encSt->a_SpeechVecOld[FRAME_SIZE_GSMAMR], &encSt->a_SpeechVecOld[0], (SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR));
      return 0; 
   }
   
   A = A_t;      
   Aq = Aq_t;    
   if (irate == IPP_SPCHBR_12200 || irate == IPP_SPCHBR_10200) g1 = pTableGamma1_M122; 
   else                                                        g1 = pTableGamma1;      

   evenSubfr = 0;                                                 
   subfrNr = -1;                                                  
   for(i_subfr = 0; i_subfr < FRAME_SIZE_GSMAMR; i_subfr += SUBFR_SIZE_GSMAMR) {
      subfrNr++;
      evenSubfr = 1 - evenSubfr;

      if ((evenSubfr != 0) && (usedRate == IPP_SPCHBR_4750)) {
         ippsCopy_16s(encSt->a_MemorySyn, pMemSynSave, LP_ORDER_SIZE);
         ippsCopy_16s(encSt->a_Memory_W0, pMemW0Save, LP_ORDER_SIZE);         
         ippsCopy_16s(encSt->a_MemoryErr, pMemErrSave, LP_ORDER_SIZE);         
         sharp_save = encSt->vFlagSharp;
      }

      ippsMul_NR_16s_Sfs(A,g1,pSrcWgtLpc1,LP_ORDER_SIZE+1,15);
      ippsMul_NR_16s_Sfs(A,pTableGamma2,pSrcWgtLpc2,LP_ORDER_SIZE+1,15);

      if (usedRate != IPP_SPCHBR_4750) {
         ippsImpulseResponseTarget_GSMAMR_16s(&encSt->pSpeechPtr[i_subfr-LP_ORDER_SIZE],pSrcWgtLpc1,pSrcWgtLpc2,
            Aq,encSt->a_MemoryErr,encSt->a_Memory_W0,encSt->pImpResVec, pLPCPredRes, pPitchSrch);

      } else { /* MR475 */
         ippsImpulseResponseTarget_GSMAMR_16s(&encSt->pSpeechPtr[i_subfr-LP_ORDER_SIZE],pSrcWgtLpc1,pSrcWgtLpc2,
            Aq,encSt->a_MemoryErr,pMemW0Save,encSt->pImpResVec, pLPCPredRes, pPitchSrch);

         if (evenSubfr != 0)
             ippsCopy_16s (encSt->pImpResVec, pImpResM475, SUBFR_SIZE_GSMAMR);

      }

      ippsCopy_16s(pLPCPredRes, &encSt->pExcVec[i_subfr], SUBFR_SIZE_GSMAMR);
      ippsCopy_16s (pLPCPredRes, pLPCPredRes2, SUBFR_SIZE_GSMAMR);
    
      ownCloseLoopFracPitchSearch_GSMAMR(&encSt->vTimePrevSubframe, encSt->a_GainHistory, usedRate, i_subfr, 
		                                 T_op, encSt->pImpResVec, &encSt->pExcVec[i_subfr], pLPCPredRes2, 
										 pPitchSrch, lsp_flag, pCodebookSrch, pFltAdaptExc, &T0, &T0_frac, 
										 &gain_pit, &pAnaParam, &gainPitchLimit);

      if ((subfrNr == 0) && (encSt->a_GainFlg[0] > 0))
         encSt->a_LTPStateOld[1] = T0;    
      if ((subfrNr == 3) && (encSt->a_GainFlg[1] > 0))
         encSt->a_LTPStateOld[0] = T0;   
      
      if (usedRate == IPP_SPCHBR_12200)
        ippsAlgebraicCodebookSearch_GSMAMR_16s(T0, gain_pit, 
            pCodebookSrch, pLPCPredRes2, encSt->pImpResVec, pFixCodebookExc, pFltFixExc, pAnaParam, subfrNr, IPP_SPCHBR_12200);    
      else
        ippsAlgebraicCodebookSearch_GSMAMR_16s(T0, encSt->vFlagSharp, 
            pCodebookSrch, pLPCPredRes2, encSt->pImpResVec, pFixCodebookExc, pFltFixExc, pAnaParam, subfrNr, usedRate);
      
      if(usedRate == IPP_SPCHBR_10200)       pAnaParam+=7;
      else if (usedRate == IPP_SPCHBR_12200) pAnaParam+=10;
      else                                   pAnaParam+=2;

      ownGainQuant_GSMAMR(&encSt->stGainQntSt, usedRate, pLPCPredRes, &encSt->pExcVec[i_subfr], pFixCodebookExc,
                          pPitchSrch, pCodebookSrch,  pFltAdaptExc, pFltFixExc, evenSubfr, gainPitchLimit,
                          &gain_pit_sf0, &gain_code_sf0, &gain_pit, &gain_code, &pAnaParam);

      ippsCopy_16s(&encSt->a_GainHistory[1], &encSt->a_GainHistory[0], PG_NUM_FRAME-1);
      encSt->a_GainHistory[PG_NUM_FRAME-1] = gain_pit >> 3;
      
      if (usedRate != IPP_SPCHBR_4750)  {
         ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr, gain_pit,
                                    gain_code, Aq, pSynthVec, pPitchSrch, pFixCodebookExc, 
									pFltAdaptExc, pFltFixExc, encSt->a_MemorySyn, 
									encSt->a_MemoryErr, encSt->a_Memory_W0, encSt->pExcVec, 
									&encSt->vFlagSharp);
      } else {

         if (evenSubfr != 0) {
            i_subfr_sf0 = i_subfr;             
            ippsCopy_16s(pPitchSrch, pPitchSrchM475, SUBFR_SIZE_GSMAMR);
            ippsCopy_16s(pFltFixExc, pFltCodebookM475, SUBFR_SIZE_GSMAMR);          
            ippsCopy_16s(pFixCodebookExc, pFixCodebookExcM475, SUBFR_SIZE_GSMAMR);
            T0_sf0 = T0;                       
            T0_frac_sf0 = T0_frac;            
            
            ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr, gain_pit,
                                       gain_code, Aq, pSynthVec, pPitchSrch, pFixCodebookExc, 
									   pFltAdaptExc, pFltFixExc, pMemSynSave, encSt->a_MemoryErr, 
									   pMemW0Save, encSt->pExcVec, &encSt->vFlagSharp);
            encSt->vFlagSharp = sharp_save;                        
         } else {

            ippsCopy_16s(pMemErrSave, encSt->a_MemoryErr, LP_ORDER_SIZE);         
            ownPredExcMode3_6_GSMAMR(&encSt->pExcVec[i_subfr_sf0], T0_sf0, T0_frac_sf0,
                         SUBFR_SIZE_GSMAMR, 1);

            ippsConvPartial_16s_Sfs(&encSt->pExcVec[i_subfr_sf0], pImpResM475,  pFltAdaptExc, SUBFR_SIZE_GSMAMR, 12);
            
			Aq -= LP1_ORDER_SIZE;
            ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr_sf0, gain_pit_sf0, gain_code_sf0, 
				                       Aq, pSynthVec, pPitchSrchM475, pFixCodebookExcM475, pFltAdaptExc, 
									   pFltCodebookM475, encSt->a_MemorySyn, encSt->a_MemoryErr, encSt->a_Memory_W0, 
									   encSt->pExcVec, &sharp_save); 
            Aq += LP1_ORDER_SIZE;
            ippsImpulseResponseTarget_GSMAMR_16s(&encSt->pSpeechPtr[i_subfr-LP_ORDER_SIZE],pSrcWgtLpc1,pSrcWgtLpc2,
            Aq,encSt->a_MemoryErr,encSt->a_Memory_W0,encSt->pImpResVec, pLPCPredRes, pPitchSrch);

            ownPredExcMode3_6_GSMAMR(&encSt->pExcVec[i_subfr], T0, T0_frac, SUBFR_SIZE_GSMAMR, 1);
            ippsConvPartial_16s_Sfs(&encSt->pExcVec[i_subfr], encSt->pImpResVec,  pFltAdaptExc, SUBFR_SIZE_GSMAMR, 12);
            
            ownSubframePostProc_GSMAMR(encSt->pSpeechPtr, usedRate, i_subfr, gain_pit, gain_code, Aq, pSynthVec, 
				                       pPitchSrch, pFixCodebookExc, pFltAdaptExc, pFltFixExc, encSt->a_MemorySyn, 
									   encSt->a_MemoryErr, encSt->a_Memory_W0, encSt->pExcVec, &encSt->vFlagSharp);
         }
      }      
          
      A += LP1_ORDER_SIZE;    
      Aq += LP1_ORDER_SIZE;
   }

   ippsCopy_16s(&encSt->a_ExcVecOld[FRAME_SIZE_GSMAMR], &encSt->a_ExcVecOld[0], PITCH_MAX_LAG + FLT_INTER_SIZE);   
   ippsCopy_16s(&encSt->a_SpeechVecOld[FRAME_SIZE_GSMAMR], &encSt->a_SpeechVecOld[0], (SPEECH_BUF_SIZE - FRAME_SIZE_GSMAMR));

   return 0;
   /* End of ownEncode_GSMAMR() */
}

