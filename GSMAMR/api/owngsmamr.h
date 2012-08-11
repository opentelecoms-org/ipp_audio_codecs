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
//  GSMAMR is a international standard promoted by IETF and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: GSMAMR speech codec internal header file
//
*/

#ifndef __OWNGSMAMR_H__
#define __OWNGSMAMR_H__
#include <stdio.h>
#include <stdlib.h>
#include "gsmamr.h"
#include "gsmamrapi.h"

#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif

#include "scratchmem.h"

#define ENC_KEY 0xecdaaa
#define DEC_KEY 0xdecaaa

/* GSMAMR constant definitions. 
   Constant names are kept same as IETF GSM AMR */
#define SPEECH_BUF_SIZE          320       
#define LP_WINDOW_SIZE           240       
#define FRAME_SIZE_GSMAMR        160       
#define SUBFR_SIZE_GSMAMR         40        
#define LP_ORDER_SIZE             10        
#define LP1_ORDER_SIZE     (LP_ORDER_SIZE+1)     
#define LSF_GAP                  205       
#define LP_ALL_FRAME       (4*LP1_ORDER_SIZE)  
#define PITCH_MIN_LAG             20       
#define PITCH_MAX_LAG            143      
#define FLT_INTER_SIZE        (10+1)  
#define PITCH_SHARP_MAX        13017        
#define PITCH_SHARP_MIN            0            
                                                                          
#define MAX_OFFSET   (PITCH_MAX_LAG +LP1_ORDER_SIZE)                                                                                                                                                        
#define MAX_NUM_PRM               57                                                                               
#define PITCH_GAIN_CLIP        15565        /* Pitch gain clipping = 0.95               */
#define PG_NUM_FRAME               7            
#define NUM_MEM_LTPG               5 
#define NUM_SUBBANDS_VAD           9        
#define DTX_HIST_SIZE              8
#define ENERGY_HIST_SIZE          60
#define CBGAIN_HIST_SIZE           7
#define NUM_PRED_TAPS              4  
#define MIN_ENERGY            -14336         /* -14 dB  */
#define MIN_ENERGY_M122        -2381          /* -14 dB / (20*log10(2)) */ 
#define PSEUDO_NOISE_SEED 0x70816958L   
#define INIT_BACKGROUND_NOISE    150        
#define INIT_LOWPOW_SEGMENT    13106      

/* Constants for pitch detection */
#define LOW_THRESHOLD              4
#define NUM_THRESHOLD              4
#define MAX_MED_SIZE               9  
#define LTP_GAIN_LOG10_1        2721 /* 1.0 / (10*log10(2)) */
#define LTP_GAIN_LOG10_2        5443 /* 2.0 / (10*log10(2)) */
#define MAX_UPSAMPLING             6
#define LEN_INTERPOL_10    (FLT_INTER_SIZE-1)
#define LEN_FIRFLT     (MAX_UPSAMPLING*LEN_INTERPOL_10+1)
#define EXP_CONST_016           5243               /* 0.16 */
#define EXP_CONST_084          27525               /* 0.84 */
#define LTP_GAIN_MEM_SIZE          5
#define LTP_THRESH1             9830                /* 0.6  */
#define LTP_THRESH2            14746               /* 0.9  */
#define THRESH_ENERGY_LIMIT    17578         /* 150 */
#define LOW_NOISE_LIMIT           20            /* 5 */
#define UPP_NOISE_LIMIT         1953            /* 50 */
#define LEN_QNT_GAIN              32
#define LEN_QNT_PITCH             16
#define VEC_QNT_M475_SIZE        256
#define VEC_QNT_HIGHRATES_SIZE   128
#define VEC_QNT_LOWRATES_SIZE     64
#define ALPHA_09               29491                  /* 0.9 */
#define ONE_ALPHA               3277                      /* 1.0-ALPHA */
enum enDTXStateType {SPEECH = 0, DTX, DTX_MUTE, DTX_NODATA};
#define DTX_MAX_EMPTY_THRESH      50
#define DTX_ELAPSED_FRAMES_THRESH (24 + 7 -1)
#define DTX_HANG_PERIOD            7             

