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
//
//  Purpose: G.728 decode API
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <ipps.h>
#include "g728api.h"
#include "owng728.h"
#include "g728tables.h"

static const IppSpchBitRate rate2ipp[3] = {
   IPP_SPCHBR_16000,
   IPP_SPCHBR_12800,
   IPP_SPCHBR_9600
};

#define ADD_ALIGN_MEM_BLOCK(align,addr,length) (addr + length + align -1)

G728_CODECFUN( APIG728_Status, apiG728Encoder_GetSize, 
         (G728Encoder_Obj* encoderObj, unsigned int *pCodecSize))
{
   if(NULL == encoderObj)
      return APIG728_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIG728_StsBadArgErr;
   if(encoderObj->objPrm.key != ENC_KEY)
      return APIG728_StsNotInitialized;

   *pCodecSize = encoderObj->objPrm.objSize;
   return APIG728_StsNoErr;
}

G728_CODECFUN( APIG728_Status, apiG728Decoder_GetSize, 
         (G728Decoder_Obj* decoderObj, unsigned int *pCodecSize))
{
   if(NULL == decoderObj)
      return APIG728_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIG728_StsBadArgErr;
   if(decoderObj->objPrm.key != DEC_KEY)
      return APIG728_StsNotInitialized;

   *pCodecSize = decoderObj->objPrm.objSize;
   return APIG728_StsNoErr;
}

G728_CODECFUN( APIG728_Status, apiG728Encoder_Alloc,(unsigned int* objSize))
{
   int rexpMemSize=IPP_MAX_32S;
   int rexpwMemSize=IPP_MAX_32S;
   int rexplgMemSize=IPP_MAX_32S;
   int combSize=IPP_MAX_32S;
   int iirMemSize=IPP_MAX_32S;
   char* eObj = NULL; 

   eObj = (char*)IPP_ALIGNED_PTR(eObj+sizeof(G728Encoder_Obj), 16);

   ippsIIR16sGetStateSize_G728_16s(&iirMemSize);
   eObj = (char*)IPP_ALIGNED_PTR(eObj+iirMemSize, 16);

   ippsCombinedFilterGetStateSize_G728_16s(&combSize);
   eObj = (char*)IPP_ALIGNED_PTR(eObj+combSize, 16);

   ippsWinHybridGetStateSize_G728_16s(LPCLG, NUPDATE, NONRLG, 0, &rexplgMemSize);
   eObj = (char*)IPP_ALIGNED_PTR(eObj+rexplgMemSize, 16);

   ippsWinHybridGetStateSize_G728_16s(LPCW, NFRSZ, NONRW, 0, &rexpwMemSize);
   eObj = (char*)IPP_ALIGNED_PTR(eObj+rexpwMemSize, 16);

   ippsWinHybridGetStateSize_G728_16s(LPC, NFRSZ, NONR, IDIM, &rexpMemSize);
   eObj = (char*)IPP_ALIGNED_PTR(eObj+rexpMemSize, 16);

   *objSize = (Ipp32u)(eObj - (char*)NULL);
   return APIG728_StsNoErr;
}

