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
//  Purpose: AMRWB speech codec.
//
*/

#ifndef AMRWB_TYPES_H
#define AMRWB_TYPES_H
/*************************************************************************
       AMRWB recieved and transmitted frame types
**************************************************************************/
typedef enum  {
   RX_SPEECH_GOOD=0,
   RX_SPEECH_PROBABLY_DEGRADED,
   RX_SPEECH_LOST,
   RX_SPEECH_BAD,
   RX_SID_FIRST,
   RX_SID_UPDATE,
   RX_SID_BAD,
   RX_NO_DATA
}RXFrameType;

typedef enum  {
   TX_SPEECH=0,
   TX_SID_FIRST,
   TX_SID_UPDATE,
   TX_NO_DATA,
   TX_SPEECH_PROBABLY_DEGRADED,
   TX_SPEECH_LOST,
   TX_SPEECH_BAD,
   TX_SID_BAD
}TXFrameType;



#define EHF_MASK (short)0x0008             /* homing frame pattern                       */
#define L_FRAME16k   320                   /* Frame size at 16kHz                        */
#define IO_MASK (short)0xfffC              /* 14-bit input/output                        */
#define AMRWB_Frame   L_FRAME16k           /* Frame size at 16kHz                        */

#define NUMBITS6600     132                  /* 6.60k  */
#define NUMBITS8850     177                  /* 8.85k  */
#define NUMBITS12650    253                  /* 12.65k */
#define NUMBITS14250    285                  /* 14.25k */
#define NUMBITS15850    317                  /* 15.85k */
#define NUMBITS18250    365                  /* 18.25k */
#define NUMBITS19850    397                  /* 19.85k */
#define NUMBITS23050    461                  /* 23.05k */
#define NUMBITS23850    477                  /* 23.85k */
#define NUMBITS_SID       35

#define NUM_OF_MODES  11                   /* see bits.h for bits definition             */

/* AMR WB has nine possible bit rates... */

typedef enum {
    AMRWB_RATE_UNDEFINED = -1,
    AMRWB_RATE_6600 = 0,   /* 6.60 kbps   */
    AMRWB_RATE_8850,       /* 8.85 kbps   */
    AMRWB_RATE_12650,      /* 12.65  kbps */
    AMRWB_RATE_14250,      /* 14.25  kbps */
    AMRWB_RATE_15850,      /* 15.85  kbps */
    AMRWB_RATE_18250,      /* 18.25 kbps  */
    AMRWB_RATE_19850,      /* 19.85  kbps */
    AMRWB_RATE_23050,      /* 23.05  kbps */
    AMRWB_RATE_23850,      /* 23.85  kbps */
    AMRWB_RATE_DTX = 10    /* DTX Discontinuous TX mode */
} AMRWB_Rate_t;

#define NB_BITS_MAX   NUMBITS23850

#define SERIAL_FRAMESIZE (6+NB_BITS_MAX)   /* serial size max           */
#define BITSTREAM_SIZE  64                 /* max. num. of serial bytes */
#define TX_FRAME_TYPE (short)0x6b21
#define RX_FRAME_TYPE (short)0x6b20

#endif /* AMRWB_TYPES_H */