#define dVADState void
typedef struct {
   short vSinceLastSid;
   short vDTXHangCount;
   short vExpireCount;
   short vDataUpdated;      

} sDecoderSidSync;

/* post filter memory */
typedef struct{
   short a_MemSynPst[LP_ORDER_SIZE]; 
   short vMemPrevRes;        
   short vPastGainScale;    
   short a_SynthBuf[LP_ORDER_SIZE]; 
} sPostFilterSt;

typedef struct {
  short a_GainMemory[LTP_GAIN_MEM_SIZE];
  short vPrevState;
  short vPrevGain;
  short vFlagLockGain;
  short vOnSetGain;
} sPhaseDispSt;

typedef struct {
    short vExpPredCBGain;
    short vFracPredCBGain;
    short vExpTargetEnergy;
    short vFracTargetEnergy;
    short a_ExpEnCoeff[5];
    short a_FracEnCoeff[5];
    short *pGainPtr;
    short a_PastQntEnergy[4];         
    short a_PastQntEnergy_M122[4];   
    short a_PastUnQntEnergy[4];     
    short a_PastUnQntEnergy_M122[4];                 
    short vOnSetQntGain;                   
    short vPrevAdaptOut;              
    short vPrevGainCode;                 
    short a_LTPHistoryGain[NUM_MEM_LTPG]; 

} sGainQuantSt;

typedef struct {
   short a_LSPHistory[LP_ORDER_SIZE * DTX_HIST_SIZE];
   short a_LogEnergyHistory[DTX_HIST_SIZE];
   short vHistoryPtr;
   short vLogEnergyIndex;
   short vLSFQntIndex;
   short a_LSPIndex[3];
   /* DTX handler */
   short vDTXHangoverCt;
   short vDecExpireCt;
} sDTXEncoderSt;

typedef struct {
   short vLastSidFrame;
   short vSidPeriodInv;
   short vLogEnergy;
   short vLogEnergyOld;
   int   vPerfSeedDTX_long; 
   short a_LSP[LP_ORDER_SIZE];
   short a_LSP_Old[LP_ORDER_SIZE]; 
   short a_LSFHistory[LP_ORDER_SIZE*DTX_HIST_SIZE];
   short a_LSFHistoryMean[LP_ORDER_SIZE*DTX_HIST_SIZE]; 
   short vLogMean;
   short a_LogEnergyHistory[DTX_HIST_SIZE];
   short vLogEnergyHistory;
   short vLogEnergyCorrect;
   short vDTXHangoverCt;
   short vDecExpireCt;
   short vFlagSidFrame;       
   short vFlagValidData;          
   short vDTXHangAdd;
   short vSidUpdateCt;
   enum  enDTXStateType eDTXPrevState;                                           
   short vFlagDataUpdate;     

   short vSinceLastSid;
   short vDTXHangCount;
   short vExpireCount;
   short vDataUpdated;      

} sDTXDecoderSt;   


/* Decoder part */
#if defined(__ICL ) || defined ( __ECL )
/*  Intel C/C++ compiler bug for __declspec(align(8)) !!! */
  #define __ALIGN(n) __declspec(align(16))
  #define __ALIGN32 __declspec(align(32))
#else
  #define __ALIGN(n)
  #define __ALIGN32
#endif

/********************************************************
*            array & table declarations
*********************************************************/
#define N_MODES     9

extern const short TableHammingWindow[LP_WINDOW_SIZE];
extern const short TablePastLSFQnt[80];
extern const short TableMeanLSF_3[10];
extern short TableDecCode1LSF_3[256*3];
extern short TableDecCode2LSF_3[512*3];
extern short TableDecCode3LSF_3[512*4];
extern const short TableMeanLSF_5[10];
extern const short TableQuantGainPitch[16];
extern short TableLSPInitData[LP_ORDER_SIZE]; /* removed const for BE code */
extern const short TableParamPerModes[N_MODES];
extern const short *TableBitAllModes[N_MODES];
extern const IppSpchBitRate mode2rates[N_MODES];

/********************************************************
*               function declarations
*********************************************************/

void  ownVADPitchDetection_GSMAMR(IppGSMAMRVad1State *st, short *pTimeVec, short *vLagCountOld, short *vLagOld);

