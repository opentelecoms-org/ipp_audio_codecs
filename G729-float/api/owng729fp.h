/*****************************************************************************
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2003-2004 Intel Corporation. All Rights Reserved.
//
//  Purpose: G.729.C speech codec main own header file
//
***************************************************************************/
#ifndef __OWNCODEC_H__
#define __OWNCODEC_H__
#include <stdlib.h>
#include "ipps.h"
#include "ippsc.h"
#include "g729api.h"

#define ENC_KEY 0xecd729
#define DEC_KEY 0xdec729

#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif

#include "scratchmem.h"

#define G729FP_ENCODER_SCRATCH_MEMORY_SIZE   /*(2*7168+40)*/(2*5120+40)

#define SPEECH_BUFF_LEN 240     /* Total size of speech buffer               */
#define FRM_LEN         80      /* LPC update frame size                     */
#define SUBFR_LEN       40      /* Sub-frame size                            */
#define NUN_SUBFRAMES   FRM_LEN/SUBFR_LEN

#define WINDOW_LEN        240     /* LPC analysis window size                  */
#define LOOK_AHEAD_LEN          40      /* Samples of next frame needed for LPC ana. */

#define LPC_ORDER       10      /* LPC order                                 */
#define LPC_ORDERP1     (LPC_ORDER+1)    /* LPC order+1                               */

#define MOVING_AVER_ORDER           4       /* MA prediction order for LSP                     */
#define N_BITS_1ST_STAGE           7       /* number of bits in first stage for LSP index     */
#define N_ELEM_1ST_STAGE          (1<<N_BITS_1ST_STAGE) /* number of entries in first stage for LSP index  */
#define N_BITS_2ND_STAGE           5       /* number of bits in second stage for LSP index    */
#define N_ELEM_2ND_STAGE          (1<<N_BITS_2ND_STAGE) /* number of entries in second stage for LSP index */

#define LSF_LOW_LIMIT         (float)0.005
#define LSF_HI_LIMIT          (float)3.135
#define LSF_DIST              (float)0.0392

#ifdef CLIPPING_DENORMAL_MODE
   #define DENORMAL_THRESHOLD 1.0E-14

   #define CLIP_DENORMAL(src,dst) if(fabs(src) < DENORMAL_THRESHOLD) dst = 0;\
                                 else dst = src
   #define CLIP_DENORMAL_I(srcdst) if(fabs(srcdst) < DENORMAL_THRESHOLD) srcdst = 0;
#else

   #define CLIP_DENORMAL(src,dst) dst = src
   #define CLIP_DENORMAL_I(srcdst) 
#endif

#define LAR_THRESH1   (float)-1.74
#define LAR_THRESH2   (float)-1.52
#define LAR_THRESH3   (float)0.65
#define LAR_THRESH4   (float)0.43
#define GAMMA1_TILTED         (float)0.98
#define GAMMA2_TILTED_MAX     (float)0.7
#define GAMMA2_TILTED_MIN     (float)0.4
#define GAMMA1_FLAT           (float)0.94
#define GAMMA2_FLAT           (float)0.6
#define GAMMA2_TILTED_SCALE   (float)-6.0
#define GAMMA2_TILTED_SHIFT   (float)1.0

#define PITCH_LAG_MIN   20      /* Minimum pitch lag in samples              */
#define PITCH_LAG_MAX   143     /* Maximum pitch lag in samples              */
#define INTERPOL_LEN      (10+1)  /* Length of filter for interpolation.       */
#define INTER_PITCH_LEN   10      /* Length for pitch interpolation            */
#define INTERPOL4_LEN        4       /* Upsampling ration for pitch search        */
#define UP_SAMPLING     3       /* Resolution of fractional delays           */
#define PITCH_THRESH       0.85f   /* Threshold to favor smaller pitch lags     */
#define GAIN_PIT_MAX    1.2f    /* Maximum adaptive codebook gain            */

/* Constants for fixed codebook search. */
#define TOEPLIZ_MATRIX_SIZE  616 /* size of correlation matrix                            */

#define PITCH_SHARPMAX        (float)0.7945  /* Maximum value of pitch sharpening */
#define PITCH_SHARPMIN        (float)0.2     /* minimum value of pitch sharpening */

