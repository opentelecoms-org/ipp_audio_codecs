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
//  Purpose: G.723.1 coder: internal functions & tables
//
*/

#include <stdlib.h>
#include <stdio.h>

#include "owng723.h"

typedef enum {
   G723_Rate63Frame = 0,
   G723_Rate53Frame,
   G723_SIDFrame,
   G723_UntransmittedFrame
}G723_Info_Bit;

static int  ExtractBits( const char **pBitStrm, int *currBitstrmOffset, int numParamBits )
{
    int     i ;
    int  unpackedParam = 0 ;

    for ( i = 0 ; i < numParamBits ; i ++ ){
        int  lTemp;
        lTemp = ((*pBitStrm)[(i + *currBitstrmOffset)>>3] >> ((i + *currBitstrmOffset) & 0x7)) & 1;
        unpackedParam +=  lTemp << i ;
    }

    *pBitStrm += (numParamBits + *currBitstrmOffset)>>3;
    *currBitstrmOffset = (numParamBits + *currBitstrmOffset) & 0x7;

    return unpackedParam ;
}

void GetParamFromBitstream( const char *pSrcBitStream, ParamStream_G723 *Params)
{
   int   i;
   int  lTmp ;
   short  InfoBit;
   short Bound_AcGn;
   const char **pBitStream = &pSrcBitStream;
   int nBit = 0;

   /* Extract the frame type and rate info */
   InfoBit = ExtractBits(pBitStream, &nBit, 2);
   if ( InfoBit == G723_UntransmittedFrame ) {
      Params->FrameType = G723_UntransmittedFrm; Params->lLSPIdx = 0;    
      return;
   } 
   if ( InfoBit == G723_SIDFrame ) { /* SID frame*/
      Params->lLSPIdx = ExtractBits(pBitStream, &nBit, 24);/* LspId */
      Params->sAmpIndex[0] = ExtractBits(pBitStream, &nBit, 6); /* Gain */
      Params->FrameType = G723_SIDFrm; return;
   }
   if ( InfoBit == G723_Rate53Frame ) {
      Params->FrameType = G723_ActiveFrm;
      Params->currRate = G723_Rate53;
      Params->lLSPIdx = ExtractBits(pBitStream, &nBit, 24);/* LspId */
      Params->PitchLag[0] = ExtractBits(pBitStream, &nBit, 7);
      Params->AdCdbkLag[1] = ExtractBits(pBitStream, &nBit, 2);
      Params->PitchLag[1] = ExtractBits(pBitStream, &nBit, 7);
      Params->AdCdbkLag[3] = ExtractBits(pBitStream, &nBit, 2);

      Params->AdCdbkLag[0] = Params->AdCdbkLag[2] = 1;

      Params->AdCdbkGain[0] = ExtractBits(pBitStream, &nBit, 12);
      Params->AdCdbkGain[1] = ExtractBits(pBitStream, &nBit, 12);
      Params->AdCdbkGain[2] = ExtractBits(pBitStream, &nBit, 12);
      Params->AdCdbkGain[3] = ExtractBits(pBitStream, &nBit, 12);

      Params->sGrid[0] = ExtractBits(pBitStream, &nBit, 1);
      Params->sGrid[1] = ExtractBits(pBitStream, &nBit, 1);
      Params->sGrid[2] = ExtractBits(pBitStream, &nBit, 1);
      Params->sGrid[3] = ExtractBits(pBitStream, &nBit, 1);

      Params->sPosition[0] = ExtractBits(pBitStream, &nBit, 12);
      Params->sPosition[1] = ExtractBits(pBitStream, &nBit, 12);
      Params->sPosition[2] = ExtractBits(pBitStream, &nBit, 12);
      Params->sPosition[3] = ExtractBits(pBitStream, &nBit, 12);

      Params->sAmplitude[0] = ExtractBits(pBitStream, &nBit, 4);
      Params->sAmplitude[1] = ExtractBits(pBitStream, &nBit, 4);
      Params->sAmplitude[2] = ExtractBits(pBitStream, &nBit, 4);
      Params->sAmplitude[3] = ExtractBits(pBitStream, &nBit, 4);
      /* Frame erasure concealment and parameters scaling*/
      if( Params->PitchLag[0] <= 123) {
        Params->PitchLag[0] += G723_MIN_PITCH ;
      } else {
        Params->isBadFrame = 1; /* transmission error */
        return;
      }
      if( Params->PitchLag[1] <= 123) {
        Params->PitchLag[1] += G723_MIN_PITCH ;
      } else {
        Params->isBadFrame = 1; /* transmission error */
        return;
      }

      for ( i = 0 ; i < 4 ; i ++ ) {
         Params->sTrainDirac[i] = 0 ;
         Bound_AcGn = AdCdbkSizeLowRate;
         lTmp = Params->AdCdbkGain[i];
         Params->AdCdbkGain[i] = (short)(lTmp/N_GAINS) ;
         if(Params->AdCdbkGain[i] < Bound_AcGn ) {
            Params->sAmpIndex[i] = (short)(lTmp % N_GAINS) ;
         } else {          
            Params->isBadFrame = 1; /* transmission error */
            return ;
         } 
      } 
      return;
   }
   if (InfoBit == G723_Rate63Frame) {
      Params->FrameType = G723_ActiveFrm;
      Params->currRate = G723_Rate63;
      Params->lLSPIdx = ExtractBits(pBitStream, &nBit, 24);/* LspId */

      Params->PitchLag[0] = ExtractBits(pBitStream, &nBit, 7);
      Params->AdCdbkLag[1] = ExtractBits(pBitStream, &nBit, 2);
      Params->PitchLag[1] = ExtractBits(pBitStream, &nBit, 7);
      Params->AdCdbkLag[3] = ExtractBits(pBitStream, &nBit, 2);

      Params->AdCdbkLag[0] = Params->AdCdbkLag[2] = 1;

      Params->AdCdbkGain[0] = ExtractBits(pBitStream, &nBit, 12);
      Params->AdCdbkGain[1] = ExtractBits(pBitStream, &nBit, 12);
      Params->AdCdbkGain[2] = ExtractBits(pBitStream, &nBit, 12);
      Params->AdCdbkGain[3] = ExtractBits(pBitStream, &nBit, 12);

      Params->sGrid[0] = ExtractBits(pBitStream, &nBit, 1);
      Params->sGrid[1] = ExtractBits(pBitStream, &nBit, 1);
      Params->sGrid[2] = ExtractBits(pBitStream, &nBit, 1);
      Params->sGrid[3] = ExtractBits(pBitStream, &nBit, 1);

      ExtractBits(pBitStream, &nBit, 1);/* Skip the reserved bit for align pack*/

      lTmp = ExtractBits(pBitStream, &nBit, 13); /*Exract the position,s MSB*/
                                         /* lTmp = 810*x0 + 90*x1 + 9*x2 + x3*/
      Params->sPosition[0] = lTmp / 810;
      lTmp -= Params->sPosition[0]*810;       /* lTmp =          90*x1 + 9*x2 + x3*/
      Params->sPosition[1] = lTmp / 90;
      lTmp -= Params->sPosition[1]*90;        /* lTmp =                  9*x2 + x3*/
      Params->sPosition[2] = lTmp / 9;
      Params->sPosition[3] = lTmp - (Params->sPosition[2]*9);

      /* Extract all the pulse positions */
      Params->sPosition[0] = ( Params->sPosition[0] << 16 ) + ExtractBits(pBitStream, &nBit, 16) ;
      Params->sPosition[1] = ( Params->sPosition[1] << 14 ) + ExtractBits(pBitStream, &nBit, 14) ;
      Params->sPosition[2] = ( Params->sPosition[2] << 16 ) + ExtractBits(pBitStream, &nBit, 16) ;
      Params->sPosition[3] = ( Params->sPosition[3] << 14 ) + ExtractBits(pBitStream, &nBit, 14) ;

      /* Extract pulse amplitudes */
      Params->sAmplitude[0] = ExtractBits(pBitStream, &nBit, 6);
      Params->sAmplitude[1] = ExtractBits(pBitStream, &nBit, 5);
      Params->sAmplitude[2] = ExtractBits(pBitStream, &nBit, 6);
      Params->sAmplitude[3] = ExtractBits(pBitStream, &nBit, 5);

      /* Frame erasure concealment and parameters scaling*/

      if( Params->PitchLag[0] <= 123) {
        Params->PitchLag[0] += G723_MIN_PITCH ;
      } else {
        Params->isBadFrame = 1; /* transmission error */
        return;
      }
      if( Params->PitchLag[1] <= 123) {
        Params->PitchLag[1] += G723_MIN_PITCH ;
      } else {
        Params->isBadFrame = 1; /* transmission error */
        return;
      }
      for ( i = 0 ; i < 4 ; i ++ ) {
         lTmp = Params->AdCdbkGain[i];
         Params->sTrainDirac[i] = 0 ;
         Bound_AcGn = AdCdbkSizeLowRate ;
         if (Params->PitchLag[i>>1] < (G723_SBFR_LEN-2)) {
            Params->sTrainDirac[i] = (short)(lTmp >> 11) ;
            lTmp &= 0x7ff ;
            Bound_AcGn = AdCdbkSizeHighRate ;
         } 
         Params->AdCdbkGain[i] = (short)(lTmp/N_GAINS) ;
         if(Params->AdCdbkGain[i] < Bound_AcGn ) {
            Params->sAmpIndex[i] = (short)(lTmp % N_GAINS) ;
         } else {          
            Params->isBadFrame = 1; /* transmission error */
            return ;
         } 
      } 
      return;
   }
   return;
}

