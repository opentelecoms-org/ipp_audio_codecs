/*****************************************************************************
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Porpose: ITU-T G.729 Speech Encoder. 
//  Floating point implementation compliant to ITU G.729C/C+.
//
***************************************************************************/
#include <math.h>
#include "owng729fp.h"

const float InitLSP[LPC_ORDER] =
     { 0.9595f,  0.8413f,  0.6549f,  0.4154f,  0.1423f,
      -0.1423f, -0.4154f, -0.6549f, -0.8413f, -0.9595f
};
const float InitFrequences[LPC_ORDER] = {  /* previous LSP vector(init) */
 0.285599f,  0.571199f,  0.856798f,  1.142397f,  1.427997f,
 1.713596f,  1.999195f,  2.284795f,  2.570394f,  2.855993f
};     /* IPP_PI*(j+1)/(LPC_ORDER+1) */

static float b140[3] = {0.92727435E+00f, -0.18544941E+01f, 0.92727435E+00f};
static float a140[3] = {1.00000000E+00f, 0.19059465E+01f, -0.91140240E+00f};

static const float lwindow[LPC_ORDER+2] = {
   0.99879038f, 0.99546894f, 0.98995779f,
   0.98229335f, 0.97252620f, 0.96072035f,
   0.94695264f, 0.93131180f, 0.91389754f,
   0.89481964f, 0.87419660f, 0.85215437f
};

const float lagBwd[BWD_LPC_ORDER] = {
   0.999892f,  0.999869f,  0.999831f,  0.999777f,  0.999707f,
   0.999622f,  0.999522f,  0.999407f,  0.999276f,  0.999129f,
   0.998968f,  0.998790f,  0.998598f,  0.998390f,  0.998167f,
   0.997928f,  0.997674f,  0.997405f,  0.997121f,  0.996821f,
   0.996506f,  0.996175f,  0.995830f,  0.995469f,  0.995093f,
   0.994702f,  0.994295f,  0.993874f,  0.993437f,  0.992985f,
};

/* Quantization of SID gain */
const float SIDGainTbl[32] = {
      0.502f,    1.262f,    2.000f,    3.170f,
      5.024f,    7.962f,   12.619f,   15.887f,
     20.000f,   25.179f,   31.698f,   39.905f,
     50.238f,   63.246f,   79.621f,  100.237f,
    126.191f,  158.866f,  200.000f,  251.785f,
    316.979f,  399.052f,  502.377f,  632.456f,
    796.214f, 1002.374f, 1261.915f, 1588.656f,
   2000.000f, 2517.851f, 3169.786f, 3990.525f
};

#define         AVG(a,b,c,d) (int)( ((a)+(b)+(c)+(d))/4.0f + 0.5f)

static void UpdateVad_I(G729Encoder_Obj* encoderObj, float *Excitation, float *forwardLPC, float *WeightedSpeech, 
                 float *gamma1, float *gamma2, float *pSynth,
                 float *pError, float *SpeechWnd, int* dst,G729Codec_Type codecType);

static void UpdateVad_A(G729Encoder_Obj* encoderObj, float *Excitation, 
                        float *WeightedSpeech, float *SpeechWnd, int* dst);

static void UpdateCNG(float *pSrcAutoCorr, int Vad, char *cngMem);

static int CodecType2Num(G729Codec_Type codecType)
{
    switch(codecType) {
    case G729D_CODEC:
       return 0;
    case G729_CODEC:
       return 1;
    case G729A_CODEC:
       return 1;
    case G729E_CODEC:
       return 2;
    }
    return -1;
}

static int ownEncoderObjSize()
{
   int codecSize, fltsize;

   codecSize = sizeof(G729Encoder_Obj);
   ippsIIRGetStateSize_32f(2,&fltsize);
   codecSize += fltsize;
   VADGetSize(&fltsize);
   codecSize += fltsize;
   CNGGetSize(&fltsize);
   codecSize += fltsize;
   MSDGetSize(&fltsize);
   codecSize += fltsize;
   ippsWinHybridGetStateSize_G729E_32f(&fltsize);
   codecSize += fltsize;

   return codecSize;
}

G729_CODECFUN( APIG729_Status, apiG729Encoder_Alloc,
         (G729Codec_Type codecType, int *pCodecSize))
{
   if ((codecType != G729_CODEC)&&(codecType != G729A_CODEC)
      &&(codecType != G729D_CODEC)&&(codecType != G729E_CODEC)&&(codecType != G729I_CODEC)){
      return APIG729_StsBadCodecType;
   }

   *pCodecSize = ownEncoderObjSize();

   return APIG729_StsNoErr;

}

G729_CODECFUN( APIG729_Status, apiG729Codec_ScratchMemoryAlloc,(int *pCodecSize))
{
   if(NULL==pCodecSize)
      return APIG729_StsBadArgErr;
   *pCodecSize = G729FP_ENCODER_SCRATCH_MEMORY_SIZE;

   return APIG729_StsNoErr;
}

G729_CODECFUN( APIG729_Status, apiG729Encoder_Mode,
         (G729Encoder_Obj* encoderObj, G729Encode_Mode mode))
{
   if(G729Encode_VAD_Enabled != mode && G729Encode_VAD_Disabled != mode){
      return APIG729_StsBadArgErr;
   }
   encoderObj->objPrm.mode = mode;
   return APIG729_StsNoErr;
}

G729_CODECFUN( APIG729_Status, apiG729Encoder_InitBuff, 
         (G729Encoder_Obj* encoderObj, char *buff))
{
#if !defined (NO_SCRATCH_MEMORY_USED)

   if(NULL==encoderObj || NULL==buff)
      return APIG729_StsBadArgErr;

   encoderObj->Mem.base = buff;
   encoderObj->Mem.CurPtr = encoderObj->Mem.base;
   encoderObj->Mem.VecPtr = (int *)(encoderObj->Mem.base+G729FP_ENCODER_SCRATCH_MEMORY_SIZE);
#endif 
   return APIG729_StsNoErr;
}

G729_CODECFUN( APIG729_Status, apiG729Encoder_Init, 
         (G729Encoder_Obj* encoderObj, G729Codec_Type codecType, G729Encode_Mode mode))
{

   int i;
   int fltsize;
   void* pBuf;
   Ipp32f coeff[6];

   if(NULL==encoderObj)
      return APIG729_StsBadArgErr;

   if ((codecType != G729_CODEC)&&(codecType != G729A_CODEC)
      &&(codecType != G729D_CODEC)&&(codecType != G729E_CODEC)&&(codecType != G729I_CODEC)){
      return APIG729_StsBadCodecType;
   }

   ippsZero_16s((short*)encoderObj,sizeof(G729Encoder_Obj)>>1) ;

   encoderObj->objPrm.objSize = ownEncoderObjSize();
   encoderObj->objPrm.mode = mode;
   encoderObj->objPrm.key = ENC_KEY;  
   encoderObj->objPrm.codecType=codecType;

   coeff[0] = b140[0];
   coeff[1] = b140[1];
   coeff[2] = b140[2];
   coeff[3] = a140[0];
   coeff[4] = -a140[1];
   coeff[5] = -a140[2];
   pBuf = (char*)encoderObj + sizeof(G729Encoder_Obj);
   ippsIIRInit_32f(&encoderObj->iirstate,coeff,2,NULL,pBuf);
   ippsIIRGetStateSize_32f(2,&fltsize);
   encoderObj->vadMem = (char *)((char*)pBuf + fltsize);
   VADGetSize(&fltsize);
   encoderObj->cngMem = (char *)((char*)encoderObj->vadMem + fltsize);
   CNGGetSize(&fltsize);
   encoderObj->msdMem = (char *)((char*)encoderObj->cngMem + fltsize);
   MSDGetSize(&fltsize);
   encoderObj->pHWState = (IppsWinHybridState_G729E_32f *)((char*)encoderObj->msdMem + fltsize);

   /* Static vectors to zero */

   ippsZero_32f(encoderObj->OldSpeechBuffer, SPEECH_BUFF_LEN);
   ippsZero_32f(encoderObj->OldExcitationBuffer, PITCH_LAG_MAX+INTERPOL_LEN);
   ippsZero_32f(encoderObj->OldWeightedSpeechBuffer, PITCH_LAG_MAX);
   ippsZero_32f(encoderObj->WeightedFilterMemory,  BWD_LPC_ORDER);
   ippsZero_32f(encoderObj->FltMem,   BWD_LPC_ORDER);
   encoderObj->fBetaPreFilter = PITCH_SHARPMIN;
   ippsCopy_32f(InitLSP, encoderObj->OldLSP, LPC_ORDER);
   ippsCopy_32f(InitLSP, encoderObj->OldQuantLSP, LPC_ORDER);
   for(i=0; i<4; i++) encoderObj->ExcitationError[i] = 1.f;

   encoderObj->PastQuantEnergy[0]=encoderObj->PastQuantEnergy[1]=encoderObj->PastQuantEnergy[2]=encoderObj->PastQuantEnergy[3]=-14.0;
   for(i=0; i<MOVING_AVER_ORDER; i++)
     ippsCopy_32f (&InitFrequences[0], &encoderObj->PrevFreq[i][0], LPC_ORDER );
   ippsZero_32f(encoderObj->OldForwardLPC, LPC_ORDERP1);
   encoderObj->OldForwardLPC[0]= 1.f;
   ippsZero_32f(encoderObj->OldForwardRC, 2);
   encoderObj->sFrameCounter = 0;
   /* For G.729B */
   /* Initialize VAD/DTX parameters */
   if(mode == G729Encode_VAD_Enabled) {
      encoderObj->prevVADDec = 1;
      encoderObj->prevPrevVADDec = 1;
      encoderObj->sCNGSeed = INIT_SEED_VAL;
      VADInit(encoderObj->vadMem);
      CNGInit(encoderObj->cngMem);
      MSDInit(encoderObj->msdMem);
   }
   encoderObj->prevLPCMode = 0;
   if(codecType==G729A_CODEC) {
      ippsZero_32f(encoderObj->ZeroMemory, LPC_ORDER);
   } else {
      ippsZero_32f(encoderObj->SynFltMemory,  BWD_LPC_ORDER);
      ippsZero_32f(encoderObj->ErrFltMemory, BWD_LPC_ORDER);
      ippsZero_32f(&encoderObj->UnitImpulse[BWD_LPC_ORDERP1], SUBFR_LEN);
      ippsZero_32f(encoderObj->PrevFlt, BWD_LPC_ORDERP1);
      encoderObj->PrevFlt[0] = 1.f;
      ippsWinHybridInit_G729E_32f(encoderObj->pHWState);
      ippsZero_32f(encoderObj->SynthBuffer, BWD_ANALISIS_WND_LEN);
      ippsZero_32f(encoderObj->BackwardLPCMemory, BWD_LPC_ORDERP1);
      encoderObj->BackwardLPCMemory[0] = 1.f;
      encoderObj->isBWDDominant = 0;
      encoderObj->fInterpolationCoeff = 1.1f;       /* Filter interpolation parameter */
      encoderObj->sGlobalStatInd = 10000;  /* Mesure of global stationnarity */
      encoderObj->sBWDStatInd = 0;       /* Nbre of consecutive backward frames */
      encoderObj->sValBWDStatInd = 0;   /* Value associated with stat_bwd */
      ippsZero_32f(encoderObj->OldBackwardLPC, BWD_LPC_ORDERP1);
      encoderObj->OldBackwardLPC[0]= 1.f;
      ippsZero_32f(encoderObj->OldBackwardRC, 2);
      ippsSet_32s(20,encoderObj->LagBuffer,5);
      ippsSet_32f(0.7f,encoderObj->PitchGainBuffer,5);
      encoderObj->sBWDFrmCounter = 0;
      encoderObj->sFWDFrmCounter = 0;
      encoderObj->isSmooth = 1;
      encoderObj->LogAreaRatioCoeff[0] = encoderObj->LogAreaRatioCoeff[1] = 0.f;
      encoderObj->sSearchTimes = 30;
   }

   return APIG729_StsNoErr;
}

