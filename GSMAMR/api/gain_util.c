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
//  Purpose: GSM AMR-NB speech codec: GAIN utilities.
//
*/

#include "owngsmamr.h"
/*********************************************************************************
*   Scalar quantization tables of the codebook gain.
*********************************************************************************/
static const short pTblQntGainCode[LEN_QNT_GAIN*3] =
{
     159,      -3776,       -22731,         206,      -3394,       -20428,          268,      -3005,    
  -18088,        349,        -2615,      -15739,        419,        -2345,       -14113,        482,
   -2138,     -12867,          554,       -1932,     -11629,          637,        -1726,     -10387,
     733,      -1518,        -9139,         842,      -1314,        -7906,          969,      -1106,
   -6656,       1114,         -900,        -5416,      1281,         -694,        -4173,       1473,
    -487,      -2931,         1694,         -281,     -1688,         1948,          -75,       -445,
    2241,        133,          801,         2577,       339,         2044,         2963,        545,
	3285,       3408,          752,         4530,      3919,          958,         5772,       4507,
	1165,       7016,         5183,         1371,      8259,         5960,         1577,       9501,
    6855,       1784,        10745,         7883,      1991,        11988,         9065,       2197,
   13231,      10425,         2404,        14474,     12510,         2673,        16096,      16263,
    3060,      18429,        21142,         3448,     20763,        27485,         3836,      23097 
};
/****************************************************************************
 * Function: ownQntGainCodebook_GSMAMR()                                                      
 ***************************************************************************/
short ownQntGainCodebook_GSMAMR (IppSpchBitRate rate, short expGainCodeCB, short fracGainCodeCB,          
                                 short *gain, short *pQntEnergyErr_M122, short *pQntEnergyErr)
{
    const short *p;
    short i, index;
    short predGainCB, err, err_min;
    short g_q0;
    g_q0 = 0;    
    
	if(rate == IPP_SPCHBR_12200)  g_q0 = *gain >> 1; 

    predGainCB = (short)(ownPow2_GSMAMR (expGainCodeCB, fracGainCodeCB));  
	if (rate == IPP_SPCHBR_12200) predGainCB = Cnvrt_32s16s(predGainCB << 4);
    else predGainCB = Cnvrt_32s16s(predGainCB << 5);

    p = &pTblQntGainCode[0]; 
    if(rate == IPP_SPCHBR_12200)
       err_min = Abs_16s((short)(g_q0 - (short)((predGainCB * *p++) >> 15)));
    else
       err_min = Abs_16s((short)(*gain - (short)((predGainCB * *p++) >> 15)));

    p += 2;    
    index = 0;              
    for (i = 1; i < LEN_QNT_GAIN; i++)  {
       if (rate == IPP_SPCHBR_12200) err = Abs_16s((short)(g_q0 - ((predGainCB * *p++) >> 15)));
       else err = Abs_16s((short)(*gain - ((predGainCB * *p++) >> 15)));
       p += 2;                              
       if (err < err_min) { err_min = err; index = i; }
    }

    p = &pTblQntGainCode[index * 3]; 
    if (rate == IPP_SPCHBR_12200) *gain = Cnvrt_32s16s(((predGainCB * *p++) >> 15) << 1); 
    else *gain = (short)((predGainCB * *p++) >> 15);

    *pQntEnergyErr_M122 = *p++;                 
    *pQntEnergyErr = *p;                         
    return index;
}

/***************************************************************************
 * Function: ownQntGainPitch_M7950_GSMAMR()                                                     *
 ***************************************************************************/
short ownQntGainPitch_M7950_GSMAMR (short gainPitchLimit, short *gain, short *gain_cand, short *gain_cind)
{
    short i, index;
    short ii;

    index = ownQntGainPitch_M122_GSMAMR(gainPitchLimit,*gain);

    /* compute three pitchGainVar candidates around the index */

    if (index == 0) ii = index;                                     
    else {
        if (index == LEN_QNT_PITCH-1 || TableQuantGainPitch[index+1] > gainPitchLimit) ii = index - 2;
        else ii = index - 1;
    }

    /* store candidate indices and values */
    for (i = 0; i < 3; i++)  {
        gain_cind[i] = ii;                             
        gain_cand[i] = TableQuantGainPitch[ii];            
        ii++;
    }
    
    *gain = TableQuantGainPitch[index];                      
    return index;
}

short ownQntGainPitch_M122_GSMAMR (short gainPitchLimit, short gain)
{
    short i, index,  err_min;
    IPP_ALIGNED_ARRAY (16,short,gain_diff,LEN_QNT_PITCH);

    ippsSubC_16s_Sfs(TableQuantGainPitch, gain, gain_diff, LEN_QNT_PITCH, 0);
    ippsAbs_16s_I(gain_diff,LEN_QNT_PITCH);

    err_min = gain_diff[0];
    index = 0;                                             

    for (i = 1; i < LEN_QNT_PITCH; i++) {
        if (TableQuantGainPitch[i] > gainPitchLimit) break; /* qua_gain_pitch is mounting */
        if (gain_diff[i] < err_min) {
            err_min = gain_diff[i];                                 
            index = i;                                     
        }
    }
    return index;
}

#define MIN_QUA_ENER         ( -5443) /* Q10 <->    log2 (0.0251189) */
#define MIN_QUA_ENER_MR122   (-32768) /* Q10 <-> 20*log10(0.0251189) */
#define MAX_QUA_ENER         (  3037) /* Q10 <->    log2 (7.8125)    */
#define MAX_QUA_ENER_MR122   ( 18284) /* Q10 <-> 20*log10(7.8125)    */