void  ownUpdateLTPFlag_GSMAMR(IppSpchBitRate rate, int L_Rmax, int L_R0, Ipp16s *vFlagLTP);

void  ownLog2_GSMAMR_norm (int inVal, short exp, short *expPart, short *fracPart);

void  ownLog2_GSMAMR(int inVal, short *expPart, short *fracPart);

int   ownPow2_GSMAMR(short expPart, short fracPart);

int   ownSqrt_Exp_GSMAMR(int inVal, short *exp);

void  ownReorderLSFVec_GSMAMR(short *lsf, short minDistance, short len);

short ownGetMedianElements_GSMAMR(short *pPastGainVal, short num);

short ownCtrlDetectBackgroundNoise_GSMAMR (short *excitation, short excEnergy, short *exEnergyHist,
                                           short vVoiceHangover, short prevBFI, short carefulFlag);

short ownComputeCodebookGain_GSMAMR(short *pTargetVec2, short *pFltVector);

void  ownGainAdaptAlpha_GSMAMR(short *vOnSetQntGain, short *vPrevAdaptOut, short *vPrevGainCode,
                    short *a_LTPHistoryGain, short ltpg, short gainCode, short *alpha);

short ownCBGainAverage_GSMAMR(short *a_GainHistory, short *vHgAverageVar, short *vHgAverageCount, IppSpchBitRate rate,
                              short gainCode, short *lsp, short *lspAver, short badFrame, short vPrevBadFr,
                              short pdfi, short vPrevDegBadFr, short vBackgroundNoise, short vVoiceHangover);

short ownCheckLSPVec_GSMAMR(short *count, short *lsp);

int   ownSubframePostProc_GSMAMR(short *pSpeechVec, IppSpchBitRate rate, short numSubfr,
      short gainPitch, short gainCode, short *pAQnt, short *pLocSynth, short *pTargetVec,
      short *code, short *pAdaptCode, short *pFltVector, short *a_MemorySyn, short *a_MemoryErr,
      short *a_Memory_W0, short *pLTPExc, short *vFlagSharp);

void  ownPredExcMode3_6_GSMAMR (short *pLTPExc, short T0, short frac, short lenSubfr, short flag3);

void  ownPhaseDispersion_GSMAMR (sPhaseDispSt *state, IppSpchBitRate rate, short *x, short cbGain,
                                short ltpGain, short *innovVec, short pitchFactor, short tmpShift);


short ownSourceChDetectorUpdate_GSMAMR (short *a_EnergyHistVector, short *vCountHangover, short *ltpGainHist, short *pSpeechVec, short *vVoiceHangover);

void  ownCalcUnFiltEnergy_GSMAMR(short *pLPResVec, short *exc, short *code, short gainPitch, short lenSubfr, short *fracEnergyCoeff,
                          short *expEnergyCoeff, short *ltpg);

void  ownCalcFiltEnergy_GSMAMR(IppSpchBitRate rate, short *pTargetVec, short *pTargetVec2, short *pAdaptCode, short *pFltVector, short *fracEnergyCoeff,
                        short *expEnergyCoeff, short *optFracCodeGain, short *optExpCodeGain);
void  ownCalcTargetEnergy_GSMAMR(short *pTargetVec, short *optExpCodeGain, short *optFracCodeGain);

void  ownConvertDirectCoeffToReflCoeff_GSMAMR(short *pDirectCoeff, short *pReflCoeff);

void  ownScaleExcitation_GSMAMR(short *pInputSignal, short *pOutputSignal);

void  ownPredEnergyMA_GSMAMR(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, IppSpchBitRate rate, short *code,
        short *expGainCodeCB, short *fracGainCodeCB, short *fracEnergyCoeff, short *frac_en);

short ownQntGainCodebook_GSMAMR(IppSpchBitRate rate, short expGainCodeCB, short fracGainCodeCB, short *gain,
                                short *pQntEnergyErr_M122, short *pQntEnergyErr);
short ownQntGainPitch_M7950_GSMAMR(short gainPitchLimit, short *gain, short *pGainCand, short *pGainCind);

