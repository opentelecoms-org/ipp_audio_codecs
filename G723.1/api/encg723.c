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
//  Purpose: G.723.1 coder: encode API
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "owng723.h"

/* Declaration of local functions */
static void PastAverageFilter_G723(G723Encoder_Obj* encoderObj);
static void GetReflectionCoeff_G723(short *pSrcLPC, short *pDstReflectCoeff, short *pDstReflectCoeffSFS);
static int ItakuraDist_G723(short *pSrcReflectCoeff, short ReflectCoeffSFS, short *pSrcAutoCorrs, short energy);

static int EncoderObjSize(void){
   int fltSize;
   int objSize = sizeof(G723Encoder_Obj);
   VoiceActivityDetectSize_G723(&fltSize);
   objSize += fltSize; /* VAD decision memory size*/
   return objSize;
}

G723_CODECFUN( APIG723_Status, apiG723Encoder_Alloc, (int *pCodecSize))
{
   *pCodecSize =  EncoderObjSize();
   return APIG723_StsNoErr;
}
G723_CODECFUN( APIG723_Status, apiG723Encoder_Mode,
         (G723Encoder_Obj* encoderObj, unsigned int mode))
{  
   if (G723Encode_VAD_Enabled == mode) 
      encoderObj->objPrm.mode |= G723Encode_VAD_Enabled;
   else{
      encoderObj->objPrm.mode |= G723Encode_VAD_Enabled;
      encoderObj->objPrm.mode ^= G723Encode_VAD_Enabled;
   }

   return APIG723_StsNoErr;
}

G723_CODECFUN( APIG723_Status, apiG723Encoder_Init, 
              (G723Encoder_Obj* encoderObj, unsigned int mode))
{
   int i;

   ippsZero_16s((short*)encoderObj,sizeof(G723Encoder_Obj)>>1) ;   
   
   encoderObj->objPrm.objSize = EncoderObjSize();
   encoderObj->objPrm.mode = mode;
   encoderObj->objPrm.key = G723_ENC_KEY;
   encoderObj->objPrm.rat = 0; /* default 6.3 KBit/s*/
   
   encoderObj->vadMem  = (char*)encoderObj + sizeof(G723Encoder_Obj);

   /* Initialize encoder data structure with zeros */
   ippsZero_16s(encoderObj->ZeroSignal, G723_SBFR_LEN);
   ippsZero_16s(encoderObj->UnitImpulseSignal, G723_SBFR_LEN); encoderObj->UnitImpulseSignal[0] = 0x2000 ; /* unit impulse */

   /* Initialize the previously decoded LSP vector to the DC vector */
   ippsCopy_16s(LPCDCTbl,encoderObj->PrevLPC,G723_LPC_ORDER);

   /* Initialize the taming procedure */
   for(i=0; i<5; i++) encoderObj->ExcitationError[i] = 4;
   encoderObj->sSearchTime = 120; /* reset max time */
   /* Initialize the VAD */
   if(encoderObj->objPrm.mode & G723Encode_VAD_Enabled){
     VoiceActivityDetectInit_G723(encoderObj->vadMem);
     /* Initialize the CNG */
     encoderObj->CurrGain = 0;
     ippsZero_16s(encoderObj->AutoCorrs,AUOTOCORRS_BUFF_SIZE);

     for(i=0; i <= N_AUTOCORRS_BLOCKS; i++) encoderObj->AutoCorrsSFS[i] = 40;
     encoderObj->PrevOpenLoopLags[0] = G723_SBFR_LEN;
     encoderObj->PrevOpenLoopLags[1] = G723_SBFR_LEN;

     ippsZero_16s(encoderObj->LPCSID,G723_LPC_ORDER);
     ippsZero_16s(encoderObj->SIDLSP,G723_LPC_ORDER);
     ippsZero_16s(encoderObj->prevSidLpc,G723_LPC_ORDERP1);
     encoderObj->prevSidLpc[0] = 0x2000;
     ippsZero_16s(encoderObj->ReflectionCoeff,G723_LPC_ORDER+1);
     ippsZero_16s(encoderObj->GainAverEnergies,N_GAIN_AVER_FRMS);

     encoderObj->PastFrameType = G723_ActiveFrm;

     encoderObj->CNGSeed = 12345;
     encoderObj->CasheCounter = 0;
     encoderObj->ReflectionCoeffSFS = 0;
     encoderObj->AverEnerCounter = 0;
     encoderObj->PrevQGain = 0;
     encoderObj->sSidGain = 0;
   }

   return APIG723_StsNoErr;
}

G723_CODECFUN( APIG723_Status, apiG723Encoder_InitBuff, 
         (G723Encoder_Obj* encoderObj, char *buff))
{

#if !defined (NO_SCRATCH_MEMORY_USED)

   if(NULL==encoderObj || NULL==buff)
      return APIG723_StsBadArgErr;

   encoderObj->Mem.base = buff;
   encoderObj->Mem.CurPtr = encoderObj->Mem.base;
   encoderObj->Mem.VecPtr = (int *)(encoderObj->Mem.base+G723_ENCODER_SCRATCH_MEMORY_SIZE);

#endif 

   return APIG723_StsNoErr;
}