static short* Parm2Bits( int param, short *pBitStrm, int ParamBitNum )
{
   int i;
   for ( i = 0 ; i < ParamBitNum ; i ++ ) {
      *pBitStrm ++ = (short) (param & 0x1);
      param >>= 1;
   } 

   return pBitStrm;
}
void    SetParam2Bitstream(G723Encoder_Obj* encoderObj, ParamStream_G723 *Params, char *pDstBitStream)
{
    int     i;
    short *pBitStream;
    int  MSBs;

    LOCAL_ARRAY(short, Bits,192,encoderObj);

    pBitStream = Bits;

    switch (Params->FrameType) {

    case 0 :  /* untransmitted */
       pDstBitStream[0] = 0;
       pBitStream = Parm2Bits( G723_UntransmittedFrame, pBitStream, 2 );
       for ( i = 0 ; i < 2 ; i ++ )
         pDstBitStream[i>>3] ^= Bits[i] << (i & 0x7);

       LOCAL_ARRAY_FREE(short, Bits,192,encoderObj);
       return;
    case 2 :  /* SID */
       ippsZero_8u(pDstBitStream, 4);
       pBitStream = Parm2Bits( G723_SIDFrame, pBitStream, 2 );

       pBitStream = Parm2Bits( Params->lLSPIdx, pBitStream, 24 ); /* 24 bit lLSPIdx */
       pBitStream = Parm2Bits( Params->sAmpIndex[0], pBitStream, 6 );/* Do Sid frame gain */

       for ( i = 0 ; i < 32 ; i ++ )
          pDstBitStream[i>>3] ^= Bits[i] << (i & 0x7) ;

       LOCAL_ARRAY_FREE(short, Bits,192,encoderObj);
       return;
    default : 
       if ( Params->currRate == G723_Rate63 ){
            ippsZero_8u(pDstBitStream, 24);
            pBitStream = Parm2Bits( G723_Rate63Frame, pBitStream, 2 );

            pBitStream = Parm2Bits( Params->lLSPIdx, pBitStream, 24 ) ; /* 24 bit LspId */
            pBitStream = Parm2Bits( Params->PitchLag[0] -  G723_MIN_PITCH, pBitStream, 7 ) ; /* Adaptive codebook lags */
            pBitStream = Parm2Bits( Params->AdCdbkLag[1], pBitStream, 2 ) ;
            pBitStream = Parm2Bits( Params->PitchLag[1] -  G723_MIN_PITCH, pBitStream, 7 ) ;
            pBitStream = Parm2Bits( Params->AdCdbkLag[3], pBitStream, 2 ) ;

            /* Write gains indexes of 12 bit */
            for ( i = 0 ; i < 4 ; i ++ ) {
               pBitStream = Parm2Bits( Params->AdCdbkGain[i]*N_GAINS + Params->sAmpIndex[i] + (Params->sTrainDirac[i] << 11), pBitStream, 12 ) ;
            }

            for ( i = 0 ; i < 4 ; i ++ )
               *pBitStream ++ = Params->sGrid[i] ; /* Write all the Grid indices */

            *pBitStream ++ = 0 ; /* reserved bit */
            /* Write 13 bit combined position index, pack 4 MSB from each pulse position. */
            MSBs = (Params->sPosition[0] >> 16) * 810 + ( Params->sPosition[1] >> 14)*90 + 
                   (Params->sPosition[2] >> 16) * 9 + ( Params->sPosition[3] >> 14 );
            pBitStream = Parm2Bits(MSBs, pBitStream, 13);

            /* Write all the pulse positions */
            pBitStream = Parm2Bits(Params->sPosition[0] & 0xffff, pBitStream, 16);
            pBitStream = Parm2Bits(Params->sPosition[1] & 0x3fff, pBitStream, 14);
            pBitStream = Parm2Bits(Params->sPosition[2] & 0xffff, pBitStream, 16);
            pBitStream = Parm2Bits(Params->sPosition[3] & 0x3fff, pBitStream, 14);

            /* Write pulse amplitudes */
            pBitStream = Parm2Bits(Params->sAmplitude[0], pBitStream, 6);
            pBitStream = Parm2Bits(Params->sAmplitude[1] , pBitStream, 5);
            pBitStream = Parm2Bits(Params->sAmplitude[2], pBitStream, 6);
            pBitStream = Parm2Bits(Params->sAmplitude[3], pBitStream, 5);

            for ( i = 0 ; i < 192 ; i ++ )
               pDstBitStream[i>>3] ^= Bits[i] << (i & 0x7);
       }else{
            ippsZero_8u(pDstBitStream, 20);
            pBitStream = Parm2Bits( G723_Rate53Frame, pBitStream, 2 );

            pBitStream = Parm2Bits( Params->lLSPIdx, pBitStream, 24 ) ; /* 24 bit lLSPIdx */
            pBitStream = Parm2Bits( Params->PitchLag[0] -  G723_MIN_PITCH, pBitStream, 7 ) ; /* Adaptive codebook lags */
            pBitStream = Parm2Bits( Params->AdCdbkLag[1], pBitStream, 2 ) ;
            pBitStream = Parm2Bits( Params->PitchLag[1] -  G723_MIN_PITCH, pBitStream, 7 ) ;
            pBitStream = Parm2Bits( Params->AdCdbkLag[3], pBitStream, 2 ) ;

            /* Write gains indexes of 12 bit */
            for ( i = 0 ; i < 4 ; i ++ )
               pBitStream = Parm2Bits( Params->AdCdbkGain[i]*N_GAINS + Params->sAmpIndex[i], pBitStream, 12 );

            for ( i = 0 ; i < 4 ; i ++ )
               *pBitStream ++ = Params->sGrid[i] ; /* Write all the Grid indices */

            /* Write positions */
            for ( i = 0 ; i < 4 ; i ++ ) {
                pBitStream = Parm2Bits( Params->sPosition[i], pBitStream, 12 ) ;
            }
            /* Write Pamps */
            for ( i = 0 ; i < 4 ; i ++ ) {
                pBitStream = Parm2Bits( Params->sAmplitude[i], pBitStream, 4 ) ;
            }
            for ( i = 0 ; i < 160 ; i ++ )
               pDstBitStream[i>>3] ^= Bits[i] << (i & 0x7) ;
       }
       LOCAL_ARRAY_FREE(short, Bits,192,encoderObj);
       return;
    }

    LOCAL_ARRAY_FREE(short, Bits,192,encoderObj);
    return;
}

void   UpdateSineDetector(short *SineWaveDetector, short *isNotSineWave)
{
   int i, x, lNumTimes;

   *SineWaveDetector &= 0x7fff ;
   *isNotSineWave = 1;

   x= *SineWaveDetector;
   lNumTimes = 0;
   for ( i = 0 ; i < 15 ; i ++ ) {
       lNumTimes += x&1;
       x >>= 1;
   }
   if ( lNumTimes >= 14 ) /* Sine wave is detected*/
       *isNotSineWave = -1;
}
/*Usage GenPostFiltTable(Tbl, 0.65); GenPostFiltTable(Tbl+10, 0.75); */
#define SATURATE_fs(dVal,dRez)\
   if (dVal >= 0.0) dVal += 0.5;\
   else  dVal -= 0.5;\
   dRez = (short)dVal;

static void GenPostFiltTable(short *Tbl, double dInit)
{
   double dFac,dTmp;
   int i;
   dFac = dInit;
   for(i=0;i<10;i++) {
      dTmp = dFac*32768.f;
      SATURATE_fs(dTmp,Tbl[i]);
      dFac = dFac*dInit;
   }
}
static const __ALIGN(16) short   PostFiltTable[2*G723_LPC_ORDER] = {
   21299 ,  13844,8999,5849,3802,2471,1606,1044, 679, 441 , /* Zero part */
   24576,18432,13824,10368,7776,5832,4374,3281,2460,1845 , /* Pole part */
};
void  PostFilter(G723Decoder_Obj* decoderObj, short *pSrcDstSignal, short *pSrcLPC )
{
   int   i, lTmp, lSignalEnergy, lSfs;
   short sTmp;

   LOCAL_ARRAY(short, FltCoef,2*G723_LPC_ORDER, decoderObj);
   LOCAL_ARRAY(int, pFltSignal,G723_LPC_ORDER+G723_SBFR_LEN, decoderObj);
   LOCAL_ARRAY(int, pAutoCorr,2, decoderObj);
   LOCAL_ARRAY(short, pTmpVec,G723_SBFR_LEN, decoderObj);

   /* Normalize the input speech vector.  */
   lSfs=3;
   ippsAutoScale_16s(pSrcDstSignal,  pTmpVec, (short) G723_SBFR_LEN, &lSfs ) ;

   /* Compute the first two autocorrelation coefficients*/
   ippsDotProd_16s32s_Sfs(pTmpVec,pTmpVec,G723_SBFR_LEN,pAutoCorr,0);
   ippsDotProd_16s32s_Sfs(pTmpVec,pTmpVec+1,G723_SBFR_LEN-1,pAutoCorr+1,0); 
   /* Compute new reflection coefficient.*/
   sTmp = pAutoCorr[0]>>15;
   if ( sTmp ) {
      sTmp = (pAutoCorr[1]>>1)/(sTmp); 
   }

   /* Compute interpolated reflection coefficient use the new and previouse one.*/
   lTmp =  ((decoderObj->ReflectionCoeff << 2) - decoderObj->ReflectionCoeff + sTmp);
   decoderObj->ReflectionCoeff = (lTmp+0x2)>>2;

   sTmp  = (decoderObj->ReflectionCoeff * SmoothCoeff)>>15;
   sTmp &= 0xfffc ;

   /* Compute FIR and IIR coefficients. Note the table can be generated using the GenPostFiltTable function */
   ippsMul_NR_16s_Sfs(pSrcLPC,PostFiltTable,FltCoef,G723_LPC_ORDER,15);
   ippsMul_NR_16s_Sfs(pSrcLPC,PostFiltTable+G723_LPC_ORDER,FltCoef+G723_LPC_ORDER,G723_LPC_ORDER,15);
   
   /* 32s output needs for compensate filter */
   for(i=0; i<G723_LPC_ORDER; i++){
      pFltSignal[i] = decoderObj->PostFilterMem[G723_LPC_ORDER+i]<<16; 
   }
   ippsIIR16s_G723_16s32s(FltCoef,pSrcDstSignal,&pFltSignal[G723_LPC_ORDER],decoderObj->PostFilterMem); 

   /* perform the tilt fitering. */
   ippsTiltCompensation_G723_32s16s(sTmp, &pFltSignal[G723_LPC_ORDER-1], pSrcDstSignal);

   /* Gain scaling.  Section 3.9 */
   /* Compute normalized signal energy.*/
   sTmp = (short)(2*lSfs + 3);
   if (sTmp < 0) {
      lSignalEnergy = ShiftL_32s(pAutoCorr[0],(unsigned short)(-sTmp));
   } else {
      lSignalEnergy = pAutoCorr[0]>>sTmp;
   } 
   ippsGainControl_G723_16s_I(lSignalEnergy,pSrcDstSignal,&decoderObj->PstFltGain);

   LOCAL_ARRAY_FREE(short, pTmpVec,G723_SBFR_LEN, decoderObj);
   LOCAL_ARRAY_FREE(int, pAutoCorr,2, decoderObj);
   LOCAL_ARRAY_FREE(int, pFltSignal,G723_LPC_ORDER+G723_SBFR_LEN, decoderObj);
   LOCAL_ARRAY_FREE(short, FltCoef,2*G723_LPC_ORDER, decoderObj);

   return;
}

