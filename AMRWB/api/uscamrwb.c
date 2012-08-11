/*****************************************************************************
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Purpose: AMR WB Codec API wrapper 
//
***************************************************************************/

#include "ownamrwb.h"
#include "amrwbapi.h"
#include <string.h>
#include <usc.h>

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status NumAlloc(const USC_Option *options, int *nbanks);
static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(USC_Modes *modes, USC_Handle handle );
static USC_Status Control(USC_Handle handle, USC_Modes *modes);
static USC_Status Encode(USC_Handle handle, const USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, const USC_Bitstream *in, USC_PCMStream *out);

typedef struct
{
    short updateSidCount;
    TXFrameType prevFrameType;
} SIDSyncState;

typedef struct {
    int direction;
    int trunc;
    int reset_flag;
    int reset_flag_old;
    int usedRate;
    SIDSyncState sid_state;
} AMRWB_Handle_Header;

typedef struct {
    AMRWB_Handle_Header header;
    void *Object;
} AMRWB_Handle;

static int  ownSidSyncInit(SIDSyncState *st);
static void ownSidSync(SIDSyncState *st, int valRate, TXFrameType *pTXFrameType);
static int  ownTestPCMFrameHoming(const short *pSrc);
static int  ownTestBitstreamFrameHoming(char* pPrmsvec, int valRate);

/* global usc vector table */
USC_Fxns USC_AMRWB_Fxns={ 
    {
        GetInfo,
        NumAlloc,
        MemAlloc,
        Init,
        Reinit,
        Control,
        Encode,
        Decode
    }
};

static USC_Option  params;  /* what is supported  */ 
static USC_PCMType pcmType; /* codec audio source */ 

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    pInfo->name = "AMRWB";
    if (handle == NULL) {
        pInfo->params = &params;
        pInfo->pcmType = &pcmType;
        pInfo->params->modes.bitrate = 0;
        pInfo->params->modes.truncate = 1;
        pInfo->params->direction = 0;
        pInfo->params->modes.vad = 0;
    }
    pInfo->pcmType->sample_frequency = 16000;
    pInfo->pcmType->bitPerSample = 8;
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params->modes.hpf = 0;
    pInfo->params->modes.pf = 0;
    pInfo->params->law = 0;
    return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, int *nbanks)
{
    *nbanks = 1;
    return USC_NoError;
}