/* Constants for taming procedure.*/
#define MAX_GAIN_TIMING      (float)0.95     /* Maximum pitch gain if taming is needed */
#define MAX_GAIN_TIMING2     (float)0.94     /* Maximum pitch gain if taming is needed */
#define THRESH_ERR           (float)60000.   /* Error threshold taming    */
#define INV_SUBFR_LEN (float) ((float)1./(float)SUBFR_LEN) /* =0.025 */

/* Constants for postfilter */
/* short term pst parameters :  */
#define GAMMA1_POSTFLT      (float)0.7     /* denominator weighting factor           */
#define GAMMA2_POSTFLT      (float)0.55    /* numerator  weighting factor            */
#define SHORTTERM_POSTFLT_LEN       20      /* impulse response length                   */
#define GAMMA3_POSTFLT_P     (float)0.2     /* tilt weighting factor when k1>0        */
#define GAMMA3_POSTFLT_M    (float)0.9     /* tilt weighting factor when k1<0        */

/* long term pst parameters :   */
#define SUBFR_LENP1 (SUBFR_LEN + 1) /* Sub-frame size + 1                        */
#define FRAC_DELAY_RES        8       /* resolution for fractionnal delay          */

#define SHORT_INT_FLT_LEN          4       /* length of short interp. subfilters        */
#define LONG_INT_FLT_LEN           16      /* length of long interp. subfilters         */
#define SHORT_INT_FLT_LEN_BY2    (SHORT_INT_FLT_LEN/2)
#define LONG_INT_FLT_LEN_BY2     (LONG_INT_FLT_LEN/2)

#define LTPTHRESHOLD    (float)0.5f     /* threshold LT to switch off postfilter */
#define AGC_FACTOR         (float)0.9875  /* gain adjustment factor                 */

#define AGC_FACTORM1       ((float)1. - AGC_FACTOR)    /* gain adjustment factor                 */

/* Array sizes */
#define RESISDUAL_MEMORY (PITCH_LAG_MAX + 1 + LONG_INT_FLT_LEN_BY2)
#define SIZE_RESISDUAL_MEMORY (RESISDUAL_MEMORY + SUBFR_LEN)
#define SIZE_SEARCH_DEL_MEMORY  ((FRAC_DELAY_RES-1) * SUBFR_LENP1)
#define SIZE_LONG_INT_FLT_MEMORY ((FRAC_DELAY_RES-1) * LONG_INT_FLT_LEN)
#define SIZE_SHORT_INT_FLT_MEMORY ((FRAC_DELAY_RES-1) * SHORT_INT_FLT_LEN)

#define G729D_MODE      0      /* Low  rate  (6400 bit/s)       */
#define G729_BASE       1      /* Full rate  (8000 bit/s)       */
#define G729E_MODE      2      /* High rate (11800 bit/s)       */

/* backward LPC analysis parameters */
#define BWD_LPC_ORDER         30         /* Order of Backward LP filter.              */
#define BWD_LPC_ORDERP1               (BWD_LPC_ORDER+1)  /* Order of Backward LP filter + 1           */
#define NON_RECURSIVE_PART    35
#define BWD_SYNTH_MEM           (BWD_LPC_ORDER + NON_RECURSIVE_PART)
#define BWD_ANALISIS_WND_LEN             (FRM_LEN + BWD_SYNTH_MEM)
#define BWD_GAMMA 0.98f

/* Annex E adaptive short term postfilter parameters:*/
#define GAMMA1_POSTFLT_E  0.7f      /* denominator weighting factor */
#define GAMMA2_POSTFLT_E  0.65f     /* numerator  weighting factor */
#define SHORTTERM_POSTFLT_LEN_E   32        /* Lenght of the impulse response*/
#define GAMMA_HARM_POSTFLT_E 0.25f
#define GAMMA_HARM_POSTFLT   0.5f

/* Constants for backward/forward decision*/
#define THRES_ENERGY 40.f /*Low energy frame threshold*/
/* Gains levels */
#define TH1 1.f
#define TH2 2.f
#define TH3 3.f
#define TH4 4.f
#define TH5 4.7f
#define GAP_FACT (float)0.000114375f
#define INVERSE_LOG2 (float) (1./log10(2.))

