/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2001-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.729 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.729/A/B/D/E are international standards promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.729 speech codec main own header file
//
*/

#ifndef __OWNCODEC_H__
#define __OWNCODEC_H__
#include <stdlib.h>
#include <ipps.h>
#include <ippsc.h>
#include "g729api.h"

#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
    #define __INLINE static __inline
#elif defined( __GNUC__ )
    #define __INLINE static __inline__
#else
    #define __INLINE static
#endif
#include "scratchmem.h"

#define ENC_KEY 0xecd729
#define DEC_KEY 0xdec729

#define G729_ENCODER_SCRATCH_MEMORY_SIZE (5120+40)
#define SEC_STAGE_BITS      5
#define FIR_STAGE_BITS      7  
#define PARM_DIM            11
#define FIR_STAGE           (1<<FIR_STAGE_BITS)
#define SPEECH_BUF_DIM      240 
#define SEC_STAGE           (1<<SEC_STAGE_BITS)
#define LP_LOOK_AHEAD       40
#define LP_WINDOW_DIM       240     
#define LP_SUBFRAME_DIM     40      
#define LP_FRAME_DIM        80      
#define LPC_WINDOW          (speechHistory + SPEECH_BUF_DIM - LP_WINDOW_DIM)
#define PRESENT_SPEECH      (speechHistory + SPEECH_BUF_DIM - LP_FRAME_DIM - LP_LOOK_AHEAD)
#define MIN_PITCH_LAG       20      
#define LPF_DIM             10      /*LP filter order */
#define INTERPOLATION_FILTER_DIM   (10+1)
#define MAX_PITCH_LAG       143     
#define L_prevExcitat       (MAX_PITCH_LAG+INTERPOLATION_FILTER_DIM) 
#define PITCH_SHARP_MIN     3277      /*pitch sharpening min = 0.2 */
#define BWF2                22938     /*bandwitdh = 0.7 */
#define PITCH_SHARP_MAX     13017     /*pitch sharpening max = 0.8 */
#define COEFF1              29491
#define CDBK1_BIT_NUM       3
#define CDBK2_BIT_NUM       4 
#define CDBK1_DIM           (1<<CDBK1_BIT_NUM) 
#define LSF_MIN             40         /*lsf min = 0.005 */
#define CDBK2_DIM           (1<<CDBK2_BIT_NUM) 
#define LSP_MA_ORDER        4          /*MA moving average */
#define BW_EXP_FACT         321        /*bandwidth expansion factor */
#define LSF_MAX             25681      /*lsf max = 3.135 */
#define BWF2_PST            18022      /*weighting factor */
#define PITCH_GAIN_MAX      15564      /*for improving */
#define IMP_RESP_LEN        20
#define BWF1_PST            BWF2       /*weighting factor */
#define CORR_DIM            616
#define RES_FIL_DIM         (MAX_PITCH_LAG + 1 + 8)
#define SID_FRAME_MIN       3
#define SEED_INIT           11111
#define ACF_NOW             2
#define ACF_TOTAL           3 
#define GAIN0               28672
#define GAIN1               (-IPP_MIN_16S-GAIN0)
#define GAIN_NUM            2 
#define TOTAL_ACF_DIM       (ACF_TOTAL *(LPF_DIM+1)) 
#define VAD_LPC_DIM         12
#define ACF_DIM             (ACF_NOW *(LPF_DIM+1))
#define SIX_PI              19302

#define BWLPCF_DIM          30         /*backward LP filter order*/
#define SYNTH_BWD_DIM       BWLPCF_DIM + 35
#define BWLPCF1_DIM         (BWLPCF_DIM+1) 
#define TBWD_DIM            LP_FRAME_DIM + SYNTH_BWD_DIM
#define N0_98               32113
#define BWF2_PST_E          21300      /*Numerator wgt factor */
#define BWF1_PST_E          BWF2       /*Denominator wgt factor 0.70*/
#define IMP_RESP_LEN_E      32         /*Imp.resp. len */
#define BWF_HARMONIC_E      (1<<13)
#define BWF_HARMONIC        (1<<14)

#define CDBK1_BIT_NUM_6K    3 
#define CDBK2_BIT_NUM_6K    3 
#define CDBK1_DIM_6K        (1<<CDBK1_BIT_NUM_6K) 
#define CDBK2_DIM_6K        (1<<CDBK2_BIT_NUM_6K) 
#define CNG_STACK_SIZE      (1<<5)
#define G729_CODECFUN(type,name,arg) extern type name arg

