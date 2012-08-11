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
//      Purpose          : Encoder utilities.
//
*/

#include "util_e.h"

USC_Fxns* USC_GetCodecByName (void)
{
    return &USC_GSMAMR_Fxns;
}

static const int bitsLen[N_MODES]={
   95, 103, 118, 134, 148, 159, 204, 244, 35
};


/*************************************************************************
  FUNCTION:    encodedBitsUnPack
  PURPOSE:     converts the encoder bitstream into a ref vector format
*************************************************************************/

int Bits2Ref(USC_PCMStream in, USC_Bitstream out, char *out_buff_cur)
{
   int i,j,k=0;
   int nbytes;
   int usedRate = in.bitrate;
   short *ubits = (short*)out_buff_cur;

   if (out.frametype != TX_SPEECH_GOOD) {
        usedRate = GSMAMR_RATE_DTX;
   }

   nbytes = (bitsLen[usedRate] + 7) >> 3;

   /* zero flags and parameter bits */
   ippsZero_16s(ubits, SERIAL_FRAMESIZE);

   *(ubits)++ = out.frametype;
   if (out.frametype != TX_NO_DATA) {
       ubits[MAX_SERIAL_SIZE] = in.bitrate;
   } 
   else {
       ubits[MAX_SERIAL_SIZE] = (short)-1;
   } 

   for (i = 0; i < nbytes; i++){
      unsigned char octet = out.pBuffer[i];
      for (j = 7; j >=0 && k < bitsLen[usedRate]; j--, k++){
         if( (octet>>j)&1 )
            *ubits++ = 1;
         else
            *ubits++ = 0;
      }
   }
   return(SERIAL_FRAMESIZE*sizeof(short));
}

static const char *modetable[] = {
"MR475","MR515","MR59","MR67","MR74","MR795","MR102","MR122"
};

/*
 *  read next mode from mode file
 *
 * return 0 on success, EOF on end of file, 1 on other error
 */
static int read_mode(FILE *modes, GSMAMR_Rate_t *mode)
{
    char buf[10];
    int i;

    if (fscanf(modes, "%9s\n", buf) != 1) {
        if (feof(modes))
            return EOF;
        printf("\nError reading mode control file.\n");
        return 1;
    }
    for (i = 0; i < 8; i++) {
        if (!strcmp(modetable[i], buf)) {
            *mode = (GSMAMR_Rate_t)i;
            return 0;
        }
    }
    printf("\nInvalid mode found in mode control file: '%s'\n", buf);
    return 1;
}

int text2rate(char *strRate, char **rat_buff, int *nRates, const USC_CodecInfo *pInfo)
{
   /* rates according to modetable indexes */
    if (!strcmp("122",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 7;
    }else if(!strcmp("102",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 6;
    }else if(!strcmp("795",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 5;
    }else if(!strcmp("74",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 4;
    }else if(!strcmp("67",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 3;
    }else if(!strcmp("59",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 2;
    }else if(!strcmp("515",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 1;
    }else if(!strcmp("475",strRate)){
        ((USC_Option*)pInfo->params)->modes.bitrate = 0;
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
        while (read_mode(f_rat, &readRate) != EOF) {
            (*nRates)++;
        }
        fseek(f_rat, 0, SEEK_SET);
        if(!(*rat_buff = ippsMalloc_8s(*nRates))){
            printf("\nNo memory for buffering of %d output bytes.\n", *nRates);
            exit(MEMORY_FAIL);
        }  
        *nRates = 0;
        while (read_mode(f_rat, &readRate) != EOF) {
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
     if(!(*rat_buff = ippsMalloc_8s(*nRates))){
        printf("\nNo memory for buffering of %d output bytes", *nRates);
        exit(MEMORY_FAIL);
     }  
     *rat_buff[0] = ((USC_Option*)pInfo->params)->modes.bitrate;
     return 1;
}

int checkVad(char *strVad, int *mode)
{
    if(!strcmp("-v1",strVad)) {
        if(!(*mode & GSMAMREncode_VAD2_Enabled)){
            *mode |= GSMAMREncode_VAD1_Enabled;
        } 
        return 1;
    }else if(!strcmp("-v2",strVad)) {
        if(!(*mode & GSMAMREncode_VAD1_Enabled)){
            *mode |= GSMAMREncode_VAD2_Enabled;
        } 
        return 1;
    }
    return 0;
}

int getInFrameSize(void)
{
  return(GSMAMR_Frame*sizeof(short));
}
                    
int getOutFrameSize(void)
{
  return(SERIAL_FRAMESIZE*sizeof(short));
}


