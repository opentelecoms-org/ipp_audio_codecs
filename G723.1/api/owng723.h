/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.723.1 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.723.1 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.723.1 speech codec main own header file
//
*/

#ifndef __OWNG723_H__
#define __OWNG723_H__
#include <stdio.h>
#include <stdlib.h>
#include "g723ipp.h"
#include "g723api.h"

#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif
#include "scratchmem.h"

/* G.723.1 coder global constants */
#define  G723_FRM_LEN                  240
#define  G723_SBFR_LEN            (G723_FRM_LEN/4)
#define  G723_HALFFRM_LEN         (G723_FRM_LEN/2)
#define  G723_LPC_ORDER                 10                  /*order of LPC filters */
#define  G723_LPC_ORDERP1               11                  /*order of LPC filters plus 1*/
#define  SmoothCoeff                (short ) 0xc000     /* -0.25 */
#define  G723_MIN_PITCH                 18
#define  AdCdbkSizeHighRate             85
#define  G723_MAX_PITCH              ((G723_MIN_PITCH)+127)
#define  GRIDSIZE                        2
#define  AdCdbkSizeLowRate             170
#define  N_PULSES                        6
#define  G723_TOEPLIZ_MATRIX_SIZE      416
#define  N_GAINS                        24
#define  N_AUTOCORRS_BLOCKS              3  /* N frames for AutoCorrs averaging     */
#define  N_GAIN_AVER_FRMS                3  /* N frames for gain averaging    */
#define  NUM_PULSE_IN_BLOCK             11 /* N pulses per block             */
#define  INV_NUM_PULSE_IN_BLOCK       2979 /* 1/NUM_PULSE_IN_BLOCK                  */
#define  MAX_GAIN                       50 /* Maximum gain in CNG excitation    */
#define  AUOTOCORRS_BUFF_SIZE        ((N_AUTOCORRS_BLOCKS+1)*(G723_LPC_ORDER+1)) /* size of AutoCorrs array    */
#define  G723_MAX_GAIN               10000  /* Maximum gain for fixed CNG excitation   */

typedef enum {
   G723_Rate63 = 0,
   G723_Rate53
}G723_Rate;

typedef enum {
   G723_UntransmittedFrm = 0,
   G723_ActiveFrm,
   G723_SIDFrm
}G723_FrameType;

typedef  struct   {
   short     isBadFrame;
   G723_FrameType     FrameType;
   G723_Rate currRate;
   int       lLSPIdx ;
   short     PitchLag[2] ;
   short     AdCdbkLag[4];
   short     AdCdbkGain[4];
   short     sAmpIndex[4];
   short     sGrid[4];
   short     sTrainDirac[4];
   short     sAmplitude[4];
   int       sPosition[4];
} ParamStream_G723;

typedef  struct   {
   short    sDelay;
   short    sGain;
   short    sScalingGain;
} GainInfo_G723;

void DecoderCNG_G723(G723Decoder_Obj* decoderObj, ParamStream_G723 *CurrentParams, short *pExcitation, short *pDstLPC);

void  FixedCodebookSearch_G723_16s(G723Encoder_Obj *encoderObj, ParamStream_G723 *Params, short  *pSrcDst, short  *ImpResp, short  Sfc);
void  InterpolationIndex_G723_16s( short *pDecodedExc, short sPitchLag, short *pGain, short *pGainSFS, short *pDstIdx);
void ErrorUpdate_G723(int *pError, short openLoopLag, short AdCbbkLag, short AdCbbkGain, G723_Rate currRate);
void  PostFilter(G723Decoder_Obj* decoderObj, short *pSrcDstSignal, short *pSrcLPC );
void  LSPInterpolation(const short *pSrcLSP, const short *pSrcPrevLSP, short *pDstLPC);
void    SetParam2Bitstream(G723Encoder_Obj* encoderObj, ParamStream_G723 *Params, char *pDstBitStream);
void GetParamFromBitstream( const char *pSrcBitStream, ParamStream_G723 *Params);
void   UpdateSineDetector(short *SineWaveDetector, short *isNotSineWave);
extern short    LPCDCTbl[G723_LPC_ORDER] ;
extern short    PerceptualFltCoeffTbl[2*G723_LPC_ORDER] ;
extern short    GainDBLvls[N_GAINS] ;
extern IppSpchBitRate SA_Rate[2];

#if defined(__ICL ) || defined ( __ECL )
/*  Intel C/C++ compiler bug for __declspec(align(8)) !!! */
  #define __ALIGN(n) __declspec(align(16))
  #define __ALIGN32 __declspec(align(32))