typedef struct _SynthesisFilterState {
    int    nTaps;
    short *buffer;
}SynthesisFilterState;
typedef struct _G729Coder_Obj {
    int                 objSize;
    int                 key;
    unsigned int        mode;          
    G729Codec_Type      codecType;
}G729Coder_Obj;

#if !defined (NO_SCRATCH_MEMORY_USED)
struct _ScratchMem_Obj {
    char *base;
    char *CurPtr;
    int  *VecPtr;
    int   offset;
};
#endif

typedef struct _VADmemory {
    short    LSFMean[LPF_DIM];
    short    minBuf[16];
    short    musicRC[10];
    short    VADPrev;   
    short    VADPPrev;
    short    minPrev;          
    short    minNext;
    short    minVAD;
    short    EMean;
    short    SEMean;
    short    SLEMean; 
    short    SZCMean;
    short    VADPrevEnergy;
    short    SILcounter;
    short    updateCounter;
    short    extCounter;
    short    VADflag;
    short    VADflag2;
    short    lessCounter;  
    short    frameCounter;
    short    musicCounter;
    short    musicSEMean;
    short    musicMCounter;
    short    conscCounter;
    short    conscCounterFlagP;
    short    MeanPgain;
    short    count_pflag;
    short    conscCounterFlagR;
    short    Mcount_pflag;

}VADmemory;