G729_CODECFUN( APIG729_Status, apiG729Encode,
         (G729Encoder_Obj* encoderObj,const short* src, unsigned char *dst,G729Codec_Type codecType, int *frametype))
{
    /* LPC analysis */
	LOCAL_ALIGN_ARRAY(32, float, forwardAutoCorr, (LPC_ORDERP2+1),encoderObj);       /* Autocorrelations (forward) */
	LOCAL_ALIGN_ARRAY(32, float, backwardAutoCorr, BWD_LPC_ORDERP1,encoderObj);      /* Autocorrelations (backward) */
	LOCAL_ALIGN_ARRAY(32, float, backwardReflectCoeff, BWD_LPC_ORDER,encoderObj);       /* Reflection coefficients : backward analysis */
	LOCAL_ALIGN_ARRAY(32, float, forwardLPC, LPC_ORDERP1*2,encoderObj);      /* A(z) forward unquantized for the 2 subframes */
	LOCAL_ALIGN_ARRAY(32, float, forwardQntLPC, LPC_ORDERP1*2,encoderObj);    /* A(z) forward quantized for the 2 subframes */
	LOCAL_ALIGN_ARRAY(32, float, backwardLPC, 2*BWD_LPC_ORDERP1,encoderObj);  /* A(z) backward for the 2 subframes */
	LOCAL_ALIGN_ARRAY(32, float, WeightedLPC1, BWD_LPC_ORDERP1,encoderObj);        /* A(z) with spectral expansion         */
	LOCAL_ALIGN_ARRAY(32, float, WeightedLPC2, BWD_LPC_ORDERP1,encoderObj);        /* A(z) with spectral expansion         */
   LOCAL_ALIGN_ARRAY(32, float, CurrLSP, LPC_ORDER,encoderObj);
   LOCAL_ALIGN_ARRAY(32, float, tmpLSP, LPC_ORDER,encoderObj);
   LOCAL_ALIGN_ARRAY(32, float, CurrQntLSP, LPC_ORDER,encoderObj);        /* LSPs at 2th subframe                 */
   LOCAL_ALIGN_ARRAY(32, float, TmpAlignVec, WINDOW_LEN,encoderObj);
    /* Other vectors */
   LOCAL_ALIGN_ARRAY(32, float, ImpulseResponse, SUBFR_LEN,encoderObj);    /* Impulse response*/
	LOCAL_ALIGN_ARRAY(32, float, TargetVector, SUBFR_LEN,encoderObj);    /* Target vector for pitch search     */
	LOCAL_ALIGN_ARRAY(32, float, FltTargetVector, SUBFR_LEN,encoderObj);   /* Target vector for codebook search  */
	LOCAL_ALIGN_ARRAY(32, float, FixedCodebookExc, SUBFR_LEN,encoderObj);  /* Fixed codebook excitation          */
	LOCAL_ALIGN_ARRAY(32, float, FltAdaptExc, SUBFR_LEN,encoderObj);    /* Filtered adaptive excitation       */
	LOCAL_ALIGN_ARRAY(32, float, FltFixedCodebookExc, SUBFR_LEN,encoderObj);    /* Filtered fixed codebook excitation */
	LOCAL_ALIGN_ARRAY(32, float, PitchPredResidual, SUBFR_LEN,encoderObj);  /* Pitch prediction residual          */

   LOCAL_ARRAY(float, forwardReflectCoeff, LPC_ORDER,encoderObj);  /* Reflection coefficients : forward analysis */
   LOCAL_ARRAY(float, InterpolatedLSF, LPC_ORDER,encoderObj);  /* Interpolated LSF 1st subframe.       */
   LOCAL_ARRAY(float, CurrLSF, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, CurrFreq, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, TmpAutoCorr, LPC_ORDERP1,encoderObj);
   LOCAL_ARRAY(int,   EncodedParams, 19,encoderObj);
   LOCAL_ARRAY(float, g_coeff, 5,encoderObj); /* Correlations between TargetVector, FltAdaptExc, & FltFixedCodebookExc:
                                             <FltAdaptExc,FltAdaptExc>, <TargetVector,FltAdaptExc>, <FltFixedCodebookExc,FltFixedCodebookExc>, <TargetVector,FltFixedCodebookExc>,<FltAdaptExc,FltFixedCodebookExc>*/
   LOCAL_ARRAY(float, gamma1, 2,encoderObj); /* Weighting factor for the 2 subframes */
   LOCAL_ARRAY(float, gamma2, 2,encoderObj);
   LOCAL_ARRAY(int, code_lsp, 2,encoderObj);
   LOCAL_ARRAY(int, delayLine, 2,encoderObj);

   float *SpeechWnd, *pWindow;
   float  *newSpeech;                       /* Global variable */
   /* Weighted speech vector */
   float *WeightedSpeech;
   /* Excitation vector */
   float *Excitation;
   /* Zero vector */
   float *pZero;
   float *pError;
   float *pSynth;
   float *pQLPC;
   float *pUnQLPC;
   float *pLPC, *pQntLPC;
                                 
   /* Scalars */
   int LPCMode;                  /* LP Backward (1) / Forward (0) Indication mode */
   int apLen, aqLen;
   int   i, j, NGamma, NSbfr;
   int   openLoopPitch, pitchDelay, minPitchDelay, maxPitchDelay, fracPartPitchDelay;
   int   index, taming;
   float pitchGain, codeGain;
   int VADDecision;

   /* G.729 ANNEXE variables*/
   int isSaturateAZ;
   float EnergydB;

   int avg_lag;
   float tmp_lev;
   IppStatus sts;
   int *anau;

   if(NULL==src || NULL ==dst)
      return APIG729_StsBadArgErr;
   if ((codecType != G729_CODEC)&&(codecType != G729A_CODEC)&&(codecType != G729D_CODEC)&&(codecType != G729E_CODEC))
      return APIG729_StsBadCodecType;
   if(encoderObj->objPrm.objSize <= 0)
      return APIG729_StsNotInitialized;
   if(ENC_KEY != encoderObj->objPrm.key)
      return APIG729_StsBadCodecType;

   ippsZero_32f(WeightedLPC1,BWD_LPC_ORDERP1);
   ippsZero_32f(WeightedLPC2,BWD_LPC_ORDERP1);

   anau = &EncodedParams[0];

   newSpeech = encoderObj->OldSpeechBuffer + SPEECH_BUFF_LEN - FRM_LEN;
   SpeechWnd     = newSpeech - LOOK_AHEAD_LEN;                    /* Present frame  */
   pWindow   = encoderObj->OldSpeechBuffer + SPEECH_BUFF_LEN - WINDOW_LEN;        /* For LPC window */
   WeightedSpeech    = encoderObj->OldWeightedSpeechBuffer + PITCH_LAG_MAX;
   Excitation    = encoderObj->OldExcitationBuffer + PITCH_LAG_MAX + INTERPOL_LEN;

   if(codecType!=G729A_CODEC){
      pZero   = encoderObj->UnitImpulse + BWD_LPC_ORDERP1;
      pError  = encoderObj->ErrFltMemory + BWD_LPC_ORDER;
      pSynth = encoderObj->SynthBuffer + BWD_SYNTH_MEM;
   }

   if (encoderObj->sFrameCounter == 32767) encoderObj->sFrameCounter = 256;
   else encoderObj->sFrameCounter++;

   ippsConvert_16s32f(src,newSpeech,FRM_LEN);
   ippsIIR_32f_I(newSpeech,FRM_LEN,encoderObj->iirstate);
#ifdef CLIPPING_DENORMAL_MODE
   if((encoderObj->sFrameCounter % 5)==0) {
      ippsIIRGetDlyLine_32f(encoderObj->iirstate, TmpAlignVec);
      CLIP_DENORMAL_I(TmpAlignVec[0]);
      CLIP_DENORMAL_I(TmpAlignVec[1]);
      ippsIIRSetDlyLine_32f(encoderObj->iirstate, TmpAlignVec);
   }
#endif
   ownAutoCorr_G729_32f(pWindow, LPC_ORDERP2, forwardAutoCorr,TmpAlignVec);             /* Autocorrelations */
   ippsCopy_32f(forwardAutoCorr, TmpAutoCorr, LPC_ORDERP1);

   /* Lag windowing    */
   ippsMul_32f_I(lwindow, &forwardAutoCorr[1], LPC_ORDERP2);
   /* Levinson Durbin  */
   tmp_lev = 0;
   sts = ippsLevinsonDurbin_G729_32f(forwardAutoCorr, LPC_ORDER, &forwardLPC[LPC_ORDERP1], forwardReflectCoeff, &tmp_lev);
   if(sts == ippStsOverflow) {
      ippsCopy_32f(encoderObj->OldForwardLPC,&forwardLPC[LPC_ORDERP1],LPC_ORDER+1);
      forwardReflectCoeff[0] = encoderObj->OldForwardRC[0];
      forwardReflectCoeff[1] = encoderObj->OldForwardRC[1];
   } else {
      ippsCopy_32f(&forwardLPC[LPC_ORDERP1],encoderObj->OldForwardLPC,LPC_ORDER+1);
      encoderObj->OldForwardRC[0] = forwardReflectCoeff[0];
      encoderObj->OldForwardRC[1] = forwardReflectCoeff[1];
   }


   /* Convert A(z) to lsp */
   if(codecType==G729A_CODEC){
      ippsLPCToLSP_G729A_32f(&forwardLPC[LPC_ORDERP1], encoderObj->OldLSP, CurrLSP);
   } else {
      ippsLPCToLSP_G729_32f(&forwardLPC[LPC_ORDERP1], encoderObj->OldLSP, CurrLSP);
   }

   if (encoderObj->objPrm.mode == G729Encode_VAD_Enabled) {
      ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);
      VoiceActivityDetect_G729_32f(forwardReflectCoeff[1], CurrLSF, forwardAutoCorr, pWindow, encoderObj->sFrameCounter,
                  encoderObj->prevVADDec, encoderObj->prevPrevVADDec, &VADDecision, &EnergydB,encoderObj->vadMem,TmpAlignVec);

      if(codecType!=G729A_CODEC){
         MusicDetection_G729E_32f( encoderObj, codecType, forwardAutoCorr[0],forwardReflectCoeff, &VADDecision, EnergydB,encoderObj->msdMem,TmpAlignVec);
      }

      UpdateCNG(TmpAutoCorr, VADDecision,encoderObj->cngMem);
   } else VADDecision = 1;

   if(VADDecision == 0) {
      if(codecType==G729A_CODEC){
         UpdateVad_A(encoderObj, Excitation, WeightedSpeech, SpeechWnd, anau);
      } else {
         /* Inactive frame */
         ippsCopy_32f(&encoderObj->SynthBuffer[FRM_LEN], &encoderObj->SynthBuffer[0], BWD_SYNTH_MEM);
         /* Find interpolated LPC parameters in all subframes unquantized.      *
         * The interpolated parameters are in array forwardLPC of size (LPC_ORDER+1)*4      */
         if( encoderObj->prevLPCMode == 0) {
            ippsInterpolateC_G729_32f(encoderObj->OldLSP, 0.5f, CurrLSP, 0.5f, tmpLSP, LPC_ORDER);
    
            ippsLSPToLPC_G729_32f(tmpLSP, forwardLPC);
    
            ownACOS_G729_32f(tmpLSP, InterpolatedLSF, LPC_ORDER);
            ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);
         } else {
            /* no interpolation */
            /* unquantized */
            ippsLSPToLPC_G729_32f(CurrLSP, forwardLPC); /* Subframe 1 */
            ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);  /* transformation from LSP to LSF (freq.domain) */
            ippsCopy_32f(CurrLSF, InterpolatedLSF, LPC_ORDER);      /* Subframe 1 */
         } 
         
         if (encoderObj->sGlobalStatInd > 10000) {
            encoderObj->sGlobalStatInd -= 2621;
            if(encoderObj->sGlobalStatInd < 10000) encoderObj->sGlobalStatInd = 10000 ;
         }  
         encoderObj->isBWDDominant = 0;
         encoderObj->fInterpolationCoeff = 1.1f;

         ippsCopy_32f(CurrLSP, encoderObj->OldLSP, LPC_ORDER);

         PWGammaFactor_G729(gamma1, gamma2, InterpolatedLSF, CurrLSF, forwardReflectCoeff,&encoderObj->isSmooth, encoderObj->LogAreaRatioCoeff);
         
         UpdateVad_I(encoderObj, Excitation, forwardLPC, WeightedSpeech, gamma1, gamma2, pSynth,
                     pError, SpeechWnd, anau, codecType);

         /* update previous filter for next frame */
         ippsCopy_32f(&forwardQntLPC[LPC_ORDERP1], encoderObj->PrevFlt, LPC_ORDERP1);
         for(i=LPC_ORDERP1; i <BWD_LPC_ORDERP1; i++) encoderObj->PrevFlt[i] = 0.f;
      }
      encoderObj->prevLPCMode = 0;

      encoderObj->fBetaPreFilter = PITCH_SHARPMIN;

      /* Update memories for next frames */
      ippsCopy_32f(&encoderObj->OldSpeechBuffer[FRM_LEN], &encoderObj->OldSpeechBuffer[0], SPEECH_BUFF_LEN-FRM_LEN);
      ippsCopy_32f(&encoderObj->OldWeightedSpeechBuffer[FRM_LEN], &encoderObj->OldWeightedSpeechBuffer[0], PITCH_LAG_MAX);
      ippsCopy_32f(&encoderObj->OldExcitationBuffer[FRM_LEN], &encoderObj->OldExcitationBuffer[0], PITCH_LAG_MAX+INTERPOL_LEN);

      anau = EncodedParams+1;
      if(EncodedParams[0] == 0){ /* untransmitted*/
         *frametype=0;
      }else{
         *frametype=1; /* SID*/
         dst[0] = (unsigned char)(((anau[0] & 0x1) << 7) | ((anau[1] & 0x1f) << 2) | ((anau[2] & 0xf)>>2)); /* 1+5+2  */
         dst[1] = (unsigned char)(((anau[2] & 0x3) << 6) | ((anau[3] & 0x1f) << 1)); /* 2+5+LSB     */
      }
      CLEAR_SCRATCH_MEMORY(encoderObj);
      return APIG729_StsNoErr;
   }

   /* Active frame */

   *anau++ = CodecType2Num(codecType)+2; /* bit rate mode */
   if(encoderObj->objPrm.mode == G729Encode_VAD_Enabled) {
         encoderObj->sCNGSeed = INIT_SEED_VAL;
         encoderObj->prevPrevVADDec = encoderObj->prevVADDec;
         encoderObj->prevVADDec = VADDecision;
   }  


   /* LSP quantization */
   
   ippsLSPQuant_G729E_32f( CurrLSP, (float*)encoderObj->PrevFreq, CurrFreq, CurrQntLSP,  code_lsp);


   if( encoderObj->prevLPCMode == 0) {
      if(codecType!=G729A_CODEC){
         ippsInterpolateC_G729_32f(encoderObj->OldLSP, 0.5f, CurrLSP, 0.5f, tmpLSP, LPC_ORDER);
    
         ippsLSPToLPC_G729_32f(tmpLSP, forwardLPC);
    
         ownACOS_G729_32f(tmpLSP, InterpolatedLSF, LPC_ORDER);
         ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);
      }

      ippsInterpolateC_G729_32f(encoderObj->OldQuantLSP, 0.5f, CurrQntLSP, 0.5f, tmpLSP, LPC_ORDER);
    
      ippsLSPToLPC_G729_32f(tmpLSP, forwardQntLPC);
      ippsLSPToLPC_G729_32f(CurrQntLSP, &forwardQntLPC[LPC_ORDER+1]);
   } else {
      /* no interpolation */
      /* unquantized */
      ippsLSPToLPC_G729_32f(CurrLSP, forwardLPC); /* Subframe 1 */
      ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);  /* transformation from LSP to LSF (freq.domain) */
      ippsCopy_32f(CurrLSF, InterpolatedLSF, LPC_ORDER);      /* Subframe 1 */
      ippsLSPToLPC_G729_32f(CurrQntLSP, &forwardQntLPC[LPC_ORDERP1]); /* Subframe 2 */
      ippsCopy_32f(&forwardQntLPC[LPC_ORDERP1], forwardQntLPC, LPC_ORDERP1);      /* Subframe 1 */
   }

   /* Decision for the switch Forward / Backward mode */
   if(codecType == G729E_CODEC) {
      /* LPC recursive Window as in G728 */
      ippsWinHybrid_G729E_32f(encoderObj->SynthBuffer, backwardAutoCorr, encoderObj->pHWState);
      /* Lag windowing    */
      ippsMul_32f_I(lagBwd, &backwardAutoCorr[1], BWD_LPC_ORDER);
      if (backwardAutoCorr[0] < 1.0f) backwardAutoCorr[0] = 1.0f;
      sts = ippsLevinsonDurbin_G729_32f(backwardAutoCorr, BWD_LPC_ORDER, &backwardLPC[BWD_LPC_ORDERP1], backwardReflectCoeff, &tmp_lev);
      if(sts == ippStsOverflow) {
         ippsCopy_32f(encoderObj->OldBackwardLPC,&backwardLPC[BWD_LPC_ORDERP1],BWD_LPC_ORDER+1);
         backwardReflectCoeff[0] = encoderObj->OldBackwardRC[0];
         backwardReflectCoeff[1] = encoderObj->OldBackwardRC[1];
      } else {
         ippsCopy_32f(&backwardLPC[BWD_LPC_ORDERP1],encoderObj->OldBackwardLPC,BWD_LPC_ORDER+1);
         encoderObj->OldBackwardRC[0] = backwardReflectCoeff[0];
         encoderObj->OldBackwardRC[1] = backwardReflectCoeff[1];
      } 

      /* Tests saturation of backwardLPC */
      isSaturateAZ = 0;
      for (i=BWD_LPC_ORDERP1; i<2*BWD_LPC_ORDERP1; i++) if (backwardLPC[i] >= 8.f) {isSaturateAZ = 1;break;}
      if (isSaturateAZ == 1) ippsCopy_32f(encoderObj->BackwardLPCMemory, &backwardLPC[BWD_LPC_ORDERP1], BWD_LPC_ORDERP1);
      else ippsCopy_32f(&backwardLPC[BWD_LPC_ORDERP1], encoderObj->BackwardLPCMemory, BWD_LPC_ORDERP1);

      /* Additional bandwidth expansion on backward filter */
      WeightLPCCoeff_G729(&backwardLPC[BWD_LPC_ORDERP1], BWD_GAMMA, BWD_LPC_ORDER, &backwardLPC[BWD_LPC_ORDERP1]);

      SetLPCMode_G729E(encoderObj,SpeechWnd, forwardQntLPC, backwardLPC, &LPCMode, CurrLSP,TmpAlignVec);
      *anau++ = LPCMode;
   } else {
      if (encoderObj->sGlobalStatInd > 10000) {
         encoderObj->sGlobalStatInd -= 2621;
         if( encoderObj->sGlobalStatInd < 10000) encoderObj->sGlobalStatInd = 10000 ;
      }
      LPCMode = 0;
      encoderObj->isBWDDominant = 0;
      encoderObj->fInterpolationCoeff = 1.1f;
   }
   /* Update synthesis signal for next frame.          */
   ippsCopy_32f(&encoderObj->SynthBuffer[FRM_LEN], &encoderObj->SynthBuffer[0], BWD_SYNTH_MEM);
   /* Update the LSPs for the next frame */
   ippsCopy_32f(CurrLSP, encoderObj->OldLSP, LPC_ORDER);
   if( LPCMode == 0) {
      ippsCopy_32f(CurrQntLSP, encoderObj->OldQuantLSP, LPC_ORDER);
      ippsMove_32f(encoderObj->PrevFreq[0], encoderObj->PrevFreq[1], 3*LPC_ORDER);
      ippsCopy_32f(CurrFreq, encoderObj->PrevFreq[0], LPC_ORDER);

      *anau++ = code_lsp[0];
      *anau++ = code_lsp[1];
   }

   /* Find the weighted input speech w_sp[] for the whole speech frame   */
   if(LPCMode == 0) {
      apLen = LPC_ORDER;
      aqLen = LPC_ORDER;
      if (encoderObj->isBWDDominant == 0) pUnQLPC = forwardLPC;
      else pUnQLPC = forwardQntLPC;
      pQLPC = forwardQntLPC;
      if(codecType==G729A_CODEC) {
         /* Compute A(z/gamma) */
         WeightLPCCoeff_G729(&forwardQntLPC[0],   GAMMA1_G729A, LPC_ORDER, &forwardLPC[0]);
         WeightLPCCoeff_G729(&forwardQntLPC[LPC_ORDERP1], GAMMA1_G729A, LPC_ORDER, &forwardLPC[LPC_ORDERP1]);
      } else {
         PWGammaFactor_G729(gamma1, gamma2, InterpolatedLSF, CurrLSF, forwardReflectCoeff,&encoderObj->isSmooth, encoderObj->LogAreaRatioCoeff);
         /* update previous filter for next frame */
         ippsCopy_32f(&pQLPC[LPC_ORDERP1], encoderObj->PrevFlt, LPC_ORDERP1);
         for(i=LPC_ORDERP1; i <BWD_LPC_ORDERP1; i++) encoderObj->PrevFlt[i] = 0.f;
         for(j=LPC_ORDERP1; j<BWD_LPC_ORDERP1; j++) encoderObj->UnitImpulse[j] = 0.f;
      }
   } else {
      if (encoderObj->isBWDDominant == 0) {
         apLen = LPC_ORDER;
         pUnQLPC = forwardLPC;
      } else {
         apLen = BWD_LPC_ORDER;
         pUnQLPC = backwardLPC;
      }
      aqLen = BWD_LPC_ORDER;
      pQLPC = backwardLPC;
      /*   ADAPTIVE BANDWIDTH EXPANSION FOR THE PERCEPTUAL WEIGHTING FILTER   */
      if (encoderObj->isBWDDominant == 0) {
         gamma1[0] = 0.9f;
         gamma1[1] = 0.9f;
         gamma2[0] = 0.4f;
         gamma2[1] = 0.4f;
      } else {
         gamma1[0] = 0.98f;
         gamma1[1] = 0.98f;
         gamma2[0] = 0.4f;
         gamma2[1] = 0.4f;
      } 
      if (encoderObj->isBWDDominant == 0) {
         for(j=LPC_ORDERP1; j<BWD_LPC_ORDERP1; j++) encoderObj->UnitImpulse[j] = 0.f;
      } 
      /* update previous filter for next frame */
      ippsCopy_32f(&pQLPC[BWD_LPC_ORDERP1], encoderObj->PrevFlt, BWD_LPC_ORDERP1);
   }

   /* 3.3 Compute weighted input speech for the whole speech frame. */
   if(codecType!=G729A_CODEC){
      pLPC = pUnQLPC;
      for (i=0; i<2; i++) {
         WeightLPCCoeff_G729(pLPC, gamma1[i], apLen, WeightedLPC1);
         WeightLPCCoeff_G729(pLPC, gamma2[i], apLen, WeightedLPC2);

         ippsConvBiased_32f(WeightedLPC1,apLen+1,&SpeechWnd[i*SUBFR_LEN],SUBFR_LEN+apLen,&WeightedSpeech[i*SUBFR_LEN],SUBFR_LEN,apLen);
         ippsSynthesisFilter_G729_32f(WeightedLPC2, apLen, &WeightedSpeech[i*SUBFR_LEN], &WeightedSpeech[i*SUBFR_LEN], SUBFR_LEN, &encoderObj->FltMem[BWD_LPC_ORDER-apLen]);
         for(j=0; j<BWD_LPC_ORDER; j++) encoderObj->FltMem[j] = WeightedSpeech[i*SUBFR_LEN+SUBFR_LEN-BWD_LPC_ORDER+j];
         pLPC += apLen+1;
      }

      /* 3.4 Open-loop analysis. */
      OpenLoopPitchSearch_G729_32f(WeightedSpeech, &openLoopPitch);

      for (i= 0; i< 4; i++)
         encoderObj->LagBuffer[i] = encoderObj->LagBuffer[i+1];

      avg_lag = AVG(encoderObj->LagBuffer[0],encoderObj->LagBuffer[1],encoderObj->LagBuffer[2],encoderObj->LagBuffer[3]);
      if( abs( (int) (openLoopPitch/2.0) - avg_lag)<=2)
         encoderObj->LagBuffer[4] = (int) (openLoopPitch/2.0);
      else if( abs((int) (openLoopPitch/3.0) - avg_lag)<=2)
         encoderObj->LagBuffer[4] = (int) (openLoopPitch/3.0);
      else
         encoderObj->LagBuffer[4] = openLoopPitch;
   } else {
      ippsConvBiased_32f(&forwardQntLPC[0],LPC_ORDER+1,SpeechWnd,SUBFR_LEN+LPC_ORDER,Excitation,SUBFR_LEN,LPC_ORDER);
      ippsConvBiased_32f(&forwardQntLPC[LPC_ORDERP1],LPC_ORDER+1,&SpeechWnd[SUBFR_LEN],SUBFR_LEN+LPC_ORDER,&Excitation[SUBFR_LEN],SUBFR_LEN,LPC_ORDER);


      { 

         WeightedLPC1[0] = 1.0f;
         for(i=1; i<=LPC_ORDER; i++)
            WeightedLPC1[i] = forwardLPC[i] - 0.7f * forwardLPC[i-1];
         ippsSynthesisFilter_G729_32f(WeightedLPC1, LPC_ORDER, &Excitation[0], &WeightedSpeech[0], SUBFR_LEN, encoderObj->FltMem);
         for (i = 0; i < LPC_ORDER; i++)  CLIP_DENORMAL(WeightedSpeech[(SUBFR_LEN-LPC_ORDER)+i],encoderObj->FltMem[i]);

         for(i=1; i<=LPC_ORDER; i++)
            WeightedLPC1[i] = forwardLPC[i+LPC_ORDERP1] - 0.7f * forwardLPC[i-1+LPC_ORDERP1];
         ippsSynthesisFilter_G729_32f(WeightedLPC1, LPC_ORDER, &Excitation[SUBFR_LEN], &WeightedSpeech[SUBFR_LEN], SUBFR_LEN, encoderObj->FltMem);
         for (i = 0; i < LPC_ORDER; i++)  CLIP_DENORMAL(WeightedSpeech[SUBFR_LEN+(SUBFR_LEN-LPC_ORDER)+i],encoderObj->FltMem[i]);
      } 

     /* Annex A.3.4 Open-loop analysis */
	  ippsOpenLoopPitchSearch_G729A_32f(WeightedSpeech, &openLoopPitch);

   }
   /* Range for closed loop pitch search in 1st subframe */
   minPitchDelay = openLoopPitch - 3;
   if (minPitchDelay < PITCH_LAG_MIN) minPitchDelay = PITCH_LAG_MIN;
   maxPitchDelay = minPitchDelay + 6;
   if (maxPitchDelay > PITCH_LAG_MAX) {
      maxPitchDelay = PITCH_LAG_MAX;
      minPitchDelay = maxPitchDelay - 6;
   }

   pLPC  = pUnQLPC;     /* pointer to interpolated "unquantized"LPC parameters           */
   pQntLPC = pQLPC;      /* pointer to interpolated "quantized" LPC parameters */

   for (NSbfr = 0, NGamma = 0;  NSbfr < FRM_LEN; NSbfr += SUBFR_LEN, NGamma++) {
      if(codecType!=G729A_CODEC){
         /* LPC computing: weights of filter  */
         WeightLPCCoeff_G729(pLPC, gamma1[NGamma], apLen, WeightedLPC1);
         WeightLPCCoeff_G729(pLPC, gamma2[NGamma], apLen, WeightedLPC2);
        
         /* Clause 3.5 Computation of impulse response */
         for (i = 0; i <=apLen; i++) encoderObj->UnitImpulse[i] = WeightedLPC1[i];
         ippsSynthesisFilter_G729_32f(pQntLPC, aqLen, encoderObj->UnitImpulse, ImpulseResponse, SUBFR_LEN, pZero);
         ippsSynthesisFilter_G729_32f(WeightedLPC2, apLen, ImpulseResponse, ImpulseResponse, SUBFR_LEN, pZero);
        
         /* pass the resudual r(n) through 1/A(z) */
         ippsConvBiased_32f(pQntLPC,aqLen+1,&SpeechWnd[NSbfr],SUBFR_LEN+aqLen,&Excitation[NSbfr],SUBFR_LEN,aqLen);
         for (i=0; i<SUBFR_LEN; i++) PitchPredResidual[i] = Excitation[NSbfr+i];
         ippsSynthesisFilter_G729_32f(pQntLPC, aqLen, &Excitation[NSbfr], pError, SUBFR_LEN, &encoderObj->ErrFltMemory[BWD_LPC_ORDER-aqLen]);
         /*  then through the weighting filter W(z) where TargetVector is a target signal*/
         ippsConvBiased_32f(WeightedLPC1,aqLen+1,pError,SUBFR_LEN+aqLen,TargetVector,SUBFR_LEN,aqLen);
         ippsSynthesisFilter_G729_32f(WeightedLPC2, apLen, TargetVector, TargetVector, SUBFR_LEN, &encoderObj->WeightedFilterMemory[BWD_LPC_ORDER-apLen]);

         pitchDelay = AdaptiveCodebookSearch_G729_32f(&Excitation[NSbfr], TargetVector, ImpulseResponse, SUBFR_LEN, minPitchDelay, maxPitchDelay,
                        NSbfr, &fracPartPitchDelay, codecType,TmpAlignVec);
      } else {
         /* Computation of impulse response */
         ImpulseResponse[0] = 1.0f;
         ippsZero_32f(&ImpulseResponse[1], SUBFR_LEN-1);
         ippsSynthesisFilter_G729_32f(pLPC, LPC_ORDER, ImpulseResponse, ImpulseResponse, SUBFR_LEN, &ImpulseResponse[1]);
         /* Annex A.3.6 Computation of target signal */
         ippsSynthesisFilter_G729_32f(pLPC, LPC_ORDER, &Excitation[NSbfr], TargetVector, SUBFR_LEN, encoderObj->WeightedFilterMemory);
         /* Annex A.3.7 Adaptive-codebook search */
         pitchDelay = ownAdaptiveCodebookSearch_G729A_32f(&Excitation[NSbfr], TargetVector, ImpulseResponse, minPitchDelay, maxPitchDelay,
                    NSbfr, &fracPartPitchDelay,TmpAlignVec);
      }

      if (NSbfr == 0) {  /* if 1st subframe */
         /* encode pitch delay (with fraction) */
         if (pitchDelay <= 85)
            index = pitchDelay*3 - 58 + fracPartPitchDelay;
         else
            index = pitchDelay + 112;
        
         /* find T0_min and T0_max for second subframe */
        
         minPitchDelay = pitchDelay - 5;
         if (minPitchDelay < PITCH_LAG_MIN) minPitchDelay = PITCH_LAG_MIN;
         maxPitchDelay = minPitchDelay + 9;
         if (maxPitchDelay > PITCH_LAG_MAX) {
            maxPitchDelay = PITCH_LAG_MAX;
            minPitchDelay = maxPitchDelay - 9;
         } 
      } else {                   /* second subframe */
         if (codecType == G729D_CODEC) {      /* 4 bits in 2nd subframe (6.4 kbps) */
            if (pitchDelay < minPitchDelay + 3)
               index = pitchDelay - minPitchDelay;
            else if (pitchDelay < minPitchDelay + 7)
               index = (pitchDelay - (minPitchDelay + 3)) * 3 + fracPartPitchDelay + 3;
            else
               index = (pitchDelay - (minPitchDelay + 7)) + 13;
         } else {
            index = pitchDelay - minPitchDelay;
            index = index*3 + 2 + fracPartPitchDelay;
         } 
        
      } 
        
      *anau++ = index;
        
      if ( (NSbfr == 0) && (codecType != G729D_CODEC) ) {
         *anau = Parity(index);
         if( codecType == G729E_CODEC) {       
            *anau ^= ((index >> 1) & 0x0001);
         }
         anau++;
      }
      
      if(codecType!=G729A_CODEC){
         delayLine[0] = pitchDelay;
         delayLine[1] = fracPartPitchDelay;
         ippsDecodeAdaptiveVector_G729_32f_I(delayLine, &Excitation[NSbfr]);

         ippsConvBiased_32f( &Excitation[NSbfr], SUBFR_LEN , ImpulseResponse, SUBFR_LEN ,  FltAdaptExc, SUBFR_LEN ,0);
      } else {
         ippsSynthesisFilter_G729_32f(pLPC, LPC_ORDER, &Excitation[NSbfr], FltAdaptExc, SUBFR_LEN, encoderObj->ZeroMemory);
      }
      /* clause 3.7.3 Computation of the adaptive-codebook gain */ 
      pitchGain = ownAdaptiveCodebookGainCoeff_G729_32f(TargetVector, FltAdaptExc, g_coeff, SUBFR_LEN);

      /* clip pitch gain if taming is necessary */

      taming = TestErrorContribution_G729(pitchDelay, fracPartPitchDelay, encoderObj->ExcitationError);

      if( taming == 1){
         CLIP_TO_UPLEVEL(pitchGain,MAX_GAIN_TIMING);
      } 
      /* Annex A3.8.1 */   
      /* clause 3.8.1 Equ 50 */
      ippsAdaptiveCodebookContribution_G729_32f(pitchGain, FltAdaptExc, TargetVector, FltTargetVector);
        
      /* Fixed codebook search. */
      { 
		   LOCAL_ALIGN_ARRAY(32, float, dn, SUBFR_LEN,encoderObj);
		   LOCAL_ALIGN_ARRAY(32, float, rr, TOEPLIZ_MATRIX_SIZE,encoderObj);
         /* pitch contribution to ImpulseResponse */
         if(pitchDelay < SUBFR_LEN) {
            ippsHarmonicFilter_32f_I(encoderObj->fBetaPreFilter,pitchDelay,&ImpulseResponse[pitchDelay],SUBFR_LEN-pitchDelay);
         } 

         ippsCrossCorr_32f(ImpulseResponse, SUBFR_LEN, FltTargetVector, SUBFR_LEN, dn,  SUBFR_LEN, 0);

         switch (codecType) {
         case G729_CODEC:    /* 8 kbit/s */
         {  
            ippsToeplizMatrix_G729_32f(ImpulseResponse, rr);
            ippsFixedCodebookSearch_G729_32f(dn, rr, FixedCodebookExc, anau, &encoderObj->sSearchTimes, NSbfr);
            CodewordImpConv_G729_32f(anau[0],FixedCodebookExc,ImpulseResponse,FltFixedCodebookExc );
            anau += 2;
            break;
         }
         case G729A_CODEC:    /* 8 kbit/s */
         {  
            ippsToeplizMatrix_G729_32f(ImpulseResponse, rr);
            ippsFixedCodebookSearch_G729A_32f(dn, rr, FixedCodebookExc, anau);
            CodewordImpConv_G729_32f(anau[0],FixedCodebookExc,ImpulseResponse,FltFixedCodebookExc );
            anau += 2;
            break;
         }
         case G729D_CODEC:    /* 6.4 kbit/s */
         {
            ippsToeplizMatrix_G729D_32f(ImpulseResponse, rr);
            ippsFixedCodebookSearch_G729D_32f(dn, rr, ImpulseResponse, FixedCodebookExc, FltFixedCodebookExc, anau);
            anau += 2;
            break;
         }  
         case G729E_CODEC:    /* 11.8 kbit/s */
         {   
            /* calculate residual after long term prediction */
            ippsAdaptiveCodebookContribution_G729_32f(pitchGain, &Excitation[NSbfr], PitchPredResidual, PitchPredResidual);
            ippsFixedCodebookSearch_G729E_32f(LPCMode, dn, PitchPredResidual, ImpulseResponse, FixedCodebookExc, FltFixedCodebookExc, anau);
            anau += 5;
            break;
         }
         }   /* end of switch */
         /* Include fixed-gain pitch contribution into FixedCodebookExc. */
         if(pitchDelay < SUBFR_LEN) {
            ippsHarmonicFilter_32f_I(encoderObj->fBetaPreFilter,pitchDelay,&FixedCodebookExc[pitchDelay],SUBFR_LEN-pitchDelay);
         }
         LOCAL_ALIGN_ARRAY_FREE(32, float, rr, TOEPLIZ_MATRIX_SIZE,encoderObj);
         LOCAL_ALIGN_ARRAY_FREE(32, float, dn, SUBFR_LEN,encoderObj);
      }
      /* Annex A3.9 Quantization of gains */
      /* + Clause 3.9*/
      AdaptiveCodebookGainCoeff_G729_32f( TargetVector, FltAdaptExc, FltFixedCodebookExc, g_coeff);
        
      index = GainQuant_G729(FixedCodebookExc, g_coeff, SUBFR_LEN, &pitchGain, &codeGain, taming,encoderObj->PastQuantEnergy,codecType,(char *)TmpAlignVec);  
      *anau++ = index;

      /* Update and bound pre filter factor with quantized adaptive codebook gain */
      for (i= 0; i< 4; i++)
         encoderObj->PitchGainBuffer[i] = encoderObj->PitchGainBuffer[i+1];
      encoderObj->PitchGainBuffer[4] = pitchGain;

      encoderObj->fBetaPreFilter = pitchGain;
      CLIP_TO_UPLEVEL(encoderObj->fBetaPreFilter,PITCH_SHARPMAX);
      CLIP_TO_LOWLEVEL(encoderObj->fBetaPreFilter,PITCH_SHARPMIN);

      /* Find the total excitation. */
      ippsInterpolateC_G729_32f(&Excitation[NSbfr], pitchGain, FixedCodebookExc, codeGain, &Excitation[NSbfr], SUBFR_LEN);
      
      /* Update error function for taming process. */
      UpdateExcErr_G729(pitchGain, pitchDelay, encoderObj->ExcitationError);

      if(codecType!=G729A_CODEC){
         /* Find synthesis speech corresponding to Excitation.  */
         ippsSynthesisFilter_G729_32f(pQntLPC, aqLen, &Excitation[NSbfr], &pSynth[NSbfr], SUBFR_LEN, &encoderObj->SynFltMemory[BWD_LPC_ORDER-aqLen]);
         for(j=0; j<BWD_LPC_ORDER; j++) encoderObj->SynFltMemory[j] = pSynth[NSbfr+SUBFR_LEN-BWD_LPC_ORDER+j];
        
         /* Update filters memories. */
         for (i = SUBFR_LEN-BWD_LPC_ORDER, j = 0; i < SUBFR_LEN; i++, j++) {
            encoderObj->ErrFltMemory[j] = SpeechWnd[NSbfr+i] - pSynth[NSbfr+i];
            encoderObj->WeightedFilterMemory[j]  = TargetVector[i] - pitchGain*FltAdaptExc[i] - codeGain*FltFixedCodebookExc[i];
         } 
      } else {
         /* Update filters memories. */
         for (i = SUBFR_LEN-LPC_ORDER, j = 0; i < SUBFR_LEN; i++, j++)
            encoderObj->WeightedFilterMemory[j]  = TargetVector[i] - pitchGain*FltAdaptExc[i] - codeGain*FltFixedCodebookExc[i];
      }

      pLPC   += apLen+1;
      pQntLPC   += aqLen+1;
   }

   encoderObj->prevLPCMode = LPCMode;

   /* Update signal for next frame.                    */

   ippsCopy_32f(&encoderObj->OldSpeechBuffer[FRM_LEN], &encoderObj->OldSpeechBuffer[0], SPEECH_BUFF_LEN-FRM_LEN);
   ippsCopy_32f(&encoderObj->OldWeightedSpeechBuffer[FRM_LEN], &encoderObj->OldWeightedSpeechBuffer[0], PITCH_LAG_MAX);
   ippsCopy_32f(&encoderObj->OldExcitationBuffer[FRM_LEN], &encoderObj->OldExcitationBuffer[0], PITCH_LAG_MAX+INTERPOL_LEN);

   if((codecType == G729_CODEC)&&(VADDecision != 1)) {
      anau = EncodedParams;
   } else if(codecType == G729E_CODEC) {
      anau = EncodedParams+2;
   } else {
      anau = EncodedParams+1;
   }
   if((codecType == G729_CODEC)||(codecType == G729A_CODEC)) {   
      *frametype=3;
      dst[0] = (unsigned char)(anau[0] & 0xff);              
      dst[1] = (unsigned char)((anau[1] & 0x3ff) >> 2);      
      dst[2] = (unsigned char)(((anau[1] & 0x3) << 6) | ((anau[2]>>2)&0x3f) ); /* 2 + 6  */
      /* 2 + 1 + 5*/
      dst[3] = (unsigned char)(((anau[2] & 0x3) << 6) | ((anau[3] & 0x1) << 5) | ((anau[4] & 0x1fff) >> 8) ); 
      dst[4] = (unsigned char)(anau[4] & 0xff); /* 8*/
      dst[5] = (unsigned char)(((anau[5] & 0xf)<<4) | ((anau[6] & 0x7f) >> 3)); /* 4 + 4*/
      dst[6] = (unsigned char)(((anau[6] & 0x7)<< 5) | (anau[7] & 0x1f)); /* 3 + 5*/
      dst[7] = (unsigned char)((anau[8] & 0x1fff) >> 5); /* 8*/
      dst[8] = (unsigned char)(((anau[8] & 0x1f) << 3) | ((anau[9] & 0xf) >> 1)); /* 5 + 3*/
      dst[9] = (unsigned char)(((anau[9] & 0x1) << 7) | (anau[10] & 0x7f)); /* 1 + 7*/
   } else if(codecType == G729D_CODEC) {/* D */
      *frametype=2;
      dst[0] = (unsigned char)( anau[0] & 0xff);                                                           /*8*/
      dst[1] = (unsigned char)( (anau[1] & 0x3ff) >> 2);                                                   /*8*/
      dst[2] = (unsigned char)( ((anau[1] & 0x3) << 6) | ((anau[2]>>2)&0x3f));                             /* 2 + 6  */
      dst[3] = (unsigned char)( ((anau[2] & 0x3) << 6) | ((anau[3]>>3)&0x3f));                             /* 2 + 6  */
      dst[4] = (unsigned char)( ((anau[3] & 0x7) << 5) | ((anau[4] & 0x3) << 3) | ((anau[5] >> 3)& 0x7));  /* 3 + 2 + 3 */
      dst[5] = (unsigned char)( ((anau[5] & 0x7) << 5) | ((anau[6] & 0xf) << 1)| ((anau[7] >> 8)& 0x1));  /* 3 + 4 + 1*/
      dst[6] = (unsigned char)( anau[7] & 0xff);                                                           /* 8*/
      dst[7] = (unsigned char)( (anau[8] & 0x3) << 6 | (anau[9] & 0x3f));                                  /* 2 + 6*/
   } else if(codecType == G729E_CODEC) {/* E*/
      *frametype=4;
      if(LPCMode == 0) { /* Forward*/
         dst[0] = (unsigned char)( (anau[0] >> 2) & 0x3f);                                                  /* 2 + 6 */
         dst[1] = (unsigned char)( ((anau[0] & 0x3) << 6) | ((anau[1]>>4)&0x3f));                           /* 2 + 6 */
         dst[2] = (unsigned char)( ((anau[1] & 0xf) << 4) | ((anau[2]>>4)&0xf));                            /* 4 + 4 */  
         dst[3] = (unsigned char)( ((anau[2] & 0xf) << 4) | ((anau[3]&0x1)<<3) | ((anau[4]>>4)&0x7));       /* 4 + 1 + 3 */
         dst[4] = (unsigned char)( ((anau[4] & 0xf) << 4) | ((anau[5]>>3)&0xf));                            /* 4 + 4 */  
         dst[5] = (unsigned char)( ((anau[5] & 0x7) << 5) | ((anau[6]>>2)&0x1f));                           /* 3 + 5 */  
         dst[6] = (unsigned char)( ((anau[6] & 0x3) << 6) | ((anau[7]>>1)&0x3f));                           /* 2 + 6 */  
         dst[7] = (unsigned char)( ((anau[7]& 0x1) << 7)  | (anau[8]&0x7f));                                /* 1 + 7 */
         dst[8] = (unsigned char)( ((anau[9]& 0x7f) << 1) | ((anau[10]>>4)&0x1));                           /* 7 + 1 */
         dst[9] = (unsigned char)( ((anau[10] & 0xf) << 4) | ((anau[11]>>3)&0xf));                          /* 4 + 4 */
         dst[10] = (unsigned char)( ((anau[11] & 0x7) << 5) | ((anau[12]>>2)&0x1f));                        /* 3 + 5 */
         dst[11] = (unsigned char)( ((anau[12] & 0x3) << 6) | ((anau[13]>>1)&0x3f));                        /* 2 + 6 */
         dst[12] = (unsigned char)( ((anau[13]& 0x1) << 7)  | (anau[14]&0x7f));                             /* 1 + 7 */
         dst[13] = (unsigned char)( ((anau[15]& 0x7f) << 1) | ((anau[16]>>6)&0x1));                         /* 7 + 1 */
         dst[14] = (unsigned char)( ((anau[16] & 0x3f) << 2));
      } else { /* Backward*/
         dst[0] = (unsigned char)((3<<6) | ((anau[0] >> 2) & 0x3f));
         dst[1] = (unsigned char)(((anau[0] & 0x3) << 6) | ((anau[1]&0x1)<<5) | ((anau[2]>>8)&0x1f));       /* 2 + 1 + 5 */
         dst[2] = (unsigned char)(anau[2] & 0xff);
         dst[3] = (unsigned char)((anau[3] >> 2) & 0xff);                                                  /* 2 + 6 */
         dst[4] = (unsigned char)(((anau[3] & 0x3) << 6) | ((anau[4]>>1)&0x3f));                           /* 2 + 6 */
         dst[5] = (unsigned char)(((anau[4]& 0x1) << 7)  | (anau[5]&0x7f));                                /* 1 + 7 */
         dst[6] = (unsigned char)(((anau[6]& 0x7f) << 1) | ((anau[7]>>6)&0x1));                           /* 7 + 1 */
         dst[7] = (unsigned char)(((anau[7]&0x3f) << 2) | ((anau[8] >>3)&0x3));
         dst[8] = (unsigned char)(((anau[8] & 0x7) << 5) | ((anau[9]>>8)&0x1f));                        /* 3 + 5 */
         dst[9] = (unsigned char)(anau[9] & 0xff);
         dst[10] = (unsigned char)((anau[10] >> 2) & 0xff);                                                  /* 2 + 6 */
         dst[11] = (unsigned char)(((anau[10] & 0x3) << 6) | ((anau[11]>>1)&0x3f));                           /* 2 + 6 */
         dst[12] = (unsigned char)(((anau[11]& 0x1) << 7)  | (anau[12]&0x7f));                                /* 1 + 7 */
         dst[13] = (unsigned char)(((anau[13]& 0x7f) << 1) | ((anau[14]>>6)&0x1));                           /* 7 + 1 */
         dst[14] = (unsigned char)(((anau[14] & 0x3f) << 2));
      }
   }
   CLEAR_SCRATCH_MEMORY(encoderObj);
   return APIG729_StsNoErr;
}

