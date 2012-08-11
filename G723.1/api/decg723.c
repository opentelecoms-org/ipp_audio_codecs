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
//  Purpose: G.723.1 coder: decode API
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "owng723.h"

static int DecoderObjSize(void){
   return sizeof(G723Decoder_Obj);
}


G723_CODECFUN( APIG723_Status, apiG723Decoder_Alloc, (int *pCodecSize))
{
   *pCodecSize =  DecoderObjSize();
   return APIG723_StsNoErr;
}

G723_CODECFUN( APIG723_Status, apiG723Decoder_Init, 
              (G723Decoder_Obj* decoderObj, unsigned int mode))
{
   ippsZero_16s((short*)decoderObj,sizeof(G723Decoder_Obj)>>1) ;   /* non-initialized data, bug in debug mode AK27.08.01 */
   decoderObj->objPrm.objSize = DecoderObjSize();
   decoderObj->objPrm.mode = mode;
   decoderObj->objPrm.key = G723_DEC_KEY;
   decoderObj->objPrm.rat = 0; /* default 6.3 KBit/s*/

   ippsCopy_16s(LPCDCTbl,decoderObj->PrevLPC,G723_LPC_ORDER);

   decoderObj->PstFltGain = (short) 0x1000 ;
   /* Initialize the CNG */ 
   decoderObj->PastFrameType = G723_ActiveFrm;
   decoderObj->sSidGain = 0;
   decoderObj->CNGSeed = 12345;
   decoderObj->CasheCounter = 0;
   ippsCopy_16s(LPCDCTbl,decoderObj->SIDLSP,G723_LPC_ORDER);

   return APIG723_StsNoErr;
}

G723_CODECFUN( APIG723_Status, apiG723Decoder_InitBuff, 
         (G723Decoder_Obj* decoderObj, char *buff))
{
#if !defined (NO_SCRATCH_MEMORY_USED)

   if(NULL==decoderObj || NULL==buff)
      return APIG723_StsBadArgErr;

   decoderObj->Mem.base = buff;
   decoderObj->Mem.CurPtr = decoderObj->Mem.base;
   decoderObj->Mem.VecPtr = (int *)(decoderObj->Mem.base+G723_ENCODER_SCRATCH_MEMORY_SIZE);

#endif 

   return APIG723_StsNoErr;
}

void DecoderCNG_G723(G723Decoder_Obj* decoderObj, ParamStream_G723 *CurrentParams, short *pExcitation, short *pDstLPC)
{
    int i;

    if(CurrentParams->FrameType == G723_SIDFrm) { /* SID Frame */
        LOCAL_ARRAY(short, qIndex,3, decoderObj);

        DecodeSIDGain_G723_16s(CurrentParams->sAmpIndex[0],&decoderObj->sSidGain);

        /* LSP inverse quantization */
        qIndex[2] =  CurrentParams->lLSPIdx & 0xff;
        qIndex[1] =  (CurrentParams->lLSPIdx>>8) & 0xff;
        qIndex[0] =  (CurrentParams->lLSPIdx>>16) & 0xff;
        if(ippsLSFDecode_G723_16s(qIndex, decoderObj->PrevLPC,  0, decoderObj->SIDLSP) != ippStsNoErr)
           ippsCopy_16s(decoderObj->PrevLPC,decoderObj->SIDLSP,G723_LPC_ORDER);

        LOCAL_ARRAY_FREE(short, qIndex,3, decoderObj);
    } else { /* non SID Frame */
       if(decoderObj->PastFrameType == G723_ActiveFrm) {  /* Case of 1st SID frame erased */
            QuantSIDGain_G723_16s(&decoderObj->sSidGain, &decoderObj->CurrGain, 0, &i);
            DecodeSIDGain_G723_16s(i,&decoderObj->sSidGain);
        }
    }

    if(decoderObj->PastFrameType == G723_ActiveFrm) {
        decoderObj->CurrGain = decoderObj->sSidGain;
    } else { /* update gain */
        decoderObj->CurrGain = ( 7*decoderObj->CurrGain + decoderObj->sSidGain) >> 3 ;
    }
    {

       LOCAL_ARRAY(char, buff,ComfortNoiseExcitation_G723_16s_Buff_Size, decoderObj);

       ComfortNoiseExcitation_G723_16s(decoderObj->CurrGain, decoderObj->PrevExcitation, pExcitation,
                    &decoderObj->CNGSeed, CurrentParams->PitchLag,CurrentParams->AdCdbkLag,CurrentParams->AdCdbkGain, CurrentParams->currRate, buff, &decoderObj->CasheCounter);
       LOCAL_ARRAY_FREE(char, buff,ComfortNoiseExcitation_G723_16s_Buff_Size, decoderObj);
    }


    /* LSP interpolation */
    LSPInterpolation(decoderObj->SIDLSP, decoderObj->PrevLPC, pDstLPC) ;
    ippsCopy_16s(decoderObj->SIDLSP,decoderObj->PrevLPC,G723_LPC_ORDER);

    return;
}