short ownQntGainPitch_M122_GSMAMR(short gainPitchLimit, short gain);

void  ownUpdateUnQntPred_M475(short *a_PastQntEnergy, short *a_PastQntEnergy_M122,
      short expGainCodeCB, short fracGainCodeCB, short optExpCodeGain, short optFracCodeGain);

short ownGainQnt_M475(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, short vExpPredCBGain,
      short vFracPredCBGain, short *a_ExpEnCoeff, short *a_FracEnCoeff, short vExpTargetEnergy,
      short vFracTargetEnergy, short *codeNoSharpSF1, short expGainCodeSF1, short fracGainCodeSF1,
      short *expCoeffSF1, short *fracCoeffSF1, short expTargetEnergySF1, short fracTargetEnergySF1,
      short gainPitchLimit, short *gainPitSF0, short *gainCodeSF0, short *gainPitSF1, short *gainCodeSF1);

void  ownGainQuant_M795_GSMAMR(short *vOnSetQntGain, short *vPrevAdaptOut, short *vPrevGainCode, short *a_LTPHistoryGain, short *pLTPRes,
      short *pLTPExc, short *code, short *fracEnergyCoeff, short *expEnergyCoeff, short expCodeEnergy, short fracCodeEnergy,
      short expGainCodeCB, short fracGainCodeCB, short lenSubfr, short optFracCodeGain, short optExpCodeGain, short gainPitchLimit,
      short *gainPitch, short *gainCode, short *pQntEnergyErr_M122, short *pQntEnergyErr, short **ppAnalysisParam);

short ownGainQntInward_GSMAMR(IppSpchBitRate rate, short expGainCodeCB, short fracGainCodeCB, short *fracEnergyCoeff, short *expEnergyCoeff,
      short gainPitchLimit, short *gainPitch, short *gainCode, short *pQntEnergyErr_M122, short *pQntEnergyErr);

int   ownGainQuant_GSMAMR(sGainQuantSt *st, IppSpchBitRate rate, short *pLTPRes, short *pLTPExc, short *code, short *pTargetVec,
      short *pTargetVec2, short *pAdaptCode, short *pFltVector, short even_subframe, short gainPitchLimit, short *gainPitSF0,
      short *gainCodeSF0, short *gainPitch, short *gainCode, short **ppAnalysisParam);

void  ownDecodeFixedCodebookGain_GSMAMR(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, IppSpchBitRate rate, 
									   short index, short *code, short *gainCode);

void  ownDecodeCodebookGains_GSMAMR(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, IppSpchBitRate rate, short index,
                                   short *code, short evenSubfr, short * gainPitch, short * gain_cod);

void  ownConcealCodebookGain_GSMAMR(short *a_GainBuffer, short vPastGainCode, short *a_PastQntEnergy, 
								   short *a_PastQntEnergy_M122, short state, short *gainCode);

void  ownConcealCodebookGainUpdate_GSMAMR(short *a_GainBuffer, short *vPastGainCode, short *vPrevGainCode,   /* i   : flag: frame is bad               */
										 short badFrame, short vPrevBadFr, short *gainCode);

void  ownConcealGainPitch_GSMAMR(short *a_LSFBuffer, short vPastGainZero, short state, short *pGainPitch);

void  ownConcealGainPitchUpdate_GSMAMR(short *a_LSFBuffer, short *vPastGainZero, short *vPrevGainZero,      /* i   : flag: frame is bad                */
									  short badFrame, short vPrevBadFr, short *gain_pitch);

int   ownCloseLoopFracPitchSearch_GSMAMR(short *vTimePrevSubframe, short *a_GainHistory, IppSpchBitRate rate, short frameOffset,
      short *pLoopPitchLags, short *pImpResVec, short *pLTPExc, short *pPredRes, short *pTargetVec, short lspFlag, short *pTargetVec2,
      short *pAdaptCode, short *pExpPitchDel, short *pFracPitchDel, short *gainPitch, short **ppAnalysisParam, short *gainPitchLimit);

short ownGenNoise_GSMAMR(int *pShiftReg, short numBits);

void  ownBuildCNCode_GSMAMR(int *seed, short *pCNVec);