/*Constants for gain quantization.*/
#define MEAN_ENER        (float)36.0               /* average innovation energy */
#define NCODE1_BITS  3                             /* number of Codebook-bit */
#define NCODE2_BITS  4                             /* number of Codebook-bit */
#define SIZECODEBOOK1    (1<<NCODE1_BITS)          /* Codebook 1 size */
#define SIZECODEBOOK2    (1<<NCODE2_BITS)          /* Codebook 2 size */
#define NUM_CAND1            4                     /* Pre-selecting order for #1 */
#define NUM_CAND2            8                     /* Pre-selecting order for #2 */
#define INV_COEF_BASE   (float)-0.032623

/*Constants for gain quantization in Annex D mode*/
#define NCODE1_B_ANNEXD  3                         /* number of Codebook-bit */
#define NCODE2_B_ANNEXD  3                         /* number of Codebook-bit */
#define SIZECODEBOOK1_ANNEXD (1<<NCODE1_B_ANNEXD)  /* Codebook 1 size */
#define SIZECODEBOOK2_ANNEXD (1<<NCODE2_B_ANNEXD)  /* Codebook 2 size */
#define NUM_CAND1_ANNEXD  6                        /* Pre-selecting order for #1 */
#define NUM_CAND2_ANNEXD  6                        /* Pre-selecting order for #2 */
#define INV_COEF_ANNEXD  ((float)-0.027599)
#define NUM_TRACK_ACELP          4

/* VAD */
#define     LPC_ORDERP2            12                  /* LPC order plus 2*/
#define     VAD_NOISE         0                 /* Non-active frame*/
#define     VAD_VOICE         1                 /* Active frame*/
#define     END_OF_INIT   32
#define     ZC_START_INDEX      120
#define     ZC_END_INDEX        200

/* DTX constants */
#define ENCODER         1
#define DECODER         0
#define INIT_SEED_VAL       11111
#define N_MIN_SIM_RRAMES      3
#define ITAKURATHRESH1         (float)1.1481628/2.
#define ITAKURATHRESH2         (float)1.0966466/2.

#define GAIN_INT_FACTOR        (float)0.875
#define INV_GAIN_INT_FACTOR    ((float)1. - GAIN_INT_FACTOR)

#define MIN_ENER        (float)0.1588489319   /*- 8 dB threshold*/

/* CNG constants */
#define NORM_GAUSS      (float)3.16227766  /* sqrt(40)xalpha, alpha=0.5 */
#define K_MUL_COEFF              (float)3.          /* 4 x (1 - alpha ** 2), alpha=0.5*/
#define CNG_MAX_GAIN           (float)5000.

#define GAMMA1_G729A       (float)0.75    /* Bandwitdh expansion for W(z)             */
#define  GAMMA_POSTFLT_G729A      (float)0.50       /* Harmonic postfilt factor              */
#define  INV_GAMMA_POSTFLT_G729A  ((float)1.0/((float)1.0+GAMMA_POSTFLT_G729A))
#define  GAMMA2_POSTFLT_G729A    (GAMMA_POSTFLT_G729A/((float)1.0+GAMMA_POSTFLT_G729A))
#define  TILT_FLT_FACTOR          (float)0.8        /* Factor for tilt compensation filter   */
#define  AGC_FACTOR_G729A     (float)0.9        /* Factor for automatic gain control     */
#define  AGC_FACTOR_1M_G729A     ((float)1.-AGC_FACTOR_G729A)
#define  PST_IMPRESP_LEN 22   /* size of truncated impulse response of A(z/g1)/A(z/g2) */

#define CLIP_TO_UPLEVEL(value,maxValue)\
   if(value>maxValue) value=maxValue

#define CLIP_TO_LOWLEVEL(value,minValue)\
   if(value<minValue) value=minValue

void VADGetSize(Ipp32s *pDstSize);
void VADInit(char *vadMem);
void CNGGetSize(Ipp32s *pDstSize);
void CNGInit(char *cngMem);
void PSTGetSize(Ipp32s *pDstSize);
void PSTInit(char *cngMem);
void MSDGetSize(Ipp32s *pDstSize);
void MSDInit(char *msdMem);
void PHDGetSize(Ipp32s *pDstSize);
void PHDInit(char *phdMem);

int  ExtractBitsG729( const unsigned char **pBits, int *nBit, int Count );