void EncoderCNG_G723(G723Encoder_Obj* encoderObj, ParamStream_G723 *Params, short *pExcitation, short *pDstLPC)
{
   short sQuantGain, sTmp;
   int i;

   LOCAL_ARRAY(short, curCoeff,G723_LPC_ORDER,encoderObj) ;

   for(i=N_GAIN_AVER_FRMS-1;i>=1;i--) encoderObj->GainAverEnergies[i]=encoderObj->GainAverEnergies[i-1];

   /* Calculate the LPC filter */
   ippsLevinsonDurbin_G723_16s( encoderObj->AutoCorrs, &sTmp, encoderObj->GainAverEnergies, curCoeff);

   /* if the first frame of silence => SID frame */
   if(encoderObj->PastFrameType == G723_ActiveFrm) {
      Params->FrameType = G723_SIDFrm;
      encoderObj->AverEnerCounter = 1;
      QuantSIDGain_G723_16s(encoderObj->GainAverEnergies, encoderObj->AutoCorrsSFS, encoderObj->AverEnerCounter,&i);
      sQuantGain=i;
   } else {
      encoderObj->AverEnerCounter++;
      if(encoderObj->AverEnerCounter > N_GAIN_AVER_FRMS) encoderObj->AverEnerCounter = N_GAIN_AVER_FRMS;
      QuantSIDGain_G723_16s(encoderObj->GainAverEnergies, encoderObj->AutoCorrsSFS, encoderObj->AverEnerCounter,&i);
      sQuantGain=i;

      /* Compute stationarity of current filter versus reference filter */
      if(ItakuraDist_G723(encoderObj->ReflectionCoeff, encoderObj->ReflectionCoeffSFS, encoderObj->AutoCorrs, *encoderObj->GainAverEnergies) == 0) {           
         Params->FrameType = G723_SIDFrm; /* SID frame */
      } else {
         sTmp = abs(sQuantGain-encoderObj->PrevQGain);
         if(sTmp > 3) {
            Params->FrameType = G723_SIDFrm;/* SID frame */
         } else {               
            Params->FrameType = G723_UntransmittedFrm;/* untransmitted */
         }
      } 
   } 
 
   if(Params->FrameType == G723_SIDFrm) { /* Compute SID filter */

      LOCAL_ARRAY(short, qIndex,3,encoderObj) ;

      /* Check stationarity */
        
      PastAverageFilter_G723(encoderObj);

        
      if(!encoderObj->AdaptEnableFlag) /* adaptation enabled */
         for(i=0;i<G723_LPC_ORDER;i++)
            encoderObj->prevSidLpc[i+1] = -encoderObj->LPCSID[i];
        
      GetReflectionCoeff_G723(encoderObj->LPCSID , encoderObj->ReflectionCoeff, &encoderObj->ReflectionCoeffSFS); 

      if(ItakuraDist_G723(encoderObj->ReflectionCoeff, encoderObj->ReflectionCoeffSFS, encoderObj->AutoCorrs, *encoderObj->GainAverEnergies) == 0){
         ippsCopy_16s(curCoeff,encoderObj->LPCSID,G723_LPC_ORDER);
         GetReflectionCoeff_G723(curCoeff, encoderObj->ReflectionCoeff, &encoderObj->ReflectionCoeffSFS);
      } 

      /* Compute SID frame codes */
      ippsLPCToLSF_G723_16s(encoderObj->LPCSID,encoderObj->PrevLPC,encoderObj->SIDLSP);

      ippsLSFQuant_G723_16s32s(encoderObj->SIDLSP, encoderObj->PrevLPC, &Params->lLSPIdx);
      qIndex[2] =  Params->lLSPIdx & 0xff;
      qIndex[1] =  (Params->lLSPIdx>>8) & 0xff;
      qIndex[0] =  (Params->lLSPIdx>>16) & 0xff;
      if(ippsLSFDecode_G723_16s(qIndex, encoderObj->PrevLPC,  0, encoderObj->SIDLSP) != ippStsNoErr)
         ippsCopy_16s(encoderObj->PrevLPC,encoderObj->SIDLSP,G723_LPC_ORDER);

      Params->sAmpIndex[0] = sQuantGain;
      encoderObj->PrevQGain = sQuantGain;
      DecodeSIDGain_G723_16s(encoderObj->PrevQGain,&encoderObj->sSidGain);

      LOCAL_ARRAY_FREE(short, qIndex,3,encoderObj) ;
   }  

   /* Compute new excitation */
   if(encoderObj->PastFrameType == G723_ActiveFrm) {
      encoderObj->CurrGain = encoderObj->sSidGain;
   } else {
      encoderObj->CurrGain = ( (encoderObj->CurrGain*0xE000)+
                              (encoderObj->sSidGain*0x2000) )>>16 ;
   } 
   { 

      LOCAL_ARRAY(char, buff,ComfortNoiseExcitation_G723_16s_Buff_Size,encoderObj) ;

      ComfortNoiseExcitation_G723_16s(encoderObj->CurrGain, encoderObj->PrevExcitation, pExcitation,
                    &encoderObj->CNGSeed, Params->PitchLag,Params->AdCdbkLag,(Ipp16s*)Params->AdCdbkGain, Params->currRate, buff, &encoderObj->CasheCounter);

      LOCAL_ARRAY_FREE(char, buff,ComfortNoiseExcitation_G723_16s_Buff_Size,encoderObj) ;
   } 

   LSPInterpolation(encoderObj->SIDLSP, encoderObj->PrevLPC, pDstLPC);/* Interpolate LSPs */
   ippsCopy_16s(encoderObj->SIDLSP,encoderObj->PrevLPC,G723_LPC_ORDER); /* update prev SID LPC */

   encoderObj->PastFrameType = Params->FrameType;
   LOCAL_ARRAY_FREE(short, curCoeff,G723_LPC_ORDER,encoderObj) ;
   return;
}

