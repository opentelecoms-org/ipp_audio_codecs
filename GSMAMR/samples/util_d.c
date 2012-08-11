/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives GSMAMR Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP 
//  product installation for more information.
//
//  GSMAMR is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//      Purpose          : Decoder utilities.
//
*/

#include "util_d.h"

USC_Fxns* USC_GetCodecByName (void)
{
    return &USC_GSMAMR_Fxns;
}

int text2rate(char *strRate,  int *rate)
{
    if (!strcmp("122",strRate)){
        *rate = GSMAMR_RATE_12200;
    }else if(!strcmp("102",strRate)){
        *rate = GSMAMR_RATE_10200;
    }else if(!strcmp("795",strRate)){
        *rate = GSMAMR_RATE_7950;
    }else if(!strcmp("74",strRate)){
        *rate = GSMAMR_RATE_7400;
    }else if(!strcmp("67",strRate)){
        *rate = GSMAMR_RATE_6700;
    }else if(!strcmp("59",strRate)){
        *rate = GSMAMR_RATE_5900;
    }else if(!strcmp("515",strRate)){
        *rate = GSMAMR_RATE_5150;
    }else if(!strcmp("475",strRate)){
        *rate = GSMAMR_RATE_4750;
    }else {
        return 0;
    }
    return 1;
}

static const int bitsLen[N_MODES]={
   95, 103, 118, 134, 148, 159, 204, 244, 35
};

/* proc:  converts the ref vector format into encoder bitstream */
static void tst2bits (
    int mode,            
    const short *ubits,  
    char *bits           
)
{
   int i,j,k=0;
   int nbytes = (bitsLen[mode] + 7) >> 3;

   for (i = 0; i < nbytes; i++){
      bits[i] = 0;
      for (j = 7; j >=0 && k < bitsLen[mode]; j--, k++){
         if(*ubits++ == 1  ){
            bits[i] |= 1<<j;
         }
      }
   }

   return;
}


void Ref2Bits(const char *refBits, USC_Bitstream *in)
{
    TXFrameType tx_type;
    int mode;

    /* get frame type and mode information from frame */
    tx_type = (TXFrameType)refBits[0];
    in->frametype = tx_type;
    if (refBits[(1+MAX_SERIAL_SIZE)*2] >= 0) { /* frametype != TX_NO_DATA */
        in->bitrate = refBits[(1+MAX_SERIAL_SIZE)*2];
    }  
    
    if ((in->frametype == TX_SID_BAD) || (in->frametype == TX_SID_UPDATE)) {
        mode = GSMAMR_RATE_DTX;
    } else {
        mode = in->bitrate;
    }  
    tst2bits (mode, (short*)refBits+1, in->pBuffer);

    return;
}

/* proc: shift pointer to next bitstream frame  */
int read_frame(char **pLine, char **in_buff_cur, int *in_len_cur)
{
   int Size = SERIAL_FRAMESIZE*sizeof(short);

   if(*in_len_cur <= 0) return 0;
   *pLine = *in_buff_cur;
   
   *in_len_cur -= Size; 
   *in_buff_cur += Size;
   return Size;
}

int getOutFrameSize(void)
{
  return(GSMAMR_Frame*sizeof(short));
}
                    