G729_CODECFUN(  APIG729_Status, apiG729EncodeVAD, 
         (G729Encoder_Obj* encoderObj,const short* src, short* dst, G729Codec_Type codecType, int *frametype ))
{
   LOCAL_ALIGN_ARRAY(32, float, forwardAutoCorr, (LPC_ORDERP2+1),encoderObj);       /* Autocorrelations (forward) */
	LOCAL_ALIGN_ARRAY(32, float, forwardLPC, LPC_ORDERP1*2,encoderObj);      /* A(z) forward unquantized for the 2 subframes */
	LOCAL_ALIGN_ARRAY(32, float, forwardQntLPC, LPC_ORDERP1*2,encoderObj);    /* A(z) forward quantized for the 2 subframes */
	LOCAL_ALIGN_ARRAY(32, float, WeightedLPC1, BWD_LPC_ORDERP1,encoderObj);        /* A(z) with spectral expansion         */
	LOCAL_ALIGN_ARRAY(32, float, WeightedLPC2, BWD_LPC_ORDERP1,encoderObj);        /* A(z) with spectral expansion         */
   LOCAL_ALIGN_ARRAY(32, float, TmpAlignVec, WINDOW_LEN,encoderObj);
   LOCAL_ARRAY(float, TmpAutoCorr, LPC_ORDERP1,encoderObj);
   LOCAL_ARRAY(float, forwardReflectCoeff, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, CurrLSP, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, CurrLSF, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, tmpLSP, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, InterpolatedLSF, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(int, EncodedParams, 4,encoderObj);
   LOCAL_ARRAY(float, gamma1, 2,encoderObj);
   LOCAL_ARRAY(float, gamma2, 2,encoderObj);

   float *SpeechWnd, *pWindow;
   float  *newSpeech;
   /* Weighted speech vector */
   float *WeightedSpeech;
   /* Excitation vector */
   float *Excitation;
   float *pError;
   float *pSynth;
   int *anau;
   short  isVAD;
   float tmp_lev;
   int VADDecision, i;
   float EnergydB;
   IppStatus sts;

   if(NULL==encoderObj || NULL==src || NULL ==dst)
      return APIG729_StsBadArgErr;
   if ((codecType != G729_CODEC)&&(codecType != G729A_CODEC)
      &&(codecType != G729D_CODEC)&&(codecType != G729E_CODEC))
      return APIG729_StsBadCodecType;
   if(encoderObj->objPrm.objSize <= 0)
      return APIG729_StsNotInitialized;
   if(ENC_KEY != encoderObj->objPrm.key)
      return APIG729_StsBadCodecType;
   isVAD = (short)(encoderObj->objPrm.mode == G729Encode_VAD_Enabled);

   if(!isVAD) return APIG729_StsNoErr; 

   ippsZero_32f(WeightedLPC1,BWD_LPC_ORDERP1);
   ippsZero_32f(WeightedLPC2,BWD_LPC_ORDERP1);

   anau = &EncodedParams[0];

   newSpeech = encoderObj->OldSpeechBuffer + SPEECH_BUFF_LEN - FRM_LEN;         /* New speech     */
   SpeechWnd     = newSpeech - LOOK_AHEAD_LEN;                    /* Present frame  */
   pWindow   = encoderObj->OldSpeechBuffer + SPEECH_BUFF_LEN - WINDOW_LEN;        /* For LPC window */
   WeightedSpeech    = encoderObj->OldWeightedSpeechBuffer + PITCH_LAG_MAX;
   Excitation    = encoderObj->OldExcitationBuffer + PITCH_LAG_MAX + INTERPOL_LEN;

   if(codecType!=G729A_CODEC){
      pError  = encoderObj->ErrFltMemory + BWD_LPC_ORDER;
      pSynth = encoderObj->SynthBuffer + BWD_SYNTH_MEM;
   }

   if (encoderObj->sFrameCounter == 32767) encoderObj->sFrameCounter = 256;
   else encoderObj->sFrameCounter++;

   ippsConvert_16s32f(src,newSpeech,FRM_LEN);

   ippsIIR_32f_I(newSpeech,FRM_LEN,encoderObj->iirstate);

   ownAutoCorr_G729_32f(pWindow, LPC_ORDERP2, forwardAutoCorr,TmpAlignVec);             /* Autocorrelations */
   ippsCopy_32f(forwardAutoCorr, TmpAutoCorr, LPC_ORDERP1);

   /* Lag windowing    */
   ippsMul_32f_I(lwindow, &forwardAutoCorr[1], LPC_ORDERP2);
   /* Levinson Durbin  */
   tmp_lev = 0;
   sts = ippsLevinsonDurbin_G729_32f(forwardAutoCorr, LPC_ORDER, &forwardLPC[LPC_ORDERP1], forwardReflectCoeff, &tmp_lev);
   if(sts == ippStsOverflow) {
      ippsCopy_32f(encoderObj->OldForwardLPC,&forwardLPC[LPC_ORDERP1],LPC_ORDER+1);
      forwardReflectCoeff[0] = encoderObj->OldForwardRC[0];
      forwardReflectCoeff[1] = encoderObj->OldForwardRC[1];
   } else {
      ippsCopy_32f(&forwardLPC[LPC_ORDERP1],encoderObj->OldForwardLPC,LPC_ORDER+1);
      encoderObj->OldForwardRC[0] = forwardReflectCoeff[0];
      encoderObj->OldForwardRC[1] = forwardReflectCoeff[1];
   }


   /* Convert A(z) to lsp */
   if(codecType==G729A_CODEC){
      ippsLPCToLSP_G729A_32f(&forwardLPC[LPC_ORDERP1], encoderObj->OldLSP, CurrLSP);
   } else {
      ippsLPCToLSP_G729_32f(&forwardLPC[LPC_ORDERP1], encoderObj->OldLSP, CurrLSP);
   }

   if (encoderObj->objPrm.mode == G729Encode_VAD_Enabled) {
      ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);
      VoiceActivityDetect_G729_32f(forwardReflectCoeff[1], CurrLSF, forwardAutoCorr, pWindow, encoderObj->sFrameCounter,
                  encoderObj->prevVADDec, encoderObj->prevPrevVADDec, &VADDecision, &EnergydB,encoderObj->vadMem,TmpAlignVec);

      if(codecType!=G729A_CODEC){
         MusicDetection_G729E_32f( encoderObj, codecType, forwardAutoCorr[0],forwardReflectCoeff, &VADDecision, EnergydB,encoderObj->msdMem,TmpAlignVec);
      }

      UpdateCNG(TmpAutoCorr, VADDecision,encoderObj->cngMem);
   } else VADDecision = 1;

   if(VADDecision == 0) {
      if(codecType==G729A_CODEC){
         UpdateVad_A(encoderObj, Excitation, WeightedSpeech, SpeechWnd, anau);
      } else {
         /* Inactive frame */
         ippsCopy_32f(&encoderObj->SynthBuffer[FRM_LEN], &encoderObj->SynthBuffer[0], BWD_SYNTH_MEM);
         /* Find interpolated LPC parameters in all subframes unquantized.      *
         * The interpolated parameters are in array forwardLPC[] of size (LPC_ORDER+1)*4      */
         if( encoderObj->prevLPCMode == 0) {
            ippsInterpolateC_G729_32f(encoderObj->OldLSP, 0.5f, CurrLSP, 0.5f, tmpLSP, LPC_ORDER);
    
            ippsLSPToLPC_G729_32f(tmpLSP, forwardLPC);
    
            ownACOS_G729_32f(tmpLSP, InterpolatedLSF, LPC_ORDER);
            ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);
         } else {
            /* no interpolation */
            /* unquantized */
            ippsLSPToLPC_G729_32f(CurrLSP, forwardLPC); /* Subframe 1 */
            ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);  /* transformation from LSP to LSF (freq.domain) */
            ippsCopy_32f(CurrLSF, InterpolatedLSF, LPC_ORDER);      /* Subframe 1 */
         } 
         
         if (encoderObj->sGlobalStatInd > 10000) {
            encoderObj->sGlobalStatInd -= 2621;
            if(encoderObj->sGlobalStatInd < 10000) encoderObj->sGlobalStatInd = 10000 ;
         }  
         encoderObj->isBWDDominant = 0;
         encoderObj->fInterpolationCoeff = 1.1f;

         ippsCopy_32f(CurrLSP, encoderObj->OldLSP, LPC_ORDER);

         PWGammaFactor_G729(gamma1, gamma2, InterpolatedLSF, CurrLSF, forwardReflectCoeff,&encoderObj->isSmooth, encoderObj->LogAreaRatioCoeff);
         
         UpdateVad_I(encoderObj, Excitation, forwardLPC, WeightedSpeech, gamma1, gamma2, pSynth,
                     pError, SpeechWnd, anau, codecType);

         /* update previous filter for next frame */
         ippsCopy_32f(&forwardQntLPC[LPC_ORDERP1], encoderObj->PrevFlt, LPC_ORDERP1);
         for(i=LPC_ORDERP1; i <BWD_LPC_ORDERP1; i++) encoderObj->PrevFlt[i] = 0.f;
      }
      encoderObj->prevLPCMode = 0;

      encoderObj->fBetaPreFilter = PITCH_SHARPMIN;

      /* Update memories for next frames */
      ippsCopy_32f(&encoderObj->OldSpeechBuffer[FRM_LEN], &encoderObj->OldSpeechBuffer[0], SPEECH_BUFF_LEN-FRM_LEN);
      ippsCopy_32f(&encoderObj->OldWeightedSpeechBuffer[FRM_LEN], &encoderObj->OldWeightedSpeechBuffer[0], PITCH_LAG_MAX);
      ippsCopy_32f(&encoderObj->OldExcitationBuffer[FRM_LEN], &encoderObj->OldExcitationBuffer[0], PITCH_LAG_MAX+INTERPOL_LEN);

      anau = EncodedParams+1;
      if(EncodedParams[0] == 0){ /* untransmitted*/
         *frametype=0;
      }else{
         *frametype=1; /* SID*/
         dst[0] = (short)anau[0]; /* Switched predictor index of LSF quantizer: 1 LSB */
         dst[1] = (short)anau[1]; /* 1st stage vector of LSF quantizer: 5 LSB*/
         dst[2] = (short)anau[2]; /* 2nd stage vector of LSF quantizer: 4 LSB*/
         dst[3] = (short)anau[3]; /* Gain (Energy): 5 LSB     */
      }

      CLEAR_SCRATCH_MEMORY(encoderObj);

      return APIG729_StsNoErr;
   }

   encoderObj->sCNGSeed = INIT_SEED_VAL;
   encoderObj->prevPrevVADDec = encoderObj->prevVADDec;
   encoderObj->prevVADDec = VADDecision;
   ippsCopy_32f(&encoderObj->OldSpeechBuffer[FRM_LEN], &encoderObj->OldSpeechBuffer[0], SPEECH_BUFF_LEN-FRM_LEN);
   ippsCopy_32f(&encoderObj->OldWeightedSpeechBuffer[FRM_LEN], &encoderObj->OldWeightedSpeechBuffer[0], PITCH_LAG_MAX);
   ippsCopy_32f(&encoderObj->OldExcitationBuffer[FRM_LEN], &encoderObj->OldExcitationBuffer[0], PITCH_LAG_MAX+INTERPOL_LEN);

   CLEAR_SCRATCH_MEMORY(encoderObj);

   return APIG729_StsNoErr;
}