void UpdateAutoCorrs_G723(G723Encoder_Obj* encoderObj, const short *pSrcAutoCorrs, const short *pSrcAutoCorrsSFS)
{

   int i, lNsbfr;
   short sMinSFS, sTmp;
   short m1, m2;

   LOCAL_ARRAY(int, lSumAutoCorrs,G723_LPC_ORDER+1,encoderObj) ;

   /* Update Acf and ShAcf */
   for(i=0; i<AUOTOCORRS_BUFF_SIZE-G723_LPC_ORDER-1; i++) encoderObj->AutoCorrs[AUOTOCORRS_BUFF_SIZE-1-i] = encoderObj->AutoCorrs[AUOTOCORRS_BUFF_SIZE-G723_LPC_ORDER-2-i];

   for(i=N_AUTOCORRS_BLOCKS; i>=1; i--) encoderObj->AutoCorrsSFS[i] = encoderObj->AutoCorrsSFS[i-1];

   /* Search the min of pSrcAutoCorrsSFS */
   m1 = IPP_MIN(pSrcAutoCorrsSFS[0],pSrcAutoCorrsSFS[1]);
   m2 = IPP_MIN(pSrcAutoCorrsSFS[2],pSrcAutoCorrsSFS[3]);
   sMinSFS = (IPP_MIN(m1,m2))+14;

   /* Calculate the acfs sum */
   ippsZero_16s((short*)lSumAutoCorrs,2*G723_LPC_ORDERP1);

   for(lNsbfr=0; lNsbfr<4; lNsbfr++) {
      sTmp = sMinSFS - pSrcAutoCorrsSFS[lNsbfr];
      if(sTmp < 0) {
         sTmp = -sTmp;
         for(i=0; i <= G723_LPC_ORDER; i++) {
            lSumAutoCorrs[i] += (pSrcAutoCorrs[lNsbfr*G723_LPC_ORDERP1+i]>>sTmp);
         }
      } else {
         for(i=0; i <= G723_LPC_ORDER; i++) {
            lSumAutoCorrs[i] += (pSrcAutoCorrs[lNsbfr*G723_LPC_ORDERP1+i]<<sTmp);
         }
      }
   }
   /* Normalize */
   sTmp = Exp_32s_Pos(lSumAutoCorrs[0]);
   sTmp = 16 - sTmp; if(sTmp < 0) sTmp = 0;

   for(i=0;i<=G723_LPC_ORDER;i++) encoderObj->AutoCorrs[i]=(short)(lSumAutoCorrs[i]>>sTmp);
   encoderObj->AutoCorrsSFS[0] = sMinSFS - sTmp;

   LOCAL_ARRAY_FREE(int, lSumAutoCorrs,G723_LPC_ORDER+1,encoderObj) ;

   return;
}

void PastAverageFilter_G723(G723Encoder_Obj* encoderObj)
{
   int i, j;
   short sMinSFS, sTmp;

   LOCAL_ARRAY(int, lSumAutoCorrs,G723_LPC_ORDER+1,encoderObj) ;
   LOCAL_ARRAY(short, pCorr,G723_LPC_ORDER+1,encoderObj) ;

   /* Search ShAcf min */
   sMinSFS = IPP_MIN(encoderObj->AutoCorrsSFS[1],encoderObj->AutoCorrsSFS[2]);
   sMinSFS = (IPP_MIN(sMinSFS,encoderObj->AutoCorrsSFS[3]))+14;

   ippsZero_16s((short*)lSumAutoCorrs,2*G723_LPC_ORDERP1);

   for(i=1; i <= N_AUTOCORRS_BLOCKS; i ++) {
      sTmp = sMinSFS - encoderObj->AutoCorrsSFS[i];
      if(sTmp < 0) {
         sTmp=-sTmp;
         for(j=0; j <= G723_LPC_ORDER; j++) {
            lSumAutoCorrs[j] += (encoderObj->AutoCorrs[i*G723_LPC_ORDERP1+j]>>sTmp);
         } 
      } else {
         for(j=0; j <= G723_LPC_ORDER; j++) {
            lSumAutoCorrs[j] += (encoderObj->AutoCorrs[i*G723_LPC_ORDERP1+j]<<sTmp);
         } 
      } 
   } 

   /* Normalize */
   sTmp = Exp_32s_Pos(lSumAutoCorrs[0]);
   sTmp = 16 - sTmp;
   if(sTmp < 0) sTmp = 0;

   for(i=0; i<G723_LPC_ORDER+1; i++) {
      pCorr[i] = (short)(lSumAutoCorrs[i]>>sTmp);
   }

   ippsLevinsonDurbin_G723_16s(pCorr, &sTmp, &sTmp, encoderObj->LPCSID);

   LOCAL_ARRAY_FREE(short, pCorr,G723_LPC_ORDER+1,encoderObj) ;
   LOCAL_ARRAY_FREE(int, lSumAutoCorrs,G723_LPC_ORDER+1,encoderObj) ;

   return;
}