void  LSPInterpolation(const short *pSrcLSP, const short *pSrcPrevLSP, short *pDstLPC)
{

    pDstLPC[0] = 4096;
    ippsInterpolateC_NR_G729_16s_Sfs(pSrcLSP,4096,pSrcPrevLSP,12288,&pDstLPC[1],G723_LPC_ORDER,14);
    ippsLSFToLPC_G723_16s(&pDstLPC[1], &pDstLPC[1]);

    pDstLPC[G723_LPC_ORDERP1] = 4096;
    ippsInterpolateC_NR_G729_16s_Sfs(pSrcLSP,8192,pSrcPrevLSP,8192,&pDstLPC[1+G723_LPC_ORDERP1],G723_LPC_ORDER,14);
    ippsLSFToLPC_G723_16s(&pDstLPC[1+G723_LPC_ORDERP1], &pDstLPC[1+G723_LPC_ORDERP1]);

    pDstLPC[2*G723_LPC_ORDERP1] = 4096;
    ippsInterpolateC_NR_G729_16s_Sfs(pSrcLSP,12288,pSrcPrevLSP,4096,&pDstLPC[1+2*G723_LPC_ORDERP1],G723_LPC_ORDER,14);
    ippsLSFToLPC_G723_16s(&pDstLPC[1+2*G723_LPC_ORDERP1], &pDstLPC[1+2*G723_LPC_ORDERP1]);

    pDstLPC[3*G723_LPC_ORDERP1] = 4096;
    ippsCopy_16s(pSrcLSP,&pDstLPC[1+3*G723_LPC_ORDERP1],G723_LPC_ORDER);
    ippsLSFToLPC_G723_16s(&pDstLPC[1+3*G723_LPC_ORDERP1], &pDstLPC[1+3*G723_LPC_ORDERP1]);

    return;
}

static void GetAverScaleTable(short *pTbl)
{
   int i;
   double dTmp, dAlphaW=2.70375;
   dTmp = 32768.f/((double)G723_HALFFRM_LEN);
   SATURATE_fs(dTmp,pTbl[0]);
   for(i=1;i<4;i++) {
      dTmp = 32768.f*dAlphaW*dAlphaW/((double)i*(double)G723_FRM_LEN);
      SATURATE_fs(dTmp,pTbl[i]);
   }
   return;
}

static const short AverScaleTbl_G723[4] = {273, 998, 
                              499, 333}; /*This table can be generateg using GetAverScaleTable function.*/
static int LogEnerLevel[3] = {2048, 18432,
                 231233};
static const short FirstCode[3] = {0, 32, 
                                    96};

void QuantSIDGain_G723_16s(const Ipp16s *pSrc, const Ipp16s *pSrcSfs, int len, int *pIndx)  
{
   short sTmp, sNseg, sNsegP1;
   short j, m, k, expBase;
   int lTmp1, lTmp2, lTmp3;
   short sfs;
   int i;

   if(len == 0) {
      /* Quantize energy  */
      sTmp = (*pSrcSfs)<<1;
      sTmp = 16 - sTmp;
      lTmp1 = MulC_32s(AverScaleTbl_G723[0], (*pSrc)<<sTmp);
   } else {
      /* Compute weighted average sum*/
      sfs = pSrcSfs[0];
      for(i=1; i<len; i++) {
         if(pSrcSfs[i] < sfs) sfs = pSrcSfs[i];
      }
      for(i=0, lTmp1=0; i<len; i++) {
         sTmp = pSrcSfs[i] - sfs;
         sTmp = pSrc[i]>>sTmp;
         sTmp = ((AverScaleTbl_G723[len]*sTmp)+0x4000)>>15;
         lTmp1 += sTmp;
      }
      sTmp = 15 - sfs;
      if(sTmp < 0) lTmp1 >>= (-sTmp);
      else         lTmp1 <<= sTmp;
   }  

   *pIndx = 63; 
   if(lTmp1 < LogEnerLevel[2]) {

      /* Compute segment number */
      if(lTmp1 >= LogEnerLevel[1]) {
         expBase = 4; sNseg = 2;
      } else {
         expBase  = 3;
         sNseg=(lTmp1 >= LogEnerLevel[0])?1:0;
      } 

      sNsegP1 = sNseg + 1;
      j = 1<<expBase;
      k = j>>1;

      /* Binary search */
      for(i=0; i<expBase; i++) {
         sTmp = FirstCode[sNseg] + (j<<sNsegP1);
         lTmp2 = 2*sTmp * sTmp;
         if(lTmp1 >= lTmp2) j += k;
         else j -= k;
         k >>= 1;
      }

      sTmp = FirstCode[sNseg] + (j<<sNsegP1);
      lTmp2 = 2* sTmp * sTmp - lTmp1;
      if(lTmp2 <= 0) {
         m    = j + 1;
         sTmp  = FirstCode[sNseg] + (m<<sNsegP1);
         lTmp3 = lTmp1 - 2 * sTmp * sTmp;
         if(lTmp2 > lTmp3) sTmp = (sNseg<<4) + j;
         else sTmp = (sNseg<<4)+m;
      } else {
         m    = j - 1;
         sTmp  = FirstCode[sNseg] + (m<<sNsegP1);
         lTmp3 = lTmp1 - 2 * sTmp * sTmp;
         if(lTmp2 < lTmp3) sTmp = (sNseg<<4) + j;
         else sTmp = (sNseg<<4) + m;
      }     
      *pIndx = sTmp;
   } 
   return;
}

void DecodeSIDGain_G723_16s (int pIndx, Ipp16s *pGain)  
{
   short i, sNseg;
   short sTmp;

   sNseg = pIndx>>4;
   if(sNseg == 3) sNseg = 2;
   i = pIndx - (sNseg<<4);
   sTmp = sNseg + 1;
   sTmp = i<<sTmp;
   sTmp += FirstCode[sNseg];  /* SidGain */
   sTmp <<= 5; /* << 5 */
   *pGain = sTmp;

   return;
}

void FixedCodebookGain_G723_16s(const Ipp16s *pSrc1, const Ipp16s *pSrc2, 
                                Ipp16s *pGainCoeff, int *pGainIdx, short *pAlignTmp)
{
   short i;
   short sNormCorr, sNormEnergy, sCorrSFS, sEnergySFS, sBestQntGain, sCurrGain;
   int lCorr, lEnergy;
   short sCurrDist, sMinDist;

   ippsRShiftC_16s (pSrc2, 3, pAlignTmp, G723_SBFR_LEN); /* to avoid overflow */

   ippsDotProd_16s32s_Sfs(pSrc1, pAlignTmp, G723_SBFR_LEN,&lCorr,0);

   sBestQntGain = 0;

   if(lCorr > 0) {
      sCorrSFS = Norm_32s_I(&lCorr);
      sNormCorr = lCorr>>17; /* Be sure sNormCorr < sNormEnergy */

      ippsDotProd_16s32s_Sfs(pAlignTmp,pAlignTmp,G723_SBFR_LEN, &lEnergy,0);

      sEnergySFS = Norm_32s_I(&lEnergy);
      sNormEnergy     = lEnergy>>16;   

      sCurrGain = (sNormEnergy > 0)? (sNormCorr<<15)/sNormEnergy : IPP_MAX_16S;/* compute sCurrGain = sNormCorr/sNormEnergy */

      i = sCorrSFS - sEnergySFS + 5;          /* Denormalization of division */

      if(i < 0) sCurrGain = ShiftL_16s(sCurrGain, (unsigned short)(-i));
      else
         sCurrGain >>= i;

      sBestQntGain = 0;
      sMinDist = abs(sCurrGain + GainDBLvls[0]);
      for ( i =  1; i <N_GAINS ; i ++ ) {
         sCurrDist = abs(sCurrGain + GainDBLvls[i]);
         if ( sCurrDist< sMinDist) { sMinDist = sCurrDist; sBestQntGain = i; }
      }
   }
   *pGainCoeff = -GainDBLvls[sBestQntGain];
   *pGainIdx = sBestQntGain;
}

void ExcitationResidual_G723_16s(const Ipp16s *pSrc1, const Ipp16s *pSrc2,  Ipp16s *pSrcDst,G723Encoder_Obj *encoderObj)
{
   int i;

   LOCAL_ARRAY(int, lConv,G723_SBFR_LEN,encoderObj) ; 
   LOCAL_ARRAY(short, sNormConv,G723_SBFR_LEN,encoderObj) ;

   ippsConvPartial_16s32s(pSrc1,pSrc2,lConv,G723_SBFR_LEN);
   for ( i = 0 ; i < G723_SBFR_LEN ; i ++ ) {
       sNormConv[i] = (-lConv[i] + 0x2000) >> 14;
   }
   ippsAdd_16s_I(sNormConv,pSrcDst,G723_SBFR_LEN);

   LOCAL_ARRAY_FREE(short, sNormConv,G723_SBFR_LEN,encoderObj) ;
   LOCAL_ARRAY_FREE(int, lConv,G723_SBFR_LEN,encoderObj) ; 

}

#define COMBTBL_LINE_LEN (G723_SBFR_LEN/GRIDSIZE)
static const int CombTbl[N_PULSES*COMBTBL_LINE_LEN] =
{
   118755,  98280,  80730,  65780,  53130, 42504,  33649,  26334,  20349,  15504,
    11628,   8568,   6188,   4368,   3003,  2002,   1287,    792,    462,    252,
      126,     56,     21,      6,      1,     0,      0,      0,      0,      0,
    23751,  20475,  17550,  14950,  12650, 10626,   8855,   7315,   5985,   4845,
     3876,   3060,   2380,   1820,   1365,  1001,    715,    495,    330,    210,
      126,     70,     35,     15,      5,     1,      0,      0,      0,      0,
     3654,   3276,   2925,   2600,   2300,  2024,   1771,   1540,   1330,   1140,
      969,    816,    680,    560,    455,   364,    286,    220,    165,    120,
       84,     56,     35,     20,     10,     4,      1,      0,      0,      0,
      406,    378,    351,    325,    300,   276,    253,    231,    210,    190,
      171,    153,    136,    120,    105,    91,     78,     66,     55,     45,
       36,     28,     21,     15,     10,     6,      3,      1,      0,      0,
       29,     28,     27,     26,     25,    24,     23,     22,     21,     20,
       19,     18,     17,     16,     15,    14,     13,     12,     11,     10,
        9,      8,      7,      6,      5,     4,      3,      2,      1,      0,
        1,      1,      1,      1,      1,     1,      1,      1,      1,      1,
        1,      1,      1,      1,      1,     1,      1,      1,      1,      1,
        1,      1,      1,      1,      1,     1,      1,      1,      1,      1
};

static const int MaxPosition[4] = {593775, 142506, 593775, 142506};