G728_CODECFUN( APIG728_Status, apiG728Encoder_Init,(G728Encoder_Obj* eObj, G728_Rate rate))
{     
   int rexpMemSize=IPP_MAX_32S;
   int rexpwMemSize=IPP_MAX_32S;
   int rexplgMemSize=IPP_MAX_32S;
   int combSize=IPP_MAX_32S;
   int iirMemSize=IPP_MAX_32S;
   char* tmpObjPtr = (char*)eObj; 
   unsigned int objSize;

   if((int)eObj & 0x7){ /* shall be at least 8 bytes aligned */ 
      return APIG728_StsNotInitialized;
   }

   ippsZero_16s((Ipp16s*) eObj,sizeof(G728Encoder_Obj)/2);

   eObj->objPrm.key = ENC_KEY;
   eObj->objPrm.rate = rate;

   eObj->h[0] = 8192;
   eObj->vecLGPredictorCoeffs[0] = -16384;
   ippsSet_16s(-16384, eObj->vecLGPredictorState, LPCLG);
   ippsSet_16s(16, eObj->nlssttmp, 4);
   if(rate==G728_Rate_12800)     { 
      eObj->pGq = gq_128; 
      eObj->pNgq = nngq_128;
      eObj->pCodebookGain = cnstCodebookVectorsGain_128;
   }
   else if(rate==G728_Rate_9600) { 
      eObj->pGq = gq_96;
      eObj->pNgq = nngq_96;
      eObj->pCodebookGain = cnstCodebookVectorsGain_96;
   }
   else if(rate==G728_Rate_16000){ 
      eObj->pGq = gq;
      eObj->pNgq = nngq;
      eObj->pCodebookGain = cnstCodebookVectorsGain;
   }
   ippsImpulseResponseEnergy_G728_16s(eObj->h, eObj->y2);

   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+sizeof(G728Encoder_Obj), 16);

   eObj->wgtMem = (IppsIIRState_G728_16s*)tmpObjPtr;
   ippsIIR16sGetStateSize_G728_16s(&iirMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+iirMemSize, 16);

   eObj->combMem = (IppsCombinedFilterState_G728_16s*)tmpObjPtr;
   ippsCombinedFilterGetStateSize_G728_16s(&combSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+combSize, 16);

   eObj->rexplgMem = (IppsWinHybridState_G728_16s*)tmpObjPtr;
   ippsWinHybridGetStateSize_G728_16s(LPCLG, NUPDATE, NONRLG, 0, &rexplgMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+rexplgMemSize, 16);

   eObj->rexpwMem = (IppsWinHybridState_G728_16s*)tmpObjPtr;
   ippsWinHybridGetStateSize_G728_16s(LPCW, NFRSZ, NONRW, 0, &rexpwMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+rexpwMemSize, 16);

   eObj->rexpMem = (IppsWinHybridState_G728_16s*)tmpObjPtr;
   ippsWinHybridGetStateSize_G728_16s(LPC, NFRSZ, NONR, IDIM, &rexpMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+rexpMemSize, 16);
   
   eObj->objPrm.objSize = (Ipp32u)((char*)tmpObjPtr - (char*)eObj);
   apiG728Encoder_Alloc(&objSize); 
   if(objSize != eObj->objPrm.objSize){
      return APIG728_StsNotInitialized; /* must not occur */ 
   }
   ippsIIR16sInit_G728_16s(eObj->wgtMem);
   ippsCombinedFilterInit_G728_16s(eObj->combMem);
   ippsWinHybridInit_G728_16s(wnrlg,LPCLG, NUPDATE, NONRLG, 0, 12288, eObj->rexplgMem);
   ippsWinHybridInit_G728_16s(wnrw,LPCW, NFRSZ, NONRW, 0, 8192, eObj->rexpwMem);
   ippsWinHybridInit_G728_16s(wnr,LPC, NFRSZ, NONR, IDIM, 12288, eObj->rexpMem);


   return APIG728_StsNoErr;
}

static void prm2bits(const short* prm, unsigned char* bitstream, G728_Rate rate)
{
   if(rate==G728_Rate_12800)     { 
      bitstream[0] = (unsigned char) prm[0];                               
      bitstream[1] = (unsigned char) prm[1]; 
      bitstream[2] = (unsigned char) prm[2]; 
      bitstream[3] = (unsigned char) prm[3]; 
   }
   else if(rate==G728_Rate_9600) { 
      bitstream[0] = (unsigned char) (( prm[0] << 2 ) | ( prm[1] >> 4));       
      bitstream[1] = (unsigned char) (( (prm[1] & 0xf) << 4 ) | (prm[2] >> 2));
      bitstream[2] = (unsigned char) (( (prm[2] & 0x3) << 6 )| prm[3]);        
   }
   else if(rate==G728_Rate_16000){ 
      bitstream[0] = (unsigned char) ( prm[0] >> 2);                               
      bitstream[1] = (unsigned char) (( (prm[0] & 0x3) << 6 ) | (prm[1] >> 4)); 
      bitstream[2] = (unsigned char) (( (prm[1] & 0xf) << 4 ) | (prm[2] >> 6)); 
      bitstream[3] = (unsigned char) (( (prm[2] & 0x3f) << 2 )| (prm[3] >> 8)); 
      bitstream[4] = (unsigned char) (prm[3] & 0xff);                              
   }
}