void GetReflectionCoeff_G723(short *pSrcLPC, short *pDstReflectCoeff, short *pDstReflectCoeffSFS)
{
   int i, j;
   short SFS;
   int lCorr;

   ippsDotProd_16s32s_Sfs(pSrcLPC,pSrcLPC,G723_LPC_ORDER,&lCorr,-1);
   lCorr = lCorr >> 1;
   lCorr = lCorr + 0x04000000; 
   SFS = Exp_32s_Pos(lCorr) - 2;
   *pDstReflectCoeffSFS = SFS;
   if(SFS > 0) {
      lCorr = ShiftL_32s(lCorr, SFS); 
      pDstReflectCoeff[0] = Cnvrt_NR_32s16s(lCorr);

      for(i=1; i<=G723_LPC_ORDER; i++) {
         lCorr = -(pSrcLPC[i-1]<<13);
         for(j=0; j<G723_LPC_ORDER-i; j++) {
            lCorr = Add_32s(lCorr, pSrcLPC[j]*pSrcLPC[j+i]);
         } 
         lCorr = ShiftL_32s(lCorr, SFS+1); 
         pDstReflectCoeff[i] = Cnvrt_NR_32s16s(lCorr);
      }
   } else {
      SFS = -SFS;
      lCorr = lCorr>>SFS; 
      pDstReflectCoeff[0] = Cnvrt_NR_32s16s(lCorr);

      for(i=1; i<=G723_LPC_ORDER; i++) {
         lCorr = -(pSrcLPC[i-1]<<13);
         for(j=0; j<G723_LPC_ORDER-i; j++) {
            lCorr = Add_32s(lCorr, pSrcLPC[j]*pSrcLPC[j+i]);
         } 
         lCorr = Mul2_32s(lCorr)>>(SFS); 
         pDstReflectCoeff[i] = Cnvrt_NR_32s16s(lCorr);
      } 
   }
   return;
}


int ItakuraDist_G723(short *pSrcReflectCoeff, short ReflectCoeffSFS, short *pSrcAutoCorrs, short energy)
{
    int i, lSum, lThresh;

    lSum = 0;
    for(i=0; i <= G723_LPC_ORDER; i++) {
        lSum += pSrcReflectCoeff[i] * (pSrcAutoCorrs[i]>>2);
    }

    lThresh = Cnvrt_32s16s(((energy * 7000)+0x4000)>>15) + energy;
    lThresh <<= (ReflectCoeffSFS + 8);

    return ((lSum < lThresh));
}