static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks)
{
    unsigned int nbytes;
    pBanks->pMem = NULL;
    if (options->direction == 0) /* encode only */
    {
        AMRWBEnc_Params enc_params;
        enc_params.codecType = 0;
        enc_params.mode = options->modes.vad;
        apiAMRWBEncoder_Alloc(&enc_params, &nbytes);
    }
    else if (options->direction == 1) /* decode only */
    {
        AMRWBDec_Params dec_params;
        dec_params.codecType = 0;
        dec_params.mode = options->modes.vad;
        apiAMRWBDecoder_Alloc(&dec_params, &nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks->nbytes = nbytes + sizeof(AMRWB_Handle_Header); /* include AMRWB_Handle_Header in handle */
    return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    AMRWB_Handle *amrwb_handle; 
    *handle = (USC_Handle*)pBanks->pMem;
    amrwb_handle = (AMRWB_Handle*)*handle;
    amrwb_handle->header.direction = options->direction;
    amrwb_handle->header.trunc = options->modes.truncate;
        
    if (options->direction == 0) /* encode only */
    {
        AMRWBEncoder_Obj *EncObj = (AMRWBEncoder_Obj *)&amrwb_handle->Object;
        apiAMRWBEncoder_Init((AMRWBEncoder_Obj*)EncObj, options->modes.vad);
    }
    else if (options->direction == 1) /* decode only */
    { 
        AMRWBDecoder_Obj *DecObj = (AMRWBDecoder_Obj *)&amrwb_handle->Object;
        apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)DecObj);
        amrwb_handle->header.usedRate = 0;
        amrwb_handle->header.reset_flag = 0;
        amrwb_handle->header.reset_flag_old = 1;

    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(USC_Modes *modes, USC_Handle handle )
{
    AMRWB_Handle *amrwb_handle; 
    amrwb_handle = (AMRWB_Handle*)handle;

    if (amrwb_handle->header.direction == 0) /* encode only */
    {
        AMRWBEncoder_Obj *EncObj = (AMRWBEncoder_Obj *)&amrwb_handle->Object;
        amrwb_handle->header.reset_flag = 0;
        ownSidSyncInit(&amrwb_handle->header.sid_state);
        apiAMRWBEncoder_Init((AMRWBEncoder_Obj*)EncObj, modes->vad);
    }
    else if (amrwb_handle->header.direction == 1) /* decode only */
    { 
        AMRWBDecoder_Obj *DecObj = (AMRWBDecoder_Obj *)&amrwb_handle->Object;
        apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)DecObj);
        amrwb_handle->header.usedRate = 0;
        amrwb_handle->header.reset_flag = 0;
        amrwb_handle->header.reset_flag_old = 1;
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(USC_Handle handle, USC_Modes *modes)
{
    return USC_NoError;
}

static AMRWB_Rate_t usc2amrwb[9]={
    AMRWB_RATE_6600,   
    AMRWB_RATE_8850,       
    AMRWB_RATE_12650,      
    AMRWB_RATE_14250,      
    AMRWB_RATE_15850,      
    AMRWB_RATE_18250,      
    AMRWB_RATE_19850,      
    AMRWB_RATE_23050,      
    AMRWB_RATE_23850     
};

static USC_Status Encode(USC_Handle handle, const USC_PCMStream *in, USC_Bitstream *out)
{
    AMRWB_Handle *amrwb_handle; 
    AMRWBEncoder_Obj *EncObj;
    unsigned short work_buf[AMRWB_Frame];
    int WrkRate;
    amrwb_handle = (AMRWB_Handle*)handle;

    if(amrwb_handle == NULL) return USC_InvalidHandler;
    if(in->bitrate > 8 || in->bitrate < 0) return USC_UnsupportedBitRate;

    EncObj = (AMRWBEncoder_Obj *)&amrwb_handle->Object;

    /* check for homing frame */
    amrwb_handle->header.reset_flag = ownTestPCMFrameHoming((short*)in->pBuffer);

    if (amrwb_handle->header.trunc) {
       /* Delete the LSBs */
       ippsAndC_16u((Ipp16u*)in->pBuffer, IO_MASK, work_buf, AMRWB_Frame);
    }
    WrkRate = usc2amrwb[in->bitrate];
    if(apiAMRWBEncode(EncObj,(const unsigned short*)work_buf,(unsigned char*)out->pBuffer,&WrkRate,EncObj->iDtx) != APIAMRWB_StsNoErr){
       return USC_NoOperation;
    }
    /* include frame type and mode information in serial bitstream */
    ownSidSync(&amrwb_handle->header.sid_state, WrkRate, &out->frametype);


    if (amrwb_handle->header.reset_flag != 0) {
        ownSidSyncInit(&amrwb_handle->header.sid_state);
        apiAMRWBEncoder_Init(EncObj, EncObj->iDtx);
    } 

    return USC_NoError;
}

static RXFrameType ownTX2RX (TXFrameType valTXType)
{
    switch (valTXType) {
      case TX_SPEECH:                    return RX_SPEECH_GOOD;
      case TX_SID_FIRST:                 return RX_SID_FIRST;
      case TX_SID_UPDATE:                return RX_SID_UPDATE;
      case TX_NO_DATA:                   return RX_NO_DATA;
      case TX_SPEECH_PROBABLY_DEGRADED:  return RX_SPEECH_PROBABLY_DEGRADED;
      case TX_SPEECH_LOST:               return RX_SPEECH_LOST;
      case TX_SPEECH_BAD:                return RX_SPEECH_BAD;
      case TX_SID_BAD:                   return RX_SID_BAD;
      default:
        printf("ownTX2RX: unknown TX frame type %d\n", valTXType);
        exit(1);
    }
}

static USC_Status Decode(USC_Handle handle, const USC_Bitstream *in, USC_PCMStream *out)
{
    AMRWB_Handle *amrwb_handle; 
    AMRWBDecoder_Obj *DecObj;
    int mode;
    RXFrameType rx_type;
    amrwb_handle = (AMRWB_Handle*)handle;
    DecObj = (AMRWBDecoder_Obj *)&amrwb_handle->Object;

    out->bitrate = in->bitrate;
    rx_type = ownTX2RX(in->frametype);
    if ((rx_type == RX_SID_BAD) || (rx_type == RX_SID_UPDATE) || (rx_type == RX_NO_DATA)) {
        mode = AMRWB_RATE_DTX;
    } else {
        mode = out->bitrate;
    }
    if ((rx_type == RX_SPEECH_LOST)) {
        out->bitrate = amrwb_handle->header.usedRate;
        amrwb_handle->header.reset_flag = 0;
    } else {
        amrwb_handle->header.usedRate = out->bitrate;
        /* if homed: check if this frame is another homing frame */
        amrwb_handle->header.reset_flag = ownTestBitstreamFrameHoming(in->pBuffer, mode);
    }

    /* produce encoder homing frame if homed & input=decoder homing frame */
    if ((amrwb_handle->header.reset_flag != 0) && (amrwb_handle->header.reset_flag_old != 0)) 
    {
        ippsSet_16s(EHF_MASK, (short*)out->pBuffer, AMRWB_Frame);
    } 
    else 
    {
        if(apiAMRWBDecode(DecObj,(const unsigned char*)in->pBuffer,usc2amrwb[out->bitrate],rx_type,(unsigned short*)out->pBuffer) != APIAMRWB_StsNoErr){
            return USC_NoOperation;
        } 
        if (amrwb_handle->header.trunc) {
            /* Truncate LSBs */
            ippsAndC_16u_I(IO_MASK, (unsigned short*)out->pBuffer, AMRWB_Frame);
        }
    }
    /* reset decoder if current frame is a homing frame */
    if (amrwb_handle->header.reset_flag != 0) 
    {
        apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)DecObj);
        amrwb_handle->header.usedRate = 0;
    } 
    amrwb_handle->header.reset_flag_old = amrwb_handle->header.reset_flag;

    return USC_NoError;
}

