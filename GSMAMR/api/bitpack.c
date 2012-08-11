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
//  Purpose: GSM AMR parm to/from bitstream pack/unpack functions
//
*/

#include "owngsmamr.h"
static char* ownPar2Ser_GSMAMR( int Inp, char *Pnt, int BitNum );
static int  ownExtractBits_GSMAMR( const unsigned char **pBits, int *nBit, int Count );

void ownPrm2Bits_GSMAMR(const short* prm, unsigned char *Vout, GSMAMR_Rate_t rate )
{
    int     i ;
    char *Bsp;
    int  Temp ;
    int BitCount = 0;
    IPP_ALIGNED_ARRAY(16, char, BitStream, MAX_SERIAL_SIZE); /* max number of bits (rate 12.2 )*/

    Bsp = BitStream ;

    for( i = 0; i < TableParamPerModes[rate]; i++) {
       BitCount += TableBitAllModes[rate][i];
       Temp = prm[i];
       Bsp = ownPar2Ser_GSMAMR( Temp, Bsp, TableBitAllModes[rate][i] ) ;
    }

    /* Clear the output vector */
    ippsZero_8u((Ipp8u*)Vout, BITSTREAM_SIZE);
    for ( i = 0 ; i < BitCount ; i ++ )
        Vout[i>>3] ^= BitStream[i] << (~i & 0x7);

    return;
}

static char* ownPar2Ser_GSMAMR( int Inp, char *Pnt, int BitNum )
{
    int i   ;
    short  Temp ;
    char  *TempPnt = Pnt + BitNum -1;

    for ( i = 0 ; i < BitNum ; i ++ ) {
        Temp = (short) (Inp & 0x1);
        Inp >>= 1;
        *TempPnt-- = (char)Temp;
    }

    return (Pnt + BitNum) ;
	/* End of ownPar2Ser_GSMAMR() */
}

void  ownBits2Prm_GSMAMR( const unsigned char *bitstream, short* prm , GSMAMR_Rate_t rate )
{
    int   i  ;
    const unsigned char **pBits = &bitstream;
    int nBit = 0;

    for( i = 0; i < TableParamPerModes[rate]; i++) {
       prm[i] = ownExtractBits_GSMAMR( pBits, &nBit, TableBitAllModes[rate][i] ) ;
    }
    /* End of ownBits2Prm_GSMAMR() */
}


static int  ownExtractBits_GSMAMR( const unsigned char **pBits, int *nBit, int Count )
{
    int     i ;
    int  ResSum = 0 ;

    for ( i = 0 ; i < Count ; i ++ ){
        int  ExtVal, idx ;
		idx = i + *nBit;
        ExtVal = ((*pBits)[idx>>3] >> (~idx & 0x7)) & 1;
        ResSum +=  ExtVal << (Count-i-1) ;
    }

    *pBits += (Count + *nBit)>>3;
    *nBit = (Count + *nBit) & 0x7;

    return ResSum ;
	/* End of ownExtractBits_GSMAMR() */
}

