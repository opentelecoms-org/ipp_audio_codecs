/* /////////////////////////////////////////////////////////////////////////////
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
//  Porpose: ITU-T G.729 Speech Decoder    ANSI-C Source ACELPcodeVec compliant
//           Version 1.1
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <ippsc.h>
#include "owng729.h"

/* HPF coefficients */

static const short tab1[3] = {
 7699,
 -15398,
 7699};      
static const short tab2[3] = {BWF_HARMONIC_E, 15836, -7667};      
static const short lspSID_init[LPF_DIM] = {31441,  27566,  21458, 13612,
 4663, -4663, -13612,
 -21458, -27566, -31441};

static const short tab3[32] = {
        1, 3, 8, 6, 18, 16,
        11, 13, 38, 36, 31, 33, 
        21, 23, 28, 26, 0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0};

static const short tab4[32] = {
    0, 2, 5, 4, 12, 10, 7, 9, 25,
    24, 20, 22, 14, 15, 19, 17,
    36, 31, 21, 26, 1, 6, 16, 11, 
    27, 29, 32, 30, 39, 37, 34, 35};

static int DecoderObjSize(void) {
    int fltSize;
    int objSize = sizeof(G729Decoder_Obj);
    ippsHighPassFilterSize_G729(&fltSize);
    objSize += fltSize; /* provide memory for postprocessing high pass filter with upscaling */
    SynthesisFilterSize_G729(&fltSize);
    objSize += 2 * fltSize;/* provide memory for two synthesis filters */
    ippsPhaseDispersionGetStateSize_G729D_16s(&fltSize);
    objSize += fltSize; /* provide memory for phase dispersion */
    return objSize;
}

G729_CODECFUN( APIG729_Status, apiG729Decoder_Alloc,
               (G729Codec_Type codecType, int *pCodecSize)) {
    if((codecType != G729_CODEC)&&(codecType != G729A_CODEC)
       &&(codecType != G729D_CODEC)&&(codecType != G729E_CODEC)&&(codecType != G729I_CODEC)) {
        return APIG729_StsBadCodecType;
    }
    *pCodecSize =  DecoderObjSize();
    return APIG729_StsNoErr;
}

G729_CODECFUN( APIG729_Status, apiG729Decoder_Init,
               (G729Decoder_Obj* decoderObj, G729Codec_Type codecType)) {
    int i,fltSize;
    Ipp16s abDec[6];

    if((codecType != G729_CODEC)&&(codecType != G729A_CODEC)
       &&(codecType != G729D_CODEC)&&(codecType != G729E_CODEC)&&(codecType != G729I_CODEC)) {
        return APIG729_StsBadCodecType;
    }

    ippsZero_16s((short*)decoderObj,sizeof(*decoderObj)>>1) ;  

    decoderObj->objPrm.objSize = DecoderObjSize();
    decoderObj->objPrm.key = DEC_KEY;
    decoderObj->objPrm.codecType=codecType;

    decoderObj->codecType=codecType;
    decoderObj->synFltw=NULL;
    decoderObj->synFltw0=NULL;
    decoderObj->PhDispMem=NULL;

    decoderObj->postProc = (char*)decoderObj + sizeof(G729Decoder_Obj);
    ippsHighPassFilterSize_G729(&fltSize); 
    decoderObj->synFltw = (char*)decoderObj->postProc + fltSize;
    SynthesisFilterSize_G729(&fltSize);
    decoderObj->synFltw0 = (char*)decoderObj->synFltw + fltSize;
    decoderObj->PhDispMem = (char*)decoderObj->synFltw0 + fltSize;
    abDec[0] = tab2[0];
    abDec[1] = tab2[1];
    abDec[2] = tab2[2];
    abDec[3] = tab1[0];
    abDec[4] = tab1[1];
    abDec[5] = tab1[2];
    for(i=0;i<4;i++) decoderObj->prevFrameQuantEn[i]=-14336;
    ippsHighPassFilterInit_G729(abDec,decoderObj->postProc);
    SynthesisFilterInit_G729(decoderObj->synFltw);
    SynthesisFilterInit_G729(decoderObj->synFltw0);
    ippsPhaseDispersionInit_G729D_16s((IppsPhaseDispersion_State_G729D *)decoderObj->PhDispMem);

    /* synthesis speech buffer*/     
    ippsZero_16s(decoderObj->LTPostFilt,TBWD_DIM);
    decoderObj->voiceFlag=60;
    ippsZero_16s(decoderObj->prevExcitat, L_prevExcitat);
    decoderObj->betaPreFilter = PITCH_SHARP_MIN;
    decoderObj->prevFrameDelay = 60;
    decoderObj->gains[0] = 0;
    decoderObj->gains[1] = 0;
    for(i=0; i<LSP_MA_ORDER; i++)
        ippsCopy_16s( &resetPrevLSP[0], &decoderObj->prevLSPfreq[i][0], LPF_DIM );
    ippsCopy_16s(presetLSP, decoderObj->prevSubfrLSP, LPF_DIM );
    decoderObj->preemphFilt = 0;
    ippsZero_16s(decoderObj->resFilBuf1, MAX_PITCH_LAG+LP_SUBFRAME_DIM);
    ippsZero_16s(decoderObj->zeroPostFiltVec1 + LPF_DIM+1, BWLPCF1_DIM/*IMP_RESP_LEN*/);
    decoderObj->seedSavage = 21845;
    decoderObj->seed = SEED_INIT;
    decoderObj->CNGvar = 3;

    if(decoderObj->codecType == G729_CODEC ) {
        decoderObj->gainExact = BWF_HARMONIC;
    } else if( decoderObj->codecType == G729A_CODEC) {
        decoderObj->gainExact = (1<<12);
        decoderObj->CNGidx = 0;
        decoderObj->SIDflag0 = 0;
        decoderObj->SIDflag1 = 1;
        ippsCopy_16s( lspSID_init, decoderObj->lspSID, LPF_DIM );
    } else {
        decoderObj->prevMA = 0;
        decoderObj->gammaPost1 = BWF1_PST_E;
        decoderObj->gammaPost2 = BWF2_PST_E;
        decoderObj->gammaHarm = BWF_HARMONIC_E;
        decoderObj->BWDcounter2 = 0;
        decoderObj->FWDcounter2 = 0;

        ippsZero_16s(decoderObj->pBwdLPC, BWLPCF1_DIM);
        ippsZero_16s(decoderObj->pBwdLPC2, BWLPCF1_DIM);
        decoderObj->pBwdLPC[0] = (1<<12);
        decoderObj->pBwdLPC2[0] = (1<<12);

        decoderObj->prevVoiceFlag = 0;
        decoderObj->prevBFI = 0;
        decoderObj->prevLPmode = 0;
        decoderObj->interpCoeff2 = 0;
        decoderObj->interpCoeff2_2 = 4506;
        ippsZero_16s(decoderObj->pPrevFilt, BWLPCF1_DIM);
        decoderObj->pPrevFilt[0] = (1<<12);
        decoderObj->prevPitch = 30;
        decoderObj->stat_pitch = 0;
        ippsZero_16s(decoderObj->pPrevBwdLPC, BWLPCF1_DIM);
        decoderObj->pPrevBwdLPC[0]= (1<<12);
        ippsZero_16s(decoderObj->pPrevBwdRC, 2);
        decoderObj->valGainAttenuation = IPP_MAX_16S;
        decoderObj->BFIcount = 0;
        decoderObj->BWDFrameCounter = 0;

        decoderObj->gainExact = BWF_HARMONIC;
        ippsWinHybridGetStateSize_G729E_16s(&fltSize);
        if(fltSize > sizeof(int)*BWLPCF1_DIM) {
            return APIG729_StsNotInitialized;
        }
        ippsWinHybridInit_G729E_16s((IppsWinHybridState_G729E_16s*)&decoderObj->hwState);

        decoderObj->SIDflag0 = 0;
        decoderObj->SIDflag1 = 1;
        decoderObj->CNGidx = 0;
        ippsCopy_16s( lspSID_init, decoderObj->lspSID, LPF_DIM );
        decoderObj->sidGain = SIDgain[0];
    }
    return APIG729_StsNoErr;
}

G729_CODECFUN( APIG729_Status, apiG729Decoder_InitBuff, 
               (G729Decoder_Obj* decoderObj, char *buff)) {
#if !defined (NO_SCRATCH_MEMORY_USED)

    if(NULL==decoderObj || NULL==buff)
        return APIG729_StsBadArgErr;

    decoderObj->Mem.base = buff;
    decoderObj->Mem.CurPtr = decoderObj->Mem.base;
    decoderObj->Mem.VecPtr = (int *)(decoderObj->Mem.base+G729_ENCODER_SCRATCH_MEMORY_SIZE);
#endif /*SCRATCH_IDAIS*/
    return APIG729_StsNoErr;
}

static APIG729_Status G729Decode
(G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst);
static APIG729_Status G729ADecode
(G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst);
static APIG729_Status G729BaseDecode
(G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst);

G729_CODECFUN( APIG729_Status, apiG729Decode, 
               (G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst)) {
    if(decoderObj->codecType == G729A_CODEC) {
        if(G729ADecode(decoderObj,src,frametype,dst) != APIG729_StsNoErr) {
            return APIG729_StsErr;
        }
    } else if(decoderObj->codecType == G729_CODEC && frametype != 2 && frametype != 4) {
        if(G729BaseDecode(decoderObj,src,frametype,dst) != APIG729_StsNoErr) {
            return APIG729_StsErr;
        }
    } else {
        if(G729Decode(decoderObj,src,frametype,dst) != APIG729_StsNoErr) {
            return APIG729_StsErr;
        }
    }
    return APIG729_StsNoErr;
}

