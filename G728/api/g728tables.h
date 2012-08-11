/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.728 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.728 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.728 tables
//
*/

extern const Ipp16s wnrw[64];
extern const Ipp16s wnrlg[40];
extern const Ipp16s wnr[112];
extern const Ipp16s cnstSynthesisFilterBroadenVector[56];/*facv*/ 
extern const Ipp16s cnstGainPreditorBroadenVector[16]; /*facgpv*/ 
extern const Ipp16s cnstPoleCtrlFactor[16]; /*wpcfv*/ 
extern const Ipp16s cnstZeroCtrlFactor[16]; /*wzcfv*/ 
extern const Ipp16s cnstSTPostFltrPoleVector[16];/*spfpcfv*/ 
extern const Ipp16s cnstSTPostFltrZeroVector[16];/*spfzcfv*/ 
extern const Ipp16s gq[8];
extern const Ipp16s cnstShapeCodebookVectors[128];
extern const Ipp16s shape_all_norm[128*5];
extern const Ipp16s shape_all_nls[128];
extern const Ipp16s nngq[8];
extern const Ipp16s cnstCodebookVectorsGain[8];/*gcb*/ 

/* Rate 12.8 spicific*/

extern const Ipp16s    gq_128[4];
extern const Ipp16s  nngq_128[4];
extern const Ipp16s cnstCodebookVectorsGain_128[4];/*gcblg_128*/ 

/* Rate 9.6 specific*/

extern const Ipp16s gq_96[4];
extern const Ipp16s nngq_96[4];
extern const Ipp16s cnstCodebookVectorsGain_96[4];/*gcblg_96*/ 

extern const Ipp16s cnstSynthesisFilterBroadenVectorFE[56];/*facvfe*/ 

