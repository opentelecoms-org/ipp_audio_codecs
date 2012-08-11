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
//  Purpose: G.722.1 codec internal definitions
//
*/

#ifndef _OWN_G728_H_
#define _OWN_G728_H_
#include <ipps.h>
#include "g728api.h"

#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif

#if defined(__ICC) || defined(__ECC) || defined( __ICL ) || defined ( __ECL )
    #define __ALIGN16 __declspec (align(16))
    #define __ALIGN32 __declspec (align(32))
#else
    #define __ALIGN16 
    #define __ALIGN32 
#endif

#include "scratchmem.h"

#define ENC_KEY 0xecd728
#define DEC_KEY 0xdec728

#define   G728_CODECFUN(type,name,arg)                extern type name arg

#define G728_TRUE           1
#define G728_FALSE          0

/* G728 Constants */ 

#define AGCFAC     16220 /* AGC adaptation speed controlling factor */
#define AGCFAC1    20972 /*  The value of (1 - AGCFAC)*/
#define IDIM           5 /*  Vector dimension (excitation block size)*/
#define GOFF       16384 /*  Log-gain offset value*/
#define KPDELTA        6 /*  Allowed deviation from previous pitch period*/
#define KPMIN         20 /*  Minimum pitch period (samples)*/
#define KPMAX        140 /*  Maximum pitch period (samples)*/
#define LPC           50 /*  Synthesis filter order*/
#define LPCLG         10 /*  Log-gain predictor order*/
#define LPCW          10 /*  Perceptual weighting filter order*/
#define NCWD         128 /*  Shape codebook size (number of codevectors)*/
#define NCWD3_4      96 
#define NCWD4        32
#define NFRSZ         20 /*  Frame size (adaptation cycle size in samples)*/
#define NG             8 /*  Gain codebook size (number of gain levels)*/
#define NONR          35 /*  Number of non-recursive window samples for synthesis filter*/
#define NONRLG        20 /*  Number of non-recursive window samples for log-gain predictor*/
#define NONRW         30 /*  Number of non-recursive window samples for weighting filter*/
#define NPWSZ        100 /*  Pitch analysis window size (samples)*/
#define NUPDATE        4 /*  Predictor update period (in terms of vectors)*/
#define PPFTH       9830 /*  Tap threshold for turning off pitch postfilter*/
#define PPFZCF      9830 /*  Pitch postfilter zero controlling factor*/
#define TAPTH      26214 /*  Tap threshold for fundamental pitch replacement*/
#define TILTF       4915 /*  Spectral tilt compensation controlling factor*/

#define REXPWMEMSIZE (8+LPCW+1+1+60)
#define REXPLGMEMSIZE (8+LPCLG+1+1+34)
#define REXPMEMSIZE (8+LPC+1+1+105+21)
#define COMBMEMSIZE (LPC+11 + 2*(LPCW+1))
#define IIRMEMSIZE (2*LPCW)
#define STPMEMSIZE (2*LPCW)
#define SYNTHMEMSIZE (LPC+11)

/* Rate 12.8 specific*/

#define NG_128         4 /* Gain codebook size (number of gain levels)*/
#define NG_96          4 /* Gain codebook size (number of gain levels)*/

#define ST_BUFF_SIZ

/* G728I coder specific*/
#define FESIZE         4 /* Number of 2.5 msec adaptation cycles in a frame erasure (FE)
                            e.g.: 1 for 2.5 msec FEs, 4 for 10 msec FEs.
                            FEs may be any multiple of 2.5 msec. SET BY USER*/

#define AFTERFEMAX    16 /* Maximum 2.5 ms frames for gain clipping */

/*                                                                            */ 
void VscaleFive_16s(const Ipp16s* pSrc, Ipp16s* pDst, Ipp16s rangeScale, Ipp16s* pScale);
void VscaleOne_Range30_32s(const Ipp32s* pSrc, Ipp32s* pDst, Ipp16s* pScale);

void Divide_16s(Ipp16s num, Ipp16s numnls, Ipp16s den, Ipp16s dennls, Ipp16s *quo, Ipp16s *quonls);
/*void SimpDiv(Ipp16s num, Ipp16s den, Ipp32s *aa0);*/ 