void ownAutoCorr_G729_32f(Ipp32f *pSrc, int len, Ipp32f *pDst, float *pExtBuff);
void ownACOS_G729_32f(Ipp32f *pSrc, Ipp32f *pDst, Ipp32s len);
void ownCOS_G729_32f(Ipp32f *pSrc, Ipp32f *pDst, Ipp32s len);
Ipp32f ownAdaptiveCodebookGainCoeff_G729_32f(Ipp32f *pSrcTargetVector, Ipp32f *pSrcFltAdaptivCdbkVec,
               Ipp32f *pDstCorrCoeff, int len);

void AdaptiveCodebookGainCoeff_G729_32f( float *pSrcTargetVector, float *pSrcFltAdaptiveCodebookVector,
                                         float *pSrcFltInnovation, float *pDstCoeff);

int ownAdaptiveCodebookSearch_G729A_32f(Ipp32f *pSrcExc, Ipp32f *pSrcTargetVector, Ipp32f *pSrcImpulseResponse,
  int minPitchDelay, int maxPitchDelay, int nSbfr, int *fracPartPitchDelay, float *pExtBuff);

void VoiceActivityDetect_G729_32f(float  ReflectCoeff, float *pLSF, float *pAutoCorr, float *pSrc, int FrameCounter,
                                  int prevDecision, int prevPrevDecision, int *pVad, float *pEnergydB,char *pVADmem,float *pExtBuff);

void PWGammaFactor_G729(float *pGamma1, float *pGamma2, float *pIntLSF, float *CurrLSF,
                   float *ReflectCoeff, int   *isFlat, float *PrevLogAreaRatioCoeff);

void MusicDetection_G729E_32f( G729Encoder_Obj *encoderObj, G729Codec_Type codecType, float Energy, float *ReflectCoeff, int *Vad, 
                                 float EnergydB,char *msdMem,float *pExtBuff);

void PitchTracking_G729E(int *T0, int *T0_frac, int *lPrevPitchPT, int *lStatPitchPT, int *lStatPitch2PT,  int *lStatFracPT);

void OpenLoopPitchSearch_G729_32f(const Ipp32f *pSrc, Ipp32s* bestLag);

void WeightLPCCoeff_G729(float *pSrcLPC, float valWeightingFactor, int len, float *pDstWeightedLPC);

int TestErrorContribution_G729(int valPitchDelay, int valFracPitchDelay, float *ExcErr);

void UpdateExcErr_G729(float valPitchGain, int valPitchDelay, float *pExcErr);

void isBackwardModeDominant_G729(int *isBackwardModeDominant, int LPCMode, int *pCounterBackward, int *pCounterForward);

float CalcEnergy_dB_G729(float *pSrc, int len);

void InterpolatedBackwardFilter_G729(float *pSrcDstLPCBackwardFlt, float *pSrcPrevFilter, float *pSrcDstIntCoeff);

void PhaseDispersionUpdate_G729D(float valPitchGain, float valCodebookGain, char *phdMem);
void PhaseDispersion_G729D(float *pSrcExcSignal, float *pDstFltExcSignal, float valCodebookGain,
                           float valPitchGain, float *pSrcDstInnovation, char *phdMem,char *pExtBuff);

void SetLPCMode_G729E(G729Encoder_Obj* encoderObj, float *pSrcSignal, float *pSrcForwardLPCFilter,
                      float *pSrcBackwardLPCFilter, int *pDstLPCMode, float *pSrcLSP,float *pExtBuff);

int AdaptiveCodebookSearch_G729_32f(float *pSrcExc, float *pSrcTargetVector, float *pSrcImpulseResponse, int len,
                        int minLag, int maxLag, int valSubframeNum, int *pDstFracPitch, G729Codec_Type codecType,float *pExtBuff);

void ComfortNoiseExcitation_G729(float fCurrGain, float *exc, short *sCNGSeed, int flag_cod, float *ExcitationError, char *phdMem, char *pExtBuff);

void QuantSIDGain_G729B(float *ener, int lNumSavedEnergies, float *enerq, int *idx);

int GainQuant_G729(float *FixedCodebookExc, float *pGainCoeff, int lSbfrLen, float *gain_pit, float *fCodeGain,
                     int tamingflag, float *PastQuantEnergy, G729Codec_Type codecType,char *pExtBuff);
void DecodeGain_G729(int index, float *code, int l_subfr, float *pitchGain, float *codeGain, int rate, float *PastQuantEnergy);