static const short table_gain_MR475[VEC_QNT_M475_SIZE*4] = {
   812,          128,           542,      140,       2873,         1135,          2266,     3402,
  2067,          563,         12677,      647,       4132,         1798,          5601,     5285,
  7689,          374,          3735,      441,      10912,         2638,         11807,     2494,
 20490,          797,          5218,      675,       6724,         8354,          5282,     1696,
  1488,          428,          5882,      452,       5332,         4072,          3583,     1268,
  2469,          901,         15894,     1005,      14982,         3271,         10331,     4858,
  3635,         2021,          2596,      835,      12360,         4892,         12206,     1704,
 13432,         1604,          9118,     2341,       3968,         1538,          5479,     9936,
  3795,          417,          1359,      414,       3640,         1569,          7995,     3541,
 11405,          645,          8552,      635,       4056,         1377,         16608,     6124,
 11420,          700,          2007,      607,      12415,         1578,         11119,     4654,
 13680,         1708,         11990,     1229,       7996,         7297,         13231,     5715,
  2428,         1159,          2073,     1941,       6218,         6121,          3546,     1804,
  8925,         1802,          8679,     1580,      13935,         3576,         13313,     6237,
  6142,         1130,          5994,     1734,      14141,         4662,         11271,     3321,
 12226,         1551,         13931,     3015,       5081,        10464,          9444,     6706,
  1689,          683,          1436,     1306,       7212,         3933,          4082,     2713,
  7793,          704,         15070,      802,       6299,         5212,          4337,     5357,
  6676,          541,          6062,      626,      13651,         3700,         11498,     2408,
 16156,          716,         12177,      751,       8065,        11489,          6314,     2256,
  4466,          496,          7293,      523,      10213,         3833,          8394,     3037,
  8403,          966,         14228,     1880,       8703,         5409,         16395,     4863,
  7420,         1979,          6089,     1230,       9371,         4398,         14558,     3363,
 13559,         2873,         13163,     1465,       5534,         1678,         13138,    14771,
  7338,          600,          1318,      548,       4252,         3539,         10044,     2364,
 10587,          622,         13088,      669,      14126,         3526,          5039,     9784,
 15338,          619,          3115,      590,      16442,         3013,         15542,     4168,
 15537,         1611,         15405,     1228,      16023,         9299,          7534,     4976,
  1990,         1213,         11447,     1157,      12512,         5519,          9475,     2644,
  7716,         2034,         13280,     2239,      16011,         5093,          8066,     6761,
 10083,         1413,          5002,     2347,      12523,         5975,         15126,     2899,
 18264,         2289,         15827,     2527,      16265,        10254,         14651,    11319,
  1797,          337,          3115,      397,       3510,         2928,          4592,     2670,
  7519,          628,         11415,      656,       5946,         2435,          6544,     7367,
  8238,          829,          4000,      863,      10032,         2492,         16057,     3551,
 18204,         1054,          6103,     1454,       5884,         7900,         18752,     3468,
  1864,          544,          9198,      683,      11623,         4160,          4594,     1644,
  3158,         1157,         15953,     2560,      12349,         3733,         17420,     5260,
  6106,         2004,          2917,     1742,      16467,         5257,         16787,     1680,
 17205,         1759,          4773,     3231,       7386,         6035,         14342,    10012,
  4035,          442,          4194,      458,       9214,         2242,          7427,     4217,
 12860,          801,         11186,      825,      12648,         2084,         12956,     6554,
  9505,          996,          6629,      985,      10537,         2502,         15289,     5006,
 12602,         2055,         15484,     1653,      16194,         6921,         14231,     5790,
  2626,          828,          5615,     1686,      13663,         5778,          3668,     1554,
 11313,         2633,          9770,     1459,      14003,         4733,         15897,     6291,
  6278,         1870,          7910,     2285,      16978,         4571,         16576,     3849,
 15248,         2311,         16023,     3244,      14459,        17808,         11847,     2763,
  1981,         1407,          1400,      876,       4335,         3547,          4391,     4210,
  5405,          680,         17461,      781,       6501,         5118,          8091,     7677,
  7355,          794,          8333,     1182,      15041,         3160,         14928,     3039,
 20421,          880,         14545,      852,      12337,        14708,          6904,     1920,
  4225,          933,          8218,     1087,      10659,         4084,         10082,     4533,
  2735,          840,         20657,     1081,      16711,         5966,         15873,     4578,
 10871,         2574,          3773,     1166,      14519,         4044,         20699,     2627,
 15219,         2734,         15274,     2186,       6257,         3226,         13125,    19480,
  7196,          930,          2462,     1618,       4515,         3092,         13852,     4277,
 10460,          833,         17339,      810,      16891,         2289,         15546,     8217,
 13603,         1684,          3197,     1834,      15948,         2820,         15812,     5327,
 17006,         2438,         16788,     1326,      15671,         8156,         11726,     8556,
  3762,         2053,          9563,     1317,      13561,         6790,         12227,     1936,
  8180,         3550,         13287,     1778,      16299,         6599,         16291,     7758,
  8521,         2551,          7225,     2645,      18269,         7489,         16885,     2248,
 17882,         2884,         17265,     3328,       9417,        20162,         11042,     8320,
  1286,          620,          1431,      583,       5993,         2289,          3978,     3626,
  5144,          752,         13409,      830,       5553,         2860,         11764,     5908,
 10737,          560,          5446,      564,      13321,         3008,         11946,     3683,
 19887,          798,          9825,      728,      13663,         8748,          7391,     3053,
  2515,          778,          6050,      833,       6469,         5074,          8305,     2463,
  6141,         1865,         15308,     1262,      14408,         4547,         13663,     4515,
  3137,         2983,          2479,     1259,      15088,         4647,         15382,     2607,
 14492,         2392,         12462,     2537,       7539,         2949,         12909,    12060,
  5468,          684,          3141,      722,       5081,         1274,         12732,     4200,
 15302,          681,          7819,      592,       6534,         2021,         16478,     8737,
 13364,          882,          5397,      899,      14656,         2178,         14741,     4227,
 14270,         1298,         13929,     2029,      15477,         7482,         15815,     4572,
  2521,         2013,          5062,     1804,       5159,         6582,          7130,     3597,
 10920,         1611,         11729,     1708,      16903,         3455,         16268,     6640,
  9306,         1007,          9369,     2106,      19182,         5037,         12441,     4269,
 15919,         1332,         15357,     3512,      11898,        14141,         16101,     6854,
  2010,          737,          3779,      861,      11454,         2880,          3564,     3540,
  9057,         1241,         12391,      896,       8546,         4629,         11561,     5776,
  8129,          589,          8218,      588,      18728,         3755,         12973,     3149,
 15729,          758,         16634,      754,      15222,        11138,         15871,     2208,
  4673,          610,         10218,      678,      15257,         4146,          5729,     3327,
  8377,         1670,         19862,     2321,      15450,         5511,         14054,     5481,
  5728,         2888,          7580,     1346,      14384,         5325,         16236,     3950,
 15118,         3744,         15306,     1435,      14597,         4070,         12301,    15696,
  7617,         1699,          2170,      884,       4459,         4567,         18094,     3306,
 12742,          815,         14926,      907,      15016,         4281,         15518,     8368,
 17994,         1087,          2358,      865,      16281,         3787,         15679,     4596,
 16356,         1534,         16584,     2210,      16833,         9697,         15929,     4513,
  3277,         1085,          9643,     2187,      11973,         6068,          9199,     4462,
  8955,         1629,         10289,     3062,      16481,         5155,         15466,     7066,
 13678,         2543,          5273,     2277,      16746,         6213,         16655,     3408,
 20304,         3363,         18688,     1985,      14172,        12867,         15154,    15703,
  4473,         1020,          1681,      886,       4311,         4301,          8952,     3657,
  5893,         1147,         11647,     1452,      15886,         2227,          4582,     6644,
  6929,         1205,          6220,      799,      12415,         3409,         15968,     3877,
 19859,         2109,          9689,     2141,      14742,         8830,         14480,     2599,
  1817,         1238,          7771,      813,      19079,         4410,          5554,     2064,
  3687,         2844,         17435,     2256,      16697,         4486,         16199,     5388,
  8028,         2763,          3405,     2119,      17426,         5477,         13698,     2786,
 19879,         2720,          9098,     3880,      18172,         4833,         17336,    12207,
  5116,          996,          4935,      988,       9888,         3081,          6014,     5371,
 15881,         1667,          8405,     1183,      15087,         2366,         19777,     7002,
 11963,         1562,          7279,     1128,      16859,         1532,         15762,     5381,
 14708,         2065,         20105,     2155,      17158,         8245,         17911,     6318,
  5467,         1504,          4100,     2574,      17421,         6810,          5673,     2888,
 16636,         3382,          8975,     1831,      20159,         4737,         19550,     7294,
  6658,         2781,         11472,     3321,      19397,         5054,         18878,     4722,
 16439,         2373,         20430,     4386,      11353,        26526,         11593,     3068,
  2866,         1566,          5108,     1070,       9614,         4915,          4939,     3536,
  7541,          878,         20717,      851,       6938,         4395,         16799,     7733,
 10137,         1019,          9845,      964,      15494,         3955,         15459,     3430,
 18863,          982,         20120,      963,      16876,        12887,         14334,     4200,
  6599,         1220,          9222,      814,      16942,         5134,          5661,     4898,
  5488,         1798,         20258,     3962,      17005,         6178,         17929,     5929,
  9365,         3420,          7474,     1971,      19537,         5177,         19003,     3006,
 16454,         3788,         16070,     2367,       8664,         2743,          9445,    26358,
 10856,         1287,          3555,     1009,       5606,         3622,         19453,     5512,
 12453,          797,         20634,      911,      15427,         3066,         17037,    10275,
 18883,         2633,          3913,     1268,      19519,         3371,         18052,     5230,
 19291,         1678,         19508,     3172,      18072,        10754,         16625,     6845,
  3134,         2298,         10869,     2437,      15580,         6913,         12597,     3381,
 11116,         3297,         16762,     2424,      18853,         6715,         17171,     9887,
 12743,         2605,          8937,     3140,      19033,         7764,         18347,     3880,
 20475,         3682,         19602,     3380,      13044,        19373,         10526,    23124
};