static const short PitchContrb[2*170] = {
    60,      0,     0,  2489,     60,     0,      0,  5217,      1,  6171,      0,   3953,     0,  10364,     1,  9357,     -1,   8843,     1,  9396,
     0,   5794,    -1, 10816,      2, 11606,     -2, 12072,      0,  8616,      1,  12170,     0,  14440,     0,  7787,     -1,  13721,     0, 18205,
     0,  14471,     0, 15807,      1, 15275,      0, 13480,     -1, 18375,     -1,      0,     1,  11194,    -1, 13010,      1,  18836,    -2, 20354,
     1,  16233,    -1,     0,     60,     0,      0, 12130,      0, 13385,      1,  17834,     1,  20875,     0, 21996,      1,      0,     1, 18277, 
    -1,  21321,     1, 13738,     -1, 19094,     -1, 20387,     -1,     0,      0,  21008,    60,      0,    -2, 22807,      0,  15900,     1,     0,
     0,  17989,    -1, 22259,      1, 24395,      1, 23138,      0, 23948,      1,  22997,     2,  22604,    -1, 25942,      0,  26246,     1, 25321, 
     0,  26423,     0, 24061,      0, 27247,     60,     0,     -1, 25572,      1,  23918,     1,  25930,     2, 26408,     -1,  19049,     1, 27357, 
    -1,  24538,    60,     0,     -1, 25093,      0, 28549,      1,     0,      0,  22793,    -1,  25659,     0, 29377,      0,  30276,     0, 26198, 
     1,  22521,    -1, 28919,      0, 27384,      1, 30162,     -1,     0,      0,  24237,    -1,  30062,     0, 21763,      1,  30917,    60,     0, 
     0,  31284,     0, 29433,      1, 26821,      1, 28655,      0, 31327,      2,  30799,     1,  31389,     0, 32322,      1,  31760,    -2, 31830,
     0,  26936,    -1, 31180,      1, 30875,      0, 27873,     -1, 30429,      1,  31050,     0,      0,     0, 31912,      1,  31611,     0, 31565,
     0,  25557,     0, 31357,     60,     0,      1, 29536,      1, 28985,     -1,  26984,    -1,  31587,     2, 30836,     -2,  31133,     0, 30243,
    -1,  30742,    -1, 32090,     60,     0,      2, 30902,     60,     0,      0,  30027,     0,  29042,    60,     0,      0,  31756,     0, 24553,  
     0,  25636,    -2, 30501,     60,     0,     -1, 29617,      0, 30649,     60,      0,     0,  29274,     2, 30415,      0,  27480,     0, 31213,
    -1,  28147,     0, 30600,      1, 31652,      2, 29068,     60,     0,      1,  28571,     1,  28730,     1, 31422,      0,  28257,     0, 24797,
    60,      0,     0,     0,     60,     0,      0, 22105,      0, 27852,     60,      0,    60,      0,    -1, 24214,      0,  24642,     0, 23305,
    60,      0,    60,     0,      1, 22883,      0, 21601,     60,     0,      2,  25650,    60,      0,    -2, 31253,     -2,  25144,     0, 17998 
};

static const short NPulse[4] = 
{6,5,
 6,5};

void FixedCodebookVector_G723_16s( int lDecPos, int lAmplitude, int mamp, int lGrid,
       int lGain, int lNSbfr, G723_Rate currRate, Ipp16s *pDst, int *pLag, Ipp16s *pGain )

{
   int   i, j, lTmp, lOffset, lPos;
   short sCdbkGain, sCdbkSign, sCdbkShift, sCdbkPos;

   ippsZero_16s(pDst,G723_SBFR_LEN);

   switch(currRate)  {
      case G723_Rate63: {
         if ( lDecPos < MaxPosition[lNSbfr] ){
            /* Decode the amplitudes and positions */
            
            j = N_PULSES - NPulse[lNSbfr] ;
            lTmp = lDecPos ;
            for ( i = 0 ; i < G723_SBFR_LEN/GRIDSIZE ; i ++ )  {
               lTmp -= CombTbl[j*COMBTBL_LINE_LEN + i];
               if ( lTmp < 0 ) {
                  lTmp += CombTbl[(j++)*COMBTBL_LINE_LEN+i];
                  if ( (lAmplitude & (1 << (N_PULSES-j) )) !=  0 )
                     pDst[lGrid + GRIDSIZE*i] = GainDBLvls[mamp] ;
                  else
                     pDst[lGrid + GRIDSIZE*i] = -GainDBLvls[mamp] ;

                  if ( j == N_PULSES ) break ;
               }
            }
         }
         break;
      }

      case G723_Rate53: {
         sCdbkGain = -GainDBLvls[mamp];
         sCdbkShift = lGrid; 
         sCdbkSign = lAmplitude<<1; 
         sCdbkPos = (short) lDecPos;

         for(lOffset=0; lOffset<8; lOffset+=2) {
            lPos = (sCdbkPos & 0x7) ;
            lPos = (lPos<<3) + sCdbkShift + lOffset;
            if (lPos < G723_SBFR_LEN)
               pDst[lPos] = sCdbkGain * ((sCdbkSign & 2 ) - 1); 
            sCdbkPos >>= 3;
            sCdbkSign >>= 1;
         }

         *pLag = PitchContrb[lGain<<1]; 
         *pGain = PitchContrb[(lGain<<1) + 1];
         break;
      }
   }
}

void ResidualInterpolation_G723_16s_I
        (Ipp16s *pSrcDst, Ipp16s *pDst,  int lag, Ipp16s gain, Ipp16s *pSeed)
{
   int   i;
   if ( lag ) {/* Voiced case*/
      for ( i = 0 ; i < lag ; i ++ ){ /* attenuation  */
         pSrcDst[G723_MAX_PITCH+i-lag] = (pSrcDst[G723_MAX_PITCH+i-lag] * 0x6000)>>15;
      } 
      ippsCopy_16s(&pSrcDst[G723_MAX_PITCH-lag],&pSrcDst[G723_MAX_PITCH],G723_FRM_LEN);
      ippsCopy_16s(&pSrcDst[G723_MAX_PITCH],pDst,G723_FRM_LEN);
   } else {/* Unvoiced case*/
      for ( i = 0 ; i < G723_FRM_LEN ; i ++ )
         pDst[i] = (gain * Rand2_16s(pSeed))>>15;
      /* reset memory */
      ippsZero_16s(pSrcDst,G723_FRM_LEN+G723_MAX_PITCH);
   } 
   return;
}

static const short BetaTbl[2*170] = {
  1024,  1024,   1308,  1591,   1906, 1678,   2291, 1891,   2511,  2120, 
  2736,  2399,   3298,  2966,   3489, 3049,   3531, 3185,   3844,  3317, 
  4360,  3433,   4541,  3523,   4684, 3729,   4813, 3779,   5069,  3789, 
  5528,  4262,   5577,  4450,   5713, 4469,   5923, 4713,   5958,  4944, 
  5958,  4950,   6064,  4980,   6132, 5010,   6331, 5032,   6370,  5299, 
  6527,  5389,   6533,  5389,   6575, 5389,   6633, 5646,   6671,  5701, 
  6832,  5733,   6832,  5765,   6972, 5997,   6996, 5997,   7199,  6150, 
  7205,  6211,   7414,  6336,   7529, 6360,   7543, 6415,   7543,  6415,
  7692,  6430,   7758,  6440,   7839, 6461,   7839, 6461,   7869,  6512, 
  7992,  6601,   8000,  6787,   8016, 6872,   8055, 6931,   8079,  6972, 
  8119,  6984,   8208,  7056,   8250, 7056,   8266, 7105,   8291,  7117, 
  8300,  7123,   8325,  7136,   8402, 7161,   8445, 7167,   8605,  7180, 
  8623,  7180,   8687,  7262,   8752, 7308,   8837, 7334,   8847,  7334, 
  8973,  7387,   9002,  7407,   9012, 7434,   9184, 7441,   9593,  7441, 
  9672,  7481,   9752,  7536,   9846, 7564,   9978, 7592,  10139,  7685, 
 10202,  7714,  10317,  7758,  10476, 7772,  10598, 7794,  10598,  7802,
 10695,  7817,  11425,  7839,  11670, 7869,  14629, 7885,  15255,  7907,
     0,  7946,      0,  7992,      0, 8039,      0, 8063,      0,  8087,
     0,  8087,      0,  8167,      0, 8184,      0, 8200,      0,  8200,
     0,  8241,      0,  8266,      0, 8283,      0, 8308,      0,  8308,
     0,  8334,      0,  8376,      0, 8402,      0, 8463,      0,  8516,
     0,  8524,      0,  8533,      0, 8641,      0, 8669,      0,  8696,
     0,  8752,      0,  8761,      0, 8799,      0, 8828,      0,  8943,
     0,  9112,      0,  9122,      0, 9133,      0, 9153,      0,  9288,
     0,  9299,      0,  9373,      0, 9384,      0, 9384,      0,  9405,
     0,  9416,      0,  9471,      0, 9503,      0, 9559,      0,  9581,
     0,  9660,      0,  9660,      0, 9718,      0, 9799,      0,  9823,
     0,  9846,      0,  9846,      0, 9930,      0,10039,      0, 10164,
     0, 10227,      0, 10291,      0,10436,      0,10503,      0, 10516,
     0, 10530,      0, 10598,      0,10611,      0,10625,      0, 11040,
     0, 11070,      0, 11100,      0,11115,      0,11315,      0, 11331,
     0, 11804,      0, 12100,      0,12263,      0,12263,      0, 12300,
     0, 12337,      0, 12431,      0,12800,      0,12962,      0, 13065,
     0, 13496,      0, 13815,      0,14100,      0,14198,      0, 18409 
};

void ErrorUpdate_G723(int *pError, short openLoopLag, short AdCbbkLag, short AdCbbkGain, G723_Rate currRate)
{
    int ilag, lag, e0, e1, li0=0,li1=0;
    short sBeta;

    lag = openLoopLag - 1 + AdCbbkLag;

    AdCbbkGain<<=1;
    if ( currRate == G723_Rate63) {
        if ( openLoopLag >= (G723_SBFR_LEN-2) ) AdCbbkGain++;
    } else {
        AdCbbkGain++;
    }
    sBeta = BetaTbl[AdCbbkGain];

    if(lag > 30) {
        ilag = (lag * 273)>>13;   /*  x 1/30 */
        if(30*(ilag+1) != lag) {
            if(ilag == 1) {
               if(pError[0] <= pError[1]){
                  li0 = 1;
                  li1 = 1;
               }
            }else {
               li0 = ilag-2;
               li1 = ilag-1;
               if(pError[li1] > pError[li0]){
                  li0 = li1;
               }
               if(pError[li1] <= pError[ilag]){
                  li1 = ilag;
               }
            }
        }
        else {  /* lag = 60, 90, 120 */
           li0 = ilag-1;
           li1 = ilag;
        }
    }
    e0 = Add_32s(ShiftL_32s(MulC_32s(sBeta,pError[li0]),2),4);
    e1 = Add_32s(ShiftL_32s(MulC_32s(sBeta,pError[li1]),2),4);
    pError[4] = pError[2];
    pError[3] = pError[1];
    pError[2] = pError[0];
    pError[1] = e1;
    pError[0] = e0;
    return;
}