static Ipp16s EncOncePerFrameProcessing(G728Encoder_Obj* eObj, const Ipp16s *src, 
                                        Ipp16s index){
   Ipp32s gainLog;
   Ipp16s linearGain;
   Ipp16s scaleGain;
   Ipp16s nlstarget;
   Ipp16s codebookIdx;
   Ipp32s i, gainIdx, shapeIdx;
   Ipp32s aa0;
   Ipp16s tmp;
   Ipp16s rate = eObj->objPrm.rate;
   Ipp16s nlset;

   IPP_ALIGNED_ARRAY(16, Ipp16s, sw, (IDIM +3)); /* IDIM + 3 = 16 byte */ 
   IPP_ALIGNED_ARRAY(16, Ipp16s, target, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pn, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, tempZIR, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, et, (IDIM +3));

   /* Get backward-adapted gain */ 
   /* gstate[1:9] shifted down 1 position */ 
   LoggainLinearPrediction(eObj->vecLGPredictorCoeffs, eObj->vecLGPredictorState, &gainLog);

   if(gainLog > 14336) gainLog = 14336;
   if(gainLog < -16384) gainLog = -16384;

   InverseLogarithmCalc(gainLog, &linearGain, &scaleGain);
   /* Synthesis filter with zero input combined with perceptual weighting filter */ 
   ippsCombinedFilterZeroInput_G728_16s(eObj->vecSyntFltrCoeffs, eObj->vecWghtFltrCoeffs, tempZIR, eObj->combMem);
   /* Perceptual weighting filter */ 
   ippsIIR16s_G728_16s((Ipp16s*)eObj->vecWghtFltrCoeffs, src, sw, IDIM, eObj->wgtMem);
   /* VQ target vector computation */ 
   VQ_target_vector_calc(sw, tempZIR, target);
   /* VQ target vector normalization */ 
   nlstarget = 2;
   VQ_target_vec_norm(linearGain, scaleGain, target, &nlstarget);
   /* Time-reversed convolution */ 
   Time_reversed_conv(eObj->h, target, nlstarget, pn);
   /* Excitation codebook search */ 
   ippsCodebookSearch_G728_16s(pn, eObj->y2, &shapeIdx, &gainIdx, &codebookIdx, rate2ipp[rate]);
   /* Scale selected excitation codevector */ 
   aa0 = eObj->pGq[gainIdx] * linearGain;
   aa0 = ShiftL_32s(aa0, eObj->pNgq[gainIdx]);
   tmp = Cnvrt_NR_32s16s(aa0);
   nlset = scaleGain + eObj->pNgq[gainIdx] + shape_all_nls[shapeIdx] - 8;
   Excitation_VQ_and_gain_scaling(tmp, &shape_all_norm[shapeIdx * IDIM], et);
   /* Memory update */ 
   ippsCombinedFilterZeroState_G728_16s(eObj->vecSyntFltrCoeffs, eObj->vecWghtFltrCoeffs, 
      et, nlset, eObj->st, &eObj->nlsst, eObj->combMem);
   /* Update log-gain and gain predictor memory */ 
   eObj->vecLGPredictorState[0] = LoggainAdderLimiter(gainLog, (Ipp16s)gainIdx, (Ipp16s)shapeIdx, 
      eObj->pCodebookGain);
   
   i = (index - 1) *IDIM;
   ippsCopy_16s(eObj->st,&eObj->sttmp[i],5);
   eObj->nlssttmp[index-1] = eObj->nlsst;
   
   /* stmp - cyclic buffer                                        */ 
   /* index   stmp                                                */ 
   /*      1            0,         0, src[ 0: 4],         0,      */ 
   /*      2            0,         0, src[ 0: 4],src[ 5: 9],      */ 
   /*      3   src[10:14],         0, src[ 0: 4],src[ 5: 9],      */ 
   /*      4   src[10:14],src[15:19], src[ 0: 4],src[ 5: 9],      */ 
   /*      1   src[10:14],src[15:19], src[20:24],src[ 5: 9],      */ 
   /*      2   src[10:14],src[15:19], src[20:24],src[25:29], .... */ 
   i = (index + 1) & 0x3;
   i *= IDIM;
   ippsCopy_16s(src,&eObj->stmp[i],5);
   return codebookIdx;
}