/*********************************************************************************
*      Quantization of pitch and codebook gains for MR475.
*********************************************************************************/
static void ownQntStoreResults_M475(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, const short *p,               
                                    short predGainCB, short expGainCodeCB, short *pitchGainVar, short *codeGainVar)
{
    int i;
    short g_code, exp, frac, tmp;
    int TmpCode;

    short qntEnergyErr_M122; 
    short qntEnergyErr;      

    *pitchGainVar = *p++;                
    g_code = *p++;                   

    TmpCode = 2 * g_code * predGainCB;
    if(expGainCodeCB > 10) TmpCode <<= (expGainCodeCB - 10);
    else                   TmpCode >>= (10 - expGainCodeCB);
    *codeGainVar = (short)(TmpCode >> 16);

    ownLog2_GSMAMR (g_code, &exp, &frac); 
    exp -= 12;

    tmp = frac >> 5;
    qntEnergyErr_M122 = tmp + (exp << 10);

    TmpCode = Mul16s_32s(exp, frac, 24660); 
    qntEnergyErr = Cnvrt_NR_32s16s(TmpCode << 13); 

    for (i = 3; i > 0; i--) {
        a_PastQntEnergy[i] = a_PastQntEnergy[i - 1];             
        a_PastQntEnergy_M122[i] = a_PastQntEnergy_M122[i - 1]; 
    }
    a_PastQntEnergy_M122[0] = qntEnergyErr_M122;  
    a_PastQntEnergy[0] = qntEnergyErr;              
}

/*************************************************************************
 * Function:  ownUpdateUnQntPred_M475()
 *************************************************************************/
void ownUpdateUnQntPred_M475(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, short expGainCodeCB,         
                             short fracGainCodeCB, short codGainExp, short codGainFrac)
{
    int i;
    short tmp, exp, frac;
    short qntEnergyErr, qntEnergyErr_M122;
    int TmpQnt;

    if (codGainFrac <= 0) {
        qntEnergyErr = MIN_QUA_ENER;            
        qntEnergyErr_M122 = MIN_QUA_ENER_MR122; 
    } else {
        fracGainCodeCB = (short)(ownPow2_GSMAMR (14, fracGainCodeCB));
        if(codGainFrac >= fracGainCodeCB) {
            codGainFrac >>= 1;
            codGainExp++;
        }

        frac = (codGainFrac << 15)/fracGainCodeCB; 
        tmp = codGainExp - expGainCodeCB - 1;
        ownLog2_GSMAMR (frac, &exp, &frac);
        exp += tmp;

        qntEnergyErr_M122 = (frac >> 5) + (exp << 10);

        if (qntEnergyErr_M122 < MIN_QUA_ENER_MR122) {
            qntEnergyErr = MIN_QUA_ENER;             
            qntEnergyErr_M122 = MIN_QUA_ENER_MR122; 
        } else if (qntEnergyErr_M122 > MAX_QUA_ENER_MR122) {
            qntEnergyErr = MAX_QUA_ENER;             
            qntEnergyErr_M122 = MAX_QUA_ENER_MR122; 
        } else {
            TmpQnt = Mul16s_32s(exp, frac, 24660);
            qntEnergyErr = Cnvrt_NR_32s16s(ShiftL_32s(TmpQnt, 13)); 
        }
    }

    for (i = 3; i > 0; i--) {
        a_PastQntEnergy[i] = a_PastQntEnergy[i - 1];             
        a_PastQntEnergy_M122[i] = a_PastQntEnergy_M122[i - 1]; 
    }
    a_PastQntEnergy_M122[0] = qntEnergyErr_M122;  
    a_PastQntEnergy[0] = qntEnergyErr;             
}


/*************************************************************************
 * Function:  ownGainQnt_M475()
 *************************************************************************/
short ownGainQnt_M475(short *a_PastQntEnergy, short *a_PastQntEnergy_M122, short vExpPredCBGain, 
					  short vFracPredCBGain, short *a_ExpEnCoeff, short *a_FracEnCoeff,        
                      short vExpTargetEnergy, short vFracTargetEnergy, short *codeNoSharpSF1,       
                      short expGainCodeSF1, short fracGainCodeSF1, short *expCoeffSF1, short *fracCoeffSF1,         
                      short expTargetEnergySF1, short fracTargetEnergySF1, short gainPitchLimit,        
                      short *gainPitSF0, short *gainCodeSF0, short *gainPitSF1, short *gainCodeSF1)
{
    const short *p;
    short i, index = 0;
    short tmp;
    short exp;
    short sf0_gcode0, sf1_gcode0;
    short g_pitch, g2_pitch, g_code, g2_code, g_pit_cod;
    short coeff[10], coeff_lo[10], exp_max[10];  /* 0..4: sf0; 5..9: sf1 */
    int TmpVal, dist_min;

    sf0_gcode0 = (short)(ownPow2_GSMAMR(14, vFracPredCBGain));
    sf1_gcode0 = (short)(ownPow2_GSMAMR(14, fracGainCodeSF1));

    exp = vExpPredCBGain - 11;
    exp_max[0] = a_ExpEnCoeff[0] - 13;                        
    exp_max[1] = a_ExpEnCoeff[1] - 14;                        
    exp_max[2] = a_ExpEnCoeff[2] + 15 + (exp << 1);     
    exp_max[3] = a_ExpEnCoeff[3] + exp;                       
    exp_max[4] = a_ExpEnCoeff[4] + 1 + exp;              

    exp = expGainCodeSF1 - 11;
    exp_max[5] = expCoeffSF1[0] - 13;                        
    exp_max[6] = expCoeffSF1[1] - 14;                        
    exp_max[7] = expCoeffSF1[2] +15 + (exp << 1);      
    exp_max[8] = expCoeffSF1[3] + exp;                       
    exp_max[9] = expCoeffSF1[4] + 1 + exp;              

    exp = vExpTargetEnergy - expTargetEnergySF1;
    if (exp > 0) fracTargetEnergySF1 >>= exp;
    else         vFracTargetEnergy >>= -exp;
    
    exp = 0; 
    tmp = fracTargetEnergySF1 >> 1;   
    if (fracTargetEnergySF1 & 1) tmp++;
	if (tmp > vFracTargetEnergy) exp = 1; 
    else {
        tmp = (vFracTargetEnergy + 3) >> 2; 
        if(tmp > fracTargetEnergySF1) exp = -1; 
    }
    
    for (i = 0; i < 5; i++)
        exp_max[i] += exp; 
                                                                       
    exp = exp_max[0];                                        
    for (i = 1; i < 10; i++) {
       if(exp_max[i] > exp) exp = exp_max[i];                                
    }
    exp += 1;      

    p = &a_FracEnCoeff[0]; 
    for (i = 0; i < 5; i++) {
        tmp = exp - exp_max[i];
        TmpVal = *p++ << 16;
        if (tmp >= (short)31) TmpVal = (TmpVal < 0) ? -1 : 0;
        else                  TmpVal >>= tmp;
        L_Extract(TmpVal, &coeff[i], &coeff_lo[i]);
    }
    p = &fracCoeffSF1[0]; 
    for (; i < 10; i++) {
        tmp = exp - exp_max[i];
        TmpVal = *p++ << 16;
        if (tmp >= (short)31) TmpVal = (TmpVal < 0) ? -1 : 0;
        else                  TmpVal >>= tmp;
        L_Extract(TmpVal, &coeff[i], &coeff_lo[i]);
    }

    /* start with "infinite" MSE */
    dist_min = IPP_MAX_32S;        

    p = &table_gain_MR475[0]; 
    for (i = 0; i < VEC_QNT_M475_SIZE; i++) {

        g_pitch = *p++;       
        g_code = *p++;       

        g_code = (short)((g_code * sf0_gcode0) >> 15);
        g2_pitch = (short)((g_pitch * g_pitch) >> 15);
        g2_code = (short)((g_code * g_code) >> 15);
        g_pit_cod = (short)((g_code * g_pitch) >> 15);
        
        TmpVal = Mul16s_32s(       coeff[0], coeff_lo[0], g2_pitch);
        TmpVal = AddProduct16s_32s(TmpVal, coeff[1], coeff_lo[1], g_pitch);
        TmpVal = AddProduct16s_32s(TmpVal, coeff[2], coeff_lo[2], g2_code);
        TmpVal = AddProduct16s_32s(TmpVal, coeff[3], coeff_lo[3], g_code);
        TmpVal = AddProduct16s_32s(TmpVal, coeff[4], coeff_lo[4], g_pit_cod);

        tmp = g_pitch - gainPitchLimit;

        g_pitch = *p++;      
        g_code = *p++;       

        if(tmp <= 0 && g_pitch <= gainPitchLimit)  {
            g_code = (short)((g_code * sf1_gcode0) >> 15);
            g2_pitch = (short)((g_pitch * g_pitch) >> 15);
            g2_code = (short)((g_code * g_code) >> 15);
            g_pit_cod = (short)((g_code * g_pitch) >> 15);
            
            TmpVal = AddProduct16s_32s(TmpVal, coeff[5], coeff_lo[5], g2_pitch);
            TmpVal = AddProduct16s_32s(TmpVal, coeff[6], coeff_lo[6], g_pitch);
            TmpVal = AddProduct16s_32s(TmpVal, coeff[7], coeff_lo[7], g2_code);
            TmpVal = AddProduct16s_32s(TmpVal, coeff[8], coeff_lo[8], g_code);
            TmpVal = AddProduct16s_32s(TmpVal, coeff[9], coeff_lo[9], g_pit_cod);
            
            if(TmpVal < dist_min)  {
                dist_min = TmpVal; 
                index = i;        
            }
        }
    }

    tmp = index << 2;
    ownQntStoreResults_M475(a_PastQntEnergy, a_PastQntEnergy_M122, &table_gain_MR475[tmp],
                            sf0_gcode0, vExpPredCBGain, gainPitSF0, gainCodeSF0);

    ownPredEnergyMA_GSMAMR(a_PastQntEnergy,a_PastQntEnergy_M122, IPP_SPCHBR_4750, codeNoSharpSF1,
                           &expGainCodeSF1, &fracGainCodeSF1, &vExpPredCBGain, &sf0_gcode0); 

    sf1_gcode0 = (short)(ownPow2_GSMAMR(14, fracGainCodeSF1));
    tmp += 2;

    ownQntStoreResults_M475(a_PastQntEnergy, a_PastQntEnergy_M122, &table_gain_MR475[tmp],
                            sf1_gcode0, expGainCodeSF1, gainPitSF1, gainCodeSF1);
    return index;
}
/*************************************************************************
 * Function:  ownPreQnt3CodebookGain_M795
 *************************************************************************/