static void CodewordImpConv_G723(short *pSrc, short *pDst, short *pSrcSign, short *pPos, short* pDstSign, int* ComposedIdx)
{
   int i;
   short sImpPos0, sImpPos1, sImpPos2, sImpPos3;
   short sImpSign0, sImpSign1, sImpSign2, sImpSign3;

   sImpPos0 = pPos[0];
   sImpPos1 = pPos[1];
   sImpPos2 = pPos[2];
   sImpPos3 = pPos[3];

   sImpSign0 = pSrcSign[0];
   sImpSign1 = pSrcSign[1];
   sImpSign2 = pSrcSign[2];
   sImpSign3 = pSrcSign[3];

   /* find codebook index;  17-bit address */

   *pDstSign = 0;
   if(sImpSign0 > 0) *pDstSign=1;
   if(sImpSign1 > 0) *pDstSign += 2;
   if(sImpSign2 > 0) *pDstSign += 4;
   if(sImpSign3 > 0) *pDstSign += 8;

   i = sImpPos0 >> 3;
   i += sImpPos1 & 0xfff8;
   i += (sImpPos2 & 0xfff8) << 3;
   i += (sImpPos3 & 0xfff8) << 6;

   *ComposedIdx = i;

   /* Compute the filtered codeword */

   ippsZero_16s(pDst,G723_SBFR_LEN);

   if (sImpPos0>sImpPos1) {i=sImpPos0; sImpPos0=sImpPos1; sImpPos1=i; i=sImpSign0; sImpSign0=sImpSign1; sImpSign1=i;}
   if (sImpPos2>sImpPos3) {i=sImpPos2; sImpPos2=sImpPos3; sImpPos3=i; i=sImpSign2; sImpSign2=sImpSign3; sImpSign3=i;}
   if (sImpPos0>sImpPos2) {i=sImpPos0; sImpPos0=sImpPos2; sImpPos2=i; i=sImpSign0; sImpSign0=sImpSign2; sImpSign2=i;}
   if (sImpPos1>sImpPos3) {i=sImpPos1; sImpPos1=sImpPos3; sImpPos3=i; i=sImpSign1; sImpSign1=sImpSign3; sImpSign3=i;}
   if (sImpPos1>sImpPos2) {i=sImpPos1; sImpPos1=sImpPos2; sImpPos2=i; i=sImpSign1; sImpSign1=sImpSign2; sImpSign2=i;}

   for (i=0; i<sImpPos0; i++)        
      pDst[i]=0;
   for (; i<sImpPos1; i++)           
      pDst[i]=sImpSign0*pSrc[i-sImpPos0];
   for (; i<sImpPos2; i++)           
      pDst[i]=sImpSign0*pSrc[i-sImpPos0]+sImpSign1*pSrc[i-sImpPos1];
   for (; i<sImpPos3; i++)           
      pDst[i]=sImpSign0*pSrc[i-sImpPos0]+sImpSign1*pSrc[i-sImpPos1]+sImpSign2*pSrc[i-sImpPos2];
   for (; i<G723_SBFR_LEN; i++) 
      pDst[i]=sImpSign0*pSrc[i-sImpPos0]+sImpSign1*pSrc[i-sImpPos1]+sImpSign2*pSrc[i-sImpPos2]+sImpSign3*pSrc[i-sImpPos3];

   return;
}

void  FixedCodebookSearch_G723_16s(G723Encoder_Obj *encoderObj, ParamStream_G723 *Params, short *pSrcDst, short *ImpResp, short sNSbfr)
{
    short sPitchPeriod, sGain;

    switch(Params->currRate)  {

    case G723_Rate63: 
       {
          ippsMPMLQFixedCodebookSearch_G723(Params->PitchLag[sNSbfr>>1], ImpResp, pSrcDst, pSrcDst,
             &Params->sGrid[sNSbfr], &Params->sTrainDirac[sNSbfr], &Params->sAmpIndex[sNSbfr], &Params->sAmplitude[sNSbfr], &Params->sPosition[sNSbfr], sNSbfr);
          break;
        }

    case G723_Rate53:
       {
          short sCurrGrid, sSign, sGainCoeff;
          int lCdbkIdx, lGainIdx;

          LOCAL_ALIGN_ARRAY(16, short, pTargetImpRespCorr, G723_SBFR_LEN+4, encoderObj) ;
          LOCAL_ALIGN_ARRAY(16, short, pFixedVector,       G723_SBFR_LEN+4, encoderObj) ;
          LOCAL_ALIGN_ARRAY(16, short, pFltFixedVector,    G723_SBFR_LEN+4, encoderObj) ;
          LOCAL_ALIGN_ARRAY(16, int,   pToeplizMatrix,     G723_TOEPLIZ_MATRIX_SIZE, encoderObj) ;
          LOCAL_ALIGN_ARRAY(16, short, pAlignBuff,         G723_SBFR_LEN, encoderObj) ;
          LOCAL_ARRAY(short, pDstFixedSign,    4, encoderObj) ;
          LOCAL_ARRAY(short, pDstFixedPosition,4, encoderObj) ;

          sPitchPeriod = Params->PitchLag[sNSbfr>>1]-1+Params->AdCdbkLag[sNSbfr] + PitchContrb[Params->AdCdbkGain[sNSbfr]<<1];
          sGain = PitchContrb[(Params->AdCdbkGain[sNSbfr]<<1)+1];

          /* Find correlations of h[] needed for the codebook search. */
          ippsRShiftC_16s_I(1,ImpResp,G723_SBFR_LEN); /* Q13 -->  Q12*/
          if (sPitchPeriod < G723_SBFR_LEN-2) {
              ippsHarmonicFilter_16s_I(sGain,sPitchPeriod,&ImpResp[sPitchPeriod],G723_SBFR_LEN-sPitchPeriod);
          }
          /* Compute correlation of target vector with impulse response. */

          ippsCrossCorr_NormM_16s(ImpResp, pSrcDst, G723_SBFR_LEN, pTargetImpRespCorr);
          /* Compute the covariance matrix of the impulse response. */
          ippsToeplizMatrix_G723_16s32s(ImpResp, pToeplizMatrix);

          /* Find innovative codebook (filtered codeword) */
          ippsACELPFixedCodebookSearch_G723_32s16s(pTargetImpRespCorr, pToeplizMatrix, pDstFixedSign, pDstFixedPosition, &sCurrGrid, pFixedVector, &encoderObj->sSearchTime);
         
          CodewordImpConv_G723(ImpResp, pFltFixedVector, pDstFixedSign, pDstFixedPosition, &sSign, &lCdbkIdx);
          /* Compute innovation vector gain */
          FixedCodebookGain_G723_16s(pSrcDst,pFltFixedVector,&sGainCoeff,&lGainIdx,pAlignBuff);

          ippsMulC_16s_Sfs(pFixedVector,sGainCoeff,pSrcDst,G723_SBFR_LEN,0);
          if(sPitchPeriod < G723_SBFR_LEN-2)
              ippsHarmonicFilter_16s_I(sGain,sPitchPeriod,&pSrcDst[sPitchPeriod],G723_SBFR_LEN-sPitchPeriod);

          Params->sTrainDirac[sNSbfr] = 0;
          Params->sAmpIndex[sNSbfr] = lGainIdx;
          Params->sGrid[sNSbfr] = sCurrGrid;
          Params->sAmplitude[sNSbfr] = sSign;
          Params->sPosition[sNSbfr] = lCdbkIdx;

          LOCAL_ARRAY_FREE(short, pDstFixedPosition,4, encoderObj) ;
          LOCAL_ARRAY_FREE(short, pDstFixedSign,    4, encoderObj) ;
          LOCAL_ALIGN_ARRAY_FREE(16, short, pAlignBuff,         G723_SBFR_LEN, encoderObj) ;
          LOCAL_ALIGN_ARRAY_FREE(16, int,   pToeplizMatrix,     G723_TOEPLIZ_MATRIX_SIZE, encoderObj) ;
          LOCAL_ALIGN_ARRAY_FREE(16, short, pFltFixedVector,    G723_SBFR_LEN+4, encoderObj) ;
          LOCAL_ALIGN_ARRAY_FREE(16, short, pFixedVector,       G723_SBFR_LEN+4, encoderObj) ;
          LOCAL_ALIGN_ARRAY_FREE(16, short, pTargetImpRespCorr, G723_SBFR_LEN+4, encoderObj) ;

          break;
       }
    }
}

void InterpolationIndex_G723_16s( short *pDecodedExc, short sPitchLag, short *pGain, short *pGainSFS, short *pDstIdx)
{
    int   lSfs, lIdx, lTmp;
    short sTargetVecEnergy, sMaxCorr, sBestEnergy;
    short *pExc;

    /* Normalize the excitation */
    lSfs=3;
    ippsAutoScale_16s_I( pDecodedExc, G723_MAX_PITCH+G723_FRM_LEN, &lSfs );
    *pGainSFS = lSfs;

    if ( sPitchLag > (short) (G723_MAX_PITCH-3) )
        sPitchLag = (short) (G723_MAX_PITCH-3);

    lIdx = sPitchLag;
    pExc = &pDecodedExc[G723_MAX_PITCH+G723_FRM_LEN-2*G723_SBFR_LEN];
    ippsAutoCorrLagMax_Inv_16s(pExc,2*G723_SBFR_LEN,sPitchLag-3,sPitchLag+3,&lTmp,&lIdx);

    if(lTmp > 0 ) {
      sMaxCorr = Cnvrt_NR_32s16s( lTmp );
      /* Compute target energy */
      ippsDotProd_16s32s_Sfs(pExc,pExc,2*G723_SBFR_LEN,&lTmp,0);
	   lTmp <<=1;
      sTargetVecEnergy = Cnvrt_NR_32s16s( lTmp );
      *pGain = sTargetVecEnergy;

      /* Calculate the best energy */
      ippsDotProd_16s32s_Sfs(pExc-lIdx,pExc-lIdx,2*G723_SBFR_LEN,&lTmp,0);
	   lTmp <<=1;

      sBestEnergy = Cnvrt_NR_32s16s( lTmp );

      lTmp = sBestEnergy * sTargetVecEnergy;
      lTmp >>= 3;

      if ( lTmp < sMaxCorr * sMaxCorr ) *pDstIdx = (short)lIdx;
      else *pDstIdx = 0;
    } else *pDstIdx = 0;

    return;
}

typedef struct _G723_VADmemory{
    int     PrevEnergy;
    int     NoiseLevel;
    short   HangoverCounter;
    short   VADCounter;
    short   AdaptEnableFlag;
    short   OpenLoopDelay[4];
}G723_VADmemory;

/* 
  Name:       VoiceActivityDetectSize
  
   Purpose:     VAD decision memory size query 
   pVADmem     pointer to the VAD decision memory                   
*/