G728_CODECFUN( APIG728_Status, apiG728Encode,
         (G728Encoder_Obj* eObj, short *src, unsigned char *dst))
{
   Ipp16s gtmp[4];
   Ipp16s foo;
   Ipp16s codebookIdxs[4];/* four codebook indexes */ 
   int index;

   /* four vectors of 5 samples length  (20 short integer)*/ 
   eObj->icount = eObj->icount & 3;
   eObj->icount++;
   index = 1;
      codebookIdxs[index-1] = EncOncePerFrameProcessing(eObj, src, (Ipp16s)index);
      gtmp[0] = eObj->vecLGPredictorState[3];
      gtmp[1] = eObj->vecLGPredictorState[2];
      gtmp[2] = eObj->vecLGPredictorState[1];
      gtmp[3] = eObj->vecLGPredictorState[0];
      /* Block 43 */ 
      eObj->illcondg=0;
      if(ippsWinHybrid_G728_16s(0, gtmp, eObj->r, eObj->rexplgMem ) != ippStsNoErr){
         eObj->illcondg=1;
      };
      ippsZero_16s(eObj->tmpLGPredictorCoeffs, LPCLG);
      LevinsonDurbin(eObj->r, 0, LPCLG, eObj->tmpLGPredictorCoeffs, &foo, &foo, 
         &eObj->scaleLGPredictorCoeffs, &eObj->illcondp, &eObj->illcondg);

   src += 5;
   eObj->icount = eObj->icount & 3;
   eObj->icount++;
   index = 2;
      if(eObj->illcondg==0)
         BandwidthExpansionModul(cnstGainPreditorBroadenVector, eObj->tmpLGPredictorCoeffs, 
            eObj->scaleLGPredictorCoeffs, eObj->vecLGPredictorCoeffs, LPCLG);
      codebookIdxs[index-1] = EncOncePerFrameProcessing(eObj, src, (Ipp16s)index);
      /* Block 36 */ 
      eObj->illcondw=0;
      if(ippsWinHybrid_G728_16s(0, eObj->stmp, eObj->r, eObj->rexpwMem ) != ippStsNoErr){
         eObj->illcondw=1;
      };
      ippsZero_16s(eObj->tmpWghtFltrCoeffs, LPCW);
      LevinsonDurbin(eObj->r, 0, LPCW, eObj->tmpWghtFltrCoeffs, &foo, &foo,
         &eObj->scaleWghtFltrCoeffs, &eObj->illcondp, &eObj->illcondw);

   src += 5;
   eObj->icount = eObj->icount & 3;
   eObj->icount++;
   index = 3;
      if(eObj->illcond==0) 
         BandwidthExpansionModul(cnstSynthesisFilterBroadenVector, eObj->tmpSyntFltrCoeffs, 
            eObj->scaleSyntFltrCoeffs, eObj->vecSyntFltrCoeffs, LPC);
      if(eObj->illcondw==0)
         WeightingFilterCoeffsCalc(eObj->tmpWghtFltrCoeffs, eObj->scaleWghtFltrCoeffs, eObj->vecWghtFltrCoeffs);
      Impulse_response_vec_calc(eObj->vecSyntFltrCoeffs, eObj->vecWghtFltrCoeffs, eObj->h);
      ippsImpulseResponseEnergy_G728_16s(eObj->h, eObj->y2);
      codebookIdxs[index-1]  = EncOncePerFrameProcessing(eObj,src,(Ipp16s)index);

   src += 5;
   eObj->icount = eObj->icount & 3;
   eObj->icount++;
   index = 4;
      codebookIdxs[index-1] = EncOncePerFrameProcessing(eObj,src,(Ipp16s)index);
      eObj->illcond = 0;
      if(ippsWinHybridBlock_G728_16s(0, eObj->sttmp, eObj->nlssttmp,
         eObj->rtmp, eObj->rexpMem ) != ippStsNoErr){
         eObj->illcond = 1;
      };
      ippsZero_16s(eObj->tmpSyntFltrCoeffs, LPC);
      LevinsonDurbin(eObj->rtmp, 0, LPC, eObj->tmpSyntFltrCoeffs, &foo, &foo,
         &eObj->scaleSyntFltrCoeffs, &eObj->illcondp, &eObj->illcond);

   /* pack indexes into bitstream */ 
   prm2bits(codebookIdxs, dst, eObj->objPrm.rate);
   /* End once-per-frame processing */ 
   return APIG728_StsNoErr;
}