void  ownBuildCNParam_GSMAMR(short *seed, const short numParam, const short *pTableSizeParam, short *pCNParam);

void  ownDecLSPQuantDTX_GSMAMR(short *a_PastQntPredErr, short *a_PastLSFQnt, short BadFrInd, short *indice, short *lsp1_q);

int   ownDTXDecoder_GSMAMR(sDTXDecoderSt *st, short *a_MemorySyn, short *a_PastQntPredErr, short *a_PastLSFQnt,
      short *a_PastQntEnergy, short *a_PastQntEnergy_M122, short *vHgAverageVar, short *vHgAverageCount,
      enum enDTXStateType newState, GSMAMR_Rate_t rate, short *pParmVec, short *pSynthSpeechVec,
      short *pA_LP);

enum  enDTXStateType ownRX_DTX_Handler_GSMAMR(GSMAMRDecoder_Obj* decoderObj, RXFrameType frame_type);

int   ownDecSidSyncReset_GSMAMR(sDTXDecoderSt *st);
enum  enDTXStateType ownDecSidSync(sDTXDecoderSt *st, RXFrameType frame_type );

void  ownPrm2Bits_GSMAMR(const short* prm, unsigned char *Vout, GSMAMR_Rate_t rate );

void  ownBits2Prm_GSMAMR( const unsigned char *bitstream, short* prm , GSMAMR_Rate_t rate );

int   ownEncDetectSize_GSMAMR(int mode, int* pEncSize);

int   ownEncoderInit_GSMAMR(GSMAMREncoder_Obj* st);

int   ownDtxEncoderInit_GSMAMR(sDTXEncoderSt* st);

int   ownGainQuantInit_GSMAMR(sGainQuantSt *state);

int   ownVAD1Init_GSMAMR(IppGSMAMRVad1State *state);

int   ownVAD2Init_GSMAMR(IppGSMAMRVad2State *state);
 

/********************************************************
*               structs declarations
*********************************************************/

typedef struct _GSMAMRCoder_Obj{
   int                 objSize;
   int                 key;
   int                 mode;          /* encoder mode's */
   GSMAMRCodec_Type       codecType;
}GSMAMRCoder_Obj;

typedef struct {
   short a_SpeechVecOld[SPEECH_BUF_SIZE];
   short *pSpeechPtr, *pWindowPtr, *pWindowPtr_M122;
   short *pSpeechPtrNew;             
   short a_WeightSpeechVecOld[FRAME_SIZE_GSMAMR + PITCH_MAX_LAG];
   short *pWeightSpeechVec;
   short a_LTPStateOld[5];
   short a_GainFlg[2];
   short a_ExcVecOld[FRAME_SIZE_GSMAMR + PITCH_MAX_LAG + FLT_INTER_SIZE];
   short *pExcVec;
   short a_ZeroVec[SUBFR_SIZE_GSMAMR + LP1_ORDER_SIZE];
   short *pZeroVec;
   short *pImpResVec;
   short a_ImpResVec[SUBFR_SIZE_GSMAMR];
   short a_SubState[LP_ORDER_SIZE + 1];
   short a_LSP_Old[LP_ORDER_SIZE];
   short a_LSPQnt_Old[LP_ORDER_SIZE];
   short a_PastQntPredErr[LP_ORDER_SIZE];    
   short vTimePrevSubframe;
   sGainQuantSt stGainQntSt;
   short vTimeMedOld;
   short vFlagVADState;
   short vCount;
   short vFlagTone; 
   short a_GainHistory[PG_NUM_FRAME];
   dVADState *pVADSt;
   int vFlagDTX;
   sDTXEncoderSt stDTXEncState;
   short a_MemorySyn[LP_ORDER_SIZE], a_Memory_W0[LP_ORDER_SIZE];
   short a_MemoryErr[LP_ORDER_SIZE + SUBFR_SIZE_GSMAMR], *pErrorPtr;

   short vFlagSharp;
   short vFlagLTP; 
   short vLagCountOld, vLagOld;    
   short vBestHpCorr; 

} sEncoderState_GSMAMR;