void VoiceActivityDetectSize_G723(int* pVADsize)
{

   *pVADsize = sizeof(G723_VADmemory);
   *pVADsize = (*pVADsize+7)&(~7);
}
/* 
  Name:       VoiceActivityDetectInit
  
   Purpose:     VAD decision memory init 
   pVADmem     pointer to the VAD decision memory                   
*/

void VoiceActivityDetectInit_G723(char* pVADmem)
{
   G723_VADmemory* vadMem =  (G723_VADmemory*)pVADmem;


   vadMem->HangoverCounter = 3;
   vadMem->VADCounter = 0;
   vadMem->PrevEnergy = 0x400;
   vadMem->NoiseLevel = 0x400;

   vadMem->AdaptEnableFlag = 0;

   vadMem->OpenLoopDelay[0] = 1;
   vadMem->OpenLoopDelay[1] = 1;

}

static  const int LogAdd_Tbl[11] = {
   300482560, 300482560, 300482560, 300482560,337149952,378273792,424443904,476217344,534315008,599523328,672694272
};
static  const short  LogMul_Tbl[11] = {
      0,    0,    0,    0, 1119, 1255, 1409, 1580, 1773, 1990, 2233
};
void VoiceActivityDetect_G723(const Ipp16s *pSrc, const Ipp16s *pNoiseLPC,
         const Ipp16s *pOpenLoopDelay, int SineWaveDetector, int *pVADDecision, int *pAdaptEnableFlag, char* pVADmem, short *AlignBuff)
{
   G723_VADmemory* vadMem =  (G723_VADmemory*)pVADmem;
   int i, j, lTmp0, lTmp1;
   short  sTmp0, sTmp1, sNmult, MinOLP;

   int  VADResult = 1 ;

   vadMem->OpenLoopDelay[2] = pOpenLoopDelay[0] ;
   vadMem->OpenLoopDelay[3] = pOpenLoopDelay[1] ;

   /* Find Minimum pitch period */
   MinOLP = G723_MAX_PITCH ;
   for ( i = 0 ; i < 4 ; i ++ ) {
       if ( MinOLP > vadMem->OpenLoopDelay[i] )
           MinOLP = vadMem->OpenLoopDelay[i] ;
   }

   /* How many olps are multiplies of their min */
   sNmult = 0 ;
   for ( i = 0 ; i < 4 ; i++ ) {
       sTmp1 = MinOLP ;
       for ( j = 0 ; j < 8 ; j++ ) {
           sTmp0 = sTmp1 - vadMem->OpenLoopDelay[i];
           if(sTmp0 < 0) sTmp0 = -sTmp0;
           if ( sTmp0 <= 3 ) sNmult++ ;
           sTmp1 += MinOLP;
       }
   }

   /* Update adaptation enable counter if not periodic and not sine and clip it.*/
   if ( (sNmult == 4) || (SineWaveDetector == -1) )
      if(vadMem->AdaptEnableFlag > 5) vadMem->AdaptEnableFlag = 6;
      else vadMem->AdaptEnableFlag += 2;
   else
      if ( vadMem->AdaptEnableFlag < 1 ) vadMem->AdaptEnableFlag = 0;
      else vadMem->AdaptEnableFlag--;

   /* Inverse filter the data */

   ippsResidualFilter_AMRWB_16s_Sfs(pNoiseLPC, G723_LPC_ORDER, &pSrc[G723_SBFR_LEN], AlignBuff, G723_FRM_LEN-G723_SBFR_LEN, 14);
   ippsDotProd_16s32s_Sfs(AlignBuff,AlignBuff,G723_FRM_LEN-G723_SBFR_LEN,&lTmp1,-1);
    
    /* Scale the rezidual energy */
    lTmp1 = MulC_32s(2913,  lTmp1 ) ;
   /* Clip noise level */
   if ( vadMem->NoiseLevel > vadMem->PrevEnergy ) {
      lTmp0 = vadMem->PrevEnergy - (vadMem->PrevEnergy>>2);
      vadMem->NoiseLevel = lTmp0 + (vadMem->NoiseLevel>>2);
   }


   /* Update the noise level */
   if ( !vadMem->AdaptEnableFlag ) {
      vadMem->NoiseLevel = vadMem->NoiseLevel + (vadMem->NoiseLevel>>5);
   } else { /* Decay NoiseLevel */
      vadMem->NoiseLevel = vadMem->NoiseLevel - (vadMem->NoiseLevel>>11);
   }

   /* Update previous energy */
   vadMem->PrevEnergy = lTmp1 ;

   /* CLip Noise Level */
   if ( vadMem->NoiseLevel < 0x80 )
       vadMem->NoiseLevel = 0x80 ;
   if ( vadMem->NoiseLevel > 0x1ffff )
       vadMem->NoiseLevel = 0x1ffff ;

   /* Compute the treshold */
   lTmp0 = vadMem->NoiseLevel<<13;
   sTmp0 = Norm_32s_Pos_I(&lTmp0);
   sTmp1 = (lTmp0>>15)& 0x7e00;

   lTmp0 = LogAdd_Tbl[sTmp0] - sTmp1 * LogMul_Tbl[sTmp0]; 
   sTmp1 = lTmp0 >> 15;


   sTmp0 = (short)(vadMem->NoiseLevel>>2);
   lTmp0 = sTmp0*sTmp1;
   lTmp0 >>= 10;

   /* treshold */
   if ( lTmp0 > lTmp1 )
       VADResult = 0 ;

   /* update counters */
   if ( VADResult ) {
       vadMem->VADCounter++ ;
       vadMem->HangoverCounter++ ;
   } else {
       vadMem->VADCounter-- ;
       if ( vadMem->VADCounter < 0 )
           vadMem->VADCounter = 0 ;
   }
   if ( vadMem->VADCounter >= 2 ) {
       vadMem->HangoverCounter = 6 ;
       if ( vadMem->VADCounter >= 3 )
           vadMem->VADCounter = 3 ;
   }
   if ( vadMem->HangoverCounter ) {
       VADResult = 1 ;
       if ( vadMem->VADCounter == 0 )
           vadMem->HangoverCounter -- ;
   }
   /* Update periodicy detector */
   vadMem->OpenLoopDelay[0] = vadMem->OpenLoopDelay[2] ;
   vadMem->OpenLoopDelay[1] = vadMem->OpenLoopDelay[3] ;

   *pAdaptEnableFlag = vadMem->AdaptEnableFlag; /* adaptation enable counter  */
   *pVADDecision = VADResult; /* VAD desision : 0 - noise, 1 - voice  */

}

static const short StratingPositionTbl[G723_SBFR_LEN/GRIDSIZE] = {
   0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58};

#define MAX_CACHE_NUM 32
static const short SeedCacheTbl[MAX_CACHE_NUM] = {
    17547, -15555,  11855,  26561, -20077, -21051, 24151,   9545,
   -15205,  24141,  29791,  -5935,   -605,  21717, -3993,  12889,
    -9045,   4445,  21103, -32287,  -7757,   5093,  6775,  22377,
   -29509,  -9107, -14209,  13041,  24003,  -5387, -9081, -27527};

static const short OlpCache[MAX_CACHE_NUM][2] = {
   {129, 137}, {143, 135}, {137, 140}, {133, 136},
   {129, 124}, {139, 140}, {129, 133}, {123, 129},
   {139, 128}, {128, 125}, {140, 132}, {137, 133},
   {136, 127}, {131, 132}, {129, 138}, {133, 127},
   {142, 123}, {127, 141}, {137, 128}, {134, 133},
   {136, 135}, {137, 129}, {143, 125}, {138, 130},
   {139, 143}, {140, 139}, {127, 128}, {124, 138},
   {130, 126}, {136, 129}, {130, 138}, {136, 136}};

static const short GainCache[MAX_CACHE_NUM][4] = {
   {34, 31, 18, 35}, {38, 16, 47, 22}, { 9, 29, 48, 36}, {16, 44,  4, 15},
   {16, 27, 34, 34}, { 5, 26, 45,  8}, {14, 32, 31, 36}, {12, 18, 25,  8},
   { 7, 50, 46, 49}, {43, 26,  1, 21}, { 3, 37, 35, 27}, { 5,  7, 29,  4},
   { 6, 50,  4, 28}, { 3, 16, 16, 12}, {25, 44, 10,  8}, {43, 10, 17,  2},
   {14, 27,  8, 22}, {33, 46, 40, 30}, {31,  4,  6, 28}, {28, 29, 39,  3},
   {31, 32,  9, 31}, {34, 15, 22, 26}, {20, 17, 23, 38}, { 9, 12, 44,  6},
   { 6, 14,  5,  5}, { 7, 25, 13, 49}, {44, 31, 11, 38}, {36, 10, 33, 11},
   {40, 24, 48, 44}, { 1, 24, 12, 49}, {50, 48, 21, 28}, { 9, 23,  5, 19}};

