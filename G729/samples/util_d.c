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

USC_Fxns* USC_GetCodecByName (void)
{
    return &USC_G729I_Fxns;
}

int text2rate(char *strRate, int *rate)
{
     if(!strcmp(strRate,"A")){
        *rate=G729A_CODEC;
     }else if(!strcmp(strRate,"D")){
        *rate=G729D_CODEC;
     }else if(!strcmp(strRate,"E")){
        *rate=G729E_CODEC;
     }else if(!strcmp(strRate,"I")){
        *rate=G729I_CODEC;
     }else {
        return 0;
     }
    return 1;
}

/*-----------------------------------------------------------------------------
 * bitsPack - Pack ITU tv bits (BIT_0 or BIT_1 16bit word) into 16bit word (LSB)
 *----------------------------------------------------------------------------*/
static void bitsPack(     
  short no_of_bits,        /* input : number of bits to read */
  const short *ituBits,    /* input : array containing unpacked bits */
  unsigned char *bitstream          
)
{
   short   i;
   short bit;

   for (i = 0; i < no_of_bits; i++)
   {
     if(!(i&7)) bitstream[i>>3] = 0;
     bit = *ituBits++;
     bitstream[i>>3] <<= 1;
     if (bit == BIT_1) {
        bitstream[i>>3] += 1;
     }
   }
   if(no_of_bits&7) bitstream[i>>3] <<= (8 - no_of_bits&7);
}

static void itu2bits_G729_ActiveFrame(
 const short *ituBits,         /* input : frame unpacked serial bits (82 short: ) */
 unsigned char *bitstream      /* output: raw bitstream (80 bits, 5 short int) */
 )
{
   /* pack ITU tv into 80bit frame*/
   bitsPack(16, &ituBits[0],bitstream);
   bitsPack(16, &ituBits[16],bitstream+2);
   bitsPack(16, &ituBits[32],bitstream+4);
   bitsPack(16, &ituBits[48],bitstream+6);
   bitsPack(16, &ituBits[64],bitstream+8);
}

static void itu2bits_G729_ActiveFrame_D(
 const short *ituBits,         /* input : frame unpacked serial bits (82 short: ) */
 unsigned char *bitstream      /* output: raw bitstream (80 bits, 5 short int) */
 )
{
   /* pack ITU tv into 80bit frame*/
   bitsPack(16, &ituBits[0],bitstream);
   bitsPack(16, &ituBits[16],bitstream+2);
   bitsPack(16, &ituBits[32],bitstream+4);
   bitsPack(16, &ituBits[48],bitstream+6);
}

static void itu2bits_G729_ActiveFrame_E(
 const short *ituBits,         /* input : frame unpacked serial bits (82 short: ) */
 unsigned char *bitstream      /* output: raw bitstream (80 bits, 5 short int) */
 )
{
   /* pack ITU tv into 80bit frame*/
   bitsPack(16, &ituBits[0],bitstream);
   bitsPack(16, &ituBits[16],bitstream+2);
   bitsPack(16, &ituBits[32],bitstream+4);
   bitsPack(16, &ituBits[48],bitstream+6);
   bitsPack(16, &ituBits[64],bitstream+8);
   bitsPack(16, &ituBits[80],bitstream+10);
   bitsPack(16, &ituBits[96],bitstream+12);
   bitsPack(6, &ituBits[112],bitstream+14);
}

static void itu2bits_G729_SIDFrame(
 const short *ituBits,         /* input : SID frame unpacked serial bits (17 short: ) */
 unsigned char *bitstream      /* output: raw bitstream (15 bits, 1 short int, MSB) */
)
{
   /* pack ITU tv SID frame into n15bit */
   bitsPack(16, ituBits,bitstream); /* 15bit, MSB */
}

void Ref2Bits(
 const char *refBits,         /* input : serial bits (82 short: ) */
 USC_Bitstream *in            /* output: raw bitstream (80 bits, 5 short int) */
 )
{
   int i;
   short *ituBits = (short*)refBits;
   if (ituBits[0] != SYNC_WORD) {                 
      in->frametype = -1;
      return;   /* frame erased     */                                   
   }                                                   
   if (ituBits[1] != 0) {                                 
      for (i=0; i < ituBits[1]; i++)                      
          if (ituBits[i] == 0 ) {
              in->frametype = -1;
              return; /* frame erased     */
          }
   } 
   if(ituBits[1] == RATE_8000){
      itu2bits_G729_ActiveFrame(ituBits+2,in->pBuffer);
      in->frametype = 3;
      return; /* active frame*/
   }
   else if(ituBits[1] == RATE_SID_OCTET) {
      itu2bits_G729_SIDFrame(ituBits+2,in->pBuffer);
      in->frametype = 1;
      return; /* SID, passive frame*/
   }
   else if(ituBits[1] == RATE_6400) {
      itu2bits_G729_ActiveFrame_D(ituBits+2,in->pBuffer);
      in->frametype = 2;
      return; /* active frame D*/
   }
   else if(ituBits[1] == RATE_11800) {
      short mode, parity_mode;
      if(ituBits[2] == BIT_0) mode = 0;
      else mode = 1;
      if( ituBits[3] == BIT_0) parity_mode = 0;
      else parity_mode = 1;
      if (mode != parity_mode) {
          in->frametype = -1;
          return; /* frame erased     */
      }
      itu2bits_G729_ActiveFrame_E(ituBits+2,in->pBuffer);
      in->frametype = 4;
      return; /* active frame E*/
   }

   in->frametype = 0;
   return; /* untransmitted*/
}

int read_frame(char **buffer, char **in_buff_cur, int *in_len_cur)
{
  int Size;
  short *tmpBuff = (short*)*in_buff_cur;

  *buffer = *in_buff_cur;
  if(*in_len_cur < 2*sizeof(short)) {
    return 0;
  }

  *in_len_cur -= 2*sizeof(short);

  if(*in_len_cur < (int)(sizeof(short) * tmpBuff[1])) {
    return 0;
  }

  *in_len_cur -= sizeof(short) * tmpBuff[1];
  Size = (2 + tmpBuff[1]) * sizeof(short);
  *in_buff_cur += Size;

  return (Size);
}

int getOutFrameSize(void)
{
  return(G729_SPEECH_FRAME*sizeof(short));
}