struct _GSMAMREncoder_Obj {
   GSMAMRCoder_Obj       objPrm;
/* preprocess state */
   char                  *preProc;      /* High pass pre processing filter memory */       
   sEncoderState_GSMAMR  stEncState;
   GSMAMR_Rate_t         rate;          /* encode rate */ 
};


typedef struct{
   short a_ExcVecOld[SUBFR_SIZE_GSMAMR + PITCH_MAX_LAG + FLT_INTER_SIZE];
   short *pExcVec;
   short a_LSP_Old[LP_ORDER_SIZE];       
   short a_MemorySyn[LP_ORDER_SIZE];       
   short vFlagSharp;            
   short vPrevPitchLag;
   short vPrevBadFr;          
   short vPrevDegBadFr;   
   short vStateMachine;
   short a_EnergyHistSubFr[9];
   short vLTPLag;           
   short vBackgroundNoise;
   short vVoiceHangover;
   short a_LTPGainHistory[9];
   short a_EnergyHistVector[ENERGY_HIST_SIZE];
   short vCountHangover;       
   short vCNGen;
   short a_GainHistory[CBGAIN_HIST_SIZE];
   short vHgAverageVar;       
   short vHgAverageCount;     
   short a_LSPAveraged[LP_ORDER_SIZE];  
   short a_PastQntPredErr[LP_ORDER_SIZE];      
   short a_PastLSFQnt[LP_ORDER_SIZE];    
   short a_LSFBuffer[5];
   short vPastGainZero;
   short vPrevGainZero;
   short a_GainBuffer[5];
   short vPastGainCode;
   short vPrevGainCode;
   short a_PastQntEnergy_M122[4];   
   short a_PastQntEnergy[4];         
   sPhaseDispSt stPhDispState;
   sDTXDecoderSt dtxDecoderState; 
} sDecoderState_GSMAMR;
 
struct _GSMAMRDecoder_Obj {
/* post process state */
   GSMAMRCoder_Obj       objPrm;
   char                  *postProc;     /* High pass post processing filter memory */  
   sDecoderState_GSMAMR  stDecState;
   sPostFilterSt         stPFiltState;
   GSMAMR_Rate_t         rate;           /* decode rate */
};

int ownEncode_GSMAMR(sEncoderState_GSMAMR *st, GSMAMR_Rate_t rate, short ana[], int *pVad, short synth[]);

int ownDecoderInit_GSMAMR(sDecoderState_GSMAMR* state, GSMAMR_Rate_t rate);

int ownDtxDecoderInit_GSMAMR(sDTXDecoderSt* st);

int ownPhDispInit_GSMAMR(sPhaseDispSt* state);

int ownPostFilterInit_GSMAMR(sPostFilterSt *state); 

#define   GSMAMR_CODECFUN(type,name,arg)                extern type name arg
/********************************************************
*      auxiliary inline functions declarations
*********************************************************/
__INLINE int ShiftL_32s(int n, unsigned short x)
{
   int z = n;
   for(;x>0;x--)
   {
      if (z > IPP_MAX_32S/2)
	  {
         z = IPP_MAX_32S;
         break;
	  }
      else
	  {
        if (z < (int) 0xc0000000)
		{
           z = IPP_MIN_32S;
           break;
		}      
	  }
	  z *= 2;
   }
   return z;
}
__INLINE short ShiftL_16s(short n, unsigned short x)
{
   short z = n;
   for(;x>0;x--)
   {
      if (z > (short) 0X3fff)
	  {
         z = IPP_MAX_16S;
         break;
	  }
      else
	  {
        if (z < (short) 0xc000)
		{
           z = IPP_MIN_16S;
           break;
		}      
	  }
	  z *= 2;
   }
   return z;
}
__INLINE int ownSqrt_32s( int n )
{
    int   i  ;

    short   x =  0 ;
    short   y =  0x4000 ;

    int   z ;

    for ( i = 0 ; i < 14 ; i ++ ) {
        z = (x + y) * (x + y ) ;
        if ( n >= z )
            x += y ;
        y >>= 1 ;
    }
    return x;
}
__INLINE short Abs_16s(short x){
   if(x<0){
      if(IPP_MIN_16S == x) return IPP_MAX_16S;
      x = (short)-x;
   }
   return x;
}
__INLINE short Cnvrt_NR_32s16s(int x) {
   short s = IPP_MAX_16S;
   if(x<(int)0x7fff8000) s = (x+0x8000)>>16;
   return s;
}
__INLINE int Cnvrt_64s32s(__INT64 z) {
   if(IPP_MAX_32S < z) return IPP_MAX_32S;
   else if(IPP_MIN_32S > z) return IPP_MIN_32S;
   return (int)z;
}
__INLINE short Cnvrt_32s16s(int x){ 
   if (IPP_MAX_16S < x) return IPP_MAX_16S;
   else if (IPP_MIN_16S > x) return IPP_MIN_16S;
   return (short)(x);
} 
__INLINE int Add_32s(int x, int y) { 
   return Cnvrt_64s32s((__INT64)x + y);
}
__INLINE int MulC_32s(short val, int x) {
   int z ;
   int xh, xl;
   xh  = x >> 16;
   xl  = x & 0xffff;  
   z = 2*val*xh;
   z = Add_32s(z,(xl*val)>>15);

  return z;
}
__INLINE short Exp_16s(short x){
   short i;
   if (x == 0) return 0;
   if ((short)0xffff == x) return 15;
   if (x < 0) x = ~x;
   for(i = 0; x < (short)0x4000; i++) x <<= 1;
   return i;
}
__INLINE short   Rand2_16s( short *p ) /* GSMAMR */
{
    *p = *p * 521 + 259;
    return *p;
}