void LevinsonDurbin(const short *rtmp, int ind1, int ind2, short *pTmpSynFltrCoeffs, short *rc1, 
                    short *alphatmp, short *scaleSynFltrCoeffs, short *illcondp, short *illcond);

void GetShapeGainIdxs(Ipp16s codebookIdx, Ipp32s *pShapeIdx, Ipp32s *pGainIdx, Ipp16s rate);

/* VQ functions*/
void VQ_target_vector_calc(Ipp16s *sw, Ipp16s *zir, Ipp16s *target);
void Impulse_response_vec_calc(Ipp16s *a, Ipp16s *pWghtFltrCoeffs, Ipp16s *h);
void Time_reversed_conv(Ipp16s *h, Ipp16s *target, Ipp16s nlstarget, Ipp16s *pn);
void VQ_target_vec_norm(Ipp16s linearGain, Ipp16s scaleGain, Ipp16s* target, Ipp16s* nlstarget);
void Excitation_VQ_and_gain_scaling(Ipp16s linearGain, const Ipp16s* tbl, Ipp16s* et);

/* Adapter functions*/
void WeightingFilterCoeffsCalc(Ipp16s *pTmpWghtFltrCoeffs, Ipp16s coeffsScale, Ipp16s *pVecWghtFltrCoeffs);
void LoggainLinearPrediction(Ipp16s *pVecLGPredictorCoeffs, Ipp16s *pLGPredictorState, Ipp32s *pGainLog);
void InverseLogarithmCalc(Ipp32s gainLog, Ipp16s *pLinearGain, Ipp16s *pScaleGain);
Ipp16s LoggainAdderLimiter(Ipp32s pGainLog, Ipp16s gainIdx, Ipp16s shapeIdx,
                           const Ipp16s *pCodebookGain);
void BandwidthExpansionModul(const Ipp16s* pConstBroadenVector, Ipp16s* pTmpCoeffs, Ipp16s coeffsScale, 
                             Ipp16s* pCoeffs, int len);
void BandwidthExpansionModulFE(const Ipp16s* pConstBroadenVector, Ipp16s* pTmpCoeffs, Ipp16s coeffsScale, 
                               Ipp16s* pCoeffs, int len, short countFE, short illCond);
/* Postfilter functions*/
void AbsSum_G728_16s32s(Ipp16s* pSrc, Ipp32s* pSum);
void ScaleFactorCalc(Ipp32s sumunfil, Ipp32s sumfil, Ipp16s *scale, Ipp16s *nlsscale);
void FirstOrderLowpassFilter_OuputGainScaling(Ipp16s scale, Ipp16s nlsscale, Ipp16s *scalefil,
                                       Ipp16s *temp, Ipp16s *spf);

void STPCoeffsCalc(Ipp16s *pLPCFltrCoeffs, Ipp16s scaleLPCFltrCoeffs, 
                   Ipp16s *pstA, Ipp16s rc1, Ipp16s *tiltz);

/*Pitch search functions*/
void LTPCoeffsCalc(const Ipp16s *sst, Ipp32s kp, Ipp16s *gl, Ipp16s *glb, Ipp16s *pTab);

void Set_Flags_and_Scalin_Factor_for_Frame_Erasure(Ipp16s fecount, Ipp16s ptab, Ipp16s kp, Ipp16s* fedelay, Ipp16s* fescale, 
                                                   Ipp16s* nlsfescale, Ipp16s* voiced, Ipp16s* etpast, 
                                                   Ipp16s *avmag, Ipp16s *nlsavmag);
void Log_gain_Limiter_after_erasure(Ipp32s *pGainLog, Ipp32s ogaindb, Ipp16s afterfe);
void ExcitationSignalExtrapolation(Ipp16s voiced, Ipp16s *fedelay, Ipp16s fescale, Ipp16s nlsfescale, Ipp16s* etpast, 
                                     Ipp16s *et, Ipp16s *nlset, Ipp16s *seed);
void UpdateExcitationSignal(Ipp16s *etpast, Ipp16s *et, Ipp16s nlset);
void UpdateLGPredictorState(Ipp16s *et, Ipp16s *pLGPredictorState);


typedef struct {
   Ipp32u        objSize;
   Ipp32s        key;
   G728_Rate     rate ;      /* rate*/
   G728_Type     type;  /* Type of decoder;*/
}own_G728_Obj_t;