void Post_G729E(G729Decoder_Obj *decoderObj, int pitchDelay, float *pSignal, float *pLPC, float *pDstFltSignal, int *pVoicing,
               int len, int lenLPC, int Vad);

typedef struct _G729CCoder_Obj{
   int                 objSize;
   int                 key;
   unsigned int        mode;          /* coder mode's */
   G729Codec_Type     codecType;
}G729CCoder_Obj;

#if !defined (NO_SCRATCH_MEMORY_USED)
struct _ScratchMem_Obj{
   char *base;
   char *CurPtr;
   int  *VecPtr;
   int  offset;
};
#endif

typedef struct _VADmemory{
   float MeanLSFVec[LPC_ORDER];
   float MinimumBuff[16];
   float fMeanEnergy;
   float fMeanFullBandEnergy;
   float fMeanLowBandEnergy;
   float fMeanZeroCrossing;
   float fPrevMinEnergy;
   float fNextMinEnergy;
   float fMinEnergy;
   float fPrevEnergy;
   int lVADFlag;
   int lSilenceCounter;
   int lUpdateCounter;
   int lSmoothingCounter;
   int lFVD;
   int lLessEnergyCounter;
}VADmemory;

#define SUMAUTOCORRS_NUM       3
#define SUMAUTOCORRS_SIZE      (SUMAUTOCORRS_NUM * LPC_ORDERP1)
#define CURRAUTOCORRS_NUM       2
#define AUTOCORRS_SIZE         (CURRAUTOCORRS_NUM * LPC_ORDERP1)
#define GAINS_NUM         2

extern const float InitLSP[LPC_ORDER];
extern const float InitFrequences[LPC_ORDER];
extern const float lagBwd[BWD_LPC_ORDER];
extern const float SIDGainTbl[32];

typedef struct _CNGmemory{
   float AutoCorrs[AUTOCORRS_SIZE];
   float SumAutoCorrs[SUMAUTOCORRS_SIZE];
   float Energies[GAINS_NUM];
   int lAutoCorrsCounter;
   float fCurrGain;
   int lFltChangeFlag;
   float SIDQuantLSP[LPC_ORDER];
   float ReflectCoeffs[LPC_ORDERP1];
   int lNumSavedEnergies;
   float fSIDGain;
   float fPrevEnergy;
   int lFrameCounter0;
}CNGmemory;

typedef struct _PSTmemory{
   float STPNumCoeff[SHORTTERM_POSTFLT_LEN_E];    /* s.t. numerator coeff.        */
   float STPMemory[BWD_LPC_ORDER];           /* s.t. postfilter memory       */
   float ZeroMemory[BWD_LPC_ORDER];          /* null memory to compute h_st  */
   float ResidualMemory[SIZE_RESISDUAL_MEMORY];       /* A(gamma2) residual           */
   float gainPrec;
}PSTmemory;

typedef struct _MusDetectMemory{
   int lMusicCounter;
   float fMusicCounter;
   int lZeroMusicCounter;
   float fMeanPitchGain;
   int lPFlagCounter;
   float fMeanPFlagCounter;
   int lConscPFlagCounter;
   int lRCCounter;
   float MeanRC[10];
   float fMeanFullBandEnergy;
}MusDetectMemory;

typedef struct _PHDmemory{
   int prevDispState;
   float gainMem[6];
   float prevCbGain;
   int onset;
}PHDmemory;

