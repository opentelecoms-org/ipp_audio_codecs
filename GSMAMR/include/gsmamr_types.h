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
//  Purpose: GSM AMR-NB speech codec.
//
*/

#ifndef GSMAMR_TYPES_H
#define GSMAMR_TYPES_H
/*************************************************************************
       GSM AMR recieved and transmitted frame types
**************************************************************************/

typedef enum  {
   RX_NON_SPEECH = -1,
   RX_SPEECH_GOOD = 0,
   RX_SPEECH_DEGRADED = 1,
   RX_ONSET = 2,
   RX_SPEECH_BAD = 3,
   RX_SID_FIRST = 4,
   RX_SID_UPDATE = 5,
   RX_SID_BAD = 6,
   RX_NO_DATA = 7,
   RX_N_FRAMETYPES = 8     /* number of frame types */
}RXFrameType;
typedef enum  {
   TX_NON_SPEECH = -1,
   TX_SPEECH_GOOD = 0,
   TX_SID_FIRST = 1,
   TX_SID_UPDATE = 2,
   TX_NO_DATA = 3,
   TX_SPEECH_DEGRADED = 4,
   TX_SPEECH_BAD = 5,
   TX_SID_BAD = 6,
   TX_ONSET = 7,
   TX_N_FRAMETYPES = 8     /* number of frame types */
}TXFrameType;


/* GSM AMR has nine possible bit rates... */

typedef enum {
    GSMAMR_RATE_UNDEFINED = -1,
    GSMAMR_RATE_4750 = 0,  /* MR475  4.75 kbps  */
    GSMAMR_RATE_5150,      /* MR515  5.15 kbps  */
    GSMAMR_RATE_5900,      /* MR59   5.9  kbps  */
    GSMAMR_RATE_6700,      /* MR67   6.7  kbps  */
    GSMAMR_RATE_7400,      /* MR74   7.4  kbps  */
    GSMAMR_RATE_7950,      /* MR795  7.95 kbps  */
    GSMAMR_RATE_10200,     /* MR102 10.2  kbps  */
    GSMAMR_RATE_12200,     /* MR122 12.2  kbps  (GSM EFR) */
    GSMAMR_RATE_DTX        /* MRDTX Discontinuous TX mode */
} GSMAMR_Rate_t;

#define EHF_MASK 0x0008        /* encoder homing frame pattern             */

/* frame size in serial bitstream file (frame type + serial stream + flags) */
#define MAX_SERIAL_SIZE 244    /* max. num. of serial bits                 */
#define BITSTREAM_SIZE  ((MAX_SERIAL_SIZE+7)>>3)  /* max. num. of serial bytes                 */
#define SERIAL_FRAMESIZE (1+MAX_SERIAL_SIZE+5)
#define  GSMAMR_Frame       160
#define N_MODES     9

#endif /* GSMAMR_TYPES_H */