struct _G729Decoder_Obj {
    G729Coder_Obj objPrm;
#if !defined (NO_SCRATCH_MEMORY_USED)
    ScratchMem_Obj Mem;
#endif
    short    resFilBuf1[RES_FIL_DIM+LP_SUBFRAME_DIM];
    short    prevLSPfreq[LSP_MA_ORDER][LPF_DIM];
    short    LTPostFilt[TBWD_DIM];
    short    prevExcitat[LP_FRAME_DIM+L_prevExcitat];
    short    prevSubfrLSP[LPF_DIM]; 
    short    prevSubfrLSPquant[LPF_DIM]; 
    short    zeroPostFiltVec1[LPF_DIM+BWLPCF_DIM+2]; 
    short    decPrm[20];            /*analysis data pointer */
    int      coderErr[4];           /*memory for improving */
    short    pBwdLPC[BWLPCF1_DIM];
    short    pBwdLPC2[BWLPCF1_DIM];
    short    pPrevBwdLPC[BWLPCF1_DIM];
    int      hwState[BWLPCF1_DIM];
    short    pPrevFilt[BWLPCF1_DIM]; /* selected previously filter */
    G729Codec_Type codecType;
    short    preemphFilt; 
    short    seed;       
    short    voiceFlag;
    short    gainExact;             /*gain's precision */     
    short    betaPreFilter;         /*quant adaptive codebook gain from the previous subframe */
    short    gainNow;
    short    sidGain;
    short    seedSavage; 
    short    CNGvar; 
    short    SIDflag0;
    short    lspSID[LPF_DIM];
    short    SIDflag1;
    short    CNGidx;                /*CNG cache parameters  */
    char     *postProc;             /*High pass post processing filter memory */  
    char     *synFltw;              /*Synthesis filter memory */
    char     *synFltw0;             /*Synthesis filter memory */
    int      mode;                  /*decoder mode's */
    short    gains[2];              /*pitch + vcodebook gains */
    short    prevFrameDelay;       
    short    prevMA;                /*previous MA prediction coef.*/
    short    prevFrameQuantEn[4]; 
    short    prevVoiceFlag; 
    short    prevBFI; 
    short    prevLPmode;
    short    interpCoeff2;
    short    interpCoeff2_2;
    short    valGainAttenuation;
    short    BFIcount;
    char     *PhDispMem;
    short    pPrevBwdRC[2];
    short    BWDFrameCounter;
    short    stat_pitch;            /*pitch stationarity */
    short    prevFrameDelay2;       /*previous frame delay */
    short    pitchStatIntDelay;
    short    pitchStatFracDelay;
    short    prevPitch;
    short    gammaPost1;
    short    gammaPost2;
    short    gammaHarm;
    short    BWDcounter2;
    short    FWDcounter2;
};
struct _G729Encoder_Obj {
    G729Coder_Obj       objPrm;
#if !defined (NO_SCRATCH_MEMORY_USED)
    ScratchMem_Obj      Mem;
#endif
    short    encSyn[LP_FRAME_DIM];/*encodersynthesisbuffer*/
    short    speechHistory[SPEECH_BUF_DIM];
    short    prevLSPfreq[LSP_MA_ORDER][LPF_DIM];
    short    energySfs[GAIN_NUM];/*energyscalefactors*/
    short    pACF[ACF_DIM];
    short    ACFsum[TOTAL_ACF_DIM];
    short    BWDsynth[TBWD_DIM];
    short    energy[GAIN_NUM];
    short    prevFrameQuantEn[4];/*quantizedenergyforpreviousframes*/
    short    resFilMem0[BWLPCF_DIM];
    short    resFilMem[BWLPCF_DIM+LP_SUBFRAME_DIM];
    short    quantLspSID[LPF_DIM];
    short    prevSubfrLSP[LPF_DIM];
    short    prevSubfrLSPquant[LPF_DIM];
    int      hwState[BWLPCF1_DIM];
    short    ACFsfs[ACF_NOW];
    short    ACFsumSfs[ACF_TOTAL];
    short    pPrevFilt[BWLPCF1_DIM];
    short    pPrevBwdLPC[BWLPCF1_DIM];
    short    pBwdLPC2[BWLPCF1_DIM];
    short    betaPreFilter;/*quantadaptivecodebookgainfromtheprevioussubframe*/
    short    prevWgtSpeech[LP_FRAME_DIM+MAX_PITCH_LAG+1];
    short    prevExcitat[LP_FRAME_DIM+L_prevExcitat+2];
    short    encPrm[19];
    short    zeroPostFiltVec1[LP_SUBFRAME_DIM+BWLPCF_DIM+1];/*zeroextendedimpulseresponse*/
    short    prevCoeff[LPF_DIM+1];
    short    prevSubfrLPC[LPF_DIM+1];
    short    reflC[LPF_DIM+1];
    G729Codec_Type codecType;
    short    ACnorm;
    short    ACFcounter;
    short    gainNow;
    short    energyDim;/*energiesnumber*/
    short    SIDframeCounter;
    short    prevSubfrSmooth;/*perceptualweighting*/
    short    sidGain;
    short    speechDiff;
    short    prevLPmode;
    short    *pSynth;
    short    pPrevBwdRC[2];
    short    prevLAR[2];/*previoussubframelogarearatio*/
    short    prevDTXEnergy;
    short    seed;
    short    CNGidx;/*CNGcacheparameters*/
    char     *preProc;/*highpasspreprocessingfiltermemory*/
    char     *synFltw;/*synthesisfilter1memory*/
    char     *synFltw0;/*synthesisfilter2memory*/
    char     *vadMem;/*VADmemory*/
    int      mode;/*mode's*/
    short    extraTime;/*fixedcodebooksearchextratime*/
    short    prevRC[2];
    int      coderErr[4];
    short    dominantBWDmode;
    short    interpCoeff2_2;
    short    statGlobal;/*globalstationaritymesure*/
    short    pLag[5];
    short    pGain[5];
    short    BWDFrameCounter;/*consecutivebackwardframesNbre*/
    short    val_BWDFrameCounter;/*BWDFrameCounterassociated*/
    short    BWDcounter2;
    short    FWDcounter2;
};

int  ExtractBitsG729(const unsigned char **pSrc, int *len, int Count );
void NoiseExcitationFactorization_G729B_16s(const Ipp16s *pSrc,Ipp32s val1,
                                            Ipp16s val2, Ipp16s *pDst, int len);
int ComfortNoiseExcitation_G729B_16s_I(const Ipp16s *pSrc, const Ipp16s *pPos,
                                       const Ipp16s *pSign, Ipp16s val, Ipp16s t,
                                       Ipp16s *pSrcDst, Ipp16s *t2, short *Sfs);
void RandomCodebookParm_G729B_16s(Ipp16s *pSrc1, Ipp16s *pSrc2, Ipp16s *pSrc3, 
                                  Ipp16s *pSrc4, short *n);
void QuantSIDGain_G729B_16s(const Ipp16s *pSrc, const Ipp16s *pSrcSfs, 
                            int len, Ipp16s *p, int *pIdx); 
void Sum_G729_16s_Sfs(const Ipp16s *pSrc, const Ipp16s *pSrcSfs,
                      Ipp16s *pDst, Ipp16s *pDstSfs, int len, int*pSumMem);