static void UpdateCNG(float *pSrcAutoCorr, int Vad, char *cngMem)
{
   int i;
   CNGmemory *cngState = (CNGmemory *)cngMem;
    
   /* Update AutoCorrs */
   for(i=0; i<(AUTOCORRS_SIZE-LPC_ORDERP1); i++) {
      cngState->AutoCorrs[AUTOCORRS_SIZE - 1 - i] = cngState->AutoCorrs[AUTOCORRS_SIZE - LPC_ORDERP1 - 1 - i];
   } 
    
   /* Save current AutoCorrs */
   ippsCopy_32f(pSrcAutoCorr, cngState->AutoCorrs, LPC_ORDERP1);

   cngState->lAutoCorrsCounter++;
   if(cngState->lAutoCorrsCounter == CURRAUTOCORRS_NUM) {
      cngState->lAutoCorrsCounter = 0;
      if(Vad != 0) {
         for(i=0; i<(SUMAUTOCORRS_SIZE-LPC_ORDERP1); i++) {
            cngState->SumAutoCorrs[SUMAUTOCORRS_SIZE - 1 -i] = cngState->SumAutoCorrs[SUMAUTOCORRS_SIZE - LPC_ORDERP1 - 1 - i];
         }  
         /* Compute new SumAutoCorrs */
         for(i=0; i<LPC_ORDERP1; i++) {
            cngState->SumAutoCorrs[i] = cngState->AutoCorrs[i]+cngState->AutoCorrs[LPC_ORDERP1+i];
         }  
      } 
   } 
   return;
}