struct _G728Encoder_Obj {
   Ipp16s   vecSyntFltrCoeffs[LPC      +6]; /*a*/ 
   Ipp16s   tmpSyntFltrCoeffs[LPC      +6]; /*atmp*/ 
   Ipp16s   vecWghtFltrCoeffs[2*LPCW   +4]; /* awp + awz */ 
   Ipp16s   tmpWghtFltrCoeffs[LPCW     +6]; /* awztmp */ 
   Ipp16s   vecLGPredictorCoeffs[LPCLG +6]; /*gp*/ 
   Ipp16s   tmpLGPredictorCoeffs[LPCLG +6]; /*gptmp*/ 
   Ipp16s   vecLGPredictorState[LPCLG   +6];/*gstate*/ 
   Ipp16s   y2[NCWD        +0];
   Ipp16s   h[IDIM         +3];
   Ipp16s   st[IDIM        +3];
   Ipp16s   sttmp[4*IDIM   +4];
   Ipp16s   nlssttmp[4     +4];
   Ipp16s   stmp[4*IDIM    +4];
   Ipp16s   r[11           +5];
   Ipp16s   rtmp[LPC+1     +5];


   own_G728_Obj_t    objPrm;
   const Ipp16s *pGq;
   const Ipp16s *pNgq;
   const Ipp16s *pCodebookGain;
   IppsIIRState_G728_16s            *wgtMem;
   IppsWinHybridState_G728_16s      *rexpMem;
   IppsWinHybridState_G728_16s      *rexpwMem;
   IppsWinHybridState_G728_16s      *rexplgMem;
   IppsCombinedFilterState_G728_16s *combMem;
   Ipp16s        illcond;
   Ipp16s        illcondw;
   Ipp16s        illcondg;
   Ipp16s        illcondp;
   Ipp16s        scaleWghtFltrCoeffs; /* nlsawztmp */ 
   Ipp16s        scaleSyntFltrCoeffs; /*nlsatmp*/ 
   Ipp16s        nlsst;
   Ipp16s        scaleLGPredictorCoeffs;/*nlsgptmp*/ 
   Ipp16s        icount;
};

struct _G728Decoder_Obj { 
   Ipp16s   vecSyntFltrCoeffs[LPC                   +6]; /*a*/ 
   Ipp16s   tmpSyntFltrCoeffs[LPC                +6]; /*atmp*/ 
   Ipp16s   vecLGPredictorCoeffs[LPCLG                +6];/*gp*/ 
   Ipp16s   tmpLGPredictorCoeffs[LPCLG             +6];/*gptmp*/ 
   Ipp16s   vecLGPredictorState[LPCLG            +6];/*gstate*/ 
   Ipp16s   vecLPCFltrCoeffs[10                  +6];/*apf*/ 
   Ipp16s   pstA[20                 +4];
   Ipp16s   d_buff[239+2            +7];
   Ipp16s   sst_buff[239+IDIM+1+2   +1];
   Ipp16s   sttmp[4*IDIM            +4];
   Ipp16s   nlssttmp[4              +4];
   Ipp16s   etpast_buff[140+IDIM    +7];/* G728I coder parameter*/ 

   own_G728_Obj_t    objPrm;
   const Ipp16s *pGq;
   const Ipp16s *pNgq;
   const Ipp16s *pCodebookGain;
   IppsSynthesisFilterState_G728_16s   *syntState;
   IppsPostFilterState_G728_16s        *stpMem;
   IppsWinHybridState_G728_16s         *rexpMem;  
   IppsWinHybridState_G728_16s         *rexplgMem;
   IppsPostFilterAdapterState_G728     *postFltAdapterMem;
   Ipp16s        pst;
   Ipp16s        illcond;
   Ipp16s        illcondg;
   Ipp16s        illcondp;
   Ipp16s        scaleSyntFltrCoeffs;/*nlsatmp*/ 
   Ipp16s        scaleLGPredictorCoeffs;/*nlsgptmp*/ 
   Ipp16s        scaleLPCFltrCoeffs;/*nlsapf*/ 
   Ipp16s        rc1;
   Ipp16s        ip;
   Ipp32s        kp1;
   Ipp16s        gl;
   Ipp16s        glb;
   Ipp16s        scalefil;
   /* G728I coder parameters*/ 
   Ipp16s        ferror;                /* Frame erasure indicator*/ 
   Ipp16s        adcount;               /* G728 adaptation cycle counter (2.5 ms)*/ 
   Ipp16s        afterfe;               /* Counter for gain clipping after FE (2.5 ms)*/ 
   Ipp16s        fecount;               /* Lenght of current frame erasure (2.5 ms)*/ 
   Ipp16s        fedelay;               /* Pitch (KP) stored at beginning of FE*/ 
   Ipp16s        fescale;               /* etpast scaling factor for frame erasures*/ 
   Ipp16s        nlsfescale;            /* NLS for fescale*/ 
   Ipp32s        ogaindb;               /* Old prediction gain in dB*/ 
   Ipp16s        voiced;                /* Voiced/Unvoiced flag*/ 
   Ipp16s        avmag;                 /* Average magnitude of past 8 excitation vector*/ 
   Ipp16s        nlsavmag;              /* NLS for avmag*/ 
   Ipp16s        seed;
   Ipp16s        pTab;
};