APIG729_Status G729Decode
(G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst) {

    LOCAL_ALIGN_ARRAY(32, short, AzDec, BWLPCF1_DIM*2,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, newLSP,LPF_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, ACELPcodeVec, LP_SUBFRAME_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, BWDfiltLPC, 2*BWLPCF1_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, FWDfiltLPC, 2*LPF_DIM+2,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, BWDrc,BWLPCF_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, int,   BWDacf,BWLPCF1_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, int,   BWDhighAcf,BWLPCF1_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, phaseDispExcit,LP_SUBFRAME_DIM,decoderObj);
    LOCAL_ARRAY(short, prevFrameDelay,2,decoderObj);
    LOCAL_ARRAY(short, idx,4,decoderObj);
    LOCAL_ARRAY(short, delayVal,2,decoderObj);
    LOCAL_ARRAY(short, tmp_parm,5,decoderObj);
    short  prevFrameDelay1=0;  /* 1-st subframe pitch lag*/
    short  subfrVoiceFlag, *pAz, *pA; 
    char  *synFltw = decoderObj->synFltw; 
    short *prevExcitat = decoderObj->prevExcitat;
    short *excitation = prevExcitat + L_prevExcitat;
    short *synth = decoderObj->LTPostFilt+SYNTH_BWD_DIM;
    short *prevSubfrLSP = decoderObj->prevSubfrLSP;  
    const  unsigned char *pParm;
    const  short *parm;
    short  *ppAz, temp, badFrameIndicator, badPitch, index, pulseSign, gPl, gC;
    short  voiceFlag = decoderObj->voiceFlag;
    short  sidGain = decoderObj->sidGain;
    short  gainNow = decoderObj->gainNow;
    short *lspSID = decoderObj->lspSID;
    int    i, j, subfrIdx, index2, fType, gpVal=0, LPmode = 0;
    short  satFilter, statStat, mAq, prevM, dominantBWDmode, L_hSt;
    IppStatus status;

    if(NULL==decoderObj || NULL==src || NULL ==dst)
        return APIG729_StsBadArgErr;
    if((decoderObj->codecType != G729_CODEC)
       &&(decoderObj->codecType != G729D_CODEC)&&(decoderObj->codecType != G729E_CODEC)&&(decoderObj->codecType != G729I_CODEC)) {
        return APIG729_StsBadCodecType;
    }
    if(decoderObj->objPrm.objSize <= 0)
        return APIG729_StsNotInitialized;
    if(DEC_KEY != decoderObj->objPrm.key)
        return APIG729_StsBadCodecType;

    delayVal[0]=delayVal[1]=0;

    pA = AzDec;
    pParm = src;
    if(frametype == -1) { 
        decoderObj->decPrm[1] = 0; 
        decoderObj->decPrm[0] = 1; 
    } else if(frametype == 0) {  
        decoderObj->decPrm[1] = 0;  
        decoderObj->decPrm[0] = 0; 

    } else if(frametype == 1) { 
        decoderObj->decPrm[1] = 1; 
        decoderObj->decPrm[0] = 0; 
        i=0;
        decoderObj->decPrm[1+1] = ExtractBitsG729(&pParm,&i,1);
        decoderObj->decPrm[1+2] = ExtractBitsG729(&pParm,&i,5);
        decoderObj->decPrm[1+3] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+4] = ExtractBitsG729(&pParm,&i,5);

    } else if(frametype == 2) { 
        decoderObj->decPrm[0] = 0; 
        decoderObj->decPrm[1] = 2; 
        i=0;
        decoderObj->decPrm[2] = ExtractBitsG729(&pParm,&i,1+FIR_STAGE_BITS);
        decoderObj->decPrm[3] = ExtractBitsG729(&pParm,&i,SEC_STAGE_BITS*2);
        decoderObj->decPrm[4] = ExtractBitsG729(&pParm,&i,8);
        decoderObj->decPrm[5] = ExtractBitsG729(&pParm,&i,9);
        decoderObj->decPrm[6] = ExtractBitsG729(&pParm,&i,2);
        decoderObj->decPrm[7] = ExtractBitsG729(&pParm,&i,6);
        decoderObj->decPrm[8] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[9] = ExtractBitsG729(&pParm,&i,9);
        decoderObj->decPrm[10] = ExtractBitsG729(&pParm,&i,2);
        decoderObj->decPrm[11] = ExtractBitsG729(&pParm,&i,6);

        decoderObj->codecType = G729D_CODEC;
    } else if(frametype == 3) { 
        i=0;
        decoderObj->decPrm[1] = 3;  
        decoderObj->decPrm[0] = 0; 
        decoderObj->decPrm[1+1] = ExtractBitsG729(&pParm,&i,1+FIR_STAGE_BITS);
        decoderObj->decPrm[1+2] = ExtractBitsG729(&pParm,&i,SEC_STAGE_BITS*2);
        decoderObj->decPrm[1+3] = ExtractBitsG729(&pParm,&i,8);
        decoderObj->decPrm[1+4] = ExtractBitsG729(&pParm,&i,1);
        decoderObj->decPrm[1+5] = ExtractBitsG729(&pParm,&i,13);
        decoderObj->decPrm[1+6] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+7] = ExtractBitsG729(&pParm,&i,7);
        decoderObj->decPrm[1+8] = ExtractBitsG729(&pParm,&i,5);
        decoderObj->decPrm[1+9] = ExtractBitsG729(&pParm,&i,13);
        decoderObj->decPrm[1+10] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+11] = ExtractBitsG729(&pParm,&i,7);
        decoderObj->decPrm[1+4] = (equality(decoderObj->decPrm[1+3])+decoderObj->decPrm[1+4]) & 0x00000001; /*  parity error (1) */
        decoderObj->codecType = G729_CODEC;
    } else if(frametype == 4) { 
        short tmp;
        i=0;
        tmp = ExtractBitsG729(&pParm,&i,2);

        decoderObj->decPrm[0] = 0; 
        decoderObj->decPrm[1] = 4; 
        if(tmp==0) {
            decoderObj->decPrm[2] = 0; 

            decoderObj->decPrm[3] = ExtractBitsG729(&pParm,&i,1+FIR_STAGE_BITS);
            decoderObj->decPrm[4] = ExtractBitsG729(&pParm,&i,SEC_STAGE_BITS*2);
            decoderObj->decPrm[5] = ExtractBitsG729(&pParm,&i,8);
            decoderObj->decPrm[6] = ExtractBitsG729(&pParm,&i,1);
            decoderObj->decPrm[7] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[8] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[9] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[10] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[11] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[12] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[13] = ExtractBitsG729(&pParm,&i,5);
            decoderObj->decPrm[14] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[15] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[16] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[17] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[18] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[19] = ExtractBitsG729(&pParm,&i,7);

            i = decoderObj->decPrm[5]>>1;
            i &= 1;
            decoderObj->decPrm[6] += i;
            decoderObj->decPrm[6] = (equality(decoderObj->decPrm[5])+decoderObj->decPrm[6]) & 0x00000001;/* parm[6] = parity error (1) */
        } else {
            decoderObj->decPrm[2] = 1; /*LPmode*/

            decoderObj->decPrm[3] = ExtractBitsG729(&pParm,&i,8);
            decoderObj->decPrm[4] = ExtractBitsG729(&pParm,&i,1);
            decoderObj->decPrm[5] = ExtractBitsG729(&pParm,&i,13);
            decoderObj->decPrm[6] = ExtractBitsG729(&pParm,&i,10);
            decoderObj->decPrm[7] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[8] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[9] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[10] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[11] = ExtractBitsG729(&pParm,&i,5);
            decoderObj->decPrm[12] = ExtractBitsG729(&pParm,&i,13);
            decoderObj->decPrm[13] = ExtractBitsG729(&pParm,&i,10);
            decoderObj->decPrm[14] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[15] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[16] = ExtractBitsG729(&pParm,&i,7);
            decoderObj->decPrm[17] = ExtractBitsG729(&pParm,&i,7);

            i = decoderObj->decPrm[3]>>1;
            i &= 1;
            decoderObj->decPrm[4] += i;
            decoderObj->decPrm[4] = (equality(decoderObj->decPrm[3])+decoderObj->decPrm[4]) & 0x00000001;/* parm[4] = parity error (1) */
        }
        decoderObj->codecType = G729E_CODEC;
    }

    parm = decoderObj->decPrm;

    badFrameIndicator = *parm++;
    fType = *parm;
    if(badFrameIndicator == 1) {
        fType = decoderObj->CNGvar;
        if(fType == 1) fType = 0;
    } else {
        decoderObj->valGainAttenuation = IPP_MAX_16S;
        decoderObj->BFIcount = 0;
    }

    if( fType == 4) {
        if(badFrameIndicator != 0) {
            LPmode = decoderObj->prevLPmode; 
        } else {
            LPmode = parm[1];
        } 
        if(decoderObj->prevBFI != 0) {
            voiceFlag = decoderObj->prevVoiceFlag;
            decoderObj->voiceFlag = decoderObj->prevVoiceFlag;
        }
        ippsWinHybrid_G729E_16s32s(decoderObj->LTPostFilt, BWDacf, 
                                   (IppsWinHybridState_G729E_16s*)&decoderObj->hwState); 

        BWDLagWindow(BWDacf, BWDhighAcf);

        if(ippsLevinsonDurbin_G729_32s16s(BWDhighAcf, BWLPCF_DIM, &BWDfiltLPC[BWLPCF1_DIM], BWDrc,&temp) == ippStsOverflow) {
            ippsCopy_16s(decoderObj->pPrevBwdLPC,&BWDfiltLPC[BWLPCF1_DIM],BWLPCF1_DIM);
            BWDrc[0] = decoderObj->pPrevBwdRC[0];
            BWDrc[1] = decoderObj->pPrevBwdRC[1];

        } else {
            ippsCopy_16s(&BWDfiltLPC[BWLPCF1_DIM],decoderObj->pPrevBwdLPC,BWLPCF1_DIM);
            decoderObj->pPrevBwdRC[0] = BWDrc[0]; 
            decoderObj->pPrevBwdRC[1] = BWDrc[1];
        }

        satFilter = 0;
        for(i=BWLPCF1_DIM; i<2*BWLPCF1_DIM; i++) {
            if(BWDfiltLPC[i] >= IPP_MAX_16S) {
                satFilter = 1;
                break;
            }
        }
        if(satFilter == 1) ippsCopy_16s(decoderObj->pBwdLPC2, &BWDfiltLPC[BWLPCF1_DIM], BWLPCF1_DIM);
        else ippsCopy_16s(&BWDfiltLPC[BWLPCF1_DIM], decoderObj->pBwdLPC2, BWLPCF1_DIM);

        ippsMulPowerC_NR_16s_Sfs(&BWDfiltLPC[BWLPCF1_DIM], N0_98, &BWDfiltLPC[BWLPCF1_DIM], BWLPCF1_DIM, 15);
    }
    ippsCopy_16s(&decoderObj->LTPostFilt[LP_FRAME_DIM], &decoderObj->LTPostFilt[0], SYNTH_BWD_DIM);

    if(LPmode == 1) {
        short tmp;

        if((decoderObj->interpCoeff2 != 0)) {
            tmp = (1<<12) - decoderObj->interpCoeff2;
            ippsInterpolateC_G729_16s_Sfs(BWDfiltLPC + BWLPCF1_DIM, tmp,
                                          decoderObj->pBwdLPC, decoderObj->interpCoeff2, BWDfiltLPC + BWLPCF1_DIM, BWLPCF1_DIM, 12);
        }
    }
    if((badFrameIndicator != 0)&&(decoderObj->prevBFI == 0) && (decoderObj->CNGvar >3))
        ippsCopy_16s(&BWDfiltLPC[BWLPCF1_DIM],decoderObj->pBwdLPC,BWLPCF1_DIM);

    if(fType < 2) {
        if(fType == 1) {
            LOCAL_ALIGN_ARRAY(32, short, lsfq,LPF_DIM,decoderObj);
            sidGain = SIDgain[(int)parm[4]];           
            ippsLSFDecode_G729B_16s(&parm[1],(Ipp16s*)(decoderObj->prevLSPfreq),lsfq);
            ippsLSFToLSP_G729_16s(lsfq,lspSID);
            LOCAL_ALIGN_ARRAY_FREE(32, short, lsfq,LPF_DIM,decoderObj);
        } else {
            if(decoderObj->CNGvar > 1) {
                QuantSIDGain_G729B_16s(&decoderObj->SIDflag0, &decoderObj->SIDflag1, 0, &temp, &index2);
                sidGain = SIDgain[(int)index2];
            }
        } 
        if(decoderObj->CNGvar > 1) {
            gainNow = sidGain;  
        } else {
            gainNow = (gainNow * GAIN0 + BWF_HARMONIC)>>15;
            gainNow = Add_16s(gainNow, (short)((sidGain * GAIN1 + BWF_HARMONIC)>>15));
        } 

        if(gainNow == 0) {
            ippsZero_16s(excitation,LP_FRAME_DIM);
            gpVal = 0;
            for(subfrIdx = 0;  subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM) {
                ippsPhaseDispersionUpdate_G729D_16s(gpVal,gainNow,
                                                    (IppsPhaseDispersion_State_G729D*)decoderObj->PhDispMem);
            }
        } else {
            for(i = 0;  i < LP_FRAME_DIM; i += LP_SUBFRAME_DIM) {
                int    invSq;
                Ipp16s pG2, tmp, tmp2;
                const short *excCached;
                LOCAL_ARRAY(Ipp16s, idx, 4, decoderObj);
                LOCAL_ARRAY(Ipp16s, pulseSign, 4, decoderObj);
                LOCAL_ALIGN_ARRAY(32, Ipp16s, excg, LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ARRAY(Ipp16s,tempArray,LP_SUBFRAME_DIM, decoderObj);
                RandomCodebookParm_G729B_16s(&decoderObj->seed,idx,pulseSign,&pG2,delayVal);
                ippsDecodeAdaptiveVector_G729_16s_I(delayVal,&prevExcitat[i]);
                if(decoderObj->CNGidx > CNG_STACK_SIZE-1) { /* not cached */
                    ippsRandomNoiseExcitation_G729B_16s(&decoderObj->seed,excg,LP_SUBFRAME_DIM);
                    ippsDotProd_16s32s_Sfs(excg,excg,LP_SUBFRAME_DIM,&invSq,0);
                    ippsInvSqrt_32s_I(&invSq,1); 
                    excCached=excg;
                } else {
                    decoderObj->seed = cngSeedOut[decoderObj->CNGidx];
                    invSq = cngInvSqrt[decoderObj->CNGidx];
                    excCached=&cngCache[decoderObj->CNGidx][0];
                    decoderObj->CNGidx++;
                } 
                NoiseExcitationFactorization_G729B_16s(excCached,invSq,gainNow,excg,LP_SUBFRAME_DIM);
                tmp2 = ComfortNoiseExcitation_G729B_16s_I(excg,idx,pulseSign,gainNow,pG2,&excitation[i],&tmp,tempArray);
                if(tmp2 < 0) gpVal = 0;
                if(tmp >= 0)
                    ippsPhaseDispersionUpdate_G729D_16s(gpVal,tmp,
                                                        (IppsPhaseDispersion_State_G729D*)decoderObj->PhDispMem);
                else
                    ippsPhaseDispersionUpdate_G729D_16s(gpVal,(short)-tmp,
                                                        (IppsPhaseDispersion_State_G729D*)decoderObj->PhDispMem);

                LOCAL_ARRAY_FREE(Ipp16s,tempArray,LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ALIGN_ARRAY_FREE(32, Ipp16s, excg, LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ARRAY_FREE(Ipp16s, pulseSign, 4, decoderObj);
                LOCAL_ARRAY_FREE(Ipp16s, idx, 4, decoderObj);
            } 
        } 
        ippsInterpolate_G729_16s(prevSubfrLSP,lspSID,prevSubfrLSP, LPF_DIM );
        ippsLSPToLPC_G729_16s(prevSubfrLSP,&FWDfiltLPC[0]);
        ippsLSPToLPC_G729_16s(lspSID,&FWDfiltLPC[LPF_DIM+1]);
        ippsCopy_16s(lspSID, prevSubfrLSP, LPF_DIM );
        decoderObj->sidGain = sidGain;
        decoderObj->gainNow = gainNow;

        ppAz = FWDfiltLPC;
        for(subfrIdx = 0; subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM) {
            if(SynthesisFilter_G729_16s(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,20)==
               ippStsOverflow) {
                /* scale down excitation and redo in case of overflow */
                ippsRShiftC_16s_I(2,prevExcitat,L_prevExcitat+LP_FRAME_DIM);
                SynthesisFilterOvf_G729_16s(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,20);
            }

            ppAz += LPF_DIM+1;
            prevFrameDelay[subfrIdx/LP_SUBFRAME_DIM] = decoderObj->prevFrameDelay;
        } 
        decoderObj->betaPreFilter = PITCH_SHARP_MIN;

        prevFrameDelay1 = decoderObj->prevFrameDelay;
        decoderObj->interpCoeff2_2 = 4506;
        decoderObj->BWDFrameCounter = 0;
        decoderObj->stat_pitch = 0;
        ippsCopy_16s(&FWDfiltLPC[LPF_DIM+1], decoderObj->pPrevFilt, LPF_DIM+1);
        ippsZero_16s(&decoderObj->pPrevFilt[LPF_DIM+1], (BWLPCF1_DIM-LPF_DIM-1));

    } else { 
        decoderObj->seed = SEED_INIT;
        decoderObj->CNGidx = 0;
        parm++;
        if(decoderObj->codecType == G729E_CODEC) parm++;
        if( LPmode==0 ) {
            LOCAL_ARRAY(short, qIndex,4,decoderObj);

            qIndex[0] = (parm[0] >> FIR_STAGE_BITS) & 1;
            qIndex[1] = (Ipp16s)(parm[0] & (FIR_STAGE - 1));
            qIndex[2] = (Ipp16s)((parm[1] >> SEC_STAGE_BITS) & (SEC_STAGE - 1));
            qIndex[3] = (Ipp16s)(parm[1] & (SEC_STAGE - 1));
            if(!badFrameIndicator) {
                decoderObj->prevMA = qIndex[0];
                ippsLSFDecode_G729_16s( qIndex, (short*)decoderObj->prevLSPfreq, decoderObj->prevSubfrLSPquant);
            } else {
                ippsLSFDecodeErased_G729_16s( decoderObj->prevMA, 
                                              (short*)decoderObj->prevLSPfreq, decoderObj->prevSubfrLSPquant);
            } 

            ippsLSFToLSP_G729_16s(decoderObj->prevSubfrLSPquant, newLSP); /* Convert LSFs to LSPs */
            parm += 2;
            LOCAL_ARRAY_FREE(short, qIndex,4,decoderObj);

            if( decoderObj->prevLPmode == 0) {
                ippsInterpolate_G729_16s(newLSP,prevSubfrLSP,prevSubfrLSP, LPF_DIM );

                ippsLSPToLPC_G729_16s(prevSubfrLSP, FWDfiltLPC);            /*  1-st subframe */

                ippsLSPToLPC_G729_16s(newLSP, &FWDfiltLPC[LPF_DIM+1]);      /* 2-nd subframe */
            } else {
                ippsLSPToLPC_G729_16s(newLSP, FWDfiltLPC);                  /* 1-st subframe */
                ippsCopy_16s(FWDfiltLPC, &FWDfiltLPC[LPF_DIM+1], LPF_DIM+1);/* 2-nd subframe */
            }

            /* update the next frame LSFs*/
            ippsCopy_16s(newLSP, prevSubfrLSP, LPF_DIM );
            decoderObj->interpCoeff2_2 = 4506;
            mAq = LPF_DIM;
            pA = FWDfiltLPC;
            ippsCopy_16s(&FWDfiltLPC[LPF_DIM+1], decoderObj->pPrevFilt, LPF_DIM+1);
            ippsZero_16s(&decoderObj->pPrevFilt[LPF_DIM+1], (BWLPCF1_DIM-LPF_DIM-1));
        } else {
            short tmp;
            decoderObj->interpCoeff2_2 = decoderObj->interpCoeff2_2 - 410;
            if( decoderObj->interpCoeff2_2 < 0) decoderObj->interpCoeff2_2 = 0;
            tmp = (1<<12) - decoderObj->interpCoeff2_2;
            ippsInterpolateC_G729_16s_Sfs(BWDfiltLPC + BWLPCF1_DIM, tmp,
                                          decoderObj->pPrevFilt, decoderObj->interpCoeff2_2, BWDfiltLPC + BWLPCF1_DIM, BWLPCF1_DIM, 12);
            ippsInterpolate_G729_16s
            (BWDfiltLPC + BWLPCF1_DIM, decoderObj->pPrevFilt, BWDfiltLPC, BWLPCF1_DIM);
            mAq = BWLPCF_DIM;
            pA = BWDfiltLPC;
            ippsCopy_16s(&BWDfiltLPC[BWLPCF1_DIM], decoderObj->pPrevFilt, BWLPCF1_DIM);
        }

        for(ppAz=pA,subfrIdx = 0; subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM) {
            int pitchIndx;
            pitchIndx = *parm++; 
            badPitch = badFrameIndicator;

            if(subfrIdx == 0) {
                if(decoderObj->codecType != G729D_CODEC)
                    badPitch = badFrameIndicator + *parm++;
            }
            DecodeAdaptCodebookDelays(&decoderObj->prevFrameDelay,&decoderObj->prevFrameDelay2,delayVal,subfrIdx,badPitch,pitchIndx,decoderObj->codecType);
            if(subfrIdx == 0)
                prevFrameDelay1 = delayVal[0];         /* if first frame */
            ippsDecodeAdaptiveVector_G729_16s_I(delayVal,&prevExcitat[subfrIdx]);

            /* pitch tracking */
            if( decoderObj->codecType == G729E_CODEC) {
                PitchTracking_G729E(&decoderObj->prevFrameDelay, &decoderObj->prevFrameDelay2, &decoderObj->prevPitch, &decoderObj->stat_pitch,
                                    &decoderObj->pitchStatIntDelay, &decoderObj->pitchStatFracDelay);
            } else {
                short i, j;
                i = decoderObj->prevFrameDelay;
                j = decoderObj->prevFrameDelay2;
                PitchTracking_G729E(&i, &j, &decoderObj->prevPitch, &decoderObj->stat_pitch,
                                    &decoderObj->pitchStatIntDelay, &decoderObj->pitchStatFracDelay);
            }

            statStat = 0;
            if(decoderObj->codecType == G729_CODEC) {
                if(badFrameIndicator != 0) {
                    index = Rand_16s(&decoderObj->seedSavage) & (short)0x1fff;     /* 13 bits random */
                    pulseSign = Rand_16s(&decoderObj->seedSavage) & (short)15;     /*  4 bits random */
                } else {
                    index = parm[0];
                    pulseSign = parm[1];
                } 
                i      = index & 7;
                idx[0] = 5 * i; 
                index  = index >> 3;
                i      = index & 7;
                idx[1] = 5 * i + 1; 
                index  = index >> 3;
                i      = index & 7;
                idx[2] = 5 * i + 2; 
                index  = index >> 3;
                j      = index & 1;
                index  = index >> 1;
                i      = index & 7;   
                idx[3] = i * 5 + 3 + j;

                /* decode the signs & build the codeword */
                ippsZero_16s(ACELPcodeVec,LP_SUBFRAME_DIM);
                for(j=0; j<4; j++) {
                    if((pulseSign & 1) != 0) {
                        ACELPcodeVec[idx[j]] = 8191;     
                    } else {
                        ACELPcodeVec[idx[j]] = -BWF_HARMONIC_E;    
                    }  
                    pulseSign = pulseSign >> 1; 
                }

                parm += 2;
                decoderObj->BWDFrameCounter = 0;
            } else if(decoderObj->codecType == G729D_CODEC) {
                short idx;

                if(badFrameIndicator != 0) { 
                    index = Rand_16s(&decoderObj->seedSavage);    
                    pulseSign = Rand_16s(&decoderObj->seedSavage);     
                } else {
                    index = parm[0];
                    pulseSign = parm[1];
                }
                ippsZero_16s(ACELPcodeVec,LP_SUBFRAME_DIM);
                idx = tab3[index & 15];
                if((pulseSign & 1) != 0) {
                    ACELPcodeVec[idx] += 8191; 
                } else {
                    ACELPcodeVec[idx] -= BWF_HARMONIC_E; 
                } 
                index >>= 4;
                pulseSign >>= 1;
                idx = tab4[index & 31];

                if((pulseSign & 1) != 0) {
                    ACELPcodeVec[idx] += 8191; 
                } else {
                    ACELPcodeVec[idx] -= BWF_HARMONIC_E; 
                }

                parm += 2;
                decoderObj->BWDFrameCounter = 0;
            } else if(decoderObj->codecType == G729E_CODEC) {
                short j, trackVal;
                short pos1, pos2, pos3, pulseSign;
                ippsZero_16s(ACELPcodeVec,LP_SUBFRAME_DIM);

                if(badFrameIndicator != 0) {
                    tmp_parm[0] = Rand_16s(&decoderObj->seedSavage);
                    tmp_parm[1] = Rand_16s(&decoderObj->seedSavage);
                    tmp_parm[2] = Rand_16s(&decoderObj->seedSavage);
                    tmp_parm[3] = Rand_16s(&decoderObj->seedSavage);
                    tmp_parm[4] = Rand_16s(&decoderObj->seedSavage);
                } else {
                    ippsCopy_16s(parm, tmp_parm, 5);
                } 
                if(LPmode == 0) {

                    pos1 = ((tmp_parm[0] & 7) * 5);               
                    if(((tmp_parm[0]>>3) & 1) == 0)
                        pulseSign = (1<<12);
                    else pulseSign = -(1<<12);                
                    ACELPcodeVec[pos1] = pulseSign;             
                    pos2 = (((tmp_parm[0]>>4) & 7) * 5);          
                    if(pos2 > pos1)
                        pulseSign = -pulseSign;
                    ACELPcodeVec[pos2] += pulseSign;
                    pos1 = ((tmp_parm[1] & 7) * 5) + 1;           
                    if(((tmp_parm[1]>>3) & 1) == 0)
                        pulseSign = (1<<12);
                    else
                        pulseSign = -(1<<12);                
                    ACELPcodeVec[pos1] = pulseSign;             

                    pos2 = (((tmp_parm[1]>>4) & 7) * 5) + 1;      
                    if(pos2 > pos1)
                        pulseSign = -pulseSign;

                    ACELPcodeVec[pos2] += pulseSign;

                    pos1 = ((tmp_parm[2] & 7) * 5) + 2;           
                    if(((tmp_parm[2]>>3) & 1) == 0)
                        pulseSign = (1<<12);
                    else
                        pulseSign = -(1<<12);               
                    ACELPcodeVec[pos1] = pulseSign;             

                    pos2 = (((tmp_parm[2]>>4) & 7) * 5) + 2;      
                    if(pos2 > pos1)
                        pulseSign = -pulseSign;
                    ACELPcodeVec[pos2] += pulseSign;
                    pos1 = ((tmp_parm[3] & 7) * 5) + 3;          
                    if(((tmp_parm[3]>>3) & 1) == 0)
                        pulseSign = (1<<12);
                    else
                        pulseSign = -(1<<12);                
                    ACELPcodeVec[pos1] = pulseSign;             
                    pos2 = (((tmp_parm[3]>>4) & 7) * 5) + 3;     
                    if(pos2 > pos1)
                        pulseSign = -pulseSign;

                    ACELPcodeVec[pos2] += pulseSign;

                    pos1 = ((tmp_parm[4] & 7) * 5) + 4;          
                    if(((tmp_parm[4]>>3) & 1) == 0)
                        pulseSign = (1<<12);
                    else pulseSign = -(1<<12);               
                    ACELPcodeVec[pos1] = pulseSign;             

                    pos2 = (((tmp_parm[4]>>4) & 7) * 5) + 4;     
                    if(pos2 > pos1) pulseSign = -pulseSign;

                    ACELPcodeVec[pos2] += pulseSign;
                    decoderObj->BWDFrameCounter = 0;
                } else {
                    trackVal = (tmp_parm[0]>>10) & 7;
                    if(trackVal > 4)
                        trackVal = 4;

                    for(j=0; j<2; j++) {
                        pos1 = ((tmp_parm[j] & 7) * 5) + trackVal;           
                        if(((tmp_parm[j]>>3) & 1) == 0)
                            pulseSign = (1<<12);
                        else
                            pulseSign = -(1<<12);               
                        ACELPcodeVec[pos1] = pulseSign;             

                        pos2 = (((tmp_parm[j]>>4) & 7) * 5) + trackVal;      
                        if(pos2 > pos1)
                            pulseSign = -pulseSign;

                        ACELPcodeVec[pos2] += pulseSign;

                        pos3 = (((tmp_parm[j]>>7) & 7) * 5) + trackVal;      
                        if(pos3 > pos2)
                            pulseSign = -pulseSign;

                        ACELPcodeVec[pos3] += pulseSign;

                        trackVal++;
                        if(trackVal > 4)
                            trackVal = 0;
                    } 

                    for(j=2; j<5; j++) {
                        pos1 = ((tmp_parm[j] & 7) * 5) + trackVal;         
                        if(((tmp_parm[j]>>3) & 1) == 0)
                            pulseSign = (1<<12);
                        else
                            pulseSign = -(1<<12);                              
                        ACELPcodeVec[pos1] = pulseSign;             

                        pos2 = (((tmp_parm[j]>>4) & 7) * 5) + trackVal;    
                        if(pos2 > pos1)
                            pulseSign = -pulseSign;
                        ACELPcodeVec[pos2] += pulseSign;
                        trackVal++;
                        if(trackVal > 4)
                            trackVal = 0;
                    } 
                    decoderObj->BWDFrameCounter++;
                    if(decoderObj->BWDFrameCounter >= 30) {
                        statStat = 1;
                        decoderObj->BWDFrameCounter = 30;
                    }
                }
                parm += 5;
            }

            decoderObj->betaPreFilter = decoderObj->betaPreFilter << 1;          
            if(delayVal[0] < LP_SUBFRAME_DIM) {
                ippsHarmonicFilter_16s_I(decoderObj->betaPreFilter,delayVal[0],&ACELPcodeVec[delayVal[0]],LP_SUBFRAME_DIM-delayVal[0]); 
            }
            pitchIndx = *parm++;      

            if(decoderObj->codecType == G729_CODEC) {
                if(!badFrameIndicator) {
                    LOCAL_ARRAY(short, gIngx, 2, decoderObj);
                    ippsDotProd_16s32s_Sfs(ACELPcodeVec, ACELPcodeVec, LP_SUBFRAME_DIM, &i, 0); /* ACELPcodeVec energy */
                    gIngx[0] = (short)(pitchIndx >> CDBK2_BIT_NUM) ;
                    gIngx[1] = (short)(pitchIndx & (CDBK2_DIM-1));
                    ippsDecodeGain_G729_16s(i, decoderObj->prevFrameQuantEn, gIngx, decoderObj->gains);
                    LOCAL_ARRAY_FREE(short, gIngx, 2, decoderObj);
                } else {
                    ippsDecodeGain_G729_16s(0, decoderObj->prevFrameQuantEn, NULL, decoderObj->gains);
                }  
            } else {
                int energy;

                ippsDotProd_16s32s_Sfs(ACELPcodeVec, ACELPcodeVec, LP_SUBFRAME_DIM, &energy, 0); /* ACELPcodeVec energy */
                if(decoderObj->codecType == G729D_CODEC) {
                    if(badFrameIndicator) {
                        ippsDecodeGain_G729_16s(0, decoderObj->prevFrameQuantEn, NULL, decoderObj->gains);
                    } else {
                        LOCAL_ARRAY(short, gIngx, 2, decoderObj);
                        short foo = 1;
                        gIngx[0] =  (short)(pitchIndx >> CDBK2_BIT_NUM_6K) ;
                        gIngx[1] =  (short)(pitchIndx & (CDBK2_DIM_6K-1)) ;
                        ippsDecodeGain_G729I_16s(energy, foo, decoderObj->prevFrameQuantEn, gIngx, decoderObj->gains );
                        LOCAL_ARRAY_FREE(short, gIngx, 2, decoderObj);
                    }
                } else { /* G729E_CODEC*/
                    if(!badFrameIndicator) {
                        LOCAL_ARRAY(short, gIngx, 2, decoderObj);
                        gIngx[0] = (short)(pitchIndx >> CDBK2_BIT_NUM) ;
                        gIngx[1] = (short)(pitchIndx & (CDBK2_DIM-1));
                        ippsDecodeGain_G729_16s(energy, decoderObj->prevFrameQuantEn, gIngx, decoderObj->gains);
                        LOCAL_ARRAY_FREE(short, gIngx, 2, decoderObj);
                    } else { /* erasure*/
                        short oldCodebookGain = decoderObj->gains[1];

                        ippsDecodeGain_G729I_16s(0, decoderObj->valGainAttenuation, decoderObj->prevFrameQuantEn, NULL, decoderObj->gains );
                        if(decoderObj->BFIcount < 2) {
                            decoderObj->gains[0] = (statStat)? BWF_HARMONIC : PITCH_GAIN_MAX;
                            decoderObj->gains[1] = oldCodebookGain;
                        } else {
                            if(statStat) {
                                if(decoderObj->BFIcount > 10) decoderObj->valGainAttenuation = ( decoderObj->valGainAttenuation * 32604)>>15;
                            } else decoderObj->valGainAttenuation = ( decoderObj->valGainAttenuation * 32112)>>15;
                        }
                    } 
                }
            }

            /* update pitch sharpening  with quantized gain pitch */
            decoderObj->betaPreFilter = decoderObj->gains[0]; 
            if(decoderObj->betaPreFilter > PITCH_SHARP_MAX)
                decoderObj->betaPreFilter = PITCH_SHARP_MAX;
            if(decoderObj->betaPreFilter < PITCH_SHARP_MIN)
                decoderObj->betaPreFilter = PITCH_SHARP_MIN;

            /* synthesis of speech corresponding to excitation*/
            if(badFrameIndicator) { 
                decoderObj->BFIcount++;
                if(voiceFlag == 0 ) {
                    gC = decoderObj->gains[1];
                    gPl = 0;
                } else {
                    gC = 0;
                    gPl = decoderObj->gains[0];
                }   
            } else {
                gC = decoderObj->gains[1];
                gPl = decoderObj->gains[0];
            }  
            ippsInterpolateC_NR_G729_16s_Sfs(&excitation[subfrIdx],gPl,ACELPcodeVec,gC,&excitation[subfrIdx],LP_SUBFRAME_DIM,14);
            if(decoderObj->codecType == G729D_CODEC)
                status = SynthesisFilter_G729_16s_update(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,BWLPCF_DIM-mAq,0);
            else
                status = SynthesisFilter_G729_16s(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,BWLPCF_DIM-mAq);
            if(status == ippStsOverflow) {

                ippsRShiftC_16s_I(2,prevExcitat,L_prevExcitat+LP_FRAME_DIM);
                SynthesisFilterOvf_G729_16s(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,BWLPCF_DIM-mAq);
            }
            if(decoderObj->codecType == G729D_CODEC) {
                ippsPhaseDispersion_G729D_16s(&excitation[subfrIdx], phaseDispExcit, decoderObj->gains[1], 
                                              decoderObj->gains[0], ACELPcodeVec, (IppsPhaseDispersion_State_G729D*)decoderObj->PhDispMem);
                SynthesisFilter_G729_16s(ppAz,phaseDispExcit,&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,BWLPCF_DIM-mAq);
            } else {
                ippsPhaseDispersionUpdate_G729D_16s(decoderObj->gains[0], decoderObj->gains[1],
                                                    (IppsPhaseDispersion_State_G729D*)decoderObj->PhDispMem);
            } 
            ppAz += mAq+1;              
        } 
    }  

    if(badFrameIndicator == 0) {
        ippsDotProd_16s32s_Sfs(excitation,excitation,LP_FRAME_DIM,&i,-1);
        decoderObj->SIDflag1 = Norm_32s16s(i);
        decoderObj->SIDflag0 = ((i << decoderObj->SIDflag1)+0x8000)>>16;
        decoderObj->SIDflag1 = 16 - decoderObj->SIDflag1;
    }
    decoderObj->CNGvar = fType;

    if(enerDB(synth, LP_FRAME_DIM) >= BWF_HARMONIC_E) tstDominantBWDmode(&decoderObj->BWDcounter2,&decoderObj->FWDcounter2,&dominantBWDmode, LPmode);

    ippsCopy_16s(&prevExcitat[LP_FRAME_DIM], &prevExcitat[0], L_prevExcitat);

    if( LPmode == 0) {
        ippsCopy_16s(FWDfiltLPC, AzDec, 2*LPF_DIM+2);
        prevM = LPF_DIM;
    } else {
        ippsCopy_16s(BWDfiltLPC, AzDec, 2*BWLPCF1_DIM);
        prevM = BWLPCF_DIM;
    } 

    decoderObj->prevBFI = badFrameIndicator;
    decoderObj->prevLPmode = LPmode;
    decoderObj->prevVoiceFlag = voiceFlag;

    if(badFrameIndicator != 0)
        decoderObj->interpCoeff2 = (1<<12);
    else {
        if(LPmode == 0)
            decoderObj->interpCoeff2 = 0;
        else {
            if(dominantBWDmode == 1)
                decoderObj->interpCoeff2 -= 410;
            else decoderObj->interpCoeff2 -= 2048;
                if(decoderObj->interpCoeff2 < 0)
                    decoderObj->interpCoeff2= 0;
        } 
    } 
    decoderObj->voiceFlag = 0;
    pAz = AzDec;

    if((decoderObj->codecType == G729_CODEC)&&(fType>2)) {
        for(subfrIdx=0; subfrIdx<LP_FRAME_DIM; subfrIdx+=LP_SUBFRAME_DIM) {
            Post_G729(prevFrameDelay1,(short)subfrIdx,pAz,&dst[subfrIdx],&subfrVoiceFlag,decoderObj);
            if(subfrVoiceFlag != 0) {
                decoderObj->voiceFlag = subfrVoiceFlag;
            }
            pAz += LPF_DIM+1;
        }  
    } else {
        parm = decoderObj->decPrm;
        if(fType<4) {
            L_hSt = IMP_RESP_LEN;
            decoderObj->gammaPost1 = BWF1_PST;
            decoderObj->gammaPost2 = BWF2_PST;
            decoderObj->gammaHarm = BWF_HARMONIC;
        } else {
            L_hSt = IMP_RESP_LEN_E;
            /* reduce postfiltering */
            if((parm[2] == 1) && (dominantBWDmode == 1)) {
                decoderObj->gammaHarm -= 410;
                if(decoderObj->gammaHarm < 0)
                    decoderObj->gammaHarm = 0;
                decoderObj->gammaPost1 -= 1147;
                if(decoderObj->gammaPost1 < 0)
                    decoderObj->gammaPost1 = 0;
                decoderObj->gammaPost2 -= 1065;
                if(decoderObj->gammaPost2 < 0)
                    decoderObj->gammaPost2 = 0;
            } else {
                decoderObj->gammaHarm += 410;
                if(decoderObj->gammaHarm > BWF_HARMONIC_E)
                    decoderObj->gammaHarm = BWF_HARMONIC_E;
                decoderObj->gammaPost1 += 1147;
                if(decoderObj->gammaPost1 > BWF1_PST_E)
                    decoderObj->gammaPost1 = BWF1_PST_E;
                decoderObj->gammaPost2 += 1065;
                if(decoderObj->gammaPost2 > BWF2_PST_E)
                    decoderObj->gammaPost2 = BWF2_PST_E;
            }
        }

        for(i=0; i<LP_FRAME_DIM; i+=LP_SUBFRAME_DIM) {
            Post_G729I(prevFrameDelay1, (short)i, pAz, &dst[i],
                       &subfrVoiceFlag, L_hSt, prevM, fType, decoderObj);
            if(subfrVoiceFlag != 0)
                decoderObj->voiceFlag = subfrVoiceFlag;
            pAz += prevM+1;
        } 
    }

    ippsHighPassFilter_G729_16s_ISfs(dst,LP_FRAME_DIM,13,decoderObj->postProc);

    CLEAR_SCRATCH_MEMORY(decoderObj);

    return APIG729_StsNoErr;
}

void Post_G729Base(
                  short delayVal,          /* pitch delayVal given by coder */
                  short subfrIdx, 
                  const short *srcLPC,     /* LPC coefficients for current subframe */
                  short *dstSignal,        /* postfiltered output */
                  short *voiceFlag,        /* voiceFlag decision 0 = uv,  > 0 delayVal */
                  short fType,
                  G729Decoder_Obj *decoderObj
                  ) {
    short bwf1 = decoderObj->gammaPost1;
    short bwf2 = decoderObj->gammaPost2;
    short gamma_harm = decoderObj->gammaHarm;
    LOCAL_ARRAY(int,irACF,2, decoderObj);             
    LOCAL_ALIGN_ARRAY(32, short,y, IMP_RESP_LEN_E, decoderObj);
    LOCAL_ALIGN_ARRAY(32,short, LTPsignal, LP_SUBFRAME_DIM+1, decoderObj);
    LOCAL_ALIGN_ARRAY(32,short, LPCdenom, LPF_DIM+1, decoderObj);  /* denominator srcLPC */
    LOCAL_ALIGN_ARRAY(32,short, LPCnum, IMP_RESP_LEN_E, decoderObj); /* numerator srcLPC */
    short tmp, g0Val, temp, ACFval0, ACFval1;
    short *iirdl = ((SynthesisFilterState*)decoderObj->synFltw0)->buffer;
    int   L_g0Val, normVal;
    const short *signal_ptr = &decoderObj->LTPostFilt[SYNTH_BWD_DIM+subfrIdx];

    ippsZero_16s(LPCnum, IMP_RESP_LEN_E);

    ippsMulPowerC_NR_16s_Sfs(srcLPC,bwf1, LPCdenom,LPF_DIM+1,15);
    ippsMulPowerC_NR_16s_Sfs(srcLPC,bwf2, LPCnum,LPF_DIM+1,15);

    ippsResidualFilter_G729_16s((short *)signal_ptr, LPCnum, &decoderObj->resFilBuf1[RES_FIL_DIM]);

    if(fType > 1)
        ippsLongTermPostFilter_G729_16s(gamma_harm,delayVal, &decoderObj->resFilBuf1[RES_FIL_DIM],
                                        LTPsignal + 1, voiceFlag);
    else {
        *voiceFlag = 0;
        ippsCopy_16s(&decoderObj->resFilBuf1[RES_FIL_DIM], LTPsignal + 1, LP_SUBFRAME_DIM);
    }
    LTPsignal[0] = decoderObj->preemphFilt;
    ippsSynthesisFilter_NR_16s_Sfs(LPCdenom, LPCnum,y,IMP_RESP_LEN, 12, &decoderObj->zeroPostFiltVec1[LPF_DIM+1]);

    ippsAutoCorr_NormE_16s32s(y,IMP_RESP_LEN,irACF,2,&normVal);
    ACFval0 = irACF[0]>>16;
    ACFval1 = irACF[1]>>16;
    if( ACFval0 < Abs_16s(ACFval1)) {
        tmp = 0;
    } else {
        tmp = (Abs_16s(ACFval1)<<15)/ACFval0;
        if(ACFval1 > 0) {
            tmp = -tmp;
        }
    }

    ippsAbs_16s_I(y,IMP_RESP_LEN);
    ippsSum_16s32s_Sfs(y,IMP_RESP_LEN,&L_g0Val,0);
    g0Val = ShiftL_32s(L_g0Val, 14)>>16;

    if(g0Val > 1024) {
        temp = (1024<<15)/g0Val;     
        ippsMulC_NR_16s_ISfs(temp,LTPsignal + 1,LP_SUBFRAME_DIM,15);
    }
    ippsSynthesisFilter_NR_16s_ISfs(LPCdenom, LTPsignal + 1, LP_SUBFRAME_DIM, 12, &iirdl[BWLPCF_DIM-LPF_DIM]);
    decoderObj->preemphFilt = LTPsignal[LP_SUBFRAME_DIM];
    ippsCopy_16s(&LTPsignal[LP_SUBFRAME_DIM-LPF_DIM+1], &iirdl[BWLPCF_DIM-LPF_DIM], LPF_DIM );
    ippsTiltCompensation_G729E_16s(tmp,LTPsignal, dstSignal);
    ippsGainControl_G729_16s_I(signal_ptr, dstSignal, &decoderObj->gainExact);
    ippsCopy_16s(&decoderObj->resFilBuf1[LP_SUBFRAME_DIM], &decoderObj->resFilBuf1[0], RES_FIL_DIM);

    LOCAL_ALIGN_ARRAY_FREE(32,short, LPCnum, IMP_RESP_LEN_E, decoderObj); /* numerator srcLPC  */
    LOCAL_ALIGN_ARRAY_FREE(32,short, LPCdenom, LPF_DIM+1, decoderObj);  /* denominator srcLPC  */
    LOCAL_ALIGN_ARRAY_FREE(32, short, LTPsignal,LP_SUBFRAME_DIM+1,decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short,y, IMP_RESP_LEN_E,decoderObj);
    LOCAL_ARRAY_FREE(int, irACF, 2,decoderObj);
    return;
}

void Post_G729( short delayVal, short subfrIdx, const short *srcLPC, short *dstSignal, 
              short *voiceFlag, G729Decoder_Obj *decoderObj) {
    short *gainExact = &decoderObj->gainExact;
    short *iirdl = ((SynthesisFilterState*)decoderObj->synFltw0)->buffer;
    LOCAL_ALIGN_ARRAY(32, short,y, IMP_RESP_LEN, decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, LTPsignalBuf, LP_SUBFRAME_DIM+1+LPF_DIM, decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, LPCnum, LPF_DIM+1, decoderObj);
    short tmp;
    const short *res = &decoderObj->LTPostFilt[SYNTH_BWD_DIM+subfrIdx];
    short *LTPsignal = LTPsignalBuf+LPF_DIM+1; 

    ippsMul_NR_16s_Sfs(g729gammaFac2_pst,srcLPC, LPCnum,LPF_DIM+1,15);
    ippsResidualFilter_G729_16s(res, LPCnum, decoderObj->resFilBuf1 + RES_FIL_DIM);
    ippsLongTermPostFilter_G729_16s(BWF_HARMONIC,delayVal, decoderObj->resFilBuf1 + RES_FIL_DIM, LTPsignal+1, &tmp);
    *voiceFlag = (tmp != 0);

    ippsCopy_16s(&decoderObj->resFilBuf1[LP_SUBFRAME_DIM], &decoderObj->resFilBuf1[0], RES_FIL_DIM);
    ippsCopy_16s(iirdl+20,LTPsignal+1-LPF_DIM, LPF_DIM );
    ippsShortTermPostFilter_G729_16s(srcLPC, LTPsignal+1,LTPsignal+1,y);
    ippsCopy_16s((LTPsignal+1+LP_SUBFRAME_DIM-BWLPCF_DIM), iirdl, BWLPCF_DIM);

    LTPsignal[0] = decoderObj->preemphFilt;
    decoderObj->preemphFilt = LTPsignal[LP_SUBFRAME_DIM];

    ippsTiltCompensation_G729_16s(y, LTPsignal+1);  
    ippsCopy_16s(LTPsignal+1, dstSignal,LP_SUBFRAME_DIM);
    ippsGainControl_G729_16s_I(res, dstSignal, gainExact);

    LOCAL_ALIGN_ARRAY_FREE(32, short, LPCnum, LPF_DIM+1,decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short, LTPsignalBuf,LP_SUBFRAME_DIM+1+LPF_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short,y, IMP_RESP_LEN,decoderObj);

    return;
}

void Post_G729AB(short delayVal, short subfrIdx, const short *srcLPC, short *syn_pst,
                short ftype, G729Decoder_Obj *decoderObj) {
    short *iirdl = ((SynthesisFilterState*)decoderObj->synFltw0)->buffer;
    short *preemphFilt = &decoderObj->preemphFilt;
    LOCAL_ALIGN_ARRAY(32, short,sndLPC,2*(LPF_DIM+1), decoderObj);        
    LOCAL_ALIGN_ARRAY(32, short, prevResidual, LP_SUBFRAME_DIM+8, decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, prevBuf, LP_SUBFRAME_DIM+LPF_DIM, decoderObj);
    short *prevResidual2 = prevResidual+8;
    short *pst = prevBuf+LPF_DIM;  

    ippsMul_NR_16s_Sfs(g729gammaFac2_pst,srcLPC, decoderObj->zeroPostFiltVec1, LPF_DIM+1,15);
    ippsMul_NR_16s_Sfs(g729gammaFac2_pst,srcLPC, sndLPC, LPF_DIM+1,15);
    ippsMul_NR_16s_Sfs(g729gammaFac1_pst,srcLPC, sndLPC+LPF_DIM+1,LPF_DIM+1,15);

    ippsLongTermPostFilter_G729A_16s(delayVal,&decoderObj->LTPostFilt[LPF_DIM+subfrIdx],
                                     sndLPC,decoderObj->resFilBuf1-LPF_DIM-1,prevResidual2);
    ippsCopy_16s(&decoderObj->resFilBuf1[LP_SUBFRAME_DIM], &decoderObj->resFilBuf1[0], MAX_PITCH_LAG);
    if(3 != ftype)
        ippsCopy_16s(decoderObj->resFilBuf1 + MAX_PITCH_LAG,prevResidual2,LP_SUBFRAME_DIM);
    prevResidual2[-1] = *preemphFilt;
    *preemphFilt = prevResidual2[LP_SUBFRAME_DIM-1];

    ippsTiltCompensation_G729A_16s(sndLPC,prevResidual2);

    ippsCopy_16s(iirdl,pst-LPF_DIM, LPF_DIM );
    ippsShortTermPostFilter_G729A_16s(sndLPC+LPF_DIM+1, prevResidual2,pst);
    ippsCopy_16s((pst+LP_SUBFRAME_DIM-LPF_DIM), iirdl, LPF_DIM );
    ippsCopy_16s(pst,syn_pst,LP_SUBFRAME_DIM);

    ippsGainControl_G729A_16s_I(&decoderObj->LTPostFilt[LPF_DIM+subfrIdx], syn_pst, &decoderObj->gainExact);

    LOCAL_ALIGN_ARRAY_FREE(32, short, prevBuf,LP_SUBFRAME_DIM+LPF_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short, prevResidual, LP_SUBFRAME_DIM+8,decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short, sndLPC, 2*(LPF_DIM+1),decoderObj);
}

void Post_G729I( short delayVal, short subfrIdx, const short *srcLPC, short *dstSignal,
               short *voiceFlag, short  L_hSt, short prevM, short fType,
               G729Decoder_Obj *decoderObj){
    short bwf1 = decoderObj->gammaPost1;
    short bwf2 = decoderObj->gammaPost2;
    short gamma_harm = decoderObj->gammaHarm;
    LOCAL_ARRAY(int,irACF,2, decoderObj);             
    LOCAL_ALIGN_ARRAY(32, short,y, IMP_RESP_LEN_E, decoderObj);
    LOCAL_ALIGN_ARRAY(32,short, LTPsignal, LP_SUBFRAME_DIM+1, decoderObj);
    LOCAL_ALIGN_ARRAY(32,short, LPCdenom, BWLPCF1_DIM, decoderObj);
    LOCAL_ALIGN_ARRAY(32,short, LPCnum, IMP_RESP_LEN_E, decoderObj);
    short  tmp, g0Val, temp, ACFval0, ACFval1;
    short *iirdl = ((SynthesisFilterState*)decoderObj->synFltw0)->buffer;
    int    L_g0Val, normVal;
    const short *signal_ptr = &decoderObj->LTPostFilt[SYNTH_BWD_DIM+subfrIdx];

    ippsZero_16s(LPCnum, IMP_RESP_LEN_E);

    ippsMulPowerC_NR_16s_Sfs(srcLPC,bwf1, LPCdenom,prevM+1,15);
    ippsMulPowerC_NR_16s_Sfs(srcLPC,bwf2, LPCnum,prevM+1,15);
    ippsResidualFilter_G729E_16s(LPCnum, prevM,(short *)signal_ptr, &decoderObj->resFilBuf1[RES_FIL_DIM], LP_SUBFRAME_DIM);
    if(fType > 1)
        ippsLongTermPostFilter_G729_16s(gamma_harm,delayVal, &decoderObj->resFilBuf1[RES_FIL_DIM],
                                        LTPsignal + 1, voiceFlag);
    else {
        *voiceFlag = 0;
        ippsCopy_16s(&decoderObj->resFilBuf1[RES_FIL_DIM], LTPsignal + 1, LP_SUBFRAME_DIM);
    }

    LTPsignal[0] = decoderObj->preemphFilt;
    ippsSynthesisFilter_G729E_16s(LPCdenom, prevM,LPCnum,y, L_hSt, &decoderObj->zeroPostFiltVec1[LPF_DIM+1]);
    ippsAutoCorr_NormE_16s32s(y,L_hSt,irACF,2,&normVal);
    ACFval0   = irACF[0]>>16;
    ACFval1  = irACF[1]>>16;
    if( ACFval0 < Abs_16s(ACFval1)) {
        tmp = 0;
    } else {
        tmp = (Abs_16s(ACFval1)<<15)/ACFval0;
        if(ACFval1 > 0) {
            tmp = -tmp;
        }
    }
    ippsAbs_16s_I(y,L_hSt);
    ippsSum_16s32s_Sfs(y,L_hSt,&L_g0Val,0);
    g0Val = ShiftL_32s(L_g0Val, 14)>>16;
    if(g0Val > 1024) {
        temp = (1024<<15)/g0Val;     
        ippsMulC_NR_16s_ISfs(temp,LTPsignal + 1,LP_SUBFRAME_DIM,15);
    }
    ippsSynthesisFilter_G729E_16s_I(LPCdenom,prevM,LTPsignal + 1, LP_SUBFRAME_DIM,&iirdl[BWLPCF_DIM-prevM]);
    decoderObj->preemphFilt = LTPsignal[LP_SUBFRAME_DIM];
    ippsCopy_16s(&LTPsignal[LP_SUBFRAME_DIM-BWLPCF_DIM+1], iirdl, BWLPCF_DIM);

    ippsTiltCompensation_G729E_16s(tmp,LTPsignal, dstSignal);

    ippsGainControl_G729_16s_I(signal_ptr, dstSignal, &decoderObj->gainExact);

    ippsCopy_16s(&decoderObj->resFilBuf1[LP_SUBFRAME_DIM], &decoderObj->resFilBuf1[0], RES_FIL_DIM);

    LOCAL_ALIGN_ARRAY_FREE(32,short, LPCnum, IMP_RESP_LEN_E, decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32,short, LPCdenom, BWLPCF1_DIM, decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short, LTPsignal,LP_SUBFRAME_DIM+1,decoderObj);
    LOCAL_ALIGN_ARRAY_FREE(32, short,y, IMP_RESP_LEN_E,decoderObj);
    LOCAL_ARRAY_FREE(int, irACF, 2,decoderObj);
    return;
}

APIG729_Status G729ADecode
(G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst) {

    LOCAL_ALIGN_ARRAY(32, short, AzDec, (LPF_DIM+1)*2,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, newLSP,LPF_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, ACELPcodeVec, LP_SUBFRAME_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, FWDfiltLPC, 2*LPF_DIM+2,decoderObj);
    LOCAL_ARRAY(short, prevFrameDelay,2,decoderObj);
    LOCAL_ARRAY(short, idx,4,decoderObj);
    LOCAL_ARRAY(short, delayVal,2,decoderObj);
    short *pAz, *pA, *ppAz, temp; 
    char  *synFltw = decoderObj->synFltw;
    short *prevExcitat = decoderObj->prevExcitat;
    short *excitation = prevExcitat + L_prevExcitat;
    short *synth = decoderObj->LTPostFilt+LPF_DIM;
    short *prevSubfrLSP = decoderObj->prevSubfrLSP;
    const unsigned char *pParm;
    const short  *parm;
    short sidGain = decoderObj->sidGain;
    short gainNow = decoderObj->gainNow;
    short *lspSID = decoderObj->lspSID;
    int   i, j, subfrIdx, index2, fType;
    short badFrameIndicator, badPitch, index, pulseSign;

    if(NULL==decoderObj || NULL==src || NULL ==dst)
        return APIG729_StsBadArgErr;
    if(decoderObj->objPrm.objSize <= 0)
        return APIG729_StsNotInitialized;
    if(DEC_KEY != decoderObj->objPrm.key)
        return APIG729_StsBadCodecType;

    delayVal[0]=delayVal[1]=0;
    pA = AzDec;
    pParm = src;

    if(frametype == -1) {  
        decoderObj->decPrm[1] = 0; 
        decoderObj->decPrm[0] = 1;  
    } else if(frametype == 0) { 
        decoderObj->decPrm[1] = 0; 
        decoderObj->decPrm[0] = 0; 
    } else if(frametype == 1) {  
        decoderObj->decPrm[1] = 1; 
        decoderObj->decPrm[0] = 0; 
        i=0;
        decoderObj->decPrm[1+1] = ExtractBitsG729(&pParm,&i,1);
        decoderObj->decPrm[1+2] = ExtractBitsG729(&pParm,&i,5);
        decoderObj->decPrm[1+3] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+4] = ExtractBitsG729(&pParm,&i,5);
    } else if(frametype == 3) { 
        i=0;
        decoderObj->decPrm[1] = 3;  
        decoderObj->decPrm[0] = 0;  
        decoderObj->decPrm[1+1] = ExtractBitsG729(&pParm,&i,1+FIR_STAGE_BITS);
        decoderObj->decPrm[1+2] = ExtractBitsG729(&pParm,&i,SEC_STAGE_BITS*2);
        decoderObj->decPrm[1+3] = ExtractBitsG729(&pParm,&i,8);
        decoderObj->decPrm[1+4] = ExtractBitsG729(&pParm,&i,1);
        decoderObj->decPrm[1+5] = ExtractBitsG729(&pParm,&i,13);
        decoderObj->decPrm[1+6] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+7] = ExtractBitsG729(&pParm,&i,7);
        decoderObj->decPrm[1+8] = ExtractBitsG729(&pParm,&i,5);
        decoderObj->decPrm[1+9] = ExtractBitsG729(&pParm,&i,13);
        decoderObj->decPrm[1+10] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+11] = ExtractBitsG729(&pParm,&i,7);
        decoderObj->decPrm[1+4] = (equality(decoderObj->decPrm[1+3])+decoderObj->decPrm[1+4]) & 0x00000001; /*  parity error (1) */
    }

    parm = decoderObj->decPrm;

    badFrameIndicator = *parm++;
    fType = *parm;
    if(badFrameIndicator == 1) {
        fType = decoderObj->CNGvar;
        if(fType == 1) fType = 0;
    }

    ippsCopy_16s(&decoderObj->LTPostFilt[LP_FRAME_DIM], &decoderObj->LTPostFilt[0], LPF_DIM );

    if(fType < 2) { 
        if(fType == 1) { 
            LOCAL_ALIGN_ARRAY(32, short, lsfq,LPF_DIM,decoderObj);
            sidGain = SIDgain[(int)parm[4]];           
            ippsLSFDecode_G729B_16s(&parm[1],(Ipp16s*)(decoderObj->prevLSPfreq),lsfq);
            ippsLSFToLSP_G729_16s(lsfq,lspSID);
            LOCAL_ALIGN_ARRAY_FREE(32, short, lsfq,LPF_DIM,decoderObj);
        } else { 
            if(decoderObj->CNGvar > 1) {
                QuantSIDGain_G729B_16s(&decoderObj->SIDflag0, &decoderObj->SIDflag1, 0, &temp, &index2);
                sidGain = SIDgain[(int)index2];
            }
        } 
        if(decoderObj->CNGvar > 1) {
            gainNow = sidGain;  
        } else {
            gainNow = (gainNow * GAIN0 + BWF_HARMONIC)>>15;
            gainNow = Add_16s(gainNow, (short)((sidGain * GAIN1 + BWF_HARMONIC)>>15));
        } 

        if(gainNow == 0) ippsZero_16s(excitation,LP_FRAME_DIM);
        else {
            for(i = 0;  i < LP_FRAME_DIM; i += LP_SUBFRAME_DIM) {
                int invSq;
                Ipp16s pG2;
                Ipp16s g;
                const short *excCached;
                LOCAL_ARRAY(Ipp16s, idx, 4, decoderObj);
                LOCAL_ARRAY(Ipp16s, pulseSign, 4, decoderObj);
                LOCAL_ALIGN_ARRAY(32, Ipp16s, excg, LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ARRAY(Ipp16s,tempArray,LP_SUBFRAME_DIM, decoderObj);

                RandomCodebookParm_G729B_16s(&decoderObj->seed,idx,pulseSign,&pG2,delayVal);
                ippsDecodeAdaptiveVector_G729_16s_I(delayVal,&prevExcitat[i]);
                if(decoderObj->CNGidx > CNG_STACK_SIZE-1) { /* not cached */
                    ippsRandomNoiseExcitation_G729B_16s(&decoderObj->seed,excg,LP_SUBFRAME_DIM);
                    ippsDotProd_16s32s_Sfs(excg,excg,LP_SUBFRAME_DIM,&invSq,0);
                    ippsInvSqrt_32s_I(&invSq,1);  /* Q30 */
                    excCached=excg;
                } else {
                    decoderObj->seed = cngSeedOut[decoderObj->CNGidx];
                    invSq = cngInvSqrt[decoderObj->CNGidx];
                    excCached=&cngCache[decoderObj->CNGidx][0];
                    decoderObj->CNGidx++;
                } 
                NoiseExcitationFactorization_G729B_16s(excCached,invSq,gainNow,excg,LP_SUBFRAME_DIM);
                ComfortNoiseExcitation_G729B_16s_I(excg,idx,pulseSign,gainNow,pG2,&excitation[i],&g,tempArray);

                LOCAL_ARRAY_FREE(Ipp16s,tempArray,LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ALIGN_ARRAY_FREE(32, Ipp16s, excg, LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ARRAY_FREE(Ipp16s, pulseSign, 4, decoderObj);
                LOCAL_ARRAY_FREE(Ipp16s, idx, 4, decoderObj);
            } 
        } 
        ippsInterpolate_G729_16s(prevSubfrLSP,lspSID,prevSubfrLSP, LPF_DIM );
        ippsLSPToLPC_G729_16s(prevSubfrLSP,&FWDfiltLPC[0]);
        ippsLSPToLPC_G729_16s(lspSID,&FWDfiltLPC[LPF_DIM+1]);
        ippsCopy_16s(lspSID, prevSubfrLSP, LPF_DIM );
        decoderObj->sidGain = sidGain;
        decoderObj->gainNow = gainNow;

        ppAz = FWDfiltLPC;
        for(subfrIdx = 0; subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM) {
            if(ippsSynthesisFilter_NR_16s_Sfs(ppAz,&excitation[subfrIdx],&synth[subfrIdx], LP_SUBFRAME_DIM, 12, 
                                              ((SynthesisFilterState*)synFltw)->buffer)==ippStsOverflow) {
                ippsRShiftC_16s_I(2,prevExcitat,L_prevExcitat+LP_FRAME_DIM);
                ippsSynthesisFilter_NR_16s_Sfs(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,12,
                                               ((SynthesisFilterState*)synFltw)->buffer);
            }
            ippsCopy_16s((&synth[subfrIdx]+LP_SUBFRAME_DIM-LPF_DIM), ((SynthesisFilterState*)synFltw)->buffer, LPF_DIM );

            ppAz += LPF_DIM+1;
            prevFrameDelay[subfrIdx/LP_SUBFRAME_DIM] = decoderObj->prevFrameDelay;
        } 
        decoderObj->betaPreFilter = PITCH_SHARP_MIN;

    } else {
        LOCAL_ARRAY(short, qIndex,4,decoderObj);
        decoderObj->seed = SEED_INIT;
        decoderObj->CNGidx = 0;
        parm++;

        qIndex[0] = (parm[0] >> FIR_STAGE_BITS) & 1;
        qIndex[1] = (Ipp16s)(parm[0] & (FIR_STAGE - 1));
        qIndex[2] = (Ipp16s)((parm[1] >> SEC_STAGE_BITS) & (SEC_STAGE - 1));
        qIndex[3] = (Ipp16s)(parm[1] & (SEC_STAGE - 1));
        if(!badFrameIndicator) {
            decoderObj->prevMA = qIndex[0];
            ippsLSFDecode_G729_16s( qIndex, (short*)decoderObj->prevLSPfreq, decoderObj->prevSubfrLSPquant);
        } else {
            ippsLSFDecodeErased_G729_16s( decoderObj->prevMA, 
                                          (short*)decoderObj->prevLSPfreq, decoderObj->prevSubfrLSPquant);
        } 

        ippsLSFToLSP_G729_16s(decoderObj->prevSubfrLSPquant, newLSP); /* Convert LSFs to LSPs */
        parm += 2;
        LOCAL_ARRAY_FREE(short, qIndex,4,decoderObj);
        ippsInterpolate_G729_16s(newLSP,prevSubfrLSP,prevSubfrLSP, LPF_DIM );

        ippsLSPToLPC_G729_16s(prevSubfrLSP, FWDfiltLPC);          /* 1-st subframe */

        ippsLSPToLPC_G729_16s(newLSP, &FWDfiltLPC[LPF_DIM+1]);    /*  2-nd subframe*/

        ippsCopy_16s(newLSP, prevSubfrLSP, LPF_DIM );
        pA = FWDfiltLPC; ppAz = pA;

        for(subfrIdx=0; subfrIdx < LP_FRAME_DIM; subfrIdx+=LP_SUBFRAME_DIM) {
            int pitchIndx;
            badPitch = badFrameIndicator;
            pitchIndx = *parm++;
            if(subfrIdx == 0) {
                i = *parm++;
                badPitch = badFrameIndicator + i;
            }
            DecodeAdaptCodebookDelays(&decoderObj->prevFrameDelay,&decoderObj->prevFrameDelay2,delayVal,subfrIdx,badPitch,pitchIndx,decoderObj->codecType);
            prevFrameDelay[subfrIdx/LP_SUBFRAME_DIM] = delayVal[0];
            ippsDecodeAdaptiveVector_G729_16s_I(delayVal,&prevExcitat[subfrIdx]);

            if(badFrameIndicator != 0) {
                index = Rand_16s(&decoderObj->seedSavage) & (short)0x1fff;    
                pulseSign = Rand_16s(&decoderObj->seedSavage) & (short)15;     
            } else {
                index = parm[0];
                pulseSign = parm[1];
            } 

            i      = index & 7;
            idx[0] = 5 * i; 

            index  = index >> 3;
            i      = index & 7;
            idx[1] = 5 * i + 1; 

            index  = index >> 3;
            i      = index & 7;
            idx[2] = 5 * i + 2; 

            index  = index >> 3;
            j      = index & 1;
            index  = index >> 1;
            i      = index & 7;   
            idx[3] = i * 5 + 3 + j;

            ippsZero_16s(ACELPcodeVec,LP_SUBFRAME_DIM);

            for(j=0; j<4; j++) {
                if((pulseSign & 1) != 0) {
                    ACELPcodeVec[idx[j]] = 8191;
                } else {
                    ACELPcodeVec[idx[j]] = -BWF_HARMONIC_E;
                }  
                pulseSign = pulseSign >> 1; 
            }
            parm += 2;

            decoderObj->betaPreFilter = decoderObj->betaPreFilter << 1;        
            if(delayVal[0] < LP_SUBFRAME_DIM) {
                ippsHarmonicFilter_16s_I(decoderObj->betaPreFilter,delayVal[0],&ACELPcodeVec[delayVal[0]],LP_SUBFRAME_DIM-delayVal[0]); 
            }

            pitchIndx = *parm++; 
            if(!badFrameIndicator) {
                LOCAL_ARRAY(short, gIngx, 2, decoderObj);
                ippsDotProd_16s32s_Sfs(ACELPcodeVec, ACELPcodeVec, LP_SUBFRAME_DIM, &i, 0);
                gIngx[0] = (short)(pitchIndx >> CDBK2_BIT_NUM) ;
                gIngx[1] = (short)(pitchIndx & (CDBK2_DIM-1));
                ippsDecodeGain_G729_16s(i, decoderObj->prevFrameQuantEn, gIngx, decoderObj->gains);
                LOCAL_ARRAY_FREE(short, gIngx, 2, decoderObj);
            } else {
                ippsDecodeGain_G729_16s(0, decoderObj->prevFrameQuantEn, NULL, decoderObj->gains);
            }  
            decoderObj->betaPreFilter = decoderObj->gains[0]; 
            if(decoderObj->betaPreFilter > PITCH_SHARP_MAX) decoderObj->betaPreFilter = PITCH_SHARP_MAX;
            if(decoderObj->betaPreFilter < PITCH_SHARP_MIN) decoderObj->betaPreFilter = PITCH_SHARP_MIN;
            ippsInterpolateC_NR_G729_16s_Sfs(
                                            &excitation[subfrIdx],decoderObj->gains[0],ACELPcodeVec,decoderObj->gains[1],&excitation[subfrIdx],LP_SUBFRAME_DIM,14);
            if(ippsSynthesisFilter_NR_16s_Sfs(ppAz,&excitation[subfrIdx],&synth[subfrIdx], LP_SUBFRAME_DIM, 12, 
                                              ((SynthesisFilterState*)synFltw)->buffer)==ippStsOverflow) {

                ippsRShiftC_16s_I(2,prevExcitat,L_prevExcitat+LP_FRAME_DIM);
                ippsSynthesisFilter_NR_16s_Sfs(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,12,
                                               ((SynthesisFilterState*)synFltw)->buffer);
            }
            ippsCopy_16s((&synth[subfrIdx]+LP_SUBFRAME_DIM-LPF_DIM), ((SynthesisFilterState*)synFltw)->buffer, LPF_DIM );
            ppAz += LPF_DIM+1;
        } 
    }  

    if(badFrameIndicator == 0) {
        ippsDotProd_16s32s_Sfs(excitation,excitation,LP_FRAME_DIM,&i,-1);
        decoderObj->SIDflag1 = Norm_32s16s(i);
        decoderObj->SIDflag0 = ((i << decoderObj->SIDflag1)+0x8000)>>16;
        decoderObj->SIDflag1 = 16 - decoderObj->SIDflag1;
    }
    decoderObj->CNGvar = fType;
    ippsCopy_16s(&prevExcitat[LP_FRAME_DIM], &prevExcitat[0], L_prevExcitat);
    pAz = FWDfiltLPC;
    for(i=0, subfrIdx = 0; subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM,i++) {
        Post_G729AB(prevFrameDelay[i],(short)subfrIdx,pAz,&dst[subfrIdx],fType,decoderObj); 
        pAz += LPF_DIM+1;
    }  
    ippsHighPassFilter_G729_16s_ISfs(dst,LP_FRAME_DIM,13,decoderObj->postProc);
    CLEAR_SCRATCH_MEMORY(decoderObj);
    return APIG729_StsNoErr;
}

APIG729_Status G729BaseDecode
(G729Decoder_Obj* decoderObj,const unsigned char* src, int frametype, short* dst) {
    LOCAL_ALIGN_ARRAY(32, short, AzDec, (LPF_DIM+1)*2,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, newLSP,LPF_DIM,decoderObj);
    LOCAL_ALIGN_ARRAY(32, short, ACELPcodeVec, LP_SUBFRAME_DIM,decoderObj);
    LOCAL_ARRAY(short, prevFrameDelay,2,decoderObj);                       
    LOCAL_ARRAY(short, idx,4,decoderObj);
    LOCAL_ARRAY(short, delayVal,2,decoderObj);
    short prevFrameDelay1=0, subfrVoiceFlag, *ppAz; 
    char *synFltw = decoderObj->synFltw;
    short *prevExcitat = decoderObj->prevExcitat;
    short *excitation = prevExcitat + L_prevExcitat;
    short *synth = decoderObj->LTPostFilt+SYNTH_BWD_DIM;
    short *prevSubfrLSP = decoderObj->prevSubfrLSP;
    const unsigned char *pParm;
    const short  *parm;
    short voiceFlag = decoderObj->voiceFlag;
    short sidGain = decoderObj->sidGain;
    short gainNow = decoderObj->gainNow;
    short *lspSID = decoderObj->lspSID;
    short temp, badFrameIndicator, badPitch, index, pulseSign, gPl, gC, fType;
    int   i, j, subfrIdx, index2;
    IppStatus status;

    if(NULL==decoderObj || NULL==src || NULL ==dst)
        return APIG729_StsBadArgErr;
    if(decoderObj->objPrm.objSize <= 0)
        return APIG729_StsNotInitialized;
    if(DEC_KEY != decoderObj->objPrm.key)
        return APIG729_StsBadCodecType;

    delayVal[0]=delayVal[1]=0;   
    pParm = src;

    if(frametype == -1) { 
        decoderObj->decPrm[1] = 0; 
        decoderObj->decPrm[0] = 1;  
    } else if(frametype == 0) { 
        decoderObj->decPrm[1] = 0;  
        decoderObj->decPrm[0] = 0; 

    } else if(frametype == 1) {  
        decoderObj->decPrm[1] = 1;  
        decoderObj->decPrm[0] = 0; 
        i=0;
        decoderObj->decPrm[1+1] = ExtractBitsG729(&pParm,&i,1);
        decoderObj->decPrm[1+2] = ExtractBitsG729(&pParm,&i,5);
        decoderObj->decPrm[1+3] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+4] = ExtractBitsG729(&pParm,&i,5);
    } else if(frametype == 3) { 
        i=0;
        decoderObj->decPrm[1] = 3;  
        decoderObj->decPrm[0] = 0; 
        decoderObj->decPrm[1+1] = ExtractBitsG729(&pParm,&i,1+FIR_STAGE_BITS);
        decoderObj->decPrm[1+2] = ExtractBitsG729(&pParm,&i,SEC_STAGE_BITS*2);
        decoderObj->decPrm[1+3] = ExtractBitsG729(&pParm,&i,8);
        decoderObj->decPrm[1+4] = ExtractBitsG729(&pParm,&i,1);
        decoderObj->decPrm[1+5] = ExtractBitsG729(&pParm,&i,13);
        decoderObj->decPrm[1+6] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+7] = ExtractBitsG729(&pParm,&i,7);
        decoderObj->decPrm[1+8] = ExtractBitsG729(&pParm,&i,5);
        decoderObj->decPrm[1+9] = ExtractBitsG729(&pParm,&i,13);
        decoderObj->decPrm[1+10] = ExtractBitsG729(&pParm,&i,4);
        decoderObj->decPrm[1+11] = ExtractBitsG729(&pParm,&i,7);
        decoderObj->decPrm[1+4] = (equality(decoderObj->decPrm[1+3])+decoderObj->decPrm[1+4]) & 0x00000001; /*  parity error (1) */
        decoderObj->codecType = G729_CODEC;
    }
    parm = decoderObj->decPrm;

    badFrameIndicator = *parm++;
    fType = *parm;
    if(badFrameIndicator == 1) {
        fType = decoderObj->CNGvar;
        if(fType == 1)
            fType = 0;
    } else {
        decoderObj->valGainAttenuation = IPP_MAX_16S;
        decoderObj->BFIcount = 0;
    }

    ippsCopy_16s(&decoderObj->LTPostFilt[LP_FRAME_DIM+SYNTH_BWD_DIM-LPF_DIM], &decoderObj->LTPostFilt[SYNTH_BWD_DIM-LPF_DIM], LPF_DIM );

    if(fType < 2) {
        if(fType == 1) {
            LOCAL_ALIGN_ARRAY(32, short, lsfq,LPF_DIM,decoderObj);
            sidGain = SIDgain[(int)parm[4]];           
            ippsLSFDecode_G729B_16s(&parm[1],(Ipp16s*)(decoderObj->prevLSPfreq),lsfq);
            ippsLSFToLSP_G729_16s(lsfq,lspSID);
            LOCAL_ALIGN_ARRAY_FREE(32, short, lsfq,LPF_DIM,decoderObj);
        } else {
            if(decoderObj->CNGvar > 1) {
                QuantSIDGain_G729B_16s(&decoderObj->SIDflag0, &decoderObj->SIDflag1, 0, &temp, &index2);
                sidGain = SIDgain[(int)index2];
            }
        } 
        if(decoderObj->CNGvar > 1) {
            gainNow = sidGain;  
        } else {
            gainNow = (gainNow * GAIN0 + BWF_HARMONIC)>>15;
            gainNow = Add_16s(gainNow, (short)((sidGain * GAIN1 + BWF_HARMONIC)>>15));
        } 

        if(gainNow == 0) {
            ippsZero_16s(excitation,LP_FRAME_DIM);
        } else {
            for(i = 0;  i < LP_FRAME_DIM; i += LP_SUBFRAME_DIM) {
                int invSq;
                Ipp16s pG2, tmp;
                const short *excCached;
                LOCAL_ARRAY(Ipp16s, idx, 4, decoderObj);
                LOCAL_ARRAY(Ipp16s, pulseSign, 4, decoderObj);
                LOCAL_ALIGN_ARRAY(32, Ipp16s, excg, LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ARRAY(Ipp16s,tempArray,LP_SUBFRAME_DIM, decoderObj);
                RandomCodebookParm_G729B_16s(&decoderObj->seed,idx,pulseSign,&pG2,delayVal);
                ippsDecodeAdaptiveVector_G729_16s_I(delayVal,&prevExcitat[i]);
                if(decoderObj->CNGidx > CNG_STACK_SIZE-1) { /* not cached */
                    ippsRandomNoiseExcitation_G729B_16s(&decoderObj->seed,excg,LP_SUBFRAME_DIM);
                    ippsDotProd_16s32s_Sfs(excg,excg,LP_SUBFRAME_DIM,&invSq,0);
                    ippsInvSqrt_32s_I(&invSq,1);
                    excCached=excg;
                } else {
                    decoderObj->seed = cngSeedOut[decoderObj->CNGidx];
                    invSq = cngInvSqrt[decoderObj->CNGidx];
                    excCached=&cngCache[decoderObj->CNGidx][0];
                    decoderObj->CNGidx++;
                } 
                NoiseExcitationFactorization_G729B_16s(excCached,invSq,gainNow,excg,LP_SUBFRAME_DIM);
                ComfortNoiseExcitation_G729B_16s_I(excg,idx,pulseSign,gainNow,pG2,&excitation[i],&tmp,tempArray);

                LOCAL_ARRAY_FREE(Ipp16s,tempArray,LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ALIGN_ARRAY_FREE(32, Ipp16s, excg, LP_SUBFRAME_DIM, decoderObj);
                LOCAL_ARRAY_FREE(Ipp16s, pulseSign, 4, decoderObj);
                LOCAL_ARRAY_FREE(Ipp16s, idx, 4, decoderObj);
            } 
        } 
        ippsInterpolate_G729_16s(prevSubfrLSP,lspSID,prevSubfrLSP, LPF_DIM );
        ippsLSPToLPC_G729_16s(prevSubfrLSP,&AzDec[0]);
        ippsLSPToLPC_G729_16s(lspSID,&AzDec[LPF_DIM+1]);
        ippsCopy_16s(lspSID, prevSubfrLSP, LPF_DIM );
        decoderObj->sidGain = sidGain;
        decoderObj->gainNow = gainNow;

        ppAz = AzDec;
        for(subfrIdx = 0; subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM) {
            if(ippsSynthesisFilter_NR_16s_Sfs(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,12,
                                              ((SynthesisFilterState*)synFltw)->buffer+BWLPCF_DIM-LPF_DIM)==ippStsOverflow) {
                ippsRShiftC_16s_I(2,prevExcitat,L_prevExcitat+LP_FRAME_DIM);
                SynthesisFilterOvf_G729_16s(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,20);
            } else
                ippsCopy_16s((&synth[subfrIdx]+LP_SUBFRAME_DIM-LPF_DIM), ((SynthesisFilterState*)synFltw)->buffer+BWLPCF_DIM-LPF_DIM, LPF_DIM );

            ppAz += LPF_DIM+1;
            prevFrameDelay[subfrIdx/LP_SUBFRAME_DIM] = decoderObj->prevFrameDelay;
        } 
        decoderObj->betaPreFilter = PITCH_SHARP_MIN;
    } else {
        decoderObj->seed = SEED_INIT;
        decoderObj->CNGidx = 0;
        parm++;
        {
            LOCAL_ARRAY(short, qIndex,4,decoderObj);

            qIndex[0] = (parm[0] >> FIR_STAGE_BITS) & 1;
            qIndex[1] = (Ipp16s)(parm[0] & (FIR_STAGE - 1));
            qIndex[2] = (Ipp16s)((parm[1] >> SEC_STAGE_BITS) & (SEC_STAGE - 1));
            qIndex[3] = (Ipp16s)(parm[1] & (SEC_STAGE - 1));
            if(!badFrameIndicator) {
                decoderObj->prevMA = qIndex[0];
                ippsLSFDecode_G729_16s( qIndex, (short*)decoderObj->prevLSPfreq, decoderObj->prevSubfrLSPquant);
            } else {
                ippsLSFDecodeErased_G729_16s( decoderObj->prevMA, 
                                              (short*)decoderObj->prevLSPfreq, decoderObj->prevSubfrLSPquant);
            } 

            ippsLSFToLSP_G729_16s(decoderObj->prevSubfrLSPquant, newLSP); 
            parm += 2;
            LOCAL_ARRAY_FREE(short, qIndex,4,decoderObj);
            ippsInterpolate_G729_16s(newLSP,prevSubfrLSP,prevSubfrLSP, LPF_DIM );
            ippsLSPToLPC_G729_16s(prevSubfrLSP, AzDec);          /* 1-st subframe */
            ippsLSPToLPC_G729_16s(newLSP, &AzDec[LPF_DIM+1]);    /* 2-nd one */
            ippsCopy_16s(newLSP, prevSubfrLSP, LPF_DIM );
            decoderObj->interpCoeff2_2 = 4506;
        } 
        ppAz = AzDec; 

        for(subfrIdx = 0; subfrIdx < LP_FRAME_DIM; subfrIdx += LP_SUBFRAME_DIM) {
            int pitchIndx;
            badPitch = badFrameIndicator;
            pitchIndx = *parm++;  
            if(subfrIdx == 0) {
                i = *parm++;      
                badPitch = badFrameIndicator + i;
            }
            DecodeAdaptCodebookDelays(&decoderObj->prevFrameDelay,&decoderObj->prevFrameDelay2,delayVal,subfrIdx,badPitch,pitchIndx,decoderObj->codecType);
            if(subfrIdx == 0)
                prevFrameDelay1 = delayVal[0];

            ippsDecodeAdaptiveVector_G729_16s_I(delayVal,&prevExcitat[subfrIdx]);

            if(badFrameIndicator != 0) {
                index = Rand_16s(&decoderObj->seedSavage) & (short)0x1fff;
                pulseSign = Rand_16s(&decoderObj->seedSavage) & (short)15;
            } else {
                index = parm[0]; pulseSign = parm[1];
            } 

            i      = index & 7;
            idx[0] = 5 * i; 

            index  = index >> 3;
            i      = index & 7;
            idx[1] = 5 * i + 1; 

            index  = index >> 3;
            i      = index & 7;
            idx[2] = 5 * i + 2; 

            index  = index >> 3;
            j      = index & 1;
            index  = index >> 1;
            i      = index & 7;   
            idx[3] = i * 5 + 3 + j;

            ippsZero_16s(ACELPcodeVec,LP_SUBFRAME_DIM);
            for(j=0; j<4; j++) {
                if((pulseSign & 1) != 0) {
                    ACELPcodeVec[idx[j]] = 8191;     
                } else {
                    ACELPcodeVec[idx[j]] = -BWF_HARMONIC_E;    
                }  
                pulseSign = pulseSign >> 1; 
            }

            parm += 2;
            decoderObj->betaPreFilter = decoderObj->betaPreFilter << 1;
            if(delayVal[0] < LP_SUBFRAME_DIM) {
                ippsHarmonicFilter_16s_I(decoderObj->betaPreFilter,delayVal[0],&ACELPcodeVec[delayVal[0]],LP_SUBFRAME_DIM-delayVal[0]);
            }

            pitchIndx = *parm++;
            if(!badFrameIndicator) {
                LOCAL_ARRAY(short, gIngx, 2, decoderObj);
                ippsDotProd_16s32s_Sfs(ACELPcodeVec, ACELPcodeVec, LP_SUBFRAME_DIM, &i, 0); /* ACELPcodeVec energy */
                gIngx[0] = (short)(pitchIndx >> CDBK2_BIT_NUM) ;
                gIngx[1] = (short)(pitchIndx & (CDBK2_DIM-1));
                ippsDecodeGain_G729_16s(i, decoderObj->prevFrameQuantEn, gIngx, decoderObj->gains);
                LOCAL_ARRAY_FREE(short, gIngx, 2, decoderObj);
            } else {
                ippsDecodeGain_G729_16s(0, decoderObj->prevFrameQuantEn, NULL, decoderObj->gains);
            }  
            /* update the pitch sharpening using the quantized gain pitch */
            decoderObj->betaPreFilter = decoderObj->gains[0]; 
            if(decoderObj->betaPreFilter > PITCH_SHARP_MAX)
                decoderObj->betaPreFilter = PITCH_SHARP_MAX;
            if(decoderObj->betaPreFilter < PITCH_SHARP_MIN)
                decoderObj->betaPreFilter = PITCH_SHARP_MIN;

            if(badFrameIndicator) {
                decoderObj->BFIcount++;
                if(voiceFlag == 0 ) {
                    gC = decoderObj->gains[1];
                    gPl = 0;
                } else {
                    gC = 0;
                    gPl = decoderObj->gains[0];
                }   
            } else {
                gC = decoderObj->gains[1];
                gPl = decoderObj->gains[0];
            }  
            ippsInterpolateC_NR_G729_16s_Sfs(&excitation[subfrIdx],gPl,ACELPcodeVec,gC,&excitation[subfrIdx],LP_SUBFRAME_DIM,14);
            status = ippsSynthesisFilter_NR_16s_Sfs(ppAz,&excitation[subfrIdx],&synth[subfrIdx], LP_SUBFRAME_DIM, 12, 
                                                    ((SynthesisFilterState*)synFltw)->buffer+BWLPCF_DIM-LPF_DIM);
            if(status == ippStsOverflow) {
                ippsRShiftC_16s_I(2,prevExcitat,L_prevExcitat+LP_FRAME_DIM);
                SynthesisFilterOvf_G729_16s(ppAz,&excitation[subfrIdx],&synth[subfrIdx],LP_SUBFRAME_DIM,synFltw,BWLPCF_DIM-LPF_DIM);
            } else
                ippsCopy_16s((&synth[subfrIdx]+LP_SUBFRAME_DIM-LPF_DIM), ((SynthesisFilterState*)synFltw)->buffer+BWLPCF_DIM-LPF_DIM, LPF_DIM );
            ppAz += LPF_DIM+1;              
        } 
    }  
    if(badFrameIndicator == 0) {
        ippsDotProd_16s32s_Sfs(excitation,excitation,LP_FRAME_DIM,&i,-1);
        decoderObj->SIDflag1 = Norm_32s16s(i);
        decoderObj->SIDflag0 = ((i << decoderObj->SIDflag1)+0x8000)>>16;
        decoderObj->SIDflag1 = 16 - decoderObj->SIDflag1;
    }
    decoderObj->CNGvar = fType;
    ippsCopy_16s(&prevExcitat[LP_FRAME_DIM], &prevExcitat[0], L_prevExcitat);
    decoderObj->voiceFlag = 0;
    decoderObj->gammaPost1 = BWF1_PST;
    decoderObj->gammaPost2 = BWF2_PST;
    decoderObj->gammaHarm = BWF_HARMONIC;
    for(subfrIdx=0; subfrIdx<LP_FRAME_DIM; subfrIdx+=LP_SUBFRAME_DIM) {
        Post_G729Base(prevFrameDelay1, (short)subfrIdx, AzDec, &dst[subfrIdx],
                      &subfrVoiceFlag, fType, decoderObj);
        if(subfrVoiceFlag != 0) decoderObj->voiceFlag = subfrVoiceFlag;
        AzDec += LPF_DIM+1;
    } 
    ippsHighPassFilter_G729_16s_ISfs(dst,LP_FRAME_DIM,13,decoderObj->postProc);
    CLEAR_SCRATCH_MEMORY(decoderObj);
    return APIG729_StsNoErr;
}
