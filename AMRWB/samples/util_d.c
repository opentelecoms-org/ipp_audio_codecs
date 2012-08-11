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
//      Purpose          : Decoder utilities.
//
*/
#include "util_d.h"
#include "string.h"

static const int BitsLenTbl[NUM_OF_MODES]={
   132, 177, 253, 285, 317, 365, 397, 461, 477, 477, 35
};

#define BIT_0     (short)-127
#define BIT_1     (short)127

/* proc: shift pointer to next bitstream frame  */
int read_frame(char **pLine, char **pInBuffCur, int *pInLenCur)
{
   int valSize, valRate, valFrameType;
   short *pHeader;

   pHeader = ( short*)*pInBuffCur;
   valFrameType = pHeader[1];
   valRate = pHeader[2];

   if(*pInLenCur <= 0) return 0;
   *pLine = *pInBuffCur;

   if ((valFrameType == TX_SID_FIRST) | (valFrameType == TX_SID_UPDATE) | (valFrameType == TX_NO_DATA)) {
      valRate = AMRWB_RATE_DTX;
   }
   valSize = ((3 + BitsLenTbl[valRate])*sizeof(short));

   *pInLenCur -= valSize;
   *pInBuffCur += valSize;
   return valSize;
}

int text2rate(char *strRate,  int *rate)
{
    if (!strcmp("660",strRate)){
        *rate=AMRWB_RATE_6600;
    }else if(!strcmp("885",strRate)){
        *rate=AMRWB_RATE_8850;
    }else if(!strcmp("1265",strRate)){
        *rate=AMRWB_RATE_12650;
    }else if(!strcmp("1425",strRate)){
        *rate=AMRWB_RATE_14250;
    }else if(!strcmp("1585",strRate)){
        *rate=AMRWB_RATE_15850;
    }else if(!strcmp("1825",strRate)){
        *rate=AMRWB_RATE_18250;
    }else if(!strcmp("1985",strRate)){
        *rate=AMRWB_RATE_19850;
    }else if(!strcmp("2305",strRate)){
        *rate=AMRWB_RATE_23050;
    }else if(!strcmp("2385",strRate)){
        *rate=AMRWB_RATE_23850;
    }else {
        return 0;
    }
    return 1;
}

static short ownSerial2Parm(short valNumBits, short ** pSerial)
{
    int i;
    short valPrm, valBit;

    valPrm = 0;
    for (i = 0; i < valNumBits; i++)
    {
        valPrm <<= 1;
        valBit = *((*pSerial)++);
        if (valBit == BIT_1)
            valPrm++;
    }
    return (valPrm);
}

static void ownConvert(int valRate, const char *pSerialvec, char* pPrmvec)
{
    short i, j, tmp;
    short *pPrms = ( short*)pPrmvec;
    short *pSerial = ( short*)pSerialvec; 
    int valNumPrms = BitsLenTbl[valRate];

    j = 0;
    i = 0;

    tmp = valNumPrms - 15;
    while (tmp > j)
    {
        pPrms[i] = ownSerial2Parm(15, &pSerial);
        j += 15;
        i++;
    }
    tmp = valNumPrms - j;
    pPrms[i] = ownSerial2Parm(tmp, &pSerial);
    pPrms[i] <<= (15 - tmp);

    return;
}

void Ref2Bits(const char *refBits, USC_Bitstream *in)
{
    int  i;
    short *prns = (short*)refBits;

    i = *(prns)++;
    if (i != TX_FRAME_TYPE) {
      printf("Wrong type of frame type:%d.\n", i);
    }

    in->frametype = (TXFrameType)*(prns)++;
    in->bitrate = *(prns)++;


    if ((in->frametype == TX_SID_BAD) || (in->frametype == TX_SID_UPDATE) || (in->frametype == TX_NO_DATA)) {
        ownConvert(AMRWB_RATE_DTX, (const char*)prns, in->pBuffer);
    } else {
        ownConvert(in->bitrate, (const char*)prns, in->pBuffer);
    }

    return;
}
                   
int getOutFrameSize(void)
{
  return(AMRWB_Frame*sizeof(short));
}

USC_Fxns* USC_GetCodecByName (void)
{
    return &USC_AMRWB_Fxns;
}