/* basics operations */ 

__INLINE short ExtractHigh(int x);
__INLINE short Abs_16s(short x);
__INLINE int Abs_32s(int x);
__INLINE short Negate_16s(short x);
__INLINE int Negate_32s(int x);
__INLINE short Cnvrt_32s16s(int x);
__INLINE short Cnvrt_NR_32s16s(int x);
__INLINE int Cnvrt_64s32s(__INT64 z);
__INLINE int Sub_32s(int x,int y);
__INLINE short Sub_16s(short x, short y);
__INLINE short Add_16s(short x, short y);
__INLINE int Add_32s(int x, int y);
__INLINE short ShiftL_16s(short var1, short var2);
__INLINE int ShiftL_32s(int x, short n);

__INLINE short ExtractHigh(int x){
   return ((short)(x >> 16));
}
__INLINE short Abs_16s(short x){
   if(x<0){
      if(IPP_MIN_16S == x) return IPP_MAX_16S;
      x = (short)-x;
   }
   return x;
}
__INLINE int Abs_32s(int x){
   if(x<0){
      if(IPP_MIN_32S == x) return IPP_MAX_32S;
      x = -x;
   }
   return x;
}
__INLINE short Cnvrt_32s16s(int x){ 
   if (IPP_MAX_16S < x) return IPP_MAX_16S;
   else if (IPP_MIN_16S > x) return IPP_MIN_16S;
   return (short)(x);
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
__INLINE int Sub_32s(int x,int y){ 
   return Cnvrt_64s32s((__INT64)x - y);
}
__INLINE short Sub_16s(short x, short y) { 
   int z = x - y; 
   if (z>IPP_MAX_16S) return IPP_MAX_16S;
   else if(z<IPP_MIN_16S)return IPP_MIN_16S;
   return z;
}
__INLINE short Add_16s(short x, short y) { 
   int z = x + y; 
   if (z>IPP_MAX_16S) return IPP_MAX_16S;
   else if(z<IPP_MIN_16S)return IPP_MIN_16S;
   return z;
}
__INLINE int Add_32s(int x, int y) { 
   Ipp64s z = (Ipp64s)x + y; 
   if (z>IPP_MAX_32S) return IPP_MAX_32S;
   else if(z<IPP_MIN_32S)return IPP_MIN_32S;
   return (int)z;
}
__INLINE short Negate_16s(short x) {
   if(IPP_MIN_16S == x)
      return IPP_MAX_16S;
   return (short)-x;
}
__INLINE int Negate_32s(int x) {
   if(IPP_MIN_32S == x)
      return IPP_MAX_32S;
   return (int)-x;
}
__INLINE short ShiftL_16s(short x, short n){
   for(; n>0; n--){
      if (x > (short)0x3FFF) return (IPP_MAX_16S);
      else if (x < (short)0xC000) return (IPP_MIN_16S);
      else x *= 2;
   }
   return (x);
}
__INLINE int ShiftL_32s(int x, short n){
   for(; n>0; n--){
      if (x > (int)0x3FFFFFFF) return (IPP_MAX_32S);
      else if (x < (int)0xC0000000) return (IPP_MIN_32S);
      else x *= 2;
   }
   return (x);
}
#endif /* _OWN_G728_H_*/