static const __ALIGN(16) short SignCache[MAX_CACHE_NUM][24] = {
   { 16384,  16384, -16384, -16384,  16384,  16384,  16384, -16384,
     16384, -16384,  16384,  16384,  16384, -16384, -16384, -16384,
     16384,  16384,  16384, -16384, -16384, -16384,      0,      0},
   {-16384, -16384, -16384, -16384, -16384, -16384,  16384, -16384,
     16384, -16384, -16384, -16384, -16384, -16384, -16384,  16384,
    -16384,  16384, -16384,  16384,  16384, -16384,      0,      0},
   {-16384,  16384, -16384, -16384, -16384, -16384, -16384,  16384,
     16384,  16384,  16384,  16384,  16384, -16384, -16384, -16384,
     16384,  16384,  16384,  16384,  16384,  16384,      0,      0},
   {-16384,  16384, -16384,  16384,  16384, -16384,  16384, -16384,
     16384,  16384,  16384, -16384, -16384,  16384,  16384,  16384,
     16384, -16384,  16384, -16384,  16384,  16384,      0,      0},
   {-16384, -16384, -16384,  16384, -16384,  16384,  16384,  16384,
     16384, -16384,  16384,  16384,  16384, -16384,  16384,  16384,
     16384,  16384,  16384,  16384, -16384,  16384,      0,      0},
   {-16384, -16384,  16384,  16384, -16384, -16384,  16384, -16384,
    -16384,  16384,  16384, -16384, -16384, -16384, -16384, -16384,
    -16384,  16384,  16384,  16384, -16384, -16384,      0,      0},
   {-16384,  16384,  16384, -16384, -16384,  16384, -16384,  16384,
     16384, -16384, -16384,  16384,  16384, -16384,  16384, -16384,
     16384, -16384, -16384, -16384, -16384,  16384,      0,      0},
   {-16384,  16384,  16384, -16384,  16384, -16384, -16384, -16384,
     16384,  16384,  16384, -16384, -16384,  16384,  16384,  16384,
    -16384,  16384, -16384,  16384, -16384,  16384,      0,      0},
   {-16384, -16384,  16384,  16384,  16384,  16384, -16384,  16384,
     16384,  16384, -16384, -16384, -16384,  16384, -16384,  16384,
     16384,  16384, -16384,  16384,  16384, -16384,      0,      0},
   { 16384, -16384, -16384,  16384,  16384,  16384, -16384,  16384,
    -16384,  16384, -16384,  16384, -16384, -16384, -16384,  16384,
    -16384, -16384,  16384, -16384,  16384, -16384,      0,      0},
   { 16384,  16384, -16384,  16384, -16384,  16384, -16384, -16384,
     16384, -16384,  16384, -16384, -16384,  16384, -16384,  16384,
    -16384,  16384,  16384, -16384, -16384,  16384,      0,      0},
   { 16384,  16384, -16384, -16384,  16384,  16384, -16384, -16384,
    -16384,  16384, -16384,  16384, -16384,  16384,  16384,  16384,
    -16384,  16384, -16384, -16384,  16384, -16384,      0,      0},
   { 16384, -16384, -16384, -16384,  16384,  16384,  16384,  16384,
    -16384,  16384,  16384, -16384, -16384,  16384,  16384, -16384,
    -16384,  16384, -16384,  16384, -16384, -16384,      0,      0},
   { 16384, -16384,  16384, -16384, -16384, -16384, -16384,  16384,
    -16384,  16384,  16384,  16384, -16384, -16384, -16384, -16384,
    -16384,  16384,  16384,  16384, -16384,  16384,      0,      0},
   { 16384,  16384,  16384,  16384, -16384, -16384, -16384, -16384,
    -16384,  16384, -16384, -16384, -16384,  16384,  16384,  16384,
    -16384,  16384,  16384,  16384, -16384, -16384,      0,      0},
   { 16384,  16384,  16384,  16384, -16384,  16384, -16384,  16384,
    -16384, -16384, -16384,  16384, -16384,  16384,  16384,  16384,
     16384, -16384,  16384,  16384, -16384,  16384,      0,      0},
   { 16384, -16384,  16384, -16384, -16384, -16384, -16384,  16384,
     16384,  16384,  16384,  16384, -16384,  16384, -16384, -16384,
    -16384, -16384,  16384,  16384,  16384,  16384,      0,      0},
   {-16384,  16384, -16384, -16384,  16384,  16384, -16384,  16384,
     16384, -16384, -16384, -16384,  16384, -16384, -16384,  16384,
    -16384,  16384, -16384,  16384,  16384, -16384,      0,      0},
   {-16384, -16384,  16384, -16384,  16384, -16384,  16384, -16384,
    -16384, -16384, -16384,  16384, -16384,  16384, -16384, -16384,
    -16384,  16384, -16384,  16384,  16384, -16384,      0,      0},
   {-16384, -16384,  16384,  16384, -16384, -16384, -16384,  16384,
    -16384,  16384, -16384, -16384,  16384,  16384,  16384,  16384,
     16384,  16384, -16384,  16384,  16384,  16384,      0,      0},
   {-16384,  16384, -16384,  16384,  16384,  16384,  16384, -16384,
     16384, -16384,  16384,  16384, -16384,  16384,  16384,  16384,
    -16384, -16384, -16384, -16384,  16384,  16384,      0,      0},
   {-16384,  16384,  16384,  16384,  16384,  16384, -16384, -16384,
    -16384, -16384,  16384, -16384,  16384, -16384, -16384, -16384,
    -16384,  16384, -16384,  16384,  16384, -16384,      0,      0},
   {-16384, -16384, -16384,  16384,  16384,  16384,  16384,  16384,
     16384,  16384,  16384,  16384, -16384,  16384,  16384, -16384,
    -16384, -16384, -16384,  16384, -16384, -16384,      0,      0},
   {-16384, -16384, -16384,  16384, -16384, -16384,  16384,  16384,
     16384,  16384,  16384, -16384,  16384,  16384,  16384,  16384,
    -16384, -16384,  16384,  16384,  16384,  16384,      0,      0},
   {-16384,  16384,  16384,  16384, -16384, -16384,  16384,  16384,
    -16384, -16384, -16384, -16384,  16384,  16384, -16384,  16384,
    -16384, -16384, -16384,  16384, -16384,  16384,      0,      0},
   { 16384,  16384, -16384,  16384, -16384,  16384, -16384, -16384,
    -16384,  16384,  16384,  16384,  16384, -16384, -16384,  16384,
    -16384, -16384,  16384,  16384, -16384,  16384,      0,      0},
   { 16384, -16384,  16384,  16384,  16384,  16384,  16384,  16384,
    -16384, -16384, -16384, -16384,  16384,  16384, -16384,  16384,
     16384, -16384, -16384,  16384,  16384, -16384,      0,      0},
   { 16384, -16384,  16384, -16384, -16384,  16384,  16384, -16384,
    -16384, -16384, -16384,  16384,  16384,  16384,  16384,  16384,
    -16384, -16384, -16384, -16384,  16384,  16384,      0,      0},
   { 16384,  16384, -16384, -16384, -16384, -16384, -16384,  16384,
     16384, -16384, -16384, -16384,  16384,  16384,  16384, -16384,
     16384,  16384, -16384, -16384, -16384,  16384,      0,      0},
   { 16384,  16384,  16384, -16384,  16384,  16384,  16384, -16384,
     16384,  16384,  16384,  16384,  16384, -16384, -16384, -16384,
    -16384,  16384, -16384, -16384,  16384, -16384,      0,      0},
   { 16384, -16384, -16384, -16384, -16384,  16384,  16384, -16384,
     16384,  16384, -16384, -16384,  16384,  16384,  16384,  16384,
     16384, -16384,  16384,  16384, -16384, -16384,      0,      0},
   { 16384, -16384, -16384, -16384, -16384,  16384,  16384, -16384,
    -16384, -16384,  16384,  16384,  16384,  16384,  16384,  16384,
     16384,  16384,  16384, -16384,  16384, -16384,      0,      0}
};

static const __ALIGN(16) short PosCache[MAX_CACHE_NUM][24] = {
   { 47,  51,  27,  41,  21,   9, 117,  83,
     69,  81, 109,  54,  52,  24,  20,  10,
     58,  72,  66,  68, 104,  86,   0,   0},
   { 46,  28,  50,  38,  22,  36, 114,  64,
    100, 112,  60,  24,  34,   6,   4,  28,
     42,  60, 118,  82, 114,  86,   0,   0},
   { 42,  52,  34,  36,  58,  14,  66,  88,
     68, 100,  74,  19,  35,   9,  53,   3,
      1,  84,  64,  88,  74,  78,   0,   0},
   { 41,  17,   1,   7,  13,  29,  86,  94,
     90, 112,  66,  11,  33,  23,  51,  37,
      5, 104, 100,  60,  62, 102,   0,   0},
   { 37,  43,  11,  19,   3,   5,  94,  72,
    108,  96,  98,  14,  50,  18,  32,  12,
      0, 117, 119,  69,  67,  81,   0,   0},
   {  8,  56,   4,  20,  22,  58, 113, 109,
     99,  95,  61,  30,  40,  12,  20,  14,
     18, 111, 109, 115,  93,  79,   0,   0},
   { 38,  28,  16,  50,  14,  58,  87, 103,
     83,  77, 119,  47,  53,   5,  21,  51,
     31, 117, 113,  71,  83,  99,   0,   0},
   { 17,  37,  15,  31,  11,  33,  75, 113,
     73,  63,  97,  33,   7,  39,  29,  23,
     43,  89,  95,  81,  71,  75,   0,   0},
   { 55,  21,   5,  29,  33,  17, 113,  67,
    115, 103,  97,   6,  44,  30,  26,  18,
     46,  90,  62, 100,  78,  86,   0,   0},
   { 10,  26,  38,  44,  34,   6, 104, 112,
     76,  78,  82,  24,  56,  50,  30,  14,
     28, 104,  64,  78, 112,  98,   0,   0},
   { 30,  24,  36,  12,  10,   0, 118,  90,
     94,  70,  84,  19,  37,  45,  53,  25,
      7, 104,  84, 110, 112, 106,   0,   0},
   {  1,  31,  25,  15,  47,  39,  88, 116,
     62,  94,  72,  15,  23,   1,  33,  49,
     37, 106,  84, 114,  62, 102,   0,   0},
   { 37,  47,   9,  13,   7,  45, 112,  62,
     86,  96, 104,  30,  34,   2,  54,  24,
     14, 111,  75, 107,  87, 119,   0,   0},
   { 54,   0,  38,   4,   2,  32,  93,  75,
    119,  65, 117,   6,  24,  58,  38,  30,
     22, 103, 105,  87, 115,  95,   0,   0},
   { 18,  36,  38,  32,  26,   4, 101, 109,
    119,  79,  63,  51,  47,  19,  41,  27,
     29, 113,  95,  97, 119, 105,   0,   0},
   { 57,  59,  31,  17,  13,  47,  67, 109,
     63, 103,  95,  13,  17,  23,   3,  15,
     41,  99,  71, 111,  89,  81,   0,   0},
   { 43,   3,  23,  25,  29,  37,  91,  61,
     85,  79,  69,  28,  18,  46,  14,  34,
      4, 118,  96,  88,  90, 116,   0,   0},
   { 16,  34,   6,  54,  30,  36,  76, 112,
     90, 106,  86,  38,   2,  50,  40,   8,
     48, 102, 108,  84,  70,  66,   0,   0},
   {  0,   8,  20,  58,  14,  46,  92, 102,
    116, 104,  78,  29,  21,  37,  55,   5,
     47,  76,  86, 116,  80,  92,   0,   0},
   {  3,  57,  31,  35,  13,   7,  70,  90,
     72,  84,  60,  29,  51,  45,  57,  21,
      5,  60, 110,  68,  96, 118,   0,   0},
   { 17,   5,  45,  15,  55,  43, 110,  62,
    106, 114,  94,  58,   0,  50,   8,  46,
     14, 115,  73, 101,  91,  99,   0,   0},
   { 20,  12,  54,  52,  16,  50, 115, 109,
     71,  99, 119,  54,  50,  12,  40,   4,
     10, 105,  81,  71,  65,  69,   0,   0},
   { 38,  54,  42,  24,  22,  30,  95,  69,
    101,  91,  75,   7,  21,  43,  55,  11,
     51,  61, 117,  79,  71, 113,   0,   0},
   { 19,  29,  57,  13,  49,  21,  99, 117,
     93, 119,  75,   3,   9,  15,  55,  17,
     25, 119,  87,  97,  89,  95,   0,   0},
   { 15,  53,  21,  35,   7,  59, 111,  67,
     95, 117,  77,   0,  34,  16,  40,   8,
     44,  98, 114,  86,  84,  64,   0,   0},
   {  4,  52,  14,  32,  10,  24,  88,  64,
    118,  92,  76,   0,  48,   4,  34,  14,
     54, 110,  78, 100,  66,  96,   0,   0},
   { 10,   2,  44,  50,  56,  52, 106,  66,
     94, 114, 118,  49,  45,  37,   5,  47,
     23,  60,  70,  72,  84,  88,   0,   0},
   { 45,  35,  21,   9,  15,  37,  92,  74,
     64,  86,  84,  55,   5,  45,  53,   3,
     57,  86, 116, 118,  62,  80,   0,   0},
   { 39,  31,   7,  29,  15,   1,  90,  74,
    106,  70,  68,  36,   4,  54,  52,  26,
     46,  71, 109, 103,  77,  99,   0,   0},
   { 28,  36,  54,   2,  12,  10, 119,  97,
     87,  89, 105,  54,  56,  26,  24,  40,
     32, 119,  99,  65,  79, 103,   0,   0},
   { 40,  26,  28,  56,   2,  14,  69,  97,
     83, 113,  71,  35,  37,  21,  29,   7,
     53,  79,  63,  71,  67,  97,   0,   0},
   { 23,  13,   9,  21,  15,   5, 113,  79,
    103, 105,  91,   5,  39,  19,   9,  29,
     43,  89,  85,  95,  73,  69,   0,   0}
};

