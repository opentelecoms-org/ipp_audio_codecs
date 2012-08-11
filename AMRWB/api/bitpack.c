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
//  Purpose: Speech Codec parm to/from bitstream pack/unpack functions
//
*/
#include "ownamrwb.h"

static int ownExtractBits( const unsigned short **pBits, int *nBit, int valCnt )
{
    int  i, j = 0;
    int  Rez = 0;
    int  temp, valSht;

    for ( i = 0 ; i < valCnt ; i ++ ){
        valSht = 14 - (i + *nBit);
        if (valSht < 0) {
            valSht += 15;
            j = 1;
        }
        temp = ((*pBits)[j] >> valSht) & 1;
        Rez <<= 1;
        Rez += temp;
    }
    
    *nBit += valCnt;
    if (*nBit >= 15) {
        *nBit -= 15;
        *pBits += 1;
    }

    return Rez;
}

void ownBits2Prms(const unsigned char *pBitstream, short *pPrms , AMRWB_Rate_t rate)
{
    int i;
    const unsigned short **pBits = (const unsigned short**)&pBitstream;
    int nBit = 0;

    for( i = 0; i < NumberPrmsTbl[rate]; i++) {
       pPrms[i] = ownExtractBits( pBits, &nBit, pNumberBitsTbl[rate][i] ) ;
    }
}

static char* ownPar2Ser( int valPrm, char *pBitStreamvec, int valNumBits )
{
    int i;
    short tmp;
    char *TempPnt = pBitStreamvec + valNumBits -1;

    for ( i = 0 ; i < valNumBits ; i ++ ) {
        tmp = (short) (valPrm & 0x1);
        valPrm >>= 1;
        *TempPnt-- = (char)tmp;
    }

    return (pBitStreamvec + valNumBits);
}

void ownPrms2Bits(const short* pPrms, unsigned char *pBitstream, AMRWB_Rate_t rate)
{
    char *pBsp;
    int i, tmp, valSht, valBitCnt = 0;
    IPP_ALIGNED_ARRAY(16, char, pBitStreamvec, NB_BITS_MAX);
    unsigned short *pBits = (unsigned short*)pBitstream;

    pBsp = pBitStreamvec ;

    for( i = 0; i < NumberPrmsTbl[rate]; i++) {
       valBitCnt += pNumberBitsTbl[rate][i];
       tmp = pPrms[i];
       pBsp = ownPar2Ser( tmp, pBsp, pNumberBitsTbl[rate][i] );
    }

    /* Clear the output vector */
    ippsZero_16s((short*)pBitstream, BITSTREAM_SIZE/2);
    valSht  = (valBitCnt + 14) / 15 * 15 - valBitCnt;

    for ( i = 0 ; i < valBitCnt; i ++ ) {
        pBits[i/15] <<= 1;
        if (pBitStreamvec[i]) pBits[i/15]++;
    }
    pBits[(i-1)/15] <<= valSht;
    
    return;
}