G723_CODECFUN( APIG723_Status, apiG723Decode, 
         (G723Decoder_Obj* decoderObj, const char* src, short badFrameIndicator, short* dst))
{
    int   i,j,k   ;
    short  *pData ;

    LOCAL_ALIGN_ARRAY(32, short, SubZeroLPC,G723_LPC_ORDER+1, decoderObj);
    LOCAL_ARRAY(short, CurrLPC,4*(G723_LPC_ORDER+1), decoderObj);
    LOCAL_ARRAY(short, AdaptiveVector,G723_SBFR_LEN, decoderObj);
    LOCAL_ARRAY(short, CurrLSF,G723_LPC_ORDER, decoderObj);
    LOCAL_ARRAY(short, TemporaryVector,G723_MAX_PITCH+G723_FRM_LEN, decoderObj);
    LOCAL_ARRAY(GainInfo_G723, GainInfo,4, decoderObj);
    ParamStream_G723 CurrentParams;

    if(NULL==decoderObj || NULL==src || NULL ==dst)
      return APIG723_StsBadArgErr;
    if(decoderObj->objPrm.objSize <= 0)
      return APIG723_StsNotInitialized;
    if(G723_DEC_KEY != decoderObj->objPrm.key)
      return APIG723_StsBadCodecType;

    CurrentParams.currRate = (G723_Rate)decoderObj->objPrm.rat;  
    CurrentParams.isBadFrame = badFrameIndicator;

    if(badFrameIndicator == 0) GetParamFromBitstream( src, &CurrentParams); /* Unpack the bitstream */

    if( CurrentParams.FrameType == G723_ActiveFrm) decoderObj->objPrm.rat = CurrentParams.currRate; 
    if ( CurrentParams.isBadFrame != 0 ) {
        if(decoderObj->PastFrameType == G723_ActiveFrm) CurrentParams.FrameType = G723_ActiveFrm;  /* active */
        else CurrentParams.FrameType = G723_UntransmittedFrm;  /* untransmitted */
    }

    if(CurrentParams.FrameType != G723_ActiveFrm) { /* non active frame */

        /* noise generation */
        DecoderCNG_G723(decoderObj, &CurrentParams, dst, CurrLPC);
    } else {
        /* Update erasure count. Section 3.10*/

        if ( 0 != CurrentParams.isBadFrame )
            decoderObj->ErasedFramesCounter++;
        else
            decoderObj->ErasedFramesCounter = 0 ;

        if ( decoderObj->ErasedFramesCounter > 3 )
            decoderObj->ErasedFramesCounter = 3 ;

        {
            LOCAL_ARRAY(short, qIndex,3, decoderObj);
            /* Decode the LSP vector for subframe 3.  Section 3.2 */
            qIndex[2] =  CurrentParams.lLSPIdx & 0xff;
            qIndex[1] =  (CurrentParams.lLSPIdx>>8) & 0xff;
            qIndex[0] =  (CurrentParams.lLSPIdx>>16) & 0xff;
            if(ippsLSFDecode_G723_16s(qIndex,  decoderObj->PrevLPC, CurrentParams.isBadFrame, CurrLSF) != ippStsNoErr)
               ippsCopy_16s(decoderObj->PrevLPC,CurrLSF,G723_LPC_ORDER);

            LOCAL_ARRAY_FREE(short, qIndex,3, decoderObj);
        }

        /* LSP interpolation.  Section 3.3  */
        LSPInterpolation(CurrLSF, decoderObj->PrevLPC, CurrLPC) ;
        ippsCopy_16s(CurrLSF,decoderObj->PrevLPC,G723_LPC_ORDER);

        /* Generate the excitation for the frame */
        ippsCopy_16s(decoderObj->PrevExcitation,TemporaryVector,G723_MAX_PITCH);

        pData = &TemporaryVector[G723_MAX_PITCH] ;

        if ( 0 == decoderObj->ErasedFramesCounter ) {
            /*update the interpolation gain memory.*/
            decoderObj->InterpolatedGain = -GainDBLvls[(CurrentParams.sAmpIndex[2]+CurrentParams.sAmpIndex[3])>>1];

            for ( i = 0 ; i < 4 ; i ++ ) {
               int lLag;
               short sGain;

                /* Fixed codebook excitation. Section 3.5 */
                FixedCodebookVector_G723_16s(CurrentParams.sPosition[i], CurrentParams.sAmplitude[i], 
                   CurrentParams.sAmpIndex[i], CurrentParams.sGrid[i], CurrentParams.AdCdbkGain[i],
                   i, CurrentParams.currRate, pData, &lLag, &sGain ) ;
                if ( G723_Rate63 == CurrentParams.currRate ){
                   if(1 == CurrentParams.sTrainDirac[i] ){
                      short   Tmp[G723_SBFR_LEN] ;
                      ippsCopy_16s(pData,Tmp,G723_SBFR_LEN);
                      /*  Generation of a Dirac train. Section 2.15 */
                      for ( j = CurrentParams.PitchLag[i>>1]; j < G723_SBFR_LEN ; j += CurrentParams.PitchLag[i>>1] ) {
                         ippsAdd_16s_I(Tmp,&pData[j],G723_SBFR_LEN-j);
                      }
                   }
                }else{ /*rate53*/
                   lLag += CurrentParams.PitchLag[i>>1]-1+CurrentParams.AdCdbkLag[i]; 
                   if(lLag < G723_SBFR_LEN-2) {
                      ippsHarmonicFilter_16s_I(sGain,lLag,&pData[lLag],G723_SBFR_LEN-lLag);
                   }
                }

                /* Adaptive codebook excitation. Section 3.4 */

                ippsDecodeAdaptiveVector_G723_16s(CurrentParams.PitchLag[i>>1],CurrentParams.AdCdbkLag[i],CurrentParams.AdCdbkGain[i],
                                                  &TemporaryVector[G723_SBFR_LEN*i], AdaptiveVector, SA_Rate[CurrentParams.currRate]);

                /* Codebook contributions to excitation. */
                ippsLShiftC_16s_I(1,pData,G723_SBFR_LEN);
                ippsAdd_16s_I(AdaptiveVector,pData,G723_SBFR_LEN);
                pData += G723_SBFR_LEN ;
            }

            ippsCopy_16s(&TemporaryVector[G723_MAX_PITCH],dst,G723_FRM_LEN);

            /* Interpolation index calculation. Section 3.10 */
             InterpolationIndex_G723_16s( TemporaryVector, CurrentParams.PitchLag[1],
                                       &decoderObj->sSidGain, &decoderObj->CurrGain, &decoderObj->InterpolationIdx );

            /* Pitch post filter coefficients calculation.  Section 3.6 */
            if ( decoderObj->objPrm.mode&G723Decode_PF_Enabled )
                for ( i = 0 ; i < 4 ; i ++ )
                   ippsPitchPostFilter_G723_16s(CurrentParams.PitchLag[i>>1], &TemporaryVector[G723_MAX_PITCH], &GainInfo[i].sDelay, 
                                    &GainInfo[i].sGain, &GainInfo[i].sScalingGain, (short)i, SA_Rate[CurrentParams.currRate]);                             
                         

            ippsCopy_16s(decoderObj->PrevExcitation,TemporaryVector,G723_MAX_PITCH);
            ippsCopy_16s(dst,&TemporaryVector[G723_MAX_PITCH],G723_FRM_LEN);

            /* Pitch post filtering. Section 3.6 */
            if ( decoderObj->objPrm.mode&G723Decode_PF_Enabled )
                for ( i = 0 ; i < 4 ; i ++ ){
                   ippsInterpolateC_NR_G729_16s_Sfs(
                      &TemporaryVector[G723_MAX_PITCH+i*G723_SBFR_LEN], GainInfo[i].sScalingGain,
                      &TemporaryVector[G723_MAX_PITCH+i*G723_SBFR_LEN+GainInfo[i].sDelay], GainInfo[i].sGain,
                      &dst[i*G723_SBFR_LEN],G723_SBFR_LEN,15);
                }

            ippsCopy_16s(decoderObj->PrevLPC,decoderObj->SIDLSP,G723_LPC_ORDER);
        } else {
            /* Frame erasure. Section 3.10 */
            /*compute the interpolation gain.*/
            decoderObj->InterpolatedGain =  (3 * decoderObj->InterpolatedGain + 2 ) >> 2 ;
            if ( decoderObj->ErasedFramesCounter >= 3 ) {    /* Test for clearing */
               ippsZero_16s(dst,G723_FRM_LEN);
               ippsZero_16s(TemporaryVector,G723_FRM_LEN+G723_MAX_PITCH);
            }else{
               ResidualInterpolation_G723_16s_I(TemporaryVector,  dst, decoderObj->InterpolationIdx, 
                                                    decoderObj->InterpolatedGain, &decoderObj->ResIntSeed ) ;
            }
        }

        ippsCopy_16s(&TemporaryVector[G723_FRM_LEN],decoderObj->PrevExcitation,G723_MAX_PITCH);
        decoderObj->CNGSeed = 12345;
        decoderObj->CasheCounter = 0;
    }

    decoderObj->PastFrameType = CurrentParams.FrameType;

    /* Speech synthesis. Section 3.7 */
    pData = dst ;
    for ( i = 0 ; i < 4 ; i ++ ) {

        SubZeroLPC[0] = CurrLPC[i*(G723_LPC_ORDER+1)];
        for(k=1; k<G723_LPC_ORDER+1; k++) SubZeroLPC[k] = -CurrLPC[i*(G723_LPC_ORDER+1)+k];  
        ippsSynthesisFilter_NR_16s_ISfs(SubZeroLPC, pData, G723_SBFR_LEN, 13, decoderObj->SyntFltIIRMem);
        ippsCopy_16s(&pData[G723_SBFR_LEN-G723_LPC_ORDER], decoderObj->SyntFltIIRMem, G723_LPC_ORDER);

        if ( decoderObj->objPrm.mode&G723Decode_PF_Enabled ) {

           /* Formant post filter. Section 3.8 */
           PostFilter(decoderObj, pData, &CurrLPC[i*(G723_LPC_ORDER+1)+1] ) ;
        }else{
           ippsMulC_16s_I(2,pData,G723_SBFR_LEN);
        }
        pData += G723_SBFR_LEN ;
    }

    CLEAR_SCRATCH_MEMORY(decoderObj);

    return APIG723_StsNoErr;
}