void VoiceActivityDetectSize_G729(int *pSrc);
void VoiceActivityDetectInit_G729(char *pSrc);
void VoiceActivityDetect_G729(Ipp16s *pSrc,Ipp16s *pLSF,Ipp32s *pAutoCorr,
                              Ipp16s autoExp, Ipp16s rc, Ipp16s *pVad, char*pVADmem,short *pTmp);
void VADMusicDetection( G729Codec_Type codecType, int Val, short expVal, short *rc,
                        short *lags, short *pgains, short stat_flg,
                        short *Vad, char*pVADmem);
IppStatus SynthesisFilter_G729_16s (const Ipp16s *pLPC, const Ipp16s *pSrc, 
                                    Ipp16s *pDst, int len, char *pMemUpdated, int HistLen);
IppStatus SynthesisFilter_G729_16s_update (const Ipp16s *pLPC, const Ipp16s *pSrc, 
                                           Ipp16s *pDst, int len, char *pMemUpdated, int hLen,
                                           int update);
void SynthesisFilterOvf_G729_16s_I(const Ipp16s *pLPC, Ipp16s *pSrcDst, 
                                   int len, char *pMemUpdated, int HistLen);
void SynthesisFilterOvf_G729_16s (const Ipp16s *pLPC, const Ipp16s *pSrc, 
                                  Ipp16s *pDst, int len, char *pMemUpdated, int HistLen);
void SynthesisFilterInit_G729 (char *pMemUpdated);
void SynthesisFilterSize_G729 (int *pSize);
void CodewordImpConv_G729(int index, const short *pSrc1,const short *pSrc2,short *pDst);
void _ippsRCToLAR_G729_16s (const Ipp16s*pSrc, Ipp16s*pDst, int len);
void _ippsPWGammaFactor_G729_16s (const Ipp16s*pLAR, const Ipp16s*pLSF, 
                                   Ipp16s *flat, Ipp16s*pGamma1, Ipp16s*pGamma2, Ipp16s *pMem );
void CNG_encoder(short *exc, short *prevSubfrLSPquant, short *Aq, short *ana, G729Encoder_Obj *encoderObj);
void CNG_Update(short *pSrc, short val, short vad, G729Encoder_Obj *encoderObj);
void Post_G729(short idx, short id, const short *LPC, short *pDst, short *voiceFlag, G729Decoder_Obj *decoderObj);
void Post_G729AB(short idx, short id, const short *LPC, short *pDst, short vad, G729Decoder_Obj *decoderObj);
void Post_G729I(short idx, short id, const short  *LPC, short *pDst, short *val, short val1, short val2,
                short fType, G729Decoder_Obj *decoderObj);
void Post_G729Base(short idx, short id, const short *LPC, short *pDst, short *voiceFlag, short ftyp, G729Decoder_Obj *decoderObj);
void updateExcErr_G729(short x, int y, int *err);
short calcErr_G729(int val, int *pSrc);
void BWDLagWindow(int *pSrc, int *pDst);
void SetLPCMode_G729E(short *signal_ptr, short *aFwd, short *pBwdLPC,
                      short *lpMode, short *lspnew, short *lspold,
                      G729Encoder_Obj *encoderObj);
void PitchTracking_G729E(short *val1, short *val2, short *prevPitch,
                         short *pitchStat, short *pitchStatIntDelay,  short *pitchStatFracDelay);
short enerDB(short *synth, short L);
void tstDominantBWDmode(short *BWDcounter2,short *FWDcounter2,short *highStat, short mode);
void Init_CNG_encoder(G729Encoder_Obj *encoderObj);
void Log2_G729(int val, short *pDst1, short *pDst2);

extern const short  gammaFac1[2*(LPF_DIM+1)];
extern const short  g729gammaFac1_pst[LPF_DIM+1];
extern const short  cngSeedOut[CNG_STACK_SIZE];
extern const short  g729gammaFac2_pst[LPF_DIM+1];
extern const short  cngCache[CNG_STACK_SIZE][LP_SUBFRAME_DIM];
extern const short  LUT1[CDBK1_DIM];
extern const short  presetLSP[LPF_DIM];
extern const int    cngInvSqrt[CNG_STACK_SIZE];
extern const short  resetPrevLSP[LPF_DIM];
extern const short  LUT2[CDBK2_DIM];
extern const short  presetOldA[LPF_DIM+1];
extern short        areas[L_prevExcitat-1+3];
extern short        SIDgain[32];
extern const short  NormTable[256];
extern const short  NormTable2[256];

