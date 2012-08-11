/*****************************************************************************
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
***************************************************************************/

#include "owng729.h"
#include "g729api.h"
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

#define BITSTREAM_SIZE 15

typedef struct {
    int direction;
    void *Object;
} G729_Handle;

/* global usc vector table */
USC_Fxns USC_G729I_Fxns={ 
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
    pInfo->name = "G729";
    if (handle == NULL) {
        pInfo->params = &params;
        pInfo->pcmType = &pcmType;
        pInfo->params->modes.bitrate = G729_CODEC;
        pInfo->params->modes.truncate = 0;
        pInfo->params->direction = 0;
        pInfo->params->modes.vad = G729Encode_VAD_Disabled;
    }
    pInfo->pcmType->sample_frequency = 8000;
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
        apiG729Encoder_Alloc(options->modes.bitrate, &nbytes);
        pBanks->nbytes = nbytes+sizeof(int); /* include direction in handle */
    }
    else if (options->direction == 1) /* decode only */
    {
        apiG729Decoder_Alloc(options->modes.bitrate, &nbytes);
        pBanks->nbytes = nbytes+sizeof(int); /* include direction in handle */
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    G729_Handle *g729_handle; 
    *handle = (USC_Handle*)pBanks->pMem;
    g729_handle = (G729_Handle*)*handle;
    g729_handle->direction = options->direction;
        
    if (options->direction == 0) /* encode only */
    {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)&g729_handle->Object;
        apiG729Encoder_Init((G729Encoder_Obj*)EncObj, 
            (G729Codec_Type)options->modes.bitrate,options->modes.vad);
    }
    else if (options->direction == 1) /* decode only */
    { 
        G729Decoder_Obj *DecObj = (G729Decoder_Obj *)&g729_handle->Object;
        apiG729Decoder_Init((G729Decoder_Obj*)DecObj, (G729Codec_Type)options->modes.bitrate);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(USC_Modes *modes, USC_Handle handle )
{
    G729_Handle *g729_handle; 
    g729_handle = (G729_Handle*)handle;

    if (g729_handle->direction == 0) /* encode only */
    {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)&g729_handle->Object;
        apiG729Encoder_Init((G729Encoder_Obj*)EncObj, modes->bitrate, modes->vad);
    }
    else if (g729_handle->direction == 1) /* decode only */
    { 
        G729Decoder_Obj *DecObj = (G729Decoder_Obj *)&g729_handle->Object;
        apiG729Decoder_Init((G729Decoder_Obj*)DecObj, modes->bitrate);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(USC_Handle handle, USC_Modes *modes)
{
    return USC_NoError;
}

static USC_Status Encode(USC_Handle handle, const USC_PCMStream *in, USC_Bitstream *out)
{
    G729_Handle *g729_handle; 
    G729Encoder_Obj *EncObj;
    g729_handle = (G729_Handle*)handle;
    EncObj = (G729Encoder_Obj *)&g729_handle->Object;

    if(apiG729Encode(EncObj,(const short*)in->pBuffer,out->pBuffer,in->bitrate,&out->frametype) != APIG729_StsNoErr){
       return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, const USC_Bitstream *in, USC_PCMStream *out)
{
    G729_Handle *g729_handle; 
    G729Decoder_Obj *DecObj;
    g729_handle = (G729_Handle*)handle;
    DecObj = (G729Decoder_Obj *)&g729_handle->Object;

    if(apiG729Decode(DecObj,(const unsigned char*)in->pBuffer,in->frametype,(unsigned short*)out->pBuffer) != APIG729_StsNoErr){
       return USC_NoOperation;
    }
    return USC_NoError;
}