static void CNG(G729Encoder_Obj* encoderObj, float *pSrcExc, float *pDstIntLPC, int *pDstParam, G729Codec_Type mode)
{
   LOCAL_ALIGN_ARRAY(32, float, curAcf, LPC_ORDERP1,encoderObj);
   LOCAL_ALIGN_ARRAY(32, float, curCoeff, LPC_ORDERP1,encoderObj);
   LOCAL_ALIGN_ARRAY(32, float, pastCoeff, LPC_ORDERP1,encoderObj);
   LOCAL_ALIGN_ARRAY(32, char, tmpAlignVec, 60*sizeof(float)+6*sizeof(int),encoderObj);
   LOCAL_ARRAY(float, bid, LPC_ORDERP1,encoderObj);
   LOCAL_ARRAY(float, s_sumAcf, LPC_ORDERP1,encoderObj);
   LOCAL_ARRAY(float, CurrLSP, LPC_ORDER,encoderObj);
   LOCAL_ARRAY(float, tmpLSP, LPC_ORDER,encoderObj);
   int i; 
   float *lpcCoeff, *OldQuantLSP, *old_A, *old_rc;
   int currIgain, prevVADDec;
   float energyq;
   IppStatus sts;
   float tmp;

   CNGmemory *cngState = (CNGmemory *)encoderObj->cngMem;
   prevVADDec = encoderObj->prevVADDec;
   OldQuantLSP = encoderObj->OldQuantLSP;
   old_A = encoderObj->OldForwardLPC;
   old_rc = encoderObj->OldForwardRC;
    
   /* Update Ener */
   for(i = GAINS_NUM-1; i>=1; i--) {
      cngState->Energies[i] = cngState->Energies[i-1];
   } 
    
   /* Compute current Acfs */
   for(i=0; i<LPC_ORDERP1; i++) {
      curAcf[i] = cngState->AutoCorrs[i]+cngState->AutoCorrs[LPC_ORDERP1+i];
   } 
    
   /* Compute LPC coefficients and residual energy */
   if(curAcf[0] == 0.f) {
      cngState->Energies[0] = 0.f;                /* should not happen */
   } else {
      sts = ippsLevinsonDurbin_G729_32f(curAcf, LPC_ORDER, curCoeff, bid, &cngState->Energies[0]);
      if(sts == ippStsOverflow) {
         ippsCopy_32f(old_A,curCoeff,LPC_ORDER+1);
         bid[0] = old_rc[0];
         bid[1] = old_rc[1];
      } else {
         ippsCopy_32f(curCoeff,old_A,LPC_ORDER+1);
         old_rc[0] = bid[0];
         old_rc[1] = bid[1];
      } 
   } 

   /* if first frame of silence => SID frame */
   if(prevVADDec != 0) {
      pDstParam[0] = 1;
      cngState->lFrameCounter0 = 0;
      cngState->lNumSavedEnergies = 1;
      QuantSIDGain_G729B(cngState->Energies, cngState->lNumSavedEnergies, &energyq, &currIgain);
   } else {
      cngState->lNumSavedEnergies++;
      CLIP_TO_UPLEVEL(cngState->lNumSavedEnergies,GAINS_NUM);
      QuantSIDGain_G729B(cngState->Energies, cngState->lNumSavedEnergies, &energyq, &currIgain);

      /* Compute stationarity of current filter versus reference filter.*/
      /* Compute Itakura distance and compare to threshold */
      ippsDotProd_32f(cngState->ReflectCoeffs,curAcf,LPC_ORDER+1,&tmp);
      if(tmp > (cngState->Energies[0] * ITAKURATHRESH1)) cngState->lFltChangeFlag = 1;
        
      /* compare energy difference between current frame and last frame */
      if( (float)fabs(cngState->fPrevEnergy - energyq) > 2.0f) cngState->lFltChangeFlag = 1;
        
      cngState->lFrameCounter0++;
      if(cngState->lFrameCounter0 < N_MIN_SIM_RRAMES) {
         pDstParam[0] = 0; /* no transmission */
      } else {
         if(cngState->lFltChangeFlag != 0) {
            pDstParam[0] = 1;             /* transmit SID frame */
         } else {
            pDstParam[0] = 0;
         }
         cngState->lFrameCounter0 = N_MIN_SIM_RRAMES;
      } 
   } 

   if(pDstParam[0] == 1) {
      /* Reset frame count and lFltChangeFlag */
      cngState->lFrameCounter0 = 0;
      cngState->lFltChangeFlag = 0;
        
      /* Compute past average filter */
      s_sumAcf[0] = cngState->SumAutoCorrs[0]+cngState->SumAutoCorrs[LPC_ORDERP1]+cngState->SumAutoCorrs[2*LPC_ORDERP1];
    
      if(s_sumAcf[0] == 0.f) {
         ippsZero_32f(pastCoeff,LPC_ORDERP1);
         pastCoeff[0] = 1.f;
      } else {
         for(i=1; i<LPC_ORDERP1; i++) {
            s_sumAcf[i] = cngState->SumAutoCorrs[i]+cngState->SumAutoCorrs[LPC_ORDERP1+i]+cngState->SumAutoCorrs[2*LPC_ORDERP1+i];
         }   
         sts = ippsLevinsonDurbin_G729_32f(s_sumAcf, LPC_ORDER, pastCoeff, bid, &tmp);
         if(sts == ippStsOverflow) {
            ippsCopy_32f(old_A,pastCoeff,LPC_ORDER+1);
            bid[0] = old_rc[0];
            bid[1] = old_rc[1];
         } else {
            ippsCopy_32f(pastCoeff,old_A,LPC_ORDER+1);
            old_rc[0] = bid[0];
            old_rc[1] = bid[1];
         }  
      } 

      /* Compute autocorrelation of LPC coefficients used for Itakura distance */
      ippsCrossCorr_32f(pastCoeff, LPC_ORDER+1, pastCoeff, LPC_ORDER+1, cngState->ReflectCoeffs,  LPC_ORDER+1, 0);
      cngState->ReflectCoeffs[0] = cngState->ReflectCoeffs[0]/2;
        
      /* Compute stationarity of current filter versus past average filter.*/
        
      /* if stationary transmit average filter. */
      /* Compute Itakura distance and compare to threshold */
      ippsDotProd_32f(cngState->ReflectCoeffs,curAcf,LPC_ORDER+1,&tmp);
      if(tmp <= (cngState->Energies[0] * ITAKURATHRESH2)) {
         /* transmit old filter*/
         lpcCoeff = pastCoeff;  
      } else {
         /* transmit current filter => new ref. filter */
         lpcCoeff = curCoeff;
         /* Compute autocorrelation of LPC coefficients used for Itakura distance */
         ippsCrossCorr_32f(curCoeff, LPC_ORDER+1, curCoeff, LPC_ORDER+1, cngState->ReflectCoeffs,  LPC_ORDER+1, 0);
         cngState->ReflectCoeffs[0] = cngState->ReflectCoeffs[0]/2;
      } 
        
      /* Compute SID frame codes */
      if(mode==G729A_CODEC)
         ippsLPCToLSP_G729A_32f(lpcCoeff, OldQuantLSP, CurrLSP); /* From A(z) to lsp */
      else 
         ippsLPCToLSP_G729_32f(lpcCoeff, OldQuantLSP, CurrLSP); /* From A(z) to lsp */

      /* LSP quantization */

      {
         LOCAL_ARRAY(float, fTmpLSF, LPC_ORDER,encoderObj);

         /* convert lsp to lsf */
         ownACOS_G729_32f(CurrLSP, fTmpLSF, LPC_ORDER);
          
         /* spacing to ~100Hz */
         if (fTmpLSF[0] < LSF_LOW_LIMIT)
            fTmpLSF[0] = LSF_LOW_LIMIT;
         for (i=0 ; i < LPC_ORDER-1 ; i++) {
            if (fTmpLSF[i+1]- fTmpLSF[i] < 2*LSF_DIST) fTmpLSF[i+1] = fTmpLSF[i]+ 2*LSF_DIST;
         } 
         if (fTmpLSF[LPC_ORDER-1] > LSF_HI_LIMIT)
            fTmpLSF[LPC_ORDER-1] = LSF_HI_LIMIT;
         if (fTmpLSF[LPC_ORDER-1] < fTmpLSF[LPC_ORDER-2]) 
            fTmpLSF[LPC_ORDER-2] = fTmpLSF[LPC_ORDER-1]- LSF_DIST;
          

         ippsLSFQuant_G729B_32f(fTmpLSF, (float*)encoderObj->PrevFreq, cngState->SIDQuantLSP, &pDstParam[1]);

         LOCAL_ARRAY_FREE(float, fTmpLSF, LPC_ORDER,encoderObj);
      }

        
      cngState->fPrevEnergy = energyq;
      pDstParam[4] = currIgain;
      cngState->fSIDGain = SIDGainTbl[currIgain];    
   }
    
   /* Compute new excitation */
   if(prevVADDec != 0) {
      cngState->fCurrGain = cngState->fSIDGain;
   } else {
      cngState->fCurrGain *= GAIN_INT_FACTOR;
      cngState->fCurrGain += INV_GAIN_INT_FACTOR * cngState->fSIDGain;
   } 
    
   if(cngState->fCurrGain == 0.f) {
      ippsZero_32f(pSrcExc,FRM_LEN);
      UpdateExcErr_G729(0.f, SUBFR_LEN+1,encoderObj->ExcitationError);
      UpdateExcErr_G729(0.f, SUBFR_LEN+1,encoderObj->ExcitationError);
   } else {
      ComfortNoiseExcitation_G729(cngState->fCurrGain, pSrcExc, &encoderObj->sCNGSeed, ENCODER, encoderObj->ExcitationError,NULL,tmpAlignVec);
   }
    
   ippsInterpolateC_G729_32f(OldQuantLSP, 0.5f, cngState->SIDQuantLSP, 0.5f, tmpLSP, LPC_ORDER);
    
   ippsLSPToLPC_G729_32f(tmpLSP, pDstIntLPC);
   ippsLSPToLPC_G729_32f(cngState->SIDQuantLSP, &pDstIntLPC[LPC_ORDER+1]);

   ippsCopy_32f(cngState->SIDQuantLSP, OldQuantLSP, LPC_ORDER);
    
   /* Update SumAutoCorrs if lAutoCorrsCounter = 0 */
   if(cngState->lAutoCorrsCounter == 0) {
      for(i=0; i<(SUMAUTOCORRS_SIZE-LPC_ORDERP1); i++) {
         cngState->SumAutoCorrs[SUMAUTOCORRS_SIZE - 1 -i] = cngState->SumAutoCorrs[SUMAUTOCORRS_SIZE - LPC_ORDERP1 - 1 - i];
      } 
    
      /* Compute new SumAutoCorrs */
      for(i=0; i<LPC_ORDERP1; i++) {
         cngState->SumAutoCorrs[i] = cngState->AutoCorrs[i]+cngState->AutoCorrs[LPC_ORDERP1+i];
      } 
   } 
   LOCAL_ARRAY_FREE(float, tmpLSP, LPC_ORDER,encoderObj);
   LOCAL_ARRAY_FREE(float, CurrLSP, LPC_ORDER,encoderObj);
   LOCAL_ARRAY_FREE(float, s_sumAcf, LPC_ORDERP1,encoderObj);
   LOCAL_ARRAY_FREE(float, bid, LPC_ORDERP1,encoderObj);
   LOCAL_ALIGN_ARRAY_FREE(32, float, pastCoeff, LPC_ORDERP1,encoderObj);
   LOCAL_ALIGN_ARRAY_FREE(32, float, curCoeff, LPC_ORDERP1,encoderObj);
   LOCAL_ALIGN_ARRAY_FREE(32, float, curAcf, LPC_ORDERP1,encoderObj);
   return;
}
static void UpdateVad_I(G729Encoder_Obj* encoderObj, float *Excitation, float *forwardLPC, float *WeightedSpeech,
                 float *gamma1, float *gamma2, float *pSynth,
                 float *pError, float *SpeechWnd, int* dst,G729Codec_Type codecType)
{
   LOCAL_ALIGN_ARRAY(32, float, WeightedLPC1, BWD_LPC_ORDERP1,encoderObj);    /* A(z) with spectral expansion         */
   LOCAL_ALIGN_ARRAY(32, float, WeightedLPC2, BWD_LPC_ORDERP1,encoderObj);    /* A(z) with spectral expansion         */
   LOCAL_ALIGN_ARRAY(32, float, TargetVector, SUBFR_LEN,encoderObj);
   LOCAL_ALIGN_ARRAY(32, float, forwardQntLPC, LPC_ORDERP1*2,encoderObj); /* A(z) forward quantized for the 2 subframes */
   int i, j, NGamma, NSbfr;

   CNG(encoderObj,Excitation, forwardQntLPC, dst,codecType);

   encoderObj->prevPrevVADDec = encoderObj->prevVADDec;
   encoderObj->prevVADDec = 0;

   /* Update filters memory*/
   for(NSbfr=0, NGamma = 0; NSbfr < FRM_LEN; NSbfr += SUBFR_LEN, NGamma++) {
      WeightLPCCoeff_G729(forwardLPC, gamma1[NGamma], LPC_ORDER, WeightedLPC1);
      WeightLPCCoeff_G729(forwardLPC, gamma2[NGamma], LPC_ORDER, WeightedLPC2);

      ippsConvBiased_32f(WeightedLPC1,LPC_ORDER+1,&SpeechWnd[NSbfr],SUBFR_LEN+LPC_ORDER,&WeightedSpeech[NSbfr],SUBFR_LEN,LPC_ORDER);
      ippsSynthesisFilter_G729_32f(WeightedLPC2, LPC_ORDER, &WeightedSpeech[NSbfr], &WeightedSpeech[NSbfr], SUBFR_LEN, &encoderObj->FltMem[BWD_LPC_ORDER-LPC_ORDER]);
      for(j=0; j<BWD_LPC_ORDER; j++) encoderObj->FltMem[j] = WeightedSpeech[NSbfr+SUBFR_LEN-BWD_LPC_ORDER+j];

      /* update synthesis filter's memory*/
      ippsSynthesisFilter_G729_32f(forwardQntLPC, LPC_ORDER, &Excitation[NSbfr], &pSynth[NSbfr], SUBFR_LEN, &encoderObj->SynFltMemory[BWD_LPC_ORDER-LPC_ORDER]);
      for(j=0; j<BWD_LPC_ORDER; j++) encoderObj->SynFltMemory[j] = pSynth[NSbfr+SUBFR_LEN-BWD_LPC_ORDER+j];

      /* update WeightedFilterMemory */
      ippsSub_32f(&pSynth[NSbfr],&SpeechWnd[NSbfr],pError,SUBFR_LEN);
      ippsConvBiased_32f(WeightedLPC1,LPC_ORDER+1,pError,SUBFR_LEN+LPC_ORDER,TargetVector,SUBFR_LEN,LPC_ORDER);
      ippsSynthesisFilter_G729_32f(WeightedLPC2, LPC_ORDER, TargetVector, TargetVector, SUBFR_LEN, &encoderObj->WeightedFilterMemory[BWD_LPC_ORDER-LPC_ORDER]);
      for(j=0; j<BWD_LPC_ORDER; j++) encoderObj->WeightedFilterMemory[j] = TargetVector[SUBFR_LEN-BWD_LPC_ORDER+j];

      /* update ErrFltMemory */
      for (i = SUBFR_LEN-BWD_LPC_ORDER, j = 0; i < SUBFR_LEN; i++, j++)
         encoderObj->ErrFltMemory[j] = pError[i];

      for (i= 0; i< 4; i++)
         encoderObj->PitchGainBuffer[i] = encoderObj->PitchGainBuffer[i+1];
      encoderObj->PitchGainBuffer[4] =  0.5f;

      forwardLPC += LPC_ORDERP1;
      forwardQntLPC += LPC_ORDERP1;
   }

   LOCAL_ALIGN_ARRAY_FREE(32, float, forwardQntLPC, LPC_ORDERP1*2,encoderObj); /* A(z) forward quantized for the 2 subframes */
   LOCAL_ALIGN_ARRAY_FREE(32, float, TargetVector, SUBFR_LEN,encoderObj);
   LOCAL_ALIGN_ARRAY_FREE(32, float, WeightedLPC2, BWD_LPC_ORDERP1,encoderObj);    /* A(z) with spectral expansion         */
   LOCAL_ALIGN_ARRAY_FREE(32, float, WeightedLPC1, BWD_LPC_ORDERP1,encoderObj);    /* A(z) with spectral expansion         */
   return;
}