static int ownSidSyncInit(SIDSyncState *st)
{
    st->updateSidCount = 3;
    st->prevFrameType = TX_SPEECH;
    return 0;
}

static void ownSidSync(SIDSyncState *st, int valRate, TXFrameType *pTXFrameType)
{
    if ( valRate == AMRWB_RATE_DTX)
    {
        st->updateSidCount--;
        if (st->prevFrameType == TX_SPEECH)
        {
           *pTXFrameType = TX_SID_FIRST;
           st->updateSidCount = 3;
        }
        else
        {
           if (st->updateSidCount == 0)
           {
              *pTXFrameType = TX_SID_UPDATE;
              st->updateSidCount = 8;
           } else {
              *pTXFrameType = TX_NO_DATA;
           }
        }
    }
    else
    {
       st->updateSidCount = 8 ;
       *pTXFrameType = TX_SPEECH;
    }
    st->prevFrameType = *pTXFrameType;
}

static int ownTestPCMFrameHoming(const short *pSrc)
{
    int i, fl;

    for (i = 0; i < AMRWB_Frame; i++)
    {
        fl = pSrc[i] ^ EHF_MASK;
        if (fl) break;
    }

    return (!fl);
}

#define PRML 15
#define NUMPRM6600 NUMBITS6600/PRML + 1
#define NUMPRM8850 NUMBITS8850/PRML + 1
#define NUMPRM12650 NUMBITS12650/PRML + 1
#define NUMPRM14250 NUMBITS14250/PRML + 1
#define NUMPRM15850 NUMBITS15850/PRML + 1
#define NUMPRM18250 NUMBITS18250/PRML + 1
#define NUMPRM19850 NUMBITS19850/PRML + 1
#define NUMPRM23050 NUMBITS23050/PRML + 1
#define NUMPRM23850 NUMBITS23850/PRML + 1

static const short DecHomFrm6600Tbl[NUMPRM6600] =
{
   3168, 29954, 29213, 16121, 64,
  13440, 30624, 16430, 19008
};

static const short DecHomFrm8850Tbl[NUMPRM8850] =
{
   3168, 31665, 9943,  9123, 15599,  4358, 
  20248, 2048, 17040, 27787, 16816, 13888
};

static const short DecHomFrm12650Tbl[NUMPRM12650] =
{
  3168, 31665,  9943,  9128,  3647,
  8129, 30930, 27926, 18880, 12319,
   496,  1042,  4061, 20446, 25629,
 28069, 13948
};