static void ownPreQnt3CodebookGain_M795(short expGainCodeCB, short predGainCB, short *pitchGainCand,                 
                                        short *pitchGainCandIdx, short *fracCoeff, short *expCoeff,                      
                                        short *pitchGainVar, short *pitchGainIdx, short *codeGainVar,                   
                                        short *codeGainIdx, short *qntEnergyErr_M122, short *qntEnergyErr)
{
    const short *p;
    short i, j, cod_ind, pit_ind;
    short e_max, exp_code;
    short g_pitch, g2_pitch, g_code, g2_code_h, g2_code_l;
    short g_pit_cod_h, g_pit_cod_l;
    short coeff[5], coeff_lo[5];
    short exp_max[5];
    int TmpVal, TmpVal0, dist_min;

    exp_code = expGainCodeCB - 10;
    exp_max[0] = expCoeff[0] - 13;                        
    exp_max[1] = expCoeff[1] - 14;                        
    exp_max[2] = expCoeff[2] + 15 + (exp_code << 1); 
    exp_max[3] = expCoeff[3] + exp_code;                  
    exp_max[4] = expCoeff[4] + exp_code + 1;           

    e_max = exp_max[0];                                        
    for(i = 1; i < 5; i++) {
       if (exp_max[i] > e_max) e_max = exp_max[i];                               
    }

    e_max++;      
    for (i = 0; i < 5; i++) {
        j = e_max - exp_max[i];
        TmpVal = fracCoeff[i] << 16;
        if (j >= (short)31) TmpVal = (TmpVal < 0) ? -1 : 0;
        else                TmpVal >>= j;
        L_Extract(TmpVal, &coeff[i], &coeff_lo[i]);
    }

    dist_min = IPP_MAX_32S;        
    cod_ind = 0;              
    pit_ind = 0;              

    for (j = 0; j < 3; j++) {
        g_pitch = pitchGainCand[j];    
        g2_pitch = (short)((g_pitch * g_pitch) >> 15);
        TmpVal0 = Mul16s_32s(coeff[0], coeff_lo[0], g2_pitch);
        TmpVal0 = AddProduct16s_32s(TmpVal0, coeff[1], coeff_lo[1], g_pitch);

        p = &pTblQntGainCode[0];
        for (i = 0; i < LEN_QNT_GAIN; i++)  {
            g_code = *p++;                  
            p++;                            
            p++;                            

            g_code = (short)((g_code * predGainCB) >> 15);

            TmpVal = 2 * g_code * g_code;
            L_Extract (TmpVal, &g2_code_h, &g2_code_l);

            TmpVal = 2 * g_code * g_pitch;
            L_Extract (TmpVal, &g_pit_cod_h, &g_pit_cod_l);

            TmpVal = AddProduct_32s(TmpVal0, coeff[2], coeff_lo[2], g2_code_h, g2_code_l);
            TmpVal = AddProduct16s_32s(TmpVal, coeff[3], coeff_lo[3], g_code);
            TmpVal = AddProduct_32s(TmpVal, coeff[4], coeff_lo[4], g_pit_cod_h, g_pit_cod_l);

            if (TmpVal < dist_min) {
                dist_min = TmpVal;                
                cod_ind = i;                    
                pit_ind = j;                     
            }
        }
    }

    p = &pTblQntGainCode[3* cod_ind]; 
    g_code = *p++;            
    *qntEnergyErr_M122 = *p++;  
    *qntEnergyErr = *p;           

    TmpVal = 2 * g_code * predGainCB;
    if (expGainCodeCB > 9) TmpVal <<= (expGainCodeCB - 9);
    else                   TmpVal >>= (9 - expGainCodeCB);
    *codeGainVar = (short)(TmpVal >> 16);
    *codeGainIdx = cod_ind;                
    *pitchGainVar = pitchGainCand[pit_ind];      
    *pitchGainIdx = pitchGainCandIdx[pit_ind];  
}


/*************************************************************************
 * Function:  ownGainCodebookQnt_M795
 *************************************************************************/