struct _G729Encoder_Obj{
   G729CCoder_Obj       objPrm;
#if !defined (NO_SCRATCH_MEMORY_USED)
   ScratchMem_Obj      Mem;
#endif
   float OldSpeechBuffer[SPEECH_BUFF_LEN];
   float fBetaPreFilter;
   float OldWeightedSpeechBuffer[FRM_LEN+PITCH_LAG_MAX];
   float OldExcitationBuffer[FRM_LEN+PITCH_LAG_MAX+INTERPOL_LEN];
   float WeightedFilterMemory[BWD_LPC_ORDER]; 
   float FltMem[BWD_LPC_ORDER];
   float OldLSP[LPC_ORDER];
   float OldQuantLSP[LPC_ORDER];
   float ExcitationError[4];
   IppsIIRState_32f *iirstate;
   float PastQuantEnergy[4];
   float PrevFreq[MOVING_AVER_ORDER][LPC_ORDER];    /* previous LSP vector       */
   /* Last forkward A(z) for case of unstable filter */
   float OldForwardLPC[LPC_ORDERP1];
   float OldForwardRC[2];
   short sFrameCounter; /* frame counter for VAD*/
   /* DTX variables */
   int prevVADDec;
   int prevPrevVADDec;
   short sCNGSeed;
   char *vadMem;
   char *cngMem;
   char *msdMem;
   /* G729CA_CODEC*/
   float ZeroMemory[LPC_ORDER];
   /* Not G.729A */
   float SynFltMemory[BWD_LPC_ORDER];
   float ErrFltMemory[BWD_LPC_ORDER+SUBFR_LEN];
   float UnitImpulse[SUBFR_LEN+BWD_LPC_ORDERP1];
   /* for G.729E */
   /* for the backward analysis */
   float PrevFlt[BWD_LPC_ORDERP1]; /* Previous selected filter */
   float SynthBuffer[BWD_ANALISIS_WND_LEN];
   int prevLPCMode;
   float BackwardLPCMemory[BWD_LPC_ORDERP1];
   int isBWDDominant;
   float fInterpolationCoeff;
   short sGlobalStatInd;  /* Mesure of global stationnarity */
   short sBWDStatInd;       /* Num of consecutive backward frames */
   short sValBWDStatInd;   /* Value associated with stat_bwd */
   /* Last backward A(z) for case of unstable filter */
   float OldBackwardLPC[BWD_LPC_ORDERP1];
   float OldBackwardRC[2];
   int LagBuffer[5];
   float PitchGainBuffer[5];
   int  sBWDFrmCounter;
   int sFWDFrmCounter;
   int isSmooth;
   float LogAreaRatioCoeff[2];
   int sSearchTimes;
   IppsWinHybridState_G729E_32f *pHWState;
};

struct _G729Decoder_Obj{
   G729CCoder_Obj       objPrm;
#if !defined (NO_SCRATCH_MEMORY_USED)
   ScratchMem_Obj      Mem;
#endif
   float OldExcitationBuffer[FRM_LEN+PITCH_LAG_MAX+INTERPOL_LEN];
   float fBetaPreFilter;            /* pitch sharpening of previous frame */
   int   prevPitchDelay;           /* integer delay of previous frame    */
   float fCodeGain;        /* Code gain                          */
   float fPitchGain;       /* Pitch gain                         */
   float OldLSP[LPC_ORDER];
   IppsIIRState_32f *iirstate;
   float PastQuantEnergy[4];
   float PrevFreq[MOVING_AVER_ORDER][LPC_ORDER];    /* previous LSP vector       */
   int prevMA;                  /* previous MA prediction coef.*/
   float prevLSF[LPC_ORDER];            /* previous LSF vector         */
   /* for G.729B */
   short sFESeed;
   /* CNG variables */
   int prevFrameType;
   short sCNGSeed;
   float SID;
   float fCurrGain;
   float SIDLSP[LPC_ORDER];
   float fSIDGain;
   float SynFltMemory[BWD_LPC_ORDER];        /* Synthesis filter's memory          */
   char *phdMem;
   /* for G.729A */
   float PstFltMemoryA[LPC_ORDER];
   float fPastGain;
   float ResidualBufferA[PITCH_LAG_MAX+SUBFR_LEN]; /* inverse filtered synthesis (with A(z/GAMMA2_POSTFLT))   */
   float *ResidualMemory;
   float PstSynMemoryA[LPC_ORDER];   /* memory of filter 1/A(z/GAMMA1_POSTFLT) */
   float fPreemphMemoryA;
   /* Not G.729A */
   float  SynthBuffer[BWD_ANALISIS_WND_LEN];  /* Synthesis                   */
   int   prevFracPitchDelay;    /* integer delay of previous frame    */
   /* for the backward analysis */
   float BackwardUnqLPC[BWD_LPC_ORDERP1];
   float BackwardLPCMemory[BWD_LPC_ORDERP1];
   int   lPrevVoicing;
   int   lPrevBFI;
   int   prevLPCMode;
   float fFEInterpolationCoeff;
   float fInterpolationCoeff;
   float PrevFlt[BWD_LPC_ORDERP1]; /* Previous selected filter */
   int   lPrevPitchPT;
   int   lStatPitchPT;
   int   lStatPitch2PT;
   int   lStatFracPT;
   /* Last backward A(z) for case of unstable filter */
   float OldBackwardLPC[BWD_LPC_ORDERP1];
   float OldBackwardRC[2];
   float   fPitchGainMemory;
   float   fCodeGainMemory;
   float   fGainMuting;
   int     lBFICounter;
   int     sBWDStatInd;
   int  lVoicing; /* voicing from previous frame */
   float  g1PST;
   float  g2PST;
   float  gHarmPST;
   int  sBWDFrmCounter;
   int sFWDFrmCounter;
   char *pstMem;
   IppsWinHybridState_G729E_32f *pHWState;
}; 