__INLINE short Rand_16s(short *seed){
    *seed = *seed *31821 + 13849;
    return(*seed);
}
__INLINE short ShiftR_NR_16s(short n, unsigned short x) {
    short const1 = (x==0)? 0: (1<<(x-1));
    return(n+const1)>>x;
}
__INLINE short ShiftL_16s(short x, unsigned short n)
{
   int max = IPP_MAX_16S >> n;
   int min = IPP_MIN_16S >> n;
   if(x > max) return IPP_MAX_16S;
   else if(x < min) return IPP_MIN_16S;
   return (short)(x<<n);
}
__INLINE int ShiftL_32s(int x, unsigned short n)
{
   int max = IPP_MAX_32S >> n;
   int min = IPP_MIN_32S >> n;
   if(x > max) return IPP_MAX_32S;
   else if(x < min) return IPP_MIN_32S;
   return (x<<n);
}
__INLINE int ownSqrt_32s(int val) {
    int   i, j;
    short n1=0,n2=BWF_HARMONIC;
    for( i = 0 ; i < 14 ; i ++ ) {
        j = (n1 + n2);
        j *=j;
        if( val >= j )
            n1 += n2 ;
        n2 >>= 1 ;
    }
    return n1;
}
__INLINE short Cnvrt_32s16s(int val) {
    if(IPP_MAX_16S < val)
        return IPP_MAX_16S;
    else
        if(IPP_MIN_16S > val)
        return IPP_MIN_16S;
    return(short)(val);
} 
__INLINE int Cnvrt_64s32s(__INT64 val) {
    if(IPP_MAX_32S < val)
        return IPP_MAX_32S;
    else
        if(IPP_MIN_32S > val)
        return IPP_MIN_32S;
    return(int)val;
}
__INLINE short Abs_16s(short val) {
    if(val<0) {
        if(IPP_MIN_16S == val)
            return IPP_MAX_16S;
        val = -val;
    }
    return val;
}
__INLINE int Sub_32s(int val1,int val2) {
    return Cnvrt_64s32s((__INT64)val1 - val2);
}
__INLINE short Sub_16s(int val1,int val2) {
    int tmp = val1 - val2; 
    if(tmp>IPP_MAX_16S)
        return IPP_MAX_16S;
    else
        if(tmp<IPP_MIN_16S)
        return IPP_MIN_16S;
    return tmp;
}
__INLINE short Add_16s(short val1, short val2) {
    int tmp = val1 + val2; 
    if(tmp>IPP_MAX_16S)
        return IPP_MAX_16S;
    else
        if(tmp<IPP_MIN_16S)
        return IPP_MIN_16S;
    return tmp;
}
__INLINE int Add_32s(int val1,int val2) {
    Ipp64s tmp = (Ipp64s)val1 + val2; 
    if(tmp>IPP_MAX_32S)
        return IPP_MAX_32S;
    else
        if(tmp<IPP_MIN_32S)
        return IPP_MIN_32S;
    return(int)tmp;
}
__INLINE int Round_32s(int val) {
    int tmp = Add_32s(val,0x8000);
    return(tmp >> 16);
}
__INLINE short Exp_16s_Pos(unsigned short x) 
{
   if((x>>8)==0)
      return NormTable2[x];
   else {
      return NormTable[(x>>8)];
   }
}
__INLINE short Exp_32s_Pos(unsigned int x){
   if (x == 0) return 0;
   if((x>>16)==0) return (16+Exp_16s_Pos((unsigned short) x));
   return Exp_16s_Pos((unsigned short)(x>>16));
}
__INLINE short Norm_32s_I(int *x){
   short i;
   int y = *x;
   if (y == 0) return 0;
   if (y == -1) {*x =IPP_MIN_32S;return 31;}
   if (y < 0) y = ~y;
   i = Exp_32s_Pos(y);
   *x <<= i;
   return i;
}
__INLINE short Norm_32s16s(int x) {
   short i;
   int y =  x;
   if (y == 0) return 0;
   if (y == -1) { return 31;}
   if (y < 0) y = ~y;
   i = Exp_32s_Pos(y);
   return i;
}
__INLINE short Exp_16s(short val) {
    short i;
    if(val == 0)
        return 0;
    if((short)IPP_MAX_16U == val)
        return 15;
    if(val < 0)
        val = ~val;
    for(i = 0; val < (short)BWF_HARMONIC; i++)
        val <<= 1;
    return i;
}
__INLINE int Mul2_32s(int val) {
    if(val > 0x3fffffff)
        return IPP_MAX_32S;
    else
        if( val < 0xc0000000)
        return IPP_MIN_32S;
    return(val <<= 1);
}
__INLINE short Negate_16s(short val) {
    if(IPP_MIN_16S == val)
        return IPP_MAX_16S;
    return(short)-val;
}
__INLINE int equality( int val) {
    int temp, i, bit;
    int sum;
    sum = 1;
    temp = val >> 1;
    for(i = 0; i <6; i++) {
        temp >>= 1;
        bit = temp & 1;
        sum += bit;
    }
    sum &= 1;
    return sum;
}
static const short table0[8]={
    1*1, 2*1, 0*1,
    1*1, 2*1, 0*1,
    1*1, 2*1};