#else
  #define __ALIGN(n)
  #define __ALIGN32
#endif


#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif

void EncoderCNG_G723(G723Encoder_Obj* encoderObj, ParamStream_G723 *Params, short *pExcitation, short *pDstLPC);
void UpdateAutoCorrs_G723(G723Encoder_Obj* encoderObj, const short *pSrcAutoCorrs, const short *pSrcAutoCorrsSFS);
void DecodeSIDGain_G723_16s (int pIndx, Ipp16s *pGain);

#define ComfortNoiseExcitation_G723_16s_Buff_Size (2*NUM_PULSE_IN_BLOCK+2*NUM_PULSE_IN_BLOCK+G723_SBFR_LEN/GRIDSIZE+4+2*G723_SBFR_LEN)*sizeof(short)

void ComfortNoiseExcitation_G723_16s (Ipp16s gain, Ipp16s *pPrevExc, Ipp16s *pExc,
                                      Ipp16s *pSeed, Ipp16s *pOlp, Ipp16s *pLags,
                                      Ipp16s *pGains, G723_Rate currRate, char *buff, Ipp16s *CasheCounter);
void FixedCodebookGain_G723_16s(const Ipp16s *pSrc1, const Ipp16s *pSrc2,
                                Ipp16s *pGainCoeff, int *pGainIdx, short *y);
void ExcitationResidual_G723_16s(const Ipp16s *pSrc1, const Ipp16s *pSrc2,  Ipp16s *pSrcDst,G723Encoder_Obj *encoderObj);

void FixedCodebookVector_G723_16s( int lDecPos, int lAmplitude, int mamp, int lGrid,
       int lGain, int lNSbfr, G723_Rate currRate, Ipp16s *pDst, int *pLag, Ipp16s *pGain );

void QuantSIDGain_G723_16s(const Ipp16s *pSrc, const Ipp16s *pSrcSfs, int len, int *pIndx);

void ResidualInterpolation_G723_16s_I
        (Ipp16s *pSrcDst, Ipp16s *pDst,  int lag, Ipp16s gain, Ipp16s *pSeed);

void VoiceActivityDetectSize_G723(int* pVADsize);
void VoiceActivityDetectInit_G723(char* pVADmem);
void VoiceActivityDetect_G723(const Ipp16s *pSrc, const Ipp16s *pNoiseLPC,
         const Ipp16s *pOpenLoopDelay, int SineWaveDetector, int *pVADDecision, int *pAdaptEnableFlag, char* pVADmem, short *AlignBuff);

#if !defined (NO_SCRATCH_MEMORY_USED)
struct _ScratchMem_Obj{
   char *base;
   char *CurPtr;
   int  *VecPtr;
   int  offset;
};
#endif


struct _G723Encoder_Obj{
   G723_Obj_t          objPrm;  /* must be on top     */

   short               ZeroSignal[G723_SBFR_LEN];
   short               UnitImpulseSignal[G723_SBFR_LEN];
   short               PrevWeightedSignal[G723_MAX_PITCH+3];            /* All pitch operation buffers */
   short               FltSignal[G723_MAX_PITCH+G723_SBFR_LEN+3];     /* shift 3 - aligning */
   short               PrevExcitation[G723_MAX_PITCH+3];
   short               SignalDelayLine[G723_HALFFRM_LEN];/* Required memory for the delay */
   short               WeightedFltMem[2*G723_LPC_ORDER];/* Used delay lines */
   short               RingWeightedFltMem[2*G723_LPC_ORDER];/* formant perceptual weighting filter delay line */
   short               RingSynthFltMem[G723_LPC_ORDER]; /* synthesis filter delay line */
   short               PrevOpenLoopLags[2];
   short               PrevLPC[G723_LPC_ORDER];/* Lsp previous vector */
   short               AverEnerCounter;
   short               PrevQGain;
   short               SIDLSP[G723_LPC_ORDER];
   short               CNGSeed;
   short               CurrGain;
   short               LPCSID[G723_LPC_ORDER];
   short               ReflectionCoeffSFS;
   short               sSearchTime;                 /* for ippsFixedCodebookSearch_G723_16s function*/
   short               prevSidLpc[G723_LPC_ORDERP1];
   short               SineWaveDetector;/* Sine wave detector */
   short               sSidGain;
   short               ReflectionCoeff[G723_LPC_ORDER+1];
   G723_FrameType      PastFrameType;
   short               AutoCorrs[AUOTOCORRS_BUFF_SIZE];
   short               AutoCorrsSFS[N_AUTOCORRS_BLOCKS+1];
   short               GainAverEnergies[N_GAIN_AVER_FRMS];
   int                 ExcitationError[5];/* Taming procedure errors */
   int                 HPFltMem[2];/* High pass variables */
   char                *vadMem;               /* VAD memory */
   int                 AdaptEnableFlag;
   short               CasheCounter;
#if !defined (NO_SCRATCH_MEMORY_USED)
   ScratchMem_Obj      Mem;
#endif
};