#define   G729_CODECFUN(type,name,arg)                extern type name arg

__INLINE int Parity( int val){
  int temp, nBits, bit;
  int sum;

  temp = val >> 1;
  sum = 1;
  for (nBits = 0; nBits <= 5; nBits++) {
    temp = temp >> 1;
    bit = temp & 0x00000001;
    sum += bit;
  }
  sum = sum & 0x00000001;

  return sum;
}

__INLINE short Rand_16s(short *seed)
{  
    *seed = (short)(*seed * 31821 + 13849);   
    return(*seed);
}

static const short modtab[]={1, 2, 0, 1, 2, 0, 1, 2};

__INLINE void DecodeAdaptCodebookDelays(int *prevPitchDelay, int *prevFracPitchDelay,int *delayLine,
                                         int NSbfr, int badPitch,int pitchIndx,G729Codec_Type type){
   short minPitchDelay, maxPitchDelay;
   
   if(badPitch == 0){
    
      if (NSbfr == 0)                  /* if 1st subframe */
      { 
         if (pitchIndx < 197)
         { 
            delayLine[0] = (pitchIndx+2)/3 + 19;
            delayLine[1] = pitchIndx - delayLine[0] * 3 + 58;
         } 
         else
         { 
            delayLine[0] = pitchIndx - 112;
            delayLine[1] = 0;
         } 
        
      } else  {/* second subframe */ 
         /* find minPitchDelay and maxPitchDelay for 2nd subframe */
         minPitchDelay = (short)(delayLine[0] - 5);
         CLIP_TO_LOWLEVEL(minPitchDelay,PITCH_LAG_MIN);
        
         maxPitchDelay = (short)(minPitchDelay + 9);
         if (maxPitchDelay > PITCH_LAG_MAX)
         { 
            maxPitchDelay = PITCH_LAG_MAX;
            minPitchDelay = (short)(maxPitchDelay - 9);
         } 
         if (type == G729D_MODE) {
            pitchIndx = pitchIndx & 15;
            if (pitchIndx <= 3) {
               delayLine[0] = minPitchDelay + pitchIndx;
               delayLine[1] = 0;
            }
            else if (pitchIndx < 12) {
               /* *T0_frac = index % 3; */
               delayLine[1] = modtab[pitchIndx - 4];
               delayLine[0] = (pitchIndx - delayLine[1])/3 + minPitchDelay + 2;
                
               if (delayLine[1] == 2) {
                  delayLine[1] = -1;
                  delayLine[0] += 1;
               } 
            } 
            else {
                delayLine[0] = minPitchDelay + pitchIndx - 6;
                delayLine[1] = 0;
            }
            
         } 
         else {
            delayLine[0] = minPitchDelay + (pitchIndx + 2)/3 - 1;
            delayLine[1] = pitchIndx - 2 - 3 * ((pitchIndx + 2)/3 - 1);
         } 
      }
      *prevPitchDelay = delayLine[0];
      *prevFracPitchDelay = delayLine[1];
   }else {                     /* Bad frame, or parity error */
      delayLine[0]  =  *prevPitchDelay;
      if (type == G729E_MODE) {
         delayLine[1] = *prevFracPitchDelay;
      } 
      else {
         delayLine[1] = 0;
         *prevPitchDelay += 1;
         CLIP_TO_UPLEVEL(*prevPitchDelay,PITCH_LAG_MAX);
      } 
   }
}

void CodewordImpConv_G729_32f(int index, const float *pSrc1,const float *pSrc2,float *pDst);

#endif /*__OWNCODEC_H__*/