__INLINE short random_number(short np1, short *nRandom)
{
    short temp;

    temp = Rand2_16s(nRandom) & 0x7FFF;
    temp = (temp * np1)>>15;
    return temp;
}
__INLINE short Exp_32s_Pos(int x){
   short i;
   if (x == 0) return 0;
   for(i = 0; x < (int)0x40000000; i++) x <<= 1;
   return i;
}
__INLINE short Norm_32s_Pos_I(int *x){
   short i;
   if (*x == 0) return 0;
   for(i = 0; *x < (int)0x40000000; i++) *x <<= 1;
   return i;
}
__INLINE short Norm_32s_I(int *x){
   short i;
   if (*x == 0) return 0;
   if (*x < 0){
      for(i = 0; *x >= (int)0xC0000000; i++) *x <<= 1;
   }else
      for(i = 0; *x < (int)0x40000000; i++) *x <<= 1;
   return i;
}


__INLINE int Mul16s_32s (short hi, short lo, short n)
{
    return (2 * (hi * n) +  2 * ((lo * n) >> 15));
}

__INLINE void L_Extract (int x, short *hi, short *lo)
{
    *hi = x >> 16;
    *lo = (x>>1)&0x7fff;
    return;
}
__INLINE int AddProduct_32s (int x, short hi1, short lo1, short hi2, short lo2)
{
    x += 2*(hi1*hi2) + 2*((hi1*lo2)>>15) + 2*((lo1*hi2)>>15);
    return x;
}

__INLINE int AddProduct16s_32s (int x, short hi, short lo, short n)
{
    x += 2*(hi*n) + 2*((lo*n)>>15);
    return x;
}
__INLINE int Mul2_32s(int x) { 
    if(x > IPP_MAX_32S/2) return IPP_MAX_32S; 
    else if( x < (int) 0xc0000000L) return IPP_MIN_32S; 
    return (x <<= 1);
}
__INLINE int Mul4_32s(int x) { 
    if(x > (int) 0x1fffffffL) return IPP_MAX_32S; 
    else if( x < (int) 0xe0000000L) return IPP_MIN_32S; 
    return (x <<= 2);
}
__INLINE int Mul8_32s(int x) { 
    if(x > (int) 0x0fffffffL) return IPP_MAX_32S; 
    else if( x < (int) 0xf0000000L) return IPP_MIN_32S; 
    return (x <<= 3);
}

__INLINE int Mul16_32s(int x) { 
    if(x > (int) 0x07ffffffL) return IPP_MAX_32S; 
    else if( x < (int) 0xf8000000L) return IPP_MIN_32S; 
    return (x <<= 4);
}

#endif /*__OWNGSMAMR_H__*/