G728_CODECFUN( APIG728_Status, apiG728Decoder_Alloc,(unsigned int* objSize))
{
   int rexpMemSize=IPP_MAX_32S;
   int rexplgMemSize=IPP_MAX_32S;
   int iirMemSize=IPP_MAX_32S;
   int stpMemSize=IPP_MAX_32S;
   int syntMemSize=IPP_MAX_32S;
   int postFltAdaptMemSize = IPP_MAX_32S;
   char* dObj = NULL; 

   dObj = (char*)IPP_ALIGNED_PTR(dObj+sizeof(G728Decoder_Obj), 16);

   ippsSynthesisFilterGetStateSize_G728_16s(&syntMemSize);
   dObj = (char*)IPP_ALIGNED_PTR(dObj+syntMemSize, 16);

   ippsPostFilterGetStateSize_G728_16s(&stpMemSize);
   ippsIIR16sGetStateSize_G728_16s(&iirMemSize);
   dObj = (char*)IPP_ALIGNED_PTR(dObj+IPP_MAX(stpMemSize,iirMemSize), 16);

   ippsWinHybridGetStateSize_G728_16s(LPCLG, NUPDATE, NONRLG, 0, &rexplgMemSize);
   dObj = (char*)IPP_ALIGNED_PTR(dObj+rexplgMemSize, 16);

   ippsWinHybridGetStateSize_G728_16s(LPC, NFRSZ, NONR, IDIM, &rexpMemSize);
   dObj = (char*)IPP_ALIGNED_PTR(dObj+rexpMemSize, 16);

   ippsPostFilterAdapterGetStateSize_G728(&postFltAdaptMemSize);
   dObj = (char*)IPP_ALIGNED_PTR(dObj+postFltAdaptMemSize, 16);

   *objSize = (Ipp32u)(dObj - (char*)NULL);

   return APIG728_StsNoErr;
}
G728_CODECFUN( APIG728_Status, apiG728Decoder_Init,(
              G728Decoder_Obj* dObj, G728_Type type, G728_Rate rate, int pst))
{  
   int rexpMemSize=IPP_MAX_32S;
   int rexplgMemSize=IPP_MAX_32S;
   int iirMemSize=IPP_MAX_32S;
   int stpMemSize=IPP_MAX_32S;
   int syntMemSize=IPP_MAX_32S;
   int postFltAdaptMemSize = IPP_MAX_32S;
   char* tmpObjPtr = (char*)dObj; 
   unsigned int objSize;

   if((int)dObj & 0x7){ /* shall be at least 8 bytes aligned */ 
      return APIG728_StsNotInitialized;
   }

   ippsZero_16s((Ipp16s*) dObj,sizeof(G728Decoder_Obj)/2);

   dObj->objPrm.key = DEC_KEY;
   dObj->objPrm.rate = rate;
   dObj->objPrm.type = type;
   

   dObj->gl = 16384;
   dObj->pst = pst;   
   dObj->vecLGPredictorCoeffs[0] = -16384;
   dObj->ip = NPWSZ - NFRSZ + IDIM;
   dObj->kp1 = 50;
   dObj->scalefil = 16384;
   if(rate==G728_Rate_12800)     { 
      dObj->pGq = gq_128; 
      dObj->pNgq = nngq_128;
      dObj->pCodebookGain = cnstCodebookVectorsGain_128;
   }
   else if(rate==G728_Rate_9600) { 
      dObj->pGq = gq_96;
      dObj->pNgq = nngq_96;
      dObj->pCodebookGain = cnstCodebookVectorsGain_96;
   }
   else if(rate==G728_Rate_16000){ 
      dObj->pGq = gq;
      dObj->pNgq = nngq;
      dObj->pCodebookGain = cnstCodebookVectorsGain;
   }
   ippsSet_16s(16, dObj->nlssttmp, 4);
   ippsSet_16s(-16384, dObj->vecLGPredictorState, LPCLG);
   ippsZero_16s(dObj->etpast_buff, 140+IDIM);

   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+sizeof(G728Decoder_Obj), 16);

   dObj->syntState = (IppsSynthesisFilterState_G728_16s*)tmpObjPtr;
   ippsSynthesisFilterGetStateSize_G728_16s(&syntMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+syntMemSize, 16);

   dObj->stpMem = (IppsPostFilterState_G728_16s*)tmpObjPtr;
   if(dObj->pst){
      ippsPostFilterInit_G728_16s(dObj->stpMem);
   }else{
      ippsIIR16sInit_G728_16s((IppsIIRState_G728_16s*)dObj->stpMem);
   }
   ippsPostFilterGetStateSize_G728_16s(&stpMemSize);
   ippsIIR16sGetStateSize_G728_16s(&iirMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+IPP_MAX(stpMemSize,iirMemSize), 16);

   dObj->rexplgMem = (IppsWinHybridState_G728_16s*)tmpObjPtr;
   ippsWinHybridGetStateSize_G728_16s(LPCLG, NUPDATE, NONRLG, 0, &rexplgMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+rexplgMemSize, 16);

   dObj->rexpMem = (IppsWinHybridState_G728_16s*)tmpObjPtr;
   ippsWinHybridGetStateSize_G728_16s(LPC, NFRSZ, NONR, IDIM, &rexpMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+rexpMemSize, 16);

   dObj->postFltAdapterMem = (IppsPostFilterAdapterState_G728*)tmpObjPtr;
   ippsPostFilterAdapterGetStateSize_G728(&postFltAdaptMemSize);
   tmpObjPtr = (char*)IPP_ALIGNED_PTR(tmpObjPtr+postFltAdaptMemSize, 16);
   
   
   dObj->objPrm.objSize = (Ipp32u)((char*)tmpObjPtr - (char*)dObj);
   apiG728Decoder_Alloc(&objSize); 
   if(objSize != dObj->objPrm.objSize){ 
      return APIG728_StsNotInitialized; /* must not occur */ 
   }
   ippsSynthesisFilterInit_G728_16s(dObj->syntState);
   ippsPostFilterAdapterStateInit_G728(dObj->postFltAdapterMem);
   ippsWinHybridInit_G728_16s(wnrlg,LPCLG, NUPDATE, NONRLG, 0, 12288, dObj->rexplgMem);
   ippsWinHybridInit_G728_16s(wnr,LPC, NFRSZ, NONR, IDIM, 12288, dObj->rexpMem);

   dObj->ferror = G728_FALSE;

   if (type == G728_Annex_I) {
      dObj->adcount = FESIZE;
      dObj->fecount = 0;
      dObj->afterfe = 0;
      dObj->ogaindb = -16384;
      dObj->seed = 11111;
      dObj->pTab = 0;
      dObj->fescale = 26214;
   }

   return APIG728_StsNoErr;
}