static const short DecHomFrm14250Tbl[NUMPRM14250] =
{
    3168, 31665,  9943,  9131, 24815,
     655, 26616, 26764,  7238, 19136,
    6144,    88,  4158, 25733, 30567,
    30494, 	221, 20321, 17823
};

static const short DecHomFrm15850Tbl[NUMPRM15850] =
{
    3168, 31665,  9943,  9131, 24815,
     700,  3824,  7271, 26400,  9528,
    6594, 26112,   108,  2068, 12867,
   16317, 23035, 24632,  7528,  1752, 
    6759, 24576
};

static const short DecHomFrm18250Tbl[NUMPRM18250] =
{
     3168, 31665,  9943,  9135, 14787,
    14423, 30477, 24927, 25345, 30154,
      916,  5728, 18978,  2048,   528,
    16449,  2436,  3581, 23527, 29479, 
	 8237, 16810, 27091, 19052,     0
};

static const short DecHomFrm19850Tbl[NUMPRM19850] =
{
     3168, 31665,  9943,  9129,  8637, 31807,
    24646,   736, 28643,  2977,  2566, 25564,
    12930, 13960,  2048,   834,  3270,  4100,
    26920, 16237, 31227, 17667, 15059, 20589,
    30249, 29123,     0
};

static const short DecHomFrm23050Tbl[NUMPRM23050] =
{
	 3168, 31665,  9943,  9132, 16748,  3202,  28179,
    16317, 30590, 15857, 19960,  8818, 21711,  21538,
     4260, 16690, 20224,  3666,  4194,  9497,  16320,
    15388,  5755, 31551, 14080, 3574,  15932,     50,
    23392, 26053, 31216
};

static const short DecHomFrm23850Tbl[NUMPRM23850] =
{
	 3168, 31665,  9943,  9134, 24776,  5857, 18475,
    28535, 29662, 14321, 16725,  4396, 29353, 10003,
    17068, 20504,   720,     0,  8465, 12581, 28863,
    24774,  9709, 26043,  7941, 27649, 13965, 15236,
    18026, 22047, 16681,  3968
};

static const short *DecHomFrmTbl[] =
{
  DecHomFrm6600Tbl,
  DecHomFrm8850Tbl,
  DecHomFrm12650Tbl,
  DecHomFrm14250Tbl,
  DecHomFrm15850Tbl,
  DecHomFrm18250Tbl,
  DecHomFrm19850Tbl,
  DecHomFrm23050Tbl,
  DecHomFrm23850Tbl,
  DecHomFrm23850Tbl
};

static const int BitsLenTbl[NUM_OF_MODES]={
   132, 177, 253, 285, 317, 365, 397, 461, 477, 477, 35
};

/* check if the parameters matches the parameters of the corresponding decoder homing frame */
static int ownTestBitstreamFrameHoming (char* pPrmsvec, int valRate)
{
    int i, valFlag;
    short tmp, valSht;
    short *pPrms = (short*)pPrmsvec; 
    int valNumBytes = (BitsLenTbl[valRate] + 14) / 15;
    valSht  = valNumBytes * 15 - BitsLenTbl[valRate];
        
    valFlag = 0;
    if (valRate != AMRWB_RATE_DTX)
    {
        if (valRate != AMRWB_RATE_23850)
        {
            for (i = 0; i < valNumBytes-1; i++)
            {  
                valFlag = (short)(pPrms[i] ^ DecHomFrmTbl[valRate][i]);
                if (valFlag) break;
            }  
        } else 
        {
            valSht = 0;
            for (i = 0; i < valNumBytes-1; i++)
            {  
                switch (i) 
                {
                case 10:
                    tmp = pPrms[i] & 0x61FF;
                    break;
                case 17:
                    tmp = pPrms[i] & 0xE0FF;
                    break;
                case 24:
                    tmp = pPrms[i] & 0x7F0F;
                    break;
                default:
                    tmp = pPrms[i];
                    break;
                }
                valFlag = (short) (tmp ^ DecHomFrmTbl[valRate][i]);
                if (valFlag) break;
            }  
        }
        tmp = 0x7fff;
        tmp >>= valSht;
        tmp <<= valSht;
        tmp = (short) (DecHomFrmTbl[valRate][i] & tmp);
        tmp = (short) (pPrms[i] ^ tmp);
        valFlag = (short) (valFlag | tmp);
    }
    else
    {
        valFlag = 1;
    }
    return (!valFlag);
}