static short ownGainCodebookQnt_M795(short pitchGainVar, short expGainCodeCB, short predGainCB,                   
                                     short *fracEnergyCoeff, short *expEnergyCoeff, short alpha,                        
                                     short gainCodeUnQnt, short *codeGainVar, short *qntEnergyErr_M122,           
                                     short *qntEnergyErr)
{
    const short *p;
    short i, index, tmp;
    short one_alpha;
    short exp, e_max;
    short g2_pitch, g_code;
    short g2_code_h, g2_code_l;
    short d2_code_h, d2_code_l;
    short coeff[5], coeff_lo[5], expCoeff[5];
    int TmpVal, Time0, Time1, dist_min;
    short decGainCode;


    /* calculate scalings of the constant terms */
    if (expGainCodeCB > 10)
        decGainCode = Cnvrt_32s16s(*codeGainVar >> (expGainCodeCB - 10));
    else
        decGainCode = Cnvrt_32s16s(*codeGainVar << (10 - expGainCodeCB));
    g2_pitch = (short)((pitchGainVar * pitchGainVar) >> 15);             
    one_alpha = 32767 - alpha + 1; 

    tmp = (short)((alpha * fracEnergyCoeff[1]) >> 14);
    Time1 = 2 * tmp * g2_pitch;                    
    expCoeff[1] = expEnergyCoeff[1] - 15;               


    tmp = (short)((alpha * fracEnergyCoeff[2]) >> 14);
    coeff[2] = (short)((tmp * pitchGainVar) >> 15);                  
    exp = expGainCodeCB - 10;
    expCoeff[2] = expEnergyCoeff[2] + exp;              

    coeff[3] = (short)((alpha * fracEnergyCoeff[3]) >> 14);
    exp = (expGainCodeCB << 1) - 7;
    expCoeff[3] = expEnergyCoeff[3] + exp;              

    coeff[4] = (short)((one_alpha * fracEnergyCoeff[3]) >> 15);          
    expCoeff[4] = expCoeff[3] + 1;             


    TmpVal = 2 * alpha * fracEnergyCoeff[0];
    Time0 = ownSqrt_Exp_GSMAMR(TmpVal, &exp); 
    exp += 47;
    expCoeff[0] = expEnergyCoeff[0] - exp;              

    e_max = expCoeff[0] + 31;
    for (i = 1; i <= 4; i++)  {
        if (expCoeff[i] > e_max) e_max = expCoeff[i];                     
    }

    tmp = e_max - expCoeff[1];
    Time1 >>= tmp;

    for (i = 2; i <= 4; i++) {
        tmp = e_max - expCoeff[i];
        TmpVal = coeff[i] << 16;
        TmpVal >>= tmp;
        L_Extract(TmpVal, &coeff[i], &coeff_lo[i]);
    }

    exp = e_max - 31;             /* new exponent */
    tmp = exp - expCoeff[0];
    Time0 >>= (tmp >> 1);

    if((tmp & 0x1) != 0) {
        L_Extract(Time0, &coeff[0], &coeff_lo[0]);
        Time0 = Mul16s_32s(coeff[0], coeff_lo[0], 23170); 
    }

    dist_min = IPP_MAX_32S; 
    index = 0;         
    p = &pTblQntGainCode[0]; 

    for (i = 0; i < LEN_QNT_GAIN; i++) {
        g_code = *p;
		p+=3;
        g_code = (short)((g_code * predGainCB) >> 15);

        if(g_code >= decGainCode) break;

        TmpVal = 2 * g_code * g_code;
        L_Extract (TmpVal, &g2_code_h, &g2_code_l);

        tmp = g_code - gainCodeUnQnt;
        TmpVal = 2 * tmp * tmp;
        L_Extract (TmpVal, &d2_code_h, &d2_code_l);

        TmpVal = AddProduct16s_32s (Time1, coeff[2], coeff_lo[2], g_code);
        TmpVal = AddProduct_32s(TmpVal, coeff[3], coeff_lo[3], g2_code_h, g2_code_l);

        TmpVal = ownSqrt_Exp_GSMAMR(TmpVal, &exp);
        TmpVal >>= (exp >> 1);

        tmp = Cnvrt_NR_32s16s(TmpVal - Time0);
        TmpVal = 2 * tmp * tmp;

        TmpVal = AddProduct_32s(TmpVal, coeff[4], coeff_lo[4], d2_code_h, d2_code_l);

        if (TmpVal < dist_min) {
            dist_min = TmpVal;                
            index = i;                       
        }
    }

    p = &pTblQntGainCode[3 * index]; 
    g_code = *p++;            
    *qntEnergyErr_M122 = *p++;   
    *qntEnergyErr = *p;           

    TmpVal = 2 * g_code * predGainCB;
    if (expGainCodeCB > 9) TmpVal <<= (expGainCodeCB - 9);
    else                   TmpVal >>= (9 - expGainCodeCB);
    *codeGainVar = (short)(TmpVal >> 16);

    return index;
}

/*************************************************************************
 * Function:  ownGainQuant_M795_GSMAMR
 *************************************************************************/
void ownGainQuant_M795_GSMAMR(short *vOnSetQntGain, short *vPrevAdaptOut, short *vPrevGainZero, short *a_LTPHistoryGain, 
                              short *pLTPRes, short *pLTPExc, short *code, short *fracCoeff, short *expCoeff, short expCodeEnergy,     
                              short fracCodeEnergy, short expGainCodeCB, short fracGainCodeCB,  short L_subfr, short codGainFrac,       
                              short codGainExp, short gainPitchLimit, short *pitchGainVar, short *codeGainVar,
					          short *qntEnergyErr_M122, short *qntEnergyErr, short **ppAnalysisParam)
{
    short fracEnergyCoeff[4];
    short expEnergyCoeff[4];
    short ltpg, alpha, predGainCB;
    short pitchGainCand[3];      
    short pitchGainCandIdx[3];      
    short gain_pit_index;
    short gain_cod_index;
    short exp;
    short gainCodeUnQnt;         

    gain_pit_index = ownQntGainPitch_M7950_GSMAMR (gainPitchLimit, pitchGainVar,pitchGainCand, pitchGainCandIdx); 
    predGainCB = (short)(ownPow2_GSMAMR(14, fracGainCodeCB));          /* Q14 */

    ownPreQnt3CodebookGain_M795(expGainCodeCB, predGainCB, pitchGainCand, pitchGainCandIdx,
                                fracCoeff, expCoeff, pitchGainVar, &gain_pit_index, codeGainVar, 
								&gain_cod_index, qntEnergyErr_M122, qntEnergyErr);

    ownCalcUnFiltEnergy_GSMAMR(pLTPRes, pLTPExc, code, *pitchGainVar, L_subfr, fracEnergyCoeff, expEnergyCoeff, &ltpg);

    ownGainAdaptAlpha_GSMAMR(vOnSetQntGain,vPrevAdaptOut,vPrevGainZero,a_LTPHistoryGain, ltpg, *codeGainVar, &alpha);

    if (fracEnergyCoeff[0] != 0 && alpha > 0)  {
        fracEnergyCoeff[3] = fracCodeEnergy;          
        expEnergyCoeff[3] = expCodeEnergy;            
       
        exp = codGainExp - expGainCodeCB + 10;
        if (exp < 0) gainCodeUnQnt = codGainFrac >> -exp;
        else         gainCodeUnQnt = ShiftL_16s(codGainFrac, exp);
        
        gain_cod_index = ownGainCodebookQnt_M795(*pitchGainVar, expGainCodeCB, predGainCB,
                                                 fracEnergyCoeff, expEnergyCoeff, alpha, gainCodeUnQnt,
                                                 codeGainVar, qntEnergyErr_M122, qntEnergyErr); 
    }

    *(*ppAnalysisParam)++ = gain_pit_index;       
    *(*ppAnalysisParam)++ = gain_cod_index;        
}