static void bits2prm(const unsigned char* bitstream, short* prm, G728_Rate rate)
{
   if(rate==G728_Rate_12800)     { 
      prm[0] = bitstream[0];   
      prm[1] = bitstream[1]; 
      prm[2] = bitstream[2]; 
      prm[3] = bitstream[3]; 
   }
   else if(rate==G728_Rate_9600) { 
      prm[0] = (bitstream[0] >> 2);                               
      prm[1] = ( (bitstream[0] & 0x3) << 4 ) | (bitstream[1] >> 4); 
      prm[2] = ( (bitstream[1] & 0xf) << 2 ) | (bitstream[2] >> 6);  
      prm[3] = (bitstream[2] & 0x3f);                             
   }
   else if(rate==G728_Rate_16000){ 
      prm[0] = (bitstream[0] << 2) | (bitstream[1] >> 6);            
      prm[1] = ( (bitstream[1] & 0x3f) << 4 ) | (bitstream[2] >> 4); 
      prm[2] = ( (bitstream[2] & 0xf) << 6 ) | (bitstream[3] >> 2);  
      prm[3] = ( (bitstream[3] & 0x3) << 8 ) | bitstream[4];         
   }
}

G728_CODECFUN( APIG728_Status, apiG728Decode, (G728Decoder_Obj* dObj,
              unsigned char *packet, short *dst))
{
   Ipp32s shapeIdx, gainIdx, gainLog;
   Ipp16s linearGain, scaleGain;
   Ipp16s nlset;
   Ipp16s nlsst;
   Ipp32s sumunfil, sumfil;
   Ipp16s scale, nlsscale;
   G728_Rate rate = dObj->objPrm.rate;
   G728_Type codecType = dObj->objPrm.type;
   Ipp32s i;
   Ipp32s aa0;
   Ipp16s tmp;
   Ipp16s foo;
   Ipp16s *sst = &dObj->sst_buff[239+1+1];
   Ipp16s *d = &dObj->d_buff[139+1];
   Ipp16s *etpast = &dObj->etpast_buff[140];
   Ipp16s  tiltz;
   Ipp16s codebookIdxs[4];/* four codebook indexes */ 
   int index = 0;

   IPP_ALIGNED_ARRAY(16, Ipp16s, et, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, st, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, temp, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, spf, (IDIM +3));
   IPP_ALIGNED_ARRAY(16, Ipp16s, gtmp, (4 +4));
   IPP_ALIGNED_ARRAY(16, Ipp16s, r, (11 +5));
   IPP_ALIGNED_ARRAY(16, Ipp16s, rtmp, (LPC+1 +5));

   /* unpack bitstream into indexes */ 
   if(codecType == G728_Annex_I) {
      bits2prm(&packet[1], codebookIdxs, rate);
   } else {
      bits2prm(packet, codebookIdxs, rate);
   }

   if(codecType == G728_Annex_I) {
      if((dObj->ferror == G728_FALSE)&&(dObj->afterfe > 0))
         dObj->afterfe -= 1;
      if(dObj->adcount == FESIZE) {
         dObj->adcount = 0;
         /* read dObj->ferror */ 
         dObj->ferror = (Ipp16s)packet[0];
               
         if((dObj->ferror == G728_FALSE)&&(dObj->fecount > 0)) {
            dObj->afterfe += dObj->fecount;
            if(dObj->afterfe > AFTERFEMAX)
               dObj->afterfe = AFTERFEMAX;
            dObj->fecount = 0;
            dObj->seed = 11111;
         }
      }
      if(dObj->ferror == G728_TRUE) {
         dObj->fecount += 1;
         if((dObj->fecount & 3) == 1)
            Set_Flags_and_Scalin_Factor_for_Frame_Erasure(dObj->fecount, dObj->pTab, 
               (Ipp16s)dObj->kp1, &dObj->fedelay, &dObj->fescale, &dObj->nlsfescale, 
               &dObj->voiced, etpast,&dObj->avmag, &dObj->nlsavmag);
      }
      dObj->adcount += 1;
   }

   for(index=1;  index < 5; index++){
      /* Check wether to update filter coefficients */ 
      if(index==3) {
         if((dObj->ferror == G728_FALSE)&&(dObj->illcond==G728_FALSE))
            BandwidthExpansionModul(cnstSynthesisFilterBroadenVector, dObj->tmpSyntFltrCoeffs, 
               dObj->scaleSyntFltrCoeffs, dObj->vecSyntFltrCoeffs, LPC);
         if((dObj->ferror == G728_TRUE)&&((dObj->fecount & 3) == 1))
            BandwidthExpansionModulFE(cnstSynthesisFilterBroadenVectorFE, dObj->tmpSyntFltrCoeffs, 
               dObj->scaleSyntFltrCoeffs, dObj->vecSyntFltrCoeffs, LPC, dObj->fecount, dObj->illcond);
      }
      if((index==2)&&(dObj->illcondg==0)&&(dObj->ferror == G728_FALSE))
         BandwidthExpansionModul(cnstGainPreditorBroadenVector, dObj->tmpLGPredictorCoeffs, 
            dObj->scaleLGPredictorCoeffs, dObj->vecLGPredictorCoeffs, LPCLG);
      /* Obtain the shape index and gain index from codebook idexes */ 
      GetShapeGainIdxs(codebookIdxs[index-1], &shapeIdx, &gainIdx, (Ipp16s)rate);
      /* Get backward-adapted gain */ 
      /* GSTATE[1:9] shifted down 1 position */ 
      LoggainLinearPrediction(dObj->vecLGPredictorCoeffs, dObj->vecLGPredictorState, &gainLog); /* Block 46 */ 
      if(dObj->ferror == G728_FALSE) {
         Log_gain_Limiter_after_erasure(&gainLog, dObj->ogaindb, dObj->afterfe);
         dObj->ogaindb = gainLog;
         InverseLogarithmCalc(gainLog, &linearGain, &scaleGain);
         
         aa0 = dObj->pGq[gainIdx] * linearGain;
         aa0 = ShiftL_32s(aa0, dObj->pNgq[gainIdx]);
         tmp = Cnvrt_NR_32s16s(aa0);
         nlset = scaleGain + dObj->pNgq[gainIdx] + shape_all_nls[shapeIdx] - 8;
         Excitation_VQ_and_gain_scaling(tmp, &shape_all_norm[shapeIdx * IDIM], et);
      }

      if(dObj->ferror == G728_TRUE)
         ExcitationSignalExtrapolation(dObj->voiced, &dObj->fedelay, dObj->fescale, dObj->nlsfescale,
                                       etpast, et, &nlset, &dObj->seed);

      UpdateExcitationSignal(etpast, et, nlset);

      ippsSyntesisFilterZeroInput_G728_16s(dObj->vecSyntFltrCoeffs, et, nlset, st, &nlsst,dObj->syntState);
      /* Update short-term postfilter coefficiens */ 
      if(dObj->pst==1){
         if(index==1 && dObj->illcondp != 1) 
            STPCoeffsCalc(dObj->vecLPCFltrCoeffs, dObj->scaleLPCFltrCoeffs, 
                           dObj->pstA, dObj->rc1, &tiltz);
         if (18 > nlsst) {
            for(i=0; i<IDIM; i++) {
               aa0 = st[i] << (18 - nlsst);
               sst[i] = Cnvrt_NR_32s16s(aa0);
            }
         } else {
            for(i=0; i<IDIM; i++) {
               aa0 = ((int)st[i]) >> (nlsst - 18);
               sst[i] = Cnvrt_NR_32s16s(aa0);
            }
         }
         if (dObj->ip == NPWSZ) 
            dObj->ip = NPWSZ - NFRSZ;
         ippsLPCInverseFilter_G728_16s(sst, dObj->vecLPCFltrCoeffs, d + dObj->ip, dObj->postFltAdapterMem);
         dObj->ip += IDIM;

         if(index==3) {
            /*  Pitch period extraction */ 
            ippsPitchPeriodExtraction_G728_16s (d, &(dObj->kp1), dObj->postFltAdapterMem);
            ippsMove_16s(d-KPMAX+NFRSZ, d-KPMAX, (NPWSZ-NFRSZ+KPMAX));
            /* Compute long-term postfilter coefficients */ 
            LTPCoeffsCalc(sst, dObj->kp1, &dObj->gl, &dObj->glb, &dObj->pTab);
         }  
         /* Long-term and short-term postfilters */ 
         ippsPostFilter_G728_16s(dObj->gl, dObj->glb, (Ipp16s)dObj->kp1, 
               tiltz, dObj->pstA, sst,  temp, dObj->stpMem);
      } else { /*  dObj->pst == 0 */ 
         /* Upscale on Q3 */ 
         if (19 > nlsst) {
            for(i=0; i<IDIM; i++) { 
               aa0 = st[i] << (19 - nlsst);
               sst[i] = Cnvrt_NR_32s16s(aa0);
            }
         } else {
            for(i=0; i<IDIM; i++) { 
               aa0 = ((int)st[i]) >> (nlsst - 19);
               sst[i] = Cnvrt_NR_32s16s(aa0);
            }
         }
         /* only short term postfilter */ 
         ippsIIR16s_G728_16s(dObj->pstA,sst,temp,5,(IppsIIRState_G728_16s*)dObj->stpMem);
      }

      ippsMove_16s(sst-NPWSZ-KPMAX+IDIM, sst-NPWSZ-KPMAX, NPWSZ+KPMAX-5);
      for(i=-5; i < 0; i++)
         sst[i] = sst[i+IDIM] >> 2;

      if(dObj->pst==1) {
         /* Calculate sums of absolute values */ 
         AbsSum_G728_16s32s(sst, &sumunfil);
         AbsSum_G728_16s32s(temp, &sumfil);
         /* Ratio of sums of absolute values */ 
         ScaleFactorCalc(sumunfil, sumfil, &scale, &nlsscale);
         /* Low-pass filter of scaling factor */ 
         /* Gain control of postfilter output */ 
         FirstOrderLowpassFilter_OuputGainScaling(scale, nlsscale, &dObj->scalefil,
            temp, spf);
      }
      /* Update log-gain and gain predictor memory */ 
      if(dObj->ferror == G728_FALSE)
         dObj->vecLGPredictorState[0] = LoggainAdderLimiter(gainLog, (Ipp16s)gainIdx, 
            (Ipp16s)shapeIdx, dObj->pCodebookGain);

      if(dObj->ferror == G728_TRUE) {
         UpdateLGPredictorState(et, dObj->vecLGPredictorState);
         dObj->ogaindb = dObj->vecLGPredictorState[0];
      }

      /* sttmp - cyclic buffer                                    */ 
      /* icount   sttmp                                           */ 
      /*      1   st[ 0: 4],        0,         0,        0,       */ 
      /*      2   st[ 0: 4],st[ 5: 9],         0,        0,       */ 
      /*      3   st[ 0: 4],st[ 5: 9], st[10:14],        0,       */ 
      /*      4   st[ 0: 4],st[ 5: 9], st[10:14],st[15:19],       */ 
      /*      1   st[20:24],st[ 5: 9], st[10:14],st[15:19], ....  */ 
      i = (index - 1) *IDIM;
      ippsCopy_16s(st,&dObj->sttmp[i],5);
      dObj->nlssttmp[index-1] = nlsst;
      /* Start once-per-frame processing */ 
      if(index == 4) {
         short alphatmp;
         dObj->illcond = 0;
         if(ippsWinHybridBlock_G728_16s(dObj->ferror, dObj->sttmp, dObj->nlssttmp, 
            rtmp, dObj->rexpMem ) != ippStsNoErr){
            dObj->illcond = 1;
         }
         ippsZero_16s(dObj->tmpSyntFltrCoeffs, LPC);
         if(dObj->pst==1) {
            LevinsonDurbin(rtmp, 0, 10, dObj->tmpSyntFltrCoeffs, &dObj->rc1, &alphatmp,
               &dObj->scaleSyntFltrCoeffs, &dObj->illcondp, &dObj->illcond);
            /* Save the 10th-order predictor for postfilter use later */ 
            dObj->scaleLPCFltrCoeffs = dObj->scaleSyntFltrCoeffs;
            ippsCopy_16s(dObj->tmpSyntFltrCoeffs, dObj->vecLPCFltrCoeffs, 10);
            /* Continue to finish Levinson Durbin */ 
            if(dObj->ferror == G728_FALSE)
               LevinsonDurbin(rtmp, 10, LPC, dObj->tmpSyntFltrCoeffs,  &foo,
                  &alphatmp,&dObj->scaleSyntFltrCoeffs, &dObj->illcondp, &dObj->illcond);
         } else {
            LevinsonDurbin(rtmp, 0, LPC, dObj->tmpSyntFltrCoeffs, &foo, &foo,
               &dObj->scaleSyntFltrCoeffs, &dObj->illcondp, &dObj->illcond);
         }
      }

      if(index == 1) {
         Ipp16s foo;
         gtmp[0] = dObj->vecLGPredictorState[3];
         gtmp[1] = dObj->vecLGPredictorState[2];
         gtmp[2] = dObj->vecLGPredictorState[1];
         gtmp[3] = dObj->vecLGPredictorState[0];
         /* Block 43 */ 
         dObj->illcondg=0;
         if(ippsWinHybrid_G728_16s(dObj->ferror, gtmp, r, dObj->rexplgMem) != ippStsNoErr){
            dObj->illcondg=1;
         };
         if(dObj->ferror == G728_FALSE) {
            ippsZero_16s(dObj->tmpLGPredictorCoeffs, LPCLG);
            LevinsonDurbin(r, 0, LPCLG, dObj->tmpLGPredictorCoeffs, &foo, &foo,
               &dObj->scaleLGPredictorCoeffs, &dObj->illcondp, &dObj->illcondg);
         }
      }
      /* End once-per-frame processing */ 
      if(dObj->pst==0){
         dst[0] = sst[0];
         dst[1] = sst[1];
         dst[2] = sst[2];
         dst[3] = sst[3];
         dst[4] = sst[4];
      }else{
         dst[0] = ShiftL_16s(spf[0], 1);
         dst[1] = ShiftL_16s(spf[1], 1);
         dst[2] = ShiftL_16s(spf[2], 1);
         dst[3] = ShiftL_16s(spf[3], 1);
         dst[4] = ShiftL_16s(spf[4], 1);
      }
      dst += 5;
   }
   return APIG728_StsNoErr;
}


