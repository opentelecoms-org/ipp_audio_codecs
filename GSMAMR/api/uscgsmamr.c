/*****************************************************************************
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
***************************************************************************/

#include "owngsmamr.h"
#include "gsmamrapi.h"
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

typedef struct {
    short sid_update_counter;
    TXFrameType prev_ft;
} sid_syncState;

typedef struct {
    int direction;
    int trunc;
    int reset_flag;
    int reset_flag_old;
	int bitrate_old;
    sid_syncState sid_state;
} GSMAMR_Handle_Header;

typedef struct {
    GSMAMR_Handle_Header header;
    void *Object;
} GSMAMR_Handle;

static int sid_sync_init (sid_syncState *st);
static void sid_sync(sid_syncState *st, int mode, TXFrameType *tx_frame_type);
static int is_pcm_frame_homing (short *input_frame);
static short is_bitstream_frame_homing (unsigned char* bitstream, int mode);

/* global usc vector table */
USCFUN USC_Fxns USC_GSMAMR_Fxns={ 
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

static const int bitsLen[N_MODES]={
   95, 103, 118, 134, 148, 159, 204, 244, 35
};

static const short LostFrame[GSMAMR_Frame]=
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    pInfo->name = "GSMAMR";
    pInfo->framesize = GSMAMR_Frame*sizeof(short);  
    if (handle == NULL) {
        pInfo->params = &params;
        pInfo->pcmType = &pcmType;
        pInfo->params->modes.bitrate = 0;
        pInfo->params->modes.truncate = 0;
        pInfo->params->direction = 0;
        pInfo->params->modes.vad = 0;
    }
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->pcmType->sample_frequency = 8000;
    pInfo->pcmType->bitPerSample = 8;
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
        GSMAMREnc_Params enc_params;
        enc_params.codecType = 0;
        enc_params.mode = options->modes.vad;
        apiGSMAMREncoder_Alloc(&enc_params, &nbytes);
        pBanks->nbytes = nbytes + sizeof(GSMAMR_Handle_Header); /* room for USC header */
    }
    else if (options->direction == 1) /* decode only */
    {
        GSMAMRDec_Params dec_params;
        dec_params.codecType = 0;
        dec_params.mode = options->modes.vad;
        apiGSMAMRDecoder_Alloc(&dec_params, &nbytes);
        pBanks->nbytes = nbytes + sizeof(GSMAMR_Handle_Header); /* room for USC header */
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    GSMAMR_Handle *gsmamr_handle; 
    *handle = (USC_Handle*)pBanks->pMem;
    gsmamr_handle = (GSMAMR_Handle*)*handle;
    gsmamr_handle->header.direction = options->direction;
    gsmamr_handle->header.trunc = options->modes.truncate;
	gsmamr_handle->header.bitrate_old = 0;
        
    if (options->direction == 0) /* encode only */
    {
        GSMAMREncoder_Obj *EncObj = (GSMAMREncoder_Obj *)&gsmamr_handle->Object;
        apiGSMAMREncoder_Init((GSMAMREncoder_Obj*)EncObj, options->modes.vad);
    }
    else if (options->direction == 1) /* decode only */
    { 
        GSMAMRDecoder_Obj *DecObj = (GSMAMRDecoder_Obj *)&gsmamr_handle->Object;
        apiGSMAMRDecoder_Init((GSMAMRDecoder_Obj*)DecObj, options->modes.vad);
        gsmamr_handle->header.reset_flag = 0;
        gsmamr_handle->header.reset_flag_old = 1;
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(USC_Modes *modes, USC_Handle handle )
{
    GSMAMR_Handle *gsmamr_handle; 
    gsmamr_handle = (GSMAMR_Handle*)handle;
	gsmamr_handle->header.bitrate_old = 0;

    if (gsmamr_handle->header.direction == 0) /* encode only */
    {
        GSMAMREncoder_Obj *EncObj = (GSMAMREncoder_Obj *)&gsmamr_handle->Object;
        gsmamr_handle->header.reset_flag = 0;
        sid_sync_init (&gsmamr_handle->header.sid_state);
        apiGSMAMREncoder_Init((GSMAMREncoder_Obj*)EncObj, modes->vad);
    }
    else if (gsmamr_handle->header.direction == 1) /* decode only */
    { 
        GSMAMRDecoder_Obj *DecObj = (GSMAMRDecoder_Obj *)&gsmamr_handle->Object;
        apiGSMAMRDecoder_Init((GSMAMRDecoder_Obj*)DecObj, modes->vad);
        gsmamr_handle->header.reset_flag = 0;
        gsmamr_handle->header.reset_flag_old = 1;
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(USC_Handle handle, USC_Modes *modes)
{
    return USC_NoError;
}

#define IO_MASK (short)0xfff8     /* 13-bit input/output                      */
static GSMAMR_Rate_t usc2amr[8]={
    GSMAMR_RATE_4750,  
    GSMAMR_RATE_5150,  
    GSMAMR_RATE_5900,  
    GSMAMR_RATE_6700,  
    GSMAMR_RATE_7400,  
    GSMAMR_RATE_7950,  
    GSMAMR_RATE_10200, 
    GSMAMR_RATE_12200 
};
static USC_Status Encode(USC_Handle handle, const USC_PCMStream *in, USC_Bitstream *out)
{
    GSMAMR_Handle *gsmamr_handle; 
    GSMAMREncoder_Obj *EncObj;
    unsigned short work_buf[GSMAMR_Frame];
    int pVad = 0;
    gsmamr_handle = (GSMAMR_Handle*)handle;
    EncObj = (GSMAMREncoder_Obj *)&gsmamr_handle->Object;

    /* check for homing frame */
    gsmamr_handle->header.reset_flag = is_pcm_frame_homing((short*)in->pBuffer);

    if (gsmamr_handle->header.trunc) {
        /* Delete the LSBs */
        ippsAndC_16u((Ipp16u*)in->pBuffer, IO_MASK, work_buf, GSMAMR_Frame);
    }

    if(apiGSMAMREncode(EncObj,(short*)work_buf,usc2amr[in->bitrate&7],(unsigned char*)out->pBuffer,&pVad) != APIGSMAMR_StsNoErr){
       return USC_NoOperation;
    }

	out->nbytes = ((bitsLen[in->bitrate] + 7) >> 3);

    /* include frame type and mode information in serial bitstream */
    if(!pVad) {
        sid_sync (&gsmamr_handle->header.sid_state, (int)GSMAMR_RATE_DTX, &out->frametype);
    } else {
        sid_sync (&gsmamr_handle->header.sid_state, in->bitrate, &out->frametype);
    }

    if (gsmamr_handle->header.reset_flag != 0) {
        sid_sync_init (&gsmamr_handle->header.sid_state);
        apiGSMAMREncoder_Init(EncObj, EncObj->objPrm.mode);
    } 

	out->bitrate = in->bitrate;

    return USC_NoError;
}

static RXFrameType tx2rx (TXFrameType tx_type)
{
    switch (tx_type) {
      case TX_SPEECH_GOOD:      return RX_SPEECH_GOOD;
      case TX_SID_FIRST:        return RX_SID_FIRST;
      case TX_SID_UPDATE:       return RX_SID_UPDATE;
      case TX_NO_DATA:          return RX_NO_DATA;
      case TX_SPEECH_DEGRADED:  return RX_SPEECH_DEGRADED;
      case TX_SPEECH_BAD:       return RX_SPEECH_BAD;
      case TX_SID_BAD:          return RX_SID_BAD;
      case TX_ONSET:            return RX_ONSET;
      default:
         return (RXFrameType)(-1);
    }
}

static USC_Status Decode(USC_Handle handle, const USC_Bitstream *in, USC_PCMStream *out)
{
    GSMAMR_Handle *gsmamr_handle; 
    GSMAMRDecoder_Obj *DecObj;
    gsmamr_handle = (GSMAMR_Handle*)handle;
    DecObj = (GSMAMRDecoder_Obj *)&gsmamr_handle->Object;

    if(in == NULL) {
	  /* Lost frame */
      if(apiGSMAMRDecode(DecObj,(const unsigned char*)LostFrame,(GSMAMR_Rate_t)gsmamr_handle->header.bitrate_old,
           RX_SPEECH_BAD,(short*)out->pBuffer) != APIGSMAMR_StsNoErr){ 
            return USC_NoOperation;
      }     
      if(gsmamr_handle->header.trunc) {
         /* Truncate LSBs */
         ippsAndC_16u_I(IO_MASK, (unsigned short*)out->pBuffer, GSMAMR_Frame);
      }
	  gsmamr_handle->header.reset_flag = 0;
      gsmamr_handle->header.reset_flag_old = 1;

      out->nbytes = GSMAMR_Frame*sizeof(short);
	} else {

      gsmamr_handle->header.bitrate_old = in->bitrate; 

	  gsmamr_handle->header.reset_flag = is_bitstream_frame_homing(in->pBuffer, in->bitrate);

      /* produce encoder homing frame if homed & input=decoder homing frame */
      if ((gsmamr_handle->header.reset_flag != 0) && (gsmamr_handle->header.reset_flag_old != 0)) 
	  {
         ippsSet_16s(EHF_MASK, (short*)out->pBuffer, GSMAMR_Frame);
	  } 
      else 
	  {
          if(apiGSMAMRDecode(DecObj,(const unsigned char*)in->pBuffer,(GSMAMR_Rate_t)in->bitrate,
             tx2rx(in->frametype),(short*)out->pBuffer) != APIGSMAMR_StsNoErr){ 
             return USC_NoOperation;
		  }  
          if (gsmamr_handle->header.trunc) {
             /* Truncate LSBs */
             ippsAndC_16u_I(IO_MASK, (unsigned short*)out->pBuffer, GSMAMR_Frame);
		  }
	  }
      /* reset decoder if current frame is a homing frame */
      if (gsmamr_handle->header.reset_flag != 0) 
	  {
          apiGSMAMRDecoder_Init(DecObj, DecObj->objPrm.mode);
	  } 
      gsmamr_handle->header.reset_flag_old = gsmamr_handle->header.reset_flag;

	  out->nbytes = GSMAMR_Frame*sizeof(short);
	}

    return USC_NoError;
}

static int is_pcm_frame_homing (short *input_frame)
{
    int i, j;

    /* check 160 input samples for matching EHF_MASK: defined in e_homing.h */
    for (i = 0; i < GSMAMR_Frame; i++)
    {
        j = input_frame[i] ^ EHF_MASK;

        if (j)
            break;
    }

    return !j;
}

static int sid_sync_init (sid_syncState *state)
{
    state->sid_update_counter = 3;
    state->prev_ft = TX_SPEECH_GOOD;
    return 0;
}

static void sid_sync (sid_syncState *st, GSMAMR_Rate_t mode,
               TXFrameType *tx_frame_type)
{
    if ( mode == GSMAMR_RATE_DTX){
       st->sid_update_counter--;
        if (st->prev_ft == TX_SPEECH_GOOD)
        {
           *tx_frame_type = TX_SID_FIRST;
           st->sid_update_counter = 3;
        }
        else
        {
           if (st->sid_update_counter == 0)
           {
              *tx_frame_type = TX_SID_UPDATE;
              st->sid_update_counter = 8;
           } else {
              *tx_frame_type = TX_NO_DATA;
           }
        }
    }
    else
    {
       st->sid_update_counter = 8 ;
       *tx_frame_type = TX_SPEECH_GOOD;
    }
    st->prev_ft = *tx_frame_type;
    return;
}

/*
  homing decoder frames by modes
*/
static const unsigned char home_bits_122[]={
   0x08,0x54,0xdb,0x96,
   0xaa,0xad,0x60,0x00,
   0x00,0x00,0x00,0x1b,
   0x58,0x7f,0x66,0x83,
   0x79,0x40,0x90,0x04,
   0x15,0x85,0x4f,0x10,
   0xf6,0xb0,0x24,0x03,
   0xc7,0xea,0x00
};
static const unsigned char home_bits_102[]={
   0xf8,0x71,0x8b,0xd1,
   0x40,0x00,0x00,0x00,
   0x00,0xda,0xe4,0xc6,
   0x77,0xea,0x2c,0x40,
   0xad,0x6b,0x3d,0x80,
   0x6c,0x17,0xc8,0x55,
   0xc3,0x00
};
static const unsigned char home_bits_795[]={
   0x61,0x38,0xc5,0xf7,
   0xa0,0x06,0xfa,0x07,
   0x3c,0x08,0x7a,0x5b,
   0x1c,0x69,0xbc,0x41,
   0xca,0x68,0x3c,0x82
};
static const unsigned char home_bits_74[]={
   0xf8,0x71,0x8b,0xef,
   0x40,0x0d,0xe0,0x36,
   0x20,0x8f,0xc4,0xc1,
   0xba,0x6f,0x01,0xb0,
   0x03,0x78,0x00
};
static const unsigned char home_bits_67[]={
   0xf8,0x71,0x8b,0xef,
   0x40,0x17,0x01,0xe2,
   0x63,0xe1,0x60,0xb8,
   0xbc,0x07,0xb1,0x8e,
   0x00
};
static const unsigned char home_bits_59[]={
   0xf8,0x71,0x8b,0xef,
   0x40,0x1e,0xfe,0x01,
   0xcf,0x60,0x7c,0xfb,
   0xf8,0x03,0xdc
};
static const unsigned char home_bits_515[]={
   0xf8,0x9d,0x38,0xcc,
   0x03,0xdf,0xc0,0x62,
   0xfb,0x7f,0x7f,0x47,
   0xbe
};
static const unsigned char home_bits_475[]={
   0xf8,0x9d,0x38,0xcc,
   0x03,0x28,0xf7,0x0f,
   0xb1,0x82,0x3d,0x36
};

static const unsigned char *d_homes[] =
{
   home_bits_475,
   home_bits_515,
   home_bits_59,
   home_bits_67,
   home_bits_74,
   home_bits_795,
   home_bits_102,
   home_bits_122
};

static short is_bitstream_frame_homing (unsigned char* bitstream, int mode)
{
   int i,j=0;
   int bitstremlen = (bitsLen[mode]+7)>>3;
   for (i = 0; i < bitstremlen; i++)
   {
       j = bitstream[i] ^ d_homes[mode][i];

       if (j)
           break;
   }
    return !j;

}