static const short TableGainHighRates[VEC_QNT_HIGHRATES_SIZE*4] = {
    577,      662,     -2692,   -16214,      806,     1836,     -1185,    -7135,
   3109,     1052,     -2008,   -12086,     4181,     1387,     -1600,    -9629,
   2373,     1425,     -1560,    -9394,     3248,     1985,     -1070,    -6442,
   1827,     2320,      -840,    -5056,      941,     3314,      -313,    -1885,
   2351,     2977,      -471,    -2838,     3616,     2420,      -777,    -4681,
   3451,     3096,      -414,    -2490,     2955,     4301,        72,      434,
   1848,     4500,       139,      836,     3884,     5416,       413,     2484,
   1187,     7210,       835,     5030,     3083,     9000,      1163,     7002,
   7384,      883,     -2267,   -13647,     5962,     1506,     -1478,    -8900,
   5155,     2134,      -963,    -5800,     7944,     2009,     -1052,    -6335,
   6507,     2250,      -885,    -5327,     7670,     2752,      -588,    -3537,
   5952,     3016,      -452,    -2724,     4898,     3764,      -125,     -751,
   6989,     3588,      -196,    -1177,     8174,     3978,       -43,     -260,
   6064,     4404,       107,      645,     7709,     5087,       320,     1928,
   5523,     6021,       569,     3426,     7769,     7126,       818,     4926,
   6060,     7938,       977,     5885,     5594,    11487,      1523,     9172,
  10581,     1356,     -1633,    -9831,     9049,     1597,     -1391,    -8380,
   9794,     2035,     -1033,    -6220,     8946,     2415,      -780,    -4700,
  10296,     2584,      -681,    -4099,     9407,     2734,      -597,    -3595,
   8700,     3218,      -356,    -2144,     9757,     3395,      -277,    -1669,
  10177,     3892,       -75,     -454,     9170,     4528,       148,      891,
  10152,     5004,       296,     1781,     9114,     5735,       497,     2993,
  10500,     6266,       628,     3782,    10110,     7631,       919,     5534,
   8844,     8727,      1117,     6728,     8956,    12496,      1648,     9921,
  12924,      976,     -2119,   -12753,    11435,     1755,     -1252,    -7539,
  12138,     2328,      -835,    -5024,    11388,     2368,      -810,    -4872,
  10700,     3064,      -429,    -2580,    12332,     2861,      -530,    -3192,
  11722,     3327,      -307,    -1848,    11270,     3700,      -150,     -904,
  10861,     4413,       110,      663,    12082,     4533,       150,      902,
  11283,     5205,       354,     2132,    11960,     6305,       637,     3837,
  11167,     7534,       900,     5420,    12128,     8329,      1049,     6312,
  10969,    10777,      1429,     8604,    10300,    17376,      2135,    12853,
  13899,     1681,     -1316,    -7921,    12580,     2045,     -1026,    -6179,
  13265,     2439,      -766,    -4610,    14033,     2989,      -465,    -2802,
  13452,     3098,      -413,    -2482,    12396,     3658,      -167,    -1006,
  13510,     3780,      -119,     -713,    12880,     4272,        62,      374,
  13533,     4861,       253,     1523,    12667,     5457,       424,     2552,
  13854,     6106,       590,     3551,    13031,     6483,       678,     4084,
  13557,     7721,       937,     5639,    12957,     9311,      1213,     7304,
  13714,    11551,      1532,     9221,    12591,    15206,      1938,    11667,
  15113,     1540,     -1445,    -8700,    15072,     2333,      -832,    -5007,
  14527,     2511,      -723,    -4352,    14692,     3199,      -365,    -2197,
  15382,     3560,      -207,    -1247,    14133,     3960,       -50,     -300,
  15102,     4236,        50,      298,    14332,     4824,       242,     1454,
  14846,     5451,       422,     2542,    15306,     6083,       584,     3518,
  14329,     6888,       768,     4623,    15060,     7689,       930,     5602,
  14406,     9426,      1231,     7413,    15387,     9741,      1280,     7706,
  14824,    14271,      1844,    11102,    13600,    24939,      2669,    16067,
  16396,     1969,     -1082,    -6517,    16817,     2832,      -545,    -3283,
  15713,     2843,      -539,    -3248,    16104,     3336,      -303,    -1825,
  16384,     3963,       -49,     -294,    16940,     4579,       165,      992,
  15711,     4599,       171,     1030,    16222,     5448,       421,     2537,
  16832,     6382,       655,     3945,    15745,     7141,       821,     4944,
  16326,     7469,       888,     5343,    16611,     8624,      1100,     6622,
  17028,    10418,      1379,     8303,    15905,    11817,      1565,     9423,
  16878,    14690,      1887,    11360,    16515,    20870,      2406,    14483,
  18142,     2083,      -999,    -6013,    19401,     3178,      -375,    -2257,
  17508,     3426,      -264,    -1589,    20054,     4027,       -25,     -151,
  18069,     4249,        54,      326,    18952,     5066,       314,     1890,
  17711,     5402,       409,     2461,    19835,     6192,       610,     3676,
  17950,     7014,       795,     4784,    21318,     7877,       966,     5816,
  17910,     9289,      1210,     7283,    19144,     9290,      1210,     7284,
  20517,    11381,      1510,     9089,    18075,    14485,      1866,    11234,
  19999,    17882,      2177,    13108,    18842,    32764,      3072,    18494
};

static const short TableGainLowRates[VEC_QNT_LOWRATES_SIZE*4] = {
  10813,    28753,      2879,    17333,    20480,     2785,      -570,    -3431,
  18841,     6594,       703,     4235,     6225,     7413,       876,     5276,
  17203,    10444,      1383,     8325,    21626,     1269,     -1731,   -10422,
  21135,     4423,       113,      683,    11304,     1556,     -1430,    -8609,
  19005,    12820,      1686,    10148,    17367,     2498,      -731,    -4398,
  17858,     4833,       244,     1472,     9994,     2498,      -731,    -4398,
  17530,     7864,       964,     5802,    14254,     1884,     -1147,    -6907,
  15892,     3153,      -387,    -2327,     6717,     1802,     -1213,    -7303,
  18186,    20193,      2357,    14189,    18022,     3031,      -445,    -2678,
  16711,     5857,       528,     3181,     8847,     4014,       -30,     -180,
  15892,     8970,      1158,     6972,    18022,     1392,     -1594,    -9599,
  16711,     4096,         0,        0,     8192,      655,     -2708,   -16305,
  15237,    13926,      1808,    10884,    14254,     3112,      -406,    -2444,
  14090,     4669,       193,     1165,     5406,     2703,      -614,    -3697,
  13434,     6553,       694,     4180,    12451,      901,     -2237,   -13468,
  12451,     2662,      -637,    -3833,     3768,      655,     -2708,   -16305,
  14745,    23511,      2582,    15543,    19169,     2457,      -755,    -4546,
  20152,     5079,       318,     1913,     6881,     4096,         0,        0,
  20480,     8560,      1089,     6556,    19660,      737,     -2534,   -15255,
  19005,     4259,        58,      347,     7864,     2088,      -995,    -5993,
  11468,    12288,      1623,     9771,    15892,     1474,     -1510,    -9090,
  15728,     4628,       180,     1086,     9175,     1433,     -1552,    -9341,
  16056,     7004,       793,     4772,    14827,      737,     -2534,   -15255,
  15073,     2252,      -884,    -5321,     5079,     1228,     -1780,   -10714,
  13271,    17326,      2131,    12827,    16547,     2334,      -831,    -5002,
  15073,     5816,       518,     3118,     3932,     3686,      -156,     -938,
  14254,     8601,      1096,     6598,    16875,      778,     -2454,   -14774,
  15073,     3809,      -107,     -646,     6062,      614,     -2804,   -16879,
   9338,     9256,      1204,     7251,    13271,     1761,     -1247,    -7508,
  13271,     3522,      -223,    -1343,     2457,     1966,     -1084,    -6529,
  11468,     5529,       443,     2668,    10485,      737,     -2534,   -15255,
  11632,     3194,      -367,    -2212,     1474,      778,     -2454,   -14774
};
/*************************************************************************
 * Function: ownGainQntInward_GSMAMR()
 *************************************************************************/
short ownGainQntInward_GSMAMR(IppSpchBitRate rate, short expGainCodeCB, short fracGainCodeCB,      
                              short *fracCoeff, short *expCoeff, short gainPitchLimit, short *pitchGainVar,       
                              short *codeGainVar, short *qntEnergyErr_M122, short *qntEnergyErr)
{
    const short *p;
    short i, j, index = 0;
    short predGainCB, e_max, exp_code;
    short g_pitch, g2_pitch, g_code, g2_code, g_pit_cod;
    short coeff[5], coeff_lo[5];
    short exp_max[5];
    int TmpVal, dist_min;
    const short *table_gain;
    short table_len;
    
    if (rate == IPP_SPCHBR_10200 || rate == IPP_SPCHBR_7400 || rate == IPP_SPCHBR_6700) {
       table_len = VEC_QNT_HIGHRATES_SIZE;            
       table_gain = TableGainHighRates;        
    } else {
       table_len = VEC_QNT_LOWRATES_SIZE;             
       table_gain = TableGainLowRates;         
    }
    
    predGainCB = (short)(ownPow2_GSMAMR(14, fracGainCodeCB));
    exp_code = expGainCodeCB - 11;

    exp_max[0] = expCoeff[0] - 13;                        
    exp_max[1] = expCoeff[1] - 14;                        
    exp_max[2] = expCoeff[2] + 15 + (exp_code << 1);
    exp_max[3] = expCoeff[3] + exp_code;                  
    exp_max[4] = expCoeff[4] + 1 + exp_code;          

    e_max = exp_max[0];                                        
    for(i = 1; i < 5; i++) {
       if (exp_max[i] > e_max) e_max = exp_max[i];                                
    }
    e_max++; 

    for (i = 0; i < 5; i++) {
        j = e_max - exp_max[i];
        TmpVal = fracCoeff[i] << 16;
        if (j >= (short)31) TmpVal = (TmpVal < 0) ? -1 : 0; 
        else                TmpVal >>= j;
        L_Extract(TmpVal, &coeff[i], &coeff_lo[i]);
    }


    dist_min = IPP_MAX_32S;        
    p = &table_gain[0];       

    for (i = 0; i < table_len; i++) {
        g_pitch = *p++;       
        g_code = *p++;                   
		p+=2;
            
        if (g_pitch <= gainPitchLimit) {
            g_code = (short)((g_code * predGainCB) >> 15);
            g2_pitch = (short)((g_pitch * g_pitch) >> 15);
            g2_code = (short)((g_code * g_code) >> 15);
            g_pit_cod = (short)((g_code * g_pitch) >> 15);

            TmpVal = Mul16s_32s(coeff[0], coeff_lo[0], g2_pitch);
            TmpVal += Mul16s_32s(coeff[1], coeff_lo[1], g_pitch);
            TmpVal += Mul16s_32s(coeff[2], coeff_lo[2], g2_code);
            TmpVal += Mul16s_32s(coeff[3], coeff_lo[3], g_code);
            TmpVal += Mul16s_32s(coeff[4], coeff_lo[4], g_pit_cod);

            if (TmpVal < dist_min) {
                dist_min = TmpVal; 
                index = i;        
            }
        }
    }

    p = &table_gain[index << 2]; 
    *pitchGainVar = *p++;         
    g_code = *p++;           
    *qntEnergyErr_M122 = *p++;   
    *qntEnergyErr = *p;           

    TmpVal = 2 * g_code * predGainCB;
    if (expGainCodeCB > 10) TmpVal = ShiftL_32s(TmpVal, (unsigned short)(expGainCodeCB - 10));
    else                    TmpVal >>= (10 - expGainCodeCB);
    *codeGainVar = (short)(TmpVal >> 16);

    return index;
}