struct _G723Decoder_Obj{
   G723_Obj_t          objPrm;  /* must be on top     */

   short               PrevExcitation[G723_MAX_PITCH+3]; /* 3 - shift for aligning */
   short               PostFilterMem[2*G723_LPC_ORDER]; /* Fir+Iir delays */
   short               PrevLPC[G723_LPC_ORDER];/* Lsp previous vector */
   short               ErasedFramesCounter;
   short               InterpolatedGain;
   short               SyntFltIIRMem[G723_LPC_ORDER];/* Used delay lines */
   short               InterpolationIdx;
   short               ResIntSeed;
   short               SIDLSP[G723_LPC_ORDER];
   short               ReflectionCoeff;
   short               PstFltGain;
   short               CurrGain;
   G723_FrameType      PastFrameType;
   short               sSidGain;
   short               CNGSeed;
   short               CasheCounter;
#if !defined (NO_SCRATCH_MEMORY_USED)
   ScratchMem_Obj      Mem;
#endif
};

#define   G723_CODECFUN(type,name,arg)                extern type name arg

__INLINE int ShiftL_32s(int n, unsigned short x)
{
   int max = IPP_MAX_32S >> x;
   int min = IPP_MIN_32S >> x;
   if(n > max) return IPP_MAX_32S;
   else if(n < min) return IPP_MIN_32S;
   return (n<<x);
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
__INLINE short Cnvrt_NR_32s16s(int x) {
   short s = IPP_MAX_16S;
   if(x<=(int)0x7fff8000) s = (x+0x8000)>>16;
   return s;
}
__INLINE short Cnvrt_32s16s(int x){
   if (IPP_MAX_16S < x) return IPP_MAX_16S;
   else if (IPP_MIN_16S > x) return IPP_MIN_16S;
   return (short)(x);
}
__INLINE int Cnvrt_64s32s(__INT64 z) {
   if(IPP_MAX_32S < z) return IPP_MAX_32S;
   else if(IPP_MIN_32S > z) return IPP_MIN_32S;
   return (int)z;
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

  return( z );
}

__INLINE short Norm_32s_Pos_I(int *x){
   short i;
   if (*x == 0) return 0;
   for(i = 0; *x < (int)0x40000000; i++) *x <<= 1;
   return i;
}

__INLINE short Exp_32s_Pos(int x){
   short i;
   if (x == 0) return 0;
   for(i = 0; x < (int)0x40000000; i++) x <<= 1;
   return i;
}
__INLINE short Exp_16s(short x){
   short i;
   if (x == 0) return 0;
   if ((short)0xffff == x) return 15;
   if (x < 0) x = ~x;
   for(i = 0; x < (short)0x4000; i++) x <<= 1;
   return i;
}
__INLINE short   Rand2_16s( short *pSeed ) /* G.723.1 */
{
    *pSeed = *pSeed * 521 + 259 ;
    return *pSeed ;
}

__INLINE short NormRand_16s(short N, short *nRandom)
{
    short sTmp;

    sTmp = Rand2_16s(nRandom) & 0x7FFF;
    sTmp = (sTmp * N)>>15;
    return sTmp;
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
__INLINE short ShiftL_16s(short n, unsigned short x)
{
   short max = IPP_MAX_16S >> x;
   short min = IPP_MIN_16S >> x;
   if(n > max) return IPP_MAX_16S;
   else if(n < min) return IPP_MIN_16S;
   return (n<<x);
}
__INLINE short Mul2_16s(short x) { 
    if(x > IPP_MAX_16S/2) return IPP_MAX_16S; 
    else if( x < IPP_MIN_16S/2) return IPP_MIN_16S; 
    return x <<= 1;
}
__INLINE int Mul2_32s(int x) { 
    if(x > IPP_MAX_32S/2) return IPP_MAX_32S; 
    else if( x < IPP_MIN_32S/2) return IPP_MIN_32S; 
    return x <<= 1;
}
#endif /*__OWNG723_H__*/