static void UpdateVad_A(G729Encoder_Obj* encoderObj, float *Excitation, 
                        float *WeightedSpeech, float *SpeechWnd, int* dst)
{
   LOCAL_ALIGN_ARRAY(32, float, forwardLPC, LPC_ORDERP1*2,encoderObj);    /* A(z) forward unquantized for the 2 subframes */
   LOCAL_ALIGN_ARRAY(32, float, forwardQntLPC, LPC_ORDERP1*2,encoderObj);    /* A(z) forward quantized for the 2 subframes */
   LOCAL_ALIGN_ARRAY(32, float, TargetVector, SUBFR_LEN,encoderObj);
   int i, NSbfr;
   float *pQLPC, *pUnQLPC;

   CNG(encoderObj,Excitation, forwardQntLPC, dst,G729A_CODEC);

   encoderObj->prevPrevVADDec = encoderObj->prevVADDec;
   encoderObj->prevVADDec = 0;
   /* Update WeightedSpeech, FltMem and WeightedFilterMemory */
   pQLPC = forwardQntLPC;
   for(NSbfr=0; NSbfr < FRM_LEN; NSbfr += SUBFR_LEN) {
        
      /* Residual signal in TargetVector */
      ippsConvBiased_32f(pQLPC,LPC_ORDER+1,&SpeechWnd[NSbfr],SUBFR_LEN+LPC_ORDER,TargetVector,SUBFR_LEN,LPC_ORDER);
        
      WeightLPCCoeff_G729(pQLPC, GAMMA1_G729A, LPC_ORDER, forwardLPC);
        
      /* Compute WeightedSpeech and FltMem */
      pUnQLPC = forwardLPC + LPC_ORDERP1;
      pUnQLPC[0] = 1.0f;
      for(i=1; i<=LPC_ORDER; i++)
         pUnQLPC[i] = forwardLPC[i] - 0.7f * forwardLPC[i-1];
      ippsSynthesisFilter_G729_32f(pUnQLPC, LPC_ORDER, TargetVector, &WeightedSpeech[NSbfr], SUBFR_LEN, encoderObj->FltMem);
      for (i = 0; i < LPC_ORDER; i++)  CLIP_DENORMAL(WeightedSpeech[(SUBFR_LEN-LPC_ORDER)+i],encoderObj->FltMem[i]);
        
      /* Compute WeightedFilterMemory */
      ippsSub_32f_I(&Excitation[NSbfr],TargetVector,SUBFR_LEN);
         
      ippsSynthesisFilter_G729_32f(forwardLPC, LPC_ORDER, TargetVector, TargetVector, SUBFR_LEN, encoderObj->WeightedFilterMemory);
      for (i = 0; i < LPC_ORDER; i++)  CLIP_DENORMAL(TargetVector[i],encoderObj->FltMem[i]);
                
      pQLPC += LPC_ORDERP1;
   }

   LOCAL_ALIGN_ARRAY_FREE(32, float, TargetVector, SUBFR_LEN,encoderObj);
   LOCAL_ALIGN_ARRAY_FREE(32, float, forwardQntLPC, LPC_ORDERP1*2,encoderObj);    /* A(z) forward quantized for the 2 subframes */
   LOCAL_ALIGN_ARRAY_FREE(32, float, forwardLPC, LPC_ORDERP1*2,encoderObj);    /* A(z) forward unquantized for the 2 subframes */

   return;
}