/********************************************************************************
*  Function: ownGainQuant_GSMAMR:    Quantization of gains
*********************************************************************************/
int ownGainQuant_GSMAMR(sGainQuantSt *st, IppSpchBitRate rate, short *pLTPRes, short *pLTPExc,          
                        short *code, short *pTargetVec, short *pTargetVec2, short *pAdaptCode,      
						short *pFltVector, short even_subframe, short gainPitchLimit, short *gainPitSF0,       
                        short *gainCodeSF0, short *pitchGainVar, short *codeGainVar, short **ppAnalysisParam)
{
    int i;
    short expGainCodeCB;
    short fracGainCodeCB;
    short qntEnergyErr_M122;
    short qntEnergyErr;
    short fracCoeff[5];
    short expCoeff[5];
    short expEnergyCoeff, fracEnergyCoeff;
    short codGainExp, codGainFrac;
            
    if (rate == IPP_SPCHBR_4750) {
        if (even_subframe != 0) {

            st->pGainPtr = (*ppAnalysisParam)++;
            ippsCopy_16s(st->a_PastQntEnergy, st->a_PastUnQntEnergy, NUM_PRED_TAPS);
            ippsCopy_16s(st->a_PastQntEnergy_M122, st->a_PastUnQntEnergy_M122, NUM_PRED_TAPS);
            
            ownPredEnergyMA_GSMAMR(st->a_PastUnQntEnergy,st->a_PastUnQntEnergy_M122, rate, code,
                    &st->vExpPredCBGain, &st->vFracPredCBGain, &expEnergyCoeff, &fracEnergyCoeff);
            
            ownCalcFiltEnergy_GSMAMR(rate, pTargetVec, pTargetVec2, pAdaptCode, pFltVector, st->a_FracEnCoeff, st->a_ExpEnCoeff,
                                     &codGainFrac, &codGainExp);

            if ((codGainExp + 1) > 0) 
                *codeGainVar = Cnvrt_32s16s(codGainFrac << (codGainExp + 1));
            else
                *codeGainVar = Cnvrt_32s16s(codGainFrac >> -(codGainExp + 1));
                                                        
            ownCalcTargetEnergy_GSMAMR(pTargetVec, &st->vExpTargetEnergy, &st->vFracTargetEnergy);

            ownUpdateUnQntPred_M475(st->a_PastUnQntEnergy,st->a_PastUnQntEnergy_M122,
                                  st->vExpPredCBGain, st->vFracPredCBGain,
                                  codGainExp, codGainFrac);

        } else {

            ownPredEnergyMA_GSMAMR(st->a_PastUnQntEnergy,st->a_PastUnQntEnergy_M122, rate, code,
                    &expGainCodeCB, &fracGainCodeCB, &expEnergyCoeff, &fracEnergyCoeff);
            
            ownCalcFiltEnergy_GSMAMR(rate, pTargetVec, pTargetVec2, pAdaptCode, pFltVector, 
                               fracCoeff, expCoeff, &codGainFrac, &codGainExp);

            ownCalcTargetEnergy_GSMAMR(pTargetVec, &expEnergyCoeff, &fracEnergyCoeff);

            *st->pGainPtr = ownGainQnt_M475(st->a_PastQntEnergy, st->a_PastQntEnergy_M122, st->vExpPredCBGain, 
				                            st->vFracPredCBGain, st->a_ExpEnCoeff,  st->a_FracEnCoeff,
                                            st->vExpTargetEnergy, st->vFracTargetEnergy, code, expGainCodeCB, 
											fracGainCodeCB, expCoeff, fracCoeff, expEnergyCoeff, fracEnergyCoeff, 
											gainPitchLimit, gainPitSF0, gainCodeSF0, pitchGainVar, codeGainVar);
        }
    } else {

        ownPredEnergyMA_GSMAMR(st->a_PastQntEnergy,st->a_PastQntEnergy_M122, rate, code, &expGainCodeCB, &fracGainCodeCB,
                               &expEnergyCoeff, &fracEnergyCoeff);
        
        if (rate == IPP_SPCHBR_12200) {
            *codeGainVar = ownComputeCodebookGain_GSMAMR(pTargetVec2, pFltVector);
            *(*ppAnalysisParam)++ = ownQntGainCodebook_GSMAMR(rate, expGainCodeCB, fracGainCodeCB,
                                                              codeGainVar, &qntEnergyErr_M122, &qntEnergyErr);
        } else {

            ownCalcFiltEnergy_GSMAMR(rate, pTargetVec, pTargetVec2, pAdaptCode, pFltVector, 
                                     fracCoeff, expCoeff, &codGainFrac, &codGainExp);
            
            if (rate == IPP_SPCHBR_7950) {
                ownGainQuant_M795_GSMAMR(&st->vOnSetQntGain,&st->vPrevAdaptOut,&st->vPrevGainCode,st->a_LTPHistoryGain, pLTPRes, pLTPExc, code,
                                         fracCoeff, expCoeff, expEnergyCoeff, fracEnergyCoeff, expGainCodeCB, fracGainCodeCB, SUBFR_SIZE_GSMAMR,
                                         codGainFrac, codGainExp, gainPitchLimit, pitchGainVar, codeGainVar,
                                         &qntEnergyErr_M122, &qntEnergyErr, ppAnalysisParam);
            } else {
                *(*ppAnalysisParam)++ = ownGainQntInward_GSMAMR(rate, expGainCodeCB, fracGainCodeCB,
                                                                fracCoeff, expCoeff, gainPitchLimit,
                                                                pitchGainVar, codeGainVar,
                                                                &qntEnergyErr_M122, &qntEnergyErr);
            }
        }
        
        for (i = 3; i > 0; i--) {
            st->a_PastQntEnergy[i] = st->a_PastQntEnergy[i - 1];             
            st->a_PastQntEnergy_M122[i] = st->a_PastQntEnergy_M122[i - 1]; 
        } 
        st->a_PastQntEnergy_M122[0] = qntEnergyErr_M122; 
        st->a_PastQntEnergy[0] = qntEnergyErr;           
    }
        
    return 0;
	/* End of ownGainQuant_GSMAMR() */
}
/*
**************************************************************************
*  Function    : ownDecodeFixedCodebookGain_GSMAMR
**************************************************************************
*/
void ownDecodeFixedCodebookGain_GSMAMR(short *a_PastQntEnergy, short *a_PastQntEnergy_M122,
									   IppSpchBitRate rate, short index, short *code, short *decGainCode)
{
    int i;
    short predGainCB, exp, frac;
    const short *p;
    short qntEnergyErr_M122, qntEnergyErr;
    short exp_inn_en;
    short frac_inn_en;
    int TmpVal;
    
    ownPredEnergyMA_GSMAMR(a_PastQntEnergy,a_PastQntEnergy_M122, rate, code, &exp, &frac,
            &exp_inn_en, &frac_inn_en);
    
    p = &pTblQntGainCode[index*3];

    if (rate == IPP_SPCHBR_12200) {
        predGainCB = (short)(ownPow2_GSMAMR (exp, frac));  
        predGainCB = Cnvrt_32s16s(predGainCB << 4);
        *decGainCode = Cnvrt_32s16s((predGainCB * *p++) >> 15) << 1;  
    } else {
        predGainCB = (short)(ownPow2_GSMAMR (14, frac)); 
        TmpVal = 2 * *p++ * predGainCB;
        if (exp > 9) TmpVal <<= (exp - 9);
        else         TmpVal >>= (9 - exp);
        *decGainCode = (short)(TmpVal >> 16);          
    }
    
    qntEnergyErr_M122 = *p++;                
    qntEnergyErr = *p++;                      
    for (i = 3; i > 0; i--) {
        a_PastQntEnergy[i] = a_PastQntEnergy[i - 1];             
        a_PastQntEnergy_M122[i] = a_PastQntEnergy_M122[i - 1]; 
    }
    a_PastQntEnergy_M122[0] = qntEnergyErr_M122;  
    a_PastQntEnergy[0] = qntEnergyErr;            

    return;
}
/*************************************************************************
 *   Function:  ownDecodeCodebookGains_GSMAMR()
 ************************************************************************/
