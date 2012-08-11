/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives AMRWB Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  AMRWB is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//      Purpose          : Encoder utilities.
//
*/
#include "util_e.h"

static const int bitsLen[NUM_OF_MODES]={
   132, 177, 253, 285, 317, 365, 397, 461, 477, 477, 35
};

#define BIT_0     (short)-127
#define BIT_1     (short)127

static void ownPrm2Serial(short valPrm, short valNumBits, short ** pSerial)
{
    short bit;
    int i;

    *pSerial += valNumBits;
    for (i = 0; i < valNumBits; i++)
    {
        bit = (short) (valPrm & 0x0001);
        if (bit == 0)
            *--(*pSerial) = BIT_0;
        else
            *--(*pSerial) = BIT_1;
        valPrm >>= 1;
    }
    *pSerial += valNumBits;
    return;
}

int Bits2Ref(USC_PCMStream in, USC_Bitstream out, char *out_buff_cur)
{
    int  i, tmp, nbytes;
    short *uprm = (short*)out.pBuffer;
    short *pSerial = (short*)out_buff_cur;
    int usedRate = in.bitrate;
    if (out.frametype != TX_SPEECH) {
        usedRate = AMRWB_RATE_DTX;
    }
    nbytes = (bitsLen[usedRate] + 14) / 15;

    *(pSerial)++ = TX_FRAME_TYPE;
    *(pSerial)++ = out.frametype;
    *(pSerial)++ = in.bitrate;

    for( i = 0; i < nbytes; i++) {
       tmp = uprm[i];
       ownPrm2Serial(tmp, 15, &pSerial);
    }

    return((3 + bitsLen[usedRate])*sizeof(short));
}

/* reading next mode from mode file */
static int ownReadMode(FILE *pRatesFile, int *pRates)
{
    short tmpRate;
    if (fscanf(pRatesFile, "%hd", &tmpRate) != 1) {
        if (feof(pRatesFile))
            return EOF;
        printf("\nError reading mode control file.\n");
        return 1;
    }
    if ((tmpRate < 0) || (tmpRate > 8)) {
        printf("\nInvalid mode found in mode control file: '%d'\n", tmpRate);
        return 1;
    } 
    *pRates = tmpRate;

    return 0;
}

int text2rate(char *strRate, char **rat_buff, int *nRates, USC_CodecInfo *pInfo)
{
   /* rates according to rate switching mode */
    if (!strcmp("660",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=0;
    }else if(!strcmp("885",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=1;
    }else if(!strcmp("1265",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=2;
    }else if(!strcmp("1425",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=3;
    }else if(!strcmp("1585",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=4;
    }else if(!strcmp("1825",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=5;
    }else if(!strcmp("1985",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=6;
    }else if(!strcmp("2305",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=7;
    }else if(!strcmp("2385",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate=8;
    }else {
        FILE *f_rat=NULL;           /* rate File */
        int readRate;
        if ( (f_rat = fopen(strRate, "rb")) == NULL) {
           printf("Rate file %s could not be open.\n", strRate);
           exit(FOPEN_FAIL);
        } 
        if (*rat_buff) {
           ippsFree(*rat_buff);
           *rat_buff = NULL;
        } 
        while (ownReadMode(f_rat, &readRate) != EOF) {
            (*nRates)++;
        }
        fseek(f_rat, 0, SEEK_SET);
        if(!(*rat_buff = ippsMalloc_8s(*nRates))){
            printf("\nNo memory for buffering of %d output bytes.\n", *nRates);
            exit(MEMORY_FAIL);
        }  
        *nRates = 0;
        while (ownReadMode(f_rat, &readRate) != EOF) {
            (*rat_buff)[*nRates] = (char)readRate;
            (*nRates)++;
        }
        fclose(f_rat);
        return 1;
    }
     *nRates = 1;
     if (*rat_buff) {
        ippsFree(*rat_buff);
        *rat_buff = NULL;
     } 
     if(!(*rat_buff = (char*)ippsMalloc_8u(*nRates))){
        printf("\nNo memory for buffering of %d output bytes", *nRates);
        exit(MEMORY_FAIL);
     }  
     *rat_buff[0] = ((USC_Option*)pInfo->params)->modes.bitrate;
     return 1;
}

int checkVad(char *strVad, int *mode)
{
    if(!strcmp("-v",strVad)) {
        *mode |= AMRWBEncode_VAD_Enabled;
        return 1;
    }
    return 0;
}

int getInFrameSize(void)
{
  return(AMRWB_Frame*sizeof(short));
}
                    
int getOutFrameSize(void)
{
    return SERIAL_FRAMESIZE*sizeof(short);  
}

USC_Fxns* USC_GetCodecByName (void)
{
    return &USC_AMRWB_Fxns;
}