__INLINE void DecodeAdaptCodebookDelays(short *prevFrameDelay, short *prevFrameDelay2,short *delay,
                                        int id, int bad_pitch,int pitchIndx,G729Codec_Type type) {
    short minPitchSearchDelay, maxPitchSearchDelay;

    if(bad_pitch == 0) {
        if(id == 0) {                  /*if 1-st subframe */
            if(pitchIndx < 197) {
                delay[0] = (pitchIndx+2)/3 + 19;
                delay[1] = pitchIndx - delay[0] *3 + 58;
            } else {
                delay[0] = pitchIndx - 112;
                delay[1] = 0;
            } 

        } else {
            /*find minPitchSearchDelay and maxPitchSearchDelay for 2-nd subframe */
            minPitchSearchDelay = delay[0] - 5;
            if(minPitchSearchDelay < MIN_PITCH_LAG)
                minPitchSearchDelay = MIN_PITCH_LAG;

            maxPitchSearchDelay = minPitchSearchDelay + 9;
            if(maxPitchSearchDelay > MAX_PITCH_LAG) {
                maxPitchSearchDelay = MAX_PITCH_LAG;
                minPitchSearchDelay = MAX_PITCH_LAG - 9;
            }
            if(type == G729D_CODEC /* i.e. 6.4 kbps */) {
                pitchIndx = pitchIndx & 15;
                if(pitchIndx <= 3) {
                    delay[0] = minPitchSearchDelay + pitchIndx;
                    delay[1] = 0;
                } else if(pitchIndx < 12) {
                    delay[1] = table0[pitchIndx - 4];
                    delay[0] = (pitchIndx - delay[1])/3 + 2 + minPitchSearchDelay;

                    if(delay[1] == 2) {
                        delay[1] = -1;
                        delay[0]++;
                    }
                } else {
                    delay[0] = minPitchSearchDelay + pitchIndx - 6;
                    delay[1] = 0;
                }
            } else {
                delay[0] = minPitchSearchDelay + (pitchIndx + 2)/3 - 1;
                delay[1] = pitchIndx - 2 - 3 *((pitchIndx + 2)/3 - 1);
            } 
        }
        *prevFrameDelay = delay[0];
        *prevFrameDelay2 = delay[1];
    } else {                     /* non-equal or bad frame*/
        delay[0]  =  *prevFrameDelay;
        if(type == G729E_CODEC) {
            delay[1] = *prevFrameDelay2;
        } else {
            delay[1] = 0;
            *prevFrameDelay += 1;
            if(*prevFrameDelay > MAX_PITCH_LAG) {
                *prevFrameDelay = MAX_PITCH_LAG;
            }
        } 
    }
}
__INLINE short Mul_16s_Sfs(short x, short y, int scaleFactor) {
    return(x*y) >> scaleFactor;
}
__INLINE int Mul_32s(int val1,int val2) {
    short tmpHigh,tmpLow,tmp2High,tmp2Low;
    int   tmp,tmp1,tmp2;
    tmpHigh  = val1 >> 15;
    tmpLow   = val1 & IPP_MAX_16S;  
    tmp2High = val2 >> 15;
    tmp2Low  = val2 & IPP_MAX_16S;  
    tmp1 = Mul_16s_Sfs(tmpHigh,tmp2Low,15);
    tmp2 = Mul_16s_Sfs(tmpLow,tmp2High,15);
    tmp  = tmpHigh*tmp2High;
    tmp  += tmp1;
    tmp  += tmp2;
    return(tmp<<1);
}

#endif /*__OWNCODEC_H__*/