G723_CODECFUN(  APIG723_Status, apiG723Encode, 
         (G723Encoder_Obj* encoderObj,const short* src, short rat, char* pDstBitStream ))
{
   int     i, lNSbfr;

   LOCAL_ALIGN_ARRAY(16, short, HPFltSignal,G723_FRM_LEN, encoderObj);
   LOCAL_ALIGN_ARRAY(16, short, AlignTmpVec,G723_MAX_PITCH+G723_FRM_LEN, encoderObj);
   LOCAL_ALIGN_ARRAY(16, short, CurrLPC,4*G723_LPC_ORDER, encoderObj) ;
   LOCAL_ALIGN_ARRAY(16, short, CurrQLPC,4*(G723_LPC_ORDER+1), encoderObj) ;
   LOCAL_ALIGN_ARRAY(16, short, WeightedLPC,8*G723_LPC_ORDER, encoderObj) ;
   LOCAL_ALIGN_ARRAY(16, short, CurrLSF,G723_LPC_ORDER, encoderObj) ;
   LOCAL_ARRAY(GainInfo_G723, GainInfo,4, encoderObj) ;

   ParamStream_G723 CurrentParams;

   short  *pData;
   short isNotSineWave;

   CurrentParams.FrameType = G723_ActiveFrm;

   if(NULL==encoderObj || NULL==src || NULL ==pDstBitStream)
      return APIG723_StsBadArgErr;
   if(encoderObj->objPrm.objSize <= 0)
      return APIG723_StsNotInitialized;
   if(G723_ENC_KEY != encoderObj->objPrm.key)
      return APIG723_StsBadCodecType;

   if(rat < 0 || rat > 1) {
      rat = encoderObj->objPrm.rat;
   } else {
      encoderObj->objPrm.rat = rat;
   }

   CurrentParams.currRate = (G723_Rate)rat;
   if ( CurrentParams.currRate == G723_Rate53)     encoderObj->sSearchTime = 120; /* reset max time */

   CurrentParams.isBadFrame = (short) 0 ;

   if ( encoderObj->objPrm.mode&G723Encode_HF_Enabled ) {
      /*    High-pass filtering.   Section 2.3 */
      ippsHighPassFilter_G723_16s(src,HPFltSignal,encoderObj->HPFltMem);
   } else {
      ippsRShiftC_16s(src,1,HPFltSignal,G723_FRM_LEN);
   }

   /* Compute the Unquantized Lpc */
   {

       short   sTmp;

       LOCAL_ALIGN_ARRAY(16, short, AutoCorrs,(G723_LPC_ORDER+1)*4,encoderObj) ;
       LOCAL_ARRAY(short, AutoCorrsSFS,4,encoderObj) ;

       ippsCopy_16s(encoderObj->SignalDelayLine,AlignTmpVec,G723_HALFFRM_LEN);
       ippsCopy_16s(HPFltSignal,&AlignTmpVec[G723_HALFFRM_LEN],G723_FRM_LEN);

       ippsAutoCorr_G723_16s(AlignTmpVec,&AutoCorrsSFS[0],AutoCorrs);
       ippsAutoCorr_G723_16s(&AlignTmpVec[G723_SBFR_LEN],&AutoCorrsSFS[1],&AutoCorrs[G723_LPC_ORDERP1]);
       ippsAutoCorr_G723_16s(&AlignTmpVec[2*G723_SBFR_LEN],&AutoCorrsSFS[2],&AutoCorrs[2*G723_LPC_ORDERP1]);
       ippsAutoCorr_G723_16s(&AlignTmpVec[3*G723_SBFR_LEN],&AutoCorrsSFS[3],&AutoCorrs[3*G723_LPC_ORDERP1]);

       /* LPC calculation for all subframes */

       ippsLevinsonDurbin_G723_16s( AutoCorrs, &encoderObj->SineWaveDetector, &sTmp , CurrLPC);
       ippsLevinsonDurbin_G723_16s( &AutoCorrs[G723_LPC_ORDERP1], &encoderObj->SineWaveDetector, &sTmp , &CurrLPC[G723_LPC_ORDER]);
       ippsLevinsonDurbin_G723_16s( &AutoCorrs[2*G723_LPC_ORDERP1], &encoderObj->SineWaveDetector, &sTmp , &CurrLPC[2*G723_LPC_ORDER]);
       ippsLevinsonDurbin_G723_16s( &AutoCorrs[3*G723_LPC_ORDERP1], &encoderObj->SineWaveDetector, &sTmp , &CurrLPC[3*G723_LPC_ORDER]);

       /* Update sine detector */
       UpdateSineDetector(&encoderObj->SineWaveDetector, &isNotSineWave);
       /* Update CNG Acf memories */
       UpdateAutoCorrs_G723(encoderObj, AutoCorrs, AutoCorrsSFS);

       LOCAL_ARRAY_FREE(short, AutoCorrsSFS,4,encoderObj) ;
       LOCAL_ALIGN_ARRAY_FREE(16, short, AutoCorrs,(G723_LPC_ORDER+1)*4,encoderObj) ;

   }
   /* Convert to Lsp */
   ippsLPCToLSF_G723_16s(&CurrLPC[G723_LPC_ORDER*3], encoderObj->PrevLPC, CurrLSF);

   /* Compute the Vad */
   CurrentParams.FrameType = G723_ActiveFrm;
   if( encoderObj->objPrm.mode&G723Encode_VAD_Enabled){
      VoiceActivityDetect_G723(HPFltSignal, (short*)&encoderObj->prevSidLpc,(short*)&encoderObj->PrevOpenLoopLags,
         isNotSineWave,&i,&encoderObj->AdaptEnableFlag,encoderObj->vadMem,AlignTmpVec);
      CurrentParams.FrameType = (G723_FrameType)i;
   }

   /* VQ Lsp vector */

   ippsLSFQuant_G723_16s32s(CurrLSF, encoderObj->PrevLPC, &CurrentParams.lLSPIdx);

   ippsCopy_16s(&encoderObj->SignalDelayLine[G723_SBFR_LEN],AlignTmpVec,G723_SBFR_LEN);
   ippsCopy_16s(&HPFltSignal[2*G723_SBFR_LEN],encoderObj->SignalDelayLine,2*G723_SBFR_LEN);
   ippsCopy_16s(HPFltSignal,&AlignTmpVec[G723_SBFR_LEN],3*G723_SBFR_LEN);
   ippsCopy_16s(AlignTmpVec,HPFltSignal,G723_FRM_LEN);

   /* Compute Perceptual filter Lpc coefficients */

    for ( i = 0 ; i < 4 ; i ++ ) {
       /* Compute the FIR and IIR coefficient of the perceptual weighting filter */
       ippsMul_NR_16s_Sfs(&CurrLPC[i*G723_LPC_ORDER],PerceptualFltCoeffTbl,
                                    &WeightedLPC[2*i*G723_LPC_ORDER],G723_LPC_ORDER,15);
       ippsMul_NR_16s_Sfs(&CurrLPC[i*G723_LPC_ORDER],&PerceptualFltCoeffTbl[G723_LPC_ORDER],
                                    &WeightedLPC[(2*i+1)*G723_LPC_ORDER],G723_LPC_ORDER,15);
       /* do filtering */
       ippsIIR16s_G723_16s_I(&WeightedLPC[2*i*G723_LPC_ORDER],&HPFltSignal[i*G723_SBFR_LEN],encoderObj->WeightedFltMem);
    }


   /* Compute Open loop pitch estimates */

   ippsCopy_16s(encoderObj->PrevWeightedSignal,AlignTmpVec,G723_MAX_PITCH);
   ippsCopy_16s(HPFltSignal,&AlignTmpVec[G723_MAX_PITCH],G723_FRM_LEN);


   i=3;
   ippsAutoScale_16s_I( AlignTmpVec, G723_MAX_PITCH+G723_FRM_LEN, &i);

   ippsOpenLoopPitchSearch_G723_16s( &AlignTmpVec[G723_MAX_PITCH], &CurrentParams.PitchLag[0]);
   ippsOpenLoopPitchSearch_G723_16s( &AlignTmpVec[G723_MAX_PITCH+(2*G723_SBFR_LEN)], &CurrentParams.PitchLag[1]);

   encoderObj->PrevOpenLoopLags[0] = CurrentParams.PitchLag[0];
   encoderObj->PrevOpenLoopLags[1] = CurrentParams.PitchLag[1];

   if(CurrentParams.FrameType != G723_ActiveFrm) {

      ippsCopy_16s(&HPFltSignal[G723_FRM_LEN-G723_MAX_PITCH],encoderObj->PrevWeightedSignal,G723_MAX_PITCH);
      EncoderCNG_G723(encoderObj, &CurrentParams,HPFltSignal, CurrQLPC);

      /* change the ringing delays */
      pData = HPFltSignal;

      for( i = 0 ; i < 4; i++ ) {

         LOCAL_ARRAY(int, V_AccS,G723_SBFR_LEN,encoderObj) ;
         /* Update exc_err */
         ErrorUpdate_G723(encoderObj->ExcitationError, CurrentParams.PitchLag[i>>1], CurrentParams.AdCdbkLag[i], CurrentParams.AdCdbkGain[i],CurrentParams.currRate);

         /* Shift the harmonic noise shaping filter memory */
         ippsCopy_16s(&encoderObj->FltSignal[G723_SBFR_LEN],encoderObj->FltSignal,G723_MAX_PITCH-G723_SBFR_LEN);
         /* Combined filtering */
         ippsCopy_16s(encoderObj->RingSynthFltMem,encoderObj->RingWeightedFltMem,G723_LPC_ORDER);
         ippsSynthesisFilter_G723_16s32s(&CurrQLPC[i*(G723_LPC_ORDER+1)],
                pData,V_AccS,encoderObj->RingSynthFltMem);

         ippsIIR16s_G723_32s16s_Sfs(&WeightedLPC[i*2*G723_LPC_ORDER],V_AccS,0,
                &(encoderObj->FltSignal[G723_MAX_PITCH-G723_SBFR_LEN]),encoderObj->RingWeightedFltMem);
         
         pData += G723_SBFR_LEN;
         LOCAL_ARRAY_FREE(int, V_AccS,G723_SBFR_LEN,encoderObj) ;
      }
   }

   else {

      /* Active frame */
      ippsHarmonicSearch_G723_16s(CurrentParams.PitchLag[0], &AlignTmpVec[G723_MAX_PITCH], &GainInfo[0].sDelay, &GainInfo[0].sGain);
      ippsHarmonicSearch_G723_16s(CurrentParams.PitchLag[0], &AlignTmpVec[G723_MAX_PITCH+G723_SBFR_LEN], &GainInfo[1].sDelay, &GainInfo[1].sGain);
      ippsHarmonicSearch_G723_16s(CurrentParams.PitchLag[1], &AlignTmpVec[G723_MAX_PITCH+2*G723_SBFR_LEN], &GainInfo[2].sDelay, &GainInfo[2].sGain);
      ippsHarmonicSearch_G723_16s(CurrentParams.PitchLag[1], &AlignTmpVec[G723_MAX_PITCH+3*G723_SBFR_LEN], &GainInfo[3].sDelay, &GainInfo[3].sGain);

      ippsCopy_16s(encoderObj->PrevWeightedSignal,AlignTmpVec,G723_MAX_PITCH);
      ippsCopy_16s(HPFltSignal,&AlignTmpVec[G723_MAX_PITCH],G723_FRM_LEN);

      ippsCopy_16s(&AlignTmpVec[G723_FRM_LEN],encoderObj->PrevWeightedSignal,G723_MAX_PITCH);

      ippsHarmonicFilter_NR_16s((short)(-GainInfo[0].sGain),GainInfo[0].sDelay,&AlignTmpVec[G723_MAX_PITCH],
                                                          HPFltSignal,G723_SBFR_LEN);
      ippsHarmonicFilter_NR_16s((short)(-GainInfo[1].sGain),GainInfo[1].sDelay,&AlignTmpVec[G723_MAX_PITCH+G723_SBFR_LEN],
                                                          &HPFltSignal[G723_SBFR_LEN],G723_SBFR_LEN);
      ippsHarmonicFilter_NR_16s((short)(-GainInfo[2].sGain),GainInfo[2].sDelay,&AlignTmpVec[G723_MAX_PITCH+2*G723_SBFR_LEN],
                                                          &HPFltSignal[2*G723_SBFR_LEN],G723_SBFR_LEN);
      ippsHarmonicFilter_NR_16s((short)(-GainInfo[3].sGain),GainInfo[3].sDelay,&AlignTmpVec[G723_MAX_PITCH+3*G723_SBFR_LEN],
                                                          &HPFltSignal[3*G723_SBFR_LEN],G723_SBFR_LEN);

      {
         LOCAL_ARRAY(short, qIndex,3,encoderObj) ;
         /* Inverse quantization of the LSP */
         qIndex[2] =  CurrentParams.lLSPIdx & 0xff;
         qIndex[1] =  (CurrentParams.lLSPIdx>>8) & 0xff;
         qIndex[0] =  (CurrentParams.lLSPIdx>>16) & 0xff;
         if(ippsLSFDecode_G723_16s(qIndex, encoderObj->PrevLPC, CurrentParams.isBadFrame, CurrLSF) != ippStsNoErr)
            ippsCopy_16s(encoderObj->PrevLPC,CurrLSF,G723_LPC_ORDER);
         LOCAL_ARRAY_FREE(short, qIndex,3,encoderObj) ;
      }

      /* Interpolate the Lsp vectors */
      LSPInterpolation(CurrLSF, encoderObj->PrevLPC, CurrQLPC) ;

      /* Copy the LSP vector for the next frame */
      ippsCopy_16s(CurrLSF,encoderObj->PrevLPC,G723_LPC_ORDER);

      /* sub frame processing */
      pData = HPFltSignal ;

      for ( lNSbfr = 0 ; lNSbfr < 4 ; lNSbfr ++ ) {

         LOCAL_ALIGN_ARRAY(16, short, SynDl,G723_LPC_ORDER,encoderObj) ; /* synthesis filter delay line */
         LOCAL_ALIGN_ARRAY(16, short, RingWgtDl,2*G723_LPC_ORDER,encoderObj) ;/* formant perceptual weighting filter delay line */
         LOCAL_ALIGN_ARRAY(16, int, V_AccS,G723_SBFR_LEN,encoderObj) ;
         LOCAL_ALIGN_ARRAY(16, short, ImpResp,G723_SBFR_LEN,encoderObj) ;

         /* Compute full impulse response */
         ippsZero_16s(SynDl,G723_LPC_ORDER); /* synthesis filter zero delay */

         ippsSynthesisFilter_G723_16s32s(&CurrQLPC[lNSbfr*(G723_LPC_ORDER+1)],encoderObj->UnitImpulseSignal,V_AccS,SynDl);

         {

            LOCAL_ALIGN_ARRAY(16, short, Temp,G723_MAX_PITCH+G723_SBFR_LEN,encoderObj) ;

            ippsZero_16s(RingWgtDl,2*G723_LPC_ORDER);/* formant perceptual weighting filter zero delay */
            ippsIIR16s_G723_32s16s_Sfs(&WeightedLPC[lNSbfr*2*G723_LPC_ORDER],V_AccS,1,
                                                         &Temp[G723_MAX_PITCH],RingWgtDl);
         
            ippsZero_16s(Temp,G723_MAX_PITCH);/* harmonic filter zero delay */
            ippsHarmonicFilter_NR_16s((short)(-GainInfo[lNSbfr].sGain),GainInfo[lNSbfr].sDelay,&Temp[G723_MAX_PITCH],ImpResp,G723_SBFR_LEN);

            LOCAL_ALIGN_ARRAY_FREE(16, short, Temp,G723_MAX_PITCH+G723_SBFR_LEN,encoderObj) ;
         }

         /* Subtract the ringing of previous sub-frame */
         ippsCopy_16s(encoderObj->RingSynthFltMem,SynDl,G723_LPC_ORDER);
         /*  Synthesis filter of zero input  */
         ippsSynthesisFilter_G723_16s32s(&CurrQLPC[lNSbfr*(G723_LPC_ORDER+1)],encoderObj->ZeroSignal,V_AccS,SynDl);
         
         ippsCopy_16s(encoderObj->RingSynthFltMem,RingWgtDl,G723_LPC_ORDER);/* FIR same as for synth filter */
         ippsCopy_16s(&encoderObj->RingWeightedFltMem[G723_LPC_ORDER],&RingWgtDl[G723_LPC_ORDER],G723_LPC_ORDER);/* IIR part*/
         ippsIIR16s_G723_32s16s_Sfs(&WeightedLPC[lNSbfr*2*G723_LPC_ORDER],V_AccS,0,
                                    &(encoderObj->FltSignal[G723_MAX_PITCH]),RingWgtDl);
                                       
         /* Do the harmonic noise shaping filter with subtraction the result
           from the harmonic noise weighted vector.*/
         ippsHarmonicNoiseSubtract_G723_16s_I((short)(-GainInfo[lNSbfr].sGain),GainInfo[lNSbfr].sDelay,
                                              &(encoderObj->FltSignal[G723_MAX_PITCH]),pData);
         /* Shift the harmonic noise shaping filter memory */
         ippsCopy_16s(&encoderObj->FltSignal[G723_SBFR_LEN],encoderObj->FltSignal,G723_MAX_PITCH-G723_SBFR_LEN);

         /*  Adaptive codebook contribution to exitation residual.  Section 2.14. */
         {
            short   sCloseLag;
            short  sPitchLag = CurrentParams.PitchLag[lNSbfr>>1] ;

            LOCAL_ALIGN_ARRAY(16, short, RezBuf,G723_SBFR_LEN+4,encoderObj) ;
            
            if ( (lNSbfr & 1 ) == 0 ) { /* For even frames only */
                if ( sPitchLag ==  G723_MIN_PITCH ) sPitchLag++;
                if ( sPitchLag > (G723_MAX_PITCH-5) ) sPitchLag = G723_MAX_PITCH-5 ;
            }
            
            ippsAdaptiveCodebookSearch_G723(sPitchLag, pData, ImpResp, encoderObj->PrevExcitation, encoderObj->ExcitationError,
               &sCloseLag, &CurrentParams.AdCdbkGain[lNSbfr], (short)lNSbfr, isNotSineWave, SA_Rate[CurrentParams.currRate]);

            /* Modify sPitchLag for even sub frames */
            if ( (lNSbfr & 1 ) ==  0 ) {
                sPitchLag = sPitchLag - 1 + sCloseLag ;
                sCloseLag = 1 ;
            }
            CurrentParams.AdCdbkLag[lNSbfr] = sCloseLag ;
            CurrentParams.PitchLag[lNSbfr>>1] = sPitchLag ;

            ippsDecodeAdaptiveVector_G723_16s(sPitchLag, sCloseLag, CurrentParams.AdCdbkGain[lNSbfr], encoderObj->PrevExcitation, RezBuf, SA_Rate[CurrentParams.currRate]);

            /* subtract the contribution of the pitch predictor decoded to obtain the residual */
            ExcitationResidual_G723_16s(RezBuf,ImpResp,pData,encoderObj);

            LOCAL_ALIGN_ARRAY_FREE(16, short, RezBuf,G723_SBFR_LEN+4,encoderObj) ;
         }
         /* Compute fixed code book contribution */
         FixedCodebookSearch_G723_16s(encoderObj, &CurrentParams, pData, ImpResp, (short) lNSbfr) ;
         
         ippsDecodeAdaptiveVector_G723_16s(CurrentParams.PitchLag[lNSbfr>>1], CurrentParams.AdCdbkLag[lNSbfr], CurrentParams.AdCdbkGain[lNSbfr], encoderObj->PrevExcitation, 
                                                         ImpResp, SA_Rate[CurrentParams.currRate]);

         ippsCopy_16s(&encoderObj->PrevExcitation[G723_SBFR_LEN],encoderObj->PrevExcitation,G723_MAX_PITCH-G723_SBFR_LEN);

         for ( i = 0 ; i < G723_SBFR_LEN ; i ++ ) {
            pData[i] = Cnvrt_32s16s( Mul2_16s(pData[i])+ImpResp[i]);
         }
         ippsCopy_16s(pData,&encoderObj->PrevExcitation[G723_MAX_PITCH-G723_SBFR_LEN],G723_SBFR_LEN);

         /* Update exc_err */
         ErrorUpdate_G723(encoderObj->ExcitationError, CurrentParams.PitchLag[lNSbfr>>1], CurrentParams.AdCdbkLag[lNSbfr], CurrentParams.AdCdbkGain[lNSbfr],CurrentParams.currRate);

         /* Update the ringing delays by passing excitation through the combined filter.*/
         for(i=0; i<G723_LPC_ORDER; i++){
            encoderObj->RingWeightedFltMem[i] = encoderObj->RingSynthFltMem[i]; /* FIR same as for synth filter */
         }
         ippsSynthesisFilter_G723_16s32s(&CurrQLPC[lNSbfr*(G723_LPC_ORDER+1)],
               pData,V_AccS,encoderObj->RingSynthFltMem);
         ippsIIR16s_G723_32s16s_Sfs(&WeightedLPC[lNSbfr*2*G723_LPC_ORDER],V_AccS,0,
                                    &(encoderObj->FltSignal[G723_MAX_PITCH-G723_SBFR_LEN]),encoderObj->RingWeightedFltMem);
         pData += G723_SBFR_LEN ;

         LOCAL_ALIGN_ARRAY_FREE(16, short, ImpResp,G723_SBFR_LEN,encoderObj) ;
         LOCAL_ALIGN_ARRAY_FREE(16, int, V_AccS,G723_SBFR_LEN,encoderObj) ;
         LOCAL_ALIGN_ARRAY_FREE(16, short, RingWgtDl,2*G723_LPC_ORDER,encoderObj) ;/* formant perceptual weighting filter delay line */
         LOCAL_ALIGN_ARRAY_FREE(16, short, SynDl,G723_LPC_ORDER,encoderObj) ; /* synthesis filter delay line */
      }  /* end of subframes loop */

      encoderObj->PastFrameType = G723_ActiveFrm;
      encoderObj->CNGSeed = 12345;
      encoderObj->CasheCounter = 0;

   } 

   /* Pack to the bitstream */
   SetParam2Bitstream(encoderObj, &CurrentParams, pDstBitStream);

   CLEAR_SCRATCH_MEMORY(encoderObj);

   return APIG723_StsNoErr;

}