void ComfortNoiseExcitation_G723_16s (Ipp16s gain, Ipp16s *pPrevExc, Ipp16s *pExc, 
                                      Ipp16s *pSeed, Ipp16s *pOlp, Ipp16s *pLags,
                                      Ipp16s *pGains, G723_Rate currRate, char *buff, Ipp16s *CasheCounter)

{
   int i, lNSbfr, lNBlock;
   short j, sTmp, sTmp1;
   short *pPosition, *pSign;
   short *ppPosition;
   short *pCurrExcitation;
   short sfs, sX1, sX2, sDiscr, sB, sAbsX2, sAbsX1;
   int lTmp, lC, lExcEnergy;
   short *pTmpPos;
   short *pOff;
   short *pTmpExcitation;

   pPosition = (short *)buff;
   pSign = pPosition + 2*NUM_PULSE_IN_BLOCK;
   pTmpPos = pSign + 2*NUM_PULSE_IN_BLOCK;
   pOff = pTmpPos + G723_SBFR_LEN/GRIDSIZE;
   pTmpExcitation = pOff + 4;

   /* generate LTP codes */
   if(*CasheCounter < -1) {
      int curr = *CasheCounter;
      pOlp[0] = OlpCache[curr][0];
      pOlp[1] = OlpCache[curr][1];

      pGains[0] = GainCache[curr][0];
      pGains[1] = GainCache[curr][1];
      pGains[2] = GainCache[curr][2];
      pGains[3] = GainCache[curr][3];
      
      *pSeed = SeedCacheTbl[curr];

      ippsCopy_16s(SignCache[curr],pSign,2*NUM_PULSE_IN_BLOCK);
      ippsCopy_16s(PosCache[curr],pPosition,2*NUM_PULSE_IN_BLOCK);

      *CasheCounter += 1;
   } else {
      pOlp[0] = NormRand_16s(21, pSeed) + 123;
      pOlp[1] = NormRand_16s(21, pSeed) + 123;
      for(lNSbfr=0; lNSbfr<4; lNSbfr++) {  /* in [1, MAX_GAIN] */
         pGains[lNSbfr] = NormRand_16s(MAX_GAIN, pSeed) + 1;
      }

      /* Generate signs and grids */
      for(lNBlock=0; lNBlock<4; lNBlock += 2) {
         sTmp    = NormRand_16s((1 << (NUM_PULSE_IN_BLOCK+2)), pSeed);
         pOff[lNBlock] = sTmp & 0x1;
         sTmp    >>= 1;
         pOff[lNBlock+1] = G723_SBFR_LEN + (sTmp & 0x1);
         for(i=0; i<NUM_PULSE_IN_BLOCK; i++) {
            pSign[i+NUM_PULSE_IN_BLOCK*(lNBlock>>1)] = ((sTmp & 0x2) - 1)<<14;
            sTmp >>= 1;
         } 
      } 

      /* Generate positions */
      ppPosition  = pPosition;
      for(lNSbfr=0; lNSbfr<4; lNSbfr++) {
         ippsCopy_16s(StratingPositionTbl, pTmpPos, G723_SBFR_LEN/GRIDSIZE);

         sTmp=(G723_SBFR_LEN/GRIDSIZE); for(i=0; i<NPulse[lNSbfr]; i++) {
            j = NormRand_16s(sTmp, pSeed);
            *ppPosition++ = pTmpPos[j] + pOff[lNSbfr];
            sTmp --; pTmpPos[j] = pTmpPos[sTmp];
         } 
      }
   }
   pLags[0] = 1;
   pLags[1] = 0;
   pLags[2] = 1;
   pLags[3] = 3;
   /* calculate fixed codebook gains */
   pCurrExcitation = pExc;

   lNSbfr = 0;
   for(lNBlock=0; lNBlock<2; lNBlock++) {
      /* decode LTP only */
      {
         short lag1 = pOlp[lNBlock];
         short lag2 = pLags[lNSbfr];

         ippsDecodeAdaptiveVector_G723_16s(lag1, lag2, pGains[lNSbfr], &pPrevExc[0], pCurrExcitation, SA_Rate[currRate]);
         lag2 = pLags[lNSbfr+1];
         ippsDecodeAdaptiveVector_G723_16s(lag1, lag2, pGains[lNSbfr+1], &pPrevExc[G723_SBFR_LEN], &pCurrExcitation[G723_SBFR_LEN], SA_Rate[currRate]);
      }
      /*ippsMaxAbs_16s(pCurrExcitation,2*G723_SBFR_LEN,&sTmp1);*/
      ippsMax_16s(pCurrExcitation,2*G723_SBFR_LEN,&sTmp1);
      ippsMin_16s(pCurrExcitation,2*G723_SBFR_LEN,&sTmp);
      if(-sTmp > sTmp1) sTmp1 = -sTmp;
      if(sTmp1 == 0) sfs = 0;
      else {
         sfs = 4 - Exp_16s(sTmp1); /* 4 bits of margin  */
         if(sfs < -2) sfs = -2;
      }
      if(sfs<0)
         ippsLShiftC_16s(pCurrExcitation,-sfs,pTmpExcitation,2*G723_SBFR_LEN);
      else
         ippsRShiftC_16s(pCurrExcitation,sfs,pTmpExcitation,2*G723_SBFR_LEN);
      lTmp = pTmpExcitation[pPosition[0+NUM_PULSE_IN_BLOCK*lNBlock]] * pSign[0+NUM_PULSE_IN_BLOCK*lNBlock];
      for(i=1; i<NUM_PULSE_IN_BLOCK; i++) {
         lTmp += pTmpExcitation[pPosition[i+NUM_PULSE_IN_BLOCK*lNBlock]] * pSign[i+NUM_PULSE_IN_BLOCK*lNBlock];
      }
      sTmp = lTmp>>14; 
      sB = ((sTmp*INV_NUM_PULSE_IN_BLOCK)+0x4000)>>15;   
      /* excitation energy */
      ippsDotProd_16s32s_Sfs(pTmpExcitation,pTmpExcitation,2*G723_SBFR_LEN,&lExcEnergy,-1);

      /* compute 2*G723_SBFR_LEN x gain**2 x 2**(-2sh1+1)    */
      /* gain input = 2**5 gain                     */
      sTmp = (short)((gain * G723_SBFR_LEN)>>5);
      lTmp = 2 * sTmp * gain;
      sTmp = (sfs<<1)+4;
      lTmp = lTmp>>sTmp; 
      lTmp = lExcEnergy - lTmp;
      lC  = MulC_32s(INV_NUM_PULSE_IN_BLOCK, lTmp); /*  * 1/NbPuls  */

     /* Solve EQ(X) = X**2 + 2 sB X + c */
      lTmp = 2* sB * sB - lC;             
      if(lTmp <=  0) {  
          sX1 = -sB;        
      }
      else {
         sDiscr = ownSqrt_32s(lTmp>>1);  
         sX1 = sDiscr - sB;      
         sX2 = sDiscr + sB;      
         sAbsX2 = abs(sX2);
         sAbsX1 = abs(sX1);
         if (sAbsX2 < sAbsX1) sX1 = -sX2;
      }

      /* Update Excitation */
      if(++sfs < 0) sTmp = sX1>>(-sfs);
      else sTmp = sX1<<sfs;

      if(sTmp > G723_MAX_GAIN) 
         sTmp = G723_MAX_GAIN;

      if(sTmp < -G723_MAX_GAIN) 
	      sTmp = -G723_MAX_GAIN;

      for(i=0; i<NUM_PULSE_IN_BLOCK; i++) { j = pPosition[i+NUM_PULSE_IN_BLOCK*lNBlock]; pCurrExcitation[j] = pCurrExcitation[j] + 
	    (sTmp * (pSign[i+NUM_PULSE_IN_BLOCK*lNBlock])>>15);
      }

      /* update PrevExcitation */
      ippsCopy_16s(&pPrevExc[2*G723_SBFR_LEN],pPrevExc,G723_MAX_PITCH-2*G723_SBFR_LEN);
      ippsCopy_16s(pCurrExcitation,&pPrevExc[G723_MAX_PITCH-2*G723_SBFR_LEN],2*G723_SBFR_LEN);

      pCurrExcitation += 2*G723_SBFR_LEN;
      lNSbfr += 2;
   } 
}

IppSpchBitRate SA_Rate[2] = {IPP_SPCHBR_6300, IPP_SPCHBR_5300};


__ALIGN(16) short   LPCDCTbl[G723_LPC_ORDER] = {
   3131,4721,7690,10806,13872,16495,19752,22260,25484,27718};


__ALIGN(16) short   PerceptualFltCoeffTbl[2*G723_LPC_ORDER] = {
   29491,26542,23888,21499,19349,17414,15673,14106,12695,11425,/* Zero part */
   16384, 8192, 4096, 2048, 1024,  512,  256,  128,   64,   32,/* Pole part */
};

short   GainDBLvls[N_GAINS] = {
  0xFFFF,  0xFFFE, 0xFFFD, 0xFFFC, 0xFFFA, 0xFFF7, 0xFFF3, 0xFFEE,
  0xFFE6,  0xFFDA, 0xFFC9, 0xFFB0, 0xFF8D, 0xFF5A, 0xFF10, 0xFEA4,
  0xFE0A,  0xFD2A, 0xFBE6, 0xFA13, 0xF76F, 0xF39E, 0xEE1A, 0xE621 
};