void ownDecodeCodebookGains_GSMAMR(short *a_PastQntEnergy, short *a_PastQntEnergy_M122,
                                   IppSpchBitRate rate, short index, short *code,                   
                                   short evenSubfr, short * pitchGainVar, short * codeGainVar)
{
    int i;
    const short *p;
    short frac, predGainCB, exp; 
	short qntEnergyErr, qntEnergyErr_M122;
    short g_code;
    int TmpVal;
    
    index <<= 2;
    
    if ((rate == IPP_SPCHBR_10200) || (rate == IPP_SPCHBR_7400) || (rate == IPP_SPCHBR_6700)) {
		p = &TableGainHighRates[index];                  
        *pitchGainVar = *p++;                                  
        g_code = *p++;                                     
        qntEnergyErr_M122 = *p++;                             
        qntEnergyErr = *p;                                     

    } else {
        if (rate == IPP_SPCHBR_4750) {
            index += (1 - evenSubfr) << 1;
            p = &table_gain_MR475[index];                  
            
            *pitchGainVar = *p++;                              
            g_code = *p++;                                 
            
            ownLog2_GSMAMR (g_code, &exp, &frac); 
            exp -= 12;    
            qntEnergyErr_M122 = (frac >> 5) + (exp << 10);
    
            TmpVal = Mul16s_32s(exp, frac, 24660);
            qntEnergyErr = Cnvrt_NR_32s16s(ShiftL_32s(TmpVal, 13)); 
        
		} else {
            p = &TableGainLowRates[index];                            
            *pitchGainVar = *p++;                               
            g_code = *p++;                                  
            qntEnergyErr_M122 = *p++;                          
            qntEnergyErr = *p;                                 
        }
    }
    
    ownPredEnergyMA_GSMAMR(a_PastQntEnergy,a_PastQntEnergy_M122, rate, code, &exp, &frac, NULL, NULL);
    predGainCB = (short)(ownPow2_GSMAMR(14, frac));

    TmpVal = 2 * g_code * predGainCB;
    if(exp > 10) TmpVal = ShiftL_32s(TmpVal, (unsigned short)(exp - 10));
    else         TmpVal >>= (10 - exp);
    *codeGainVar = (short)(TmpVal >> 16);

    for (i = 3; i > 0; i--) {
        a_PastQntEnergy[i] = a_PastQntEnergy[i - 1];             
        a_PastQntEnergy_M122[i] = a_PastQntEnergy_M122[i - 1]; 
    }
    a_PastQntEnergy_M122[0] = qntEnergyErr_M122;  
    a_PastQntEnergy[0] = qntEnergyErr;            

    return;
	/* End of ownDecodeCodebookGains_GSMAMR() */
}
/*
**************************************************************************
*  Function    : ownConcealCodebookGain_GSMAMR
**************************************************************************
*/
void ownConcealCodebookGain_GSMAMR(short *a_GainBuffer, short vPastGainCode, short *a_PastQntEnergy,
                                   short *a_PastQntEnergy_M122, short state, short *decGainCode)
{
    static const short cdown[7] = {32767, 32112, 32112, 32112, 32112, 32112, 22937};
    int i;
    short tmp;
    short qntEnergyErr_M122;
    short qntEnergyErr;
    
    tmp = ownGetMedianElements_GSMAMR(a_GainBuffer,5);                                
    if(tmp > vPastGainCode) tmp = vPastGainCode;                               
    tmp = (short)((tmp * cdown[state]) >> 15);
    *decGainCode = tmp;                                         

    ippsSum_16s_Sfs((const Ipp16s*)a_PastQntEnergy_M122, NUM_PRED_TAPS, &qntEnergyErr_M122, 2);
    if (qntEnergyErr_M122 < MIN_ENERGY_M122) qntEnergyErr_M122 = MIN_ENERGY_M122;

    ippsSum_16s_Sfs((const Ipp16s*)a_PastQntEnergy, NUM_PRED_TAPS, &qntEnergyErr, 2);
    if (qntEnergyErr < MIN_ENERGY) qntEnergyErr = MIN_ENERGY;

    for (i = 3; i > 0; i--) {
        a_PastQntEnergy[i] = a_PastQntEnergy[i - 1];             
        a_PastQntEnergy_M122[i] = a_PastQntEnergy_M122[i - 1]; 
    }
    a_PastQntEnergy_M122[0] = qntEnergyErr_M122;  
    a_PastQntEnergy[0] = qntEnergyErr;            
}

/**************************************************************************
*  Function    : ownConcealCodebookGainUpdate_GSMAMR
***************************************************************************/
void ownConcealCodebookGainUpdate_GSMAMR(short *a_GainBuffer, short *vPastGainCode, short *vPrevGainCode,
                                         short badFrame, short vPrevBadFr, short *decGainCode)
{
    short i;

    if (badFrame == 0) {
        if (vPrevBadFr != 0) {
            if(*decGainCode > *vPrevGainCode) *decGainCode = *vPrevGainCode;     
        }
        *vPrevGainCode = *decGainCode;                          
    }

    *vPastGainCode = *decGainCode;                       
    
    for(i = 1; i < 5; i++)
       a_GainBuffer[i - 1] = a_GainBuffer[i];                    

    a_GainBuffer[4] = *decGainCode;                              

    return;
	/* End of ownConcealCodebookGainUpdate_GSMAMR() */
}

/*
**************************************************************************
*  Function    : ownConcealGainPitch_GSMAMR
**************************************************************************
*/
void ownConcealGainPitch_GSMAMR(short *a_LSFBuffer, short vPastGainZero, 
								short state, short *pGainPitch)
{
    static const short pdown[7] = {32767, 32112, 32112, 26214, 9830, 6553, 6553};
    short tmp;

    tmp = ownGetMedianElements_GSMAMR(a_LSFBuffer, 5);                        
    if(tmp > vPastGainZero) tmp = vPastGainZero;                       
    *pGainPitch = (short)((tmp * pdown[state]) >> 15);
}

/*
**************************************************************************
*  Function    : ownConcealGainPitchUpdate_GSMAMR
**************************************************************************
*/
void ownConcealGainPitchUpdate_GSMAMR(short *a_LSFBuffer, short *vPastGainZero, short *vPrevGainCode,
                                      short badFrame, short vPrevBadFr, short *pGainPitch)
{
    short i;
    if(badFrame == 0) {
        if(vPrevBadFr != 0) {
           if(*pGainPitch > *vPrevGainCode) *pGainPitch = *vPrevGainCode;
        }
        *vPrevGainCode = *pGainPitch;                         
    }
    
    *vPastGainZero = *pGainPitch;                       
    if(*vPastGainZero > 16384) *vPastGainZero = 16384;                         

    for(i = 1; i < 5; i++) 
	   a_LSFBuffer[i - 1] = a_LSFBuffer[i];                     

    a_LSFBuffer[4] = *vPastGainZero;                       
}


