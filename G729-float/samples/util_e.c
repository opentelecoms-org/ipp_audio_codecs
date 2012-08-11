/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives Speech Codec Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  Speech Codec is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//      Purpose          : Encoder utilities.
//
*/
#include "util_e.h"

USC_Fxns* USC_GetCodecByName_xx2 (void)
{
    return &USC_G729FP_Fxns;
}

static int vad = 0;

static int ReadRates(FILE *f_rat, char **rat_buff, int *nRates)
{
    short *buff;
    unsigned char *rates;
    int rat_len, i;

    fseek(f_rat,0,SEEK_END);
    rat_len = ftell(f_rat);
    *nRates = rat_len >> 1;
    if(!(buff = (short*)ippsMalloc_8u(rat_len))){
      printf("\nNo memory for load of %d bytes from Rate file",rat_len);
      exit(MEMORY_FAIL);
    } 
    fseek(f_rat,0,SEEK_SET);
    fread(buff,sizeof(char),rat_len,f_rat);
    
    if(!(rates = (char*)ippsMalloc_8u(*nRates))){
      printf("\nNo memory for buffering of %d output bytes", *nRates);
      exit(MEMORY_FAIL);
    } 

    for (i=0; i < *nRates; i++) {
        if ((buff[i] < 0) || (buff[i] > 2)) {
            printf("Unrecognized bit-rate in rate file.\n");
            exit(UNKNOWN_FORMAT);
        } 
        if(buff[i]==0) {rates[i]=G729D_CODEC;}
        else if(buff[i]==1) {rates[i]=G729_CODEC;}
        else if(buff[i]==2) {rates[i]=G729E_CODEC;}
    }
    *rat_buff = (char*)rates;
    ippsFree(buff);
    return 0;
}

int text2rate_xx1(char *strRate, char **rat_buff, int *nRates, USC_CodecInfo *pInfo)
{
     if(!strcmp(strRate,"A")){
        ((USC_Option*)pInfo->params)->modes.bitrate=G729A_CODEC;
        pInfo->name = "G729A";
     }else if(!strcmp(strRate,"D")){
        ((USC_Option*)pInfo->params)->modes.bitrate=G729D_CODEC;
        pInfo->name = "G729D";
     }else if(!strcmp(strRate,"E")){
        ((USC_Option*)pInfo->params)->modes.bitrate=G729E_CODEC;
        pInfo->name = "G729E";
     }else if(!strcmp(strRate,"I")){
        ((USC_Option*)pInfo->params)->modes.bitrate=G729I_CODEC;
        pInfo->name = "G729I";
     }else {
        FILE *f_rat=NULL;           /* rate File */
        char* rateFileName = strRate;
        if ( (f_rat = fopen(rateFileName, "rb")) == NULL) {
           printf("Rate file %s could not be open.\n", rateFileName);
           exit(FOPEN_FAIL);
        } 
        if (*rat_buff) {
           ippsFree(*rat_buff);
           *rat_buff = NULL;
        } 

        ReadRates(f_rat, rat_buff, nRates);
        fclose(f_rat);
        pInfo->name = "G729I";
        ((USC_Option*)pInfo->params)->modes.bitrate=G729I_CODEC;
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
        *mode = G729Encode_VAD_Enabled;
        vad = G729Encode_VAD_Enabled;
        return 1;
    }
    return 0;
}

int getInFrameSize(void)
{
  return(G729_SPEECH_FRAME*sizeof(short));
}
                    
int getOutFrameSize_xx3(void)
{
  return(BITSTREAM_FILE_FRAME_SIZE*sizeof(short));
}

int Bits2Ref(USC_PCMStream in, USC_Bitstream out, char *out_buff_cur)
{
    short *buffer = (short*)out_buff_cur;
    int outSize=G729_PURE_BITSTREAM_FILE_FRAME_SIZE;
    if(G729A_CODEC == in.bitrate){
        encodedBitsUnPack_G729AB(out.pBuffer, out.frametype, buffer);
        outSize = buffer[1]+2;
    }else if((vad == G729Encode_VAD_Enabled)||(G729D_CODEC == in.bitrate)||(G729E_CODEC == in.bitrate)){
        encodedBitsUnPack_G729I(out.pBuffer, out.frametype, buffer);
        outSize = buffer[1]+2;
    }else
        encodedBitsUnPack_G729(out.pBuffer, buffer,10);
    return outSize*sizeof(short);
}

/*----------------------------------------------------------------------------
* prm2bits_ld8c -converts encoder parameter vector into vector of serial bits
* bits2prm_ld8c - converts serial received bits to  encoder parameter vector
*----------------------------------------------------------------------------
*
* G.729 main Recommendation: 8 kb/s
*
* The transmitted parameters are:
*
*     LPC:     1st codebook           7+1 bit
*              2nd codebook           5+5 bit
*
*     1st subframe:
*          pitch period                 8 bit
*          parity check on 1st period   1 bit
*          codebook index1 (positions) 13 bit
*          codebook index2 (signs)      4 bit
*          pitch and codebook gains   4+3 bit
*
*     2nd subframe:
*          pitch period (relative)      5 bit
*          codebook index1 (positions) 13 bit
*          codebook index2 (signs)      4 bit
*          pitch and codebook gains   4+3 bit
*
*----------------------------------------------------------------------------
*
* G.729 Annex D: 6.4 kb/s
*
* The transmitted parameters are:
*
*     LPC:     1st codebook           7+1 bit
*              2nd codebook           5+5 bit
*
*     1st subframe:
*          pitch period                 8 bit
*          codebook index1 (positions)  9 bit
*          codebook index2 (signs)      2 bit
*          pitch and codebook gains   3+3 bit
*
*     2nd subframe:
*          pitch period (relative)      4 bit
*          codebook index1 (positions)  9 bit
*          codebook index2 (signs)      2 bit
*          pitch and codebook gains   3+3 bit
*
*----------------------------------------------------------------------------
*
* G.729 Annex E: 11.8 kb/s
*
* The transmitted parameters in forward mode are:
*
*     mode  (including parity)        1+1 bit
*
*     LPC:     1st codebook           7+1 bit
*              2nd codebook           5+5 bit
*
*     1st subframe:
*          pitch period                 8 bit
*          parity check on 1st period   1 bit
*          codebook index             7x5 bit
*          pitch and codebook gains   4+3 bit
*
*     2nd subframe:
*          pitch period (relative)      5 bit
*          codebook index             7x5 bit
*          pitch and codebook gains   4+3 bit
*
* The transmitted parameters in backward mode are:
*
*     mode  (including parity)        1+1 bit
*
*     1st subframe:
*          pitch period                 8 bit
*          parity check on 1st period   1 bit
*          codebook index     13+10+7+7+7 bit
*          pitch and codebook gains   4+3 bit
*
*     2nd subframe:
*          pitch period (relative)      5 bit
*          codebook index     13+10+7+7+7 bit
*          pitch and codebook gains   4+3 bit
*----------------------------------------------------------------------------
*/
static void encodedBitsUnPack_SID(
  const unsigned char *bitstream,  /* input bitstream   */
  short *bits           /* output: ITU bitstream unpacked format: 0 - BIT_0, 1 - BIT_1 */
)
{
   short i,j;
   *bits++ = SYNC_WORD;     /* bit[0], at receiver this bits indicates BFI */
   *bits++ = RATE_SID_OCTET;
   for (i = 0; i < 2; i++)
     {
      for (j = 7; j >=0; j--){
         if( (bitstream[i]>>j)&1 ) 
            *bits++ = BIT_1;
         else
            *bits++ = BIT_0;
      } 
   }
}

static void encodedBitsUnPack_G729(
  const unsigned char *bitstream,  /* input bitstream   */
  short *bits,           /* output: ITU bitstream unpacked format: 0 - BIT_0, 1 - BIT_1 */
  int len
)
{
   short i,j;
   *bits++ = SYNC_WORD;     /* bit[0], at receiver this bits indicates BFI */
   if(len==10) *bits++ = SIZE_WORD;     /* bit[1], to be compatible with hardware      */
   else if(len==8) *bits++ = RATE_6400;     /* bit[1], to be compatible with hardware      */
   else if(len==15) *bits++ = RATE_11800;     /* bit[1], to be compatible with hardware      */

   for (i = 0; i < len; i++)
   {
      bits[i] = 0;
      for (j = 7; j >=0; j--){
         if( (bitstream[i]>>j)&1 ) 
            *bits++ = BIT_1;
         else
            *bits++ = BIT_0;
      } 
       
   }
   return;
}

static void encodedBitsUnPack_G729AB(
  const unsigned char *bitstream,  /* input bitstream   */
  int frametype,  
  short *bits           /* output: ITU bitstream unpacked format: 0 - BIT_0, 1 - BIT_1 */
)
{
  switch(frametype)
  {
    /* not transmitted */
    case 0 : 
        { 
            bits[0] = SYNC_WORD;     /* bit[0], at receiver this bits indicates BFI */
            bits[1] = RATE_0;
            break;
        } 
    case 3 : 
        { 
            encodedBitsUnPack_G729(bitstream,bits,10);
            break;
        } 
    case 1 : 
        { 
            encodedBitsUnPack_SID(bitstream,bits);
            break;
        } 
    default : 
        { 
            printf("Unrecognized frame type\n");
            exit(-1);
        } 
  }
  return;
}

static void encodedBitsUnPack_G729I(
  const unsigned char *bitstream,  /* input bitstream   */
  int frametype,  
  short *bits           /* output: ITU bitstream unpacked format: 0 - BIT_0, 1 - BIT_1 */
)
{
  switch(frametype){
    /* not transmitted */
    case 0 : 
        {
            bits[0] = SYNC_WORD;     /* bit[0], at receiver this bits indicates BFI */
            bits[1] = RATE_0;
            break;
        } 
    case 3 : 
        {
            encodedBitsUnPack_G729(bitstream,bits,10);
            break;
        } 
    case 2 : 
        {
            encodedBitsUnPack_G729(bitstream,bits,8);
            break;
        } 
    case 4 : 
        {
            encodedBitsUnPack_G729(bitstream,bits,15);
            break;
        } 
    case 1 : 
        {
            encodedBitsUnPack_SID(bitstream,bits);
            break;
        } 
    default : 
        {
            printf("Unrecognized frame type\n");
            exit(-1);
        } 
  }
  return;
}

