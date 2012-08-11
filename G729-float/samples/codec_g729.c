/*
 * G729 codec for Asterisk
 *
 * Based on sample applications from Intel Performance Primitives (IPP)
 * libraries.
 *
 * For Intel IPP licensing, see http://www.intel.com
 *
 * For G.729(,A,B) royalty payments, see http://www.sipro.com 
 *   WARNING: please make sure you are sitting down before looking
 *            at their price list.
 *
 * This source file is Copyright (C) 2004 Ready Technology Limited
 * This code is provided for educational purposes and is not warranted
 * to be fit for commercial use.  There is no warranty of any kind.
 * 
 * Author: daniel@readytechnology.co.uk
 */

#define _GNU_SOURCE
#define AST_MODULE "codec_g729"

#include <asterisk.h>
#include <asterisk/lock.h>
#include <asterisk/translate.h>
#include <asterisk/module.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <asterisk/utils.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

#include "usc.h"
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "encoder.h"
#include "decoder.h"

#include "slin_g729_ex.h"
#include "g729_slin_ex.h"

AST_MUTEX_DEFINE_STATIC(localuser_lock);

static int localusecnt=0;

static char *tdesc = "G729/PCM16 (signed linear) Codec Translator, based on IPP";

static USC_Fxns *USC_CODEC_Fxns;

struct ast_translator_pvt {

	struct ast_frame f;

	USC_CodecInfo pInfo;

	USC_Handle codec;	/* the encoder or decoder handle */
	USC_MemBank* pBanks;


	USC_PCMStream pcmStream;	/* Signed linear data */
	USC_Bitstream bitStream;	/* G.729 bits */

	int nbanks;
	int maxbitsize;
  	int inFrameSize;
   	int outFrameSize;

	short pcm_buf[8000];
	unsigned char bitstream_buf[1000];

	int tail;
};

#define g729_coder_pvt ast_translator_pvt

static int lintog729_new(struct ast_trans_pvt *pvt) {
	int i;
	struct g729_coder_pvt *tmp = pvt->pvt;
	//tmp = malloc(sizeof(struct g729_coder_pvt));
	if(tmp) {
		USC_CODEC_Fxns->std.GetInfo((USC_Handle)NULL, &tmp->pInfo);
		((USC_Option*)tmp->pInfo.params)->modes.truncate = 1;
  		((USC_Option*)tmp->pInfo.params)->direction = 0;
  		((USC_Option*)tmp->pInfo.params)->modes.vad = 0;
		USC_CODEC_Fxns->std.NumAlloc(tmp->pInfo.params, &(tmp->nbanks));
		if(!(tmp->pBanks = (USC_MemBank*)ippsMalloc_8u(sizeof(USC_MemBank)*(tmp->nbanks)))) {
                  ast_log(LOG_WARNING, "ippsMalloc_8u failed allocating %d bytes", sizeof(USC_MemBank)*(tmp->nbanks));
                  /* printf("\nLow memory: %d bytes not allocated\n",sizeof(USC_MemBank)*nbanks); */
                  return 1;
                }
		USC_CODEC_Fxns->std.MemAlloc(tmp->pInfo.params, tmp->pBanks);
		for(i=0; i<tmp->nbanks;i++) {
			if(!(tmp->pBanks[i].pMem = ippsMalloc_8u(tmp->pBanks->nbytes))) {
				ast_log(LOG_WARNING, "ippsMalloc_8u failed allocating %d bytes", sizeof(USC_MemBank)*(tmp->nbanks));
				/* printf("\nLow memory: %d bytes not allocated\n",tmp->pBanks->nbytes); */
				return 1;
			}
		}

		tmp->inFrameSize = getInFrameSize();
		tmp->outFrameSize = getOutFrameSize();

		/* tmp->bitstream_buf = ippsMalloc_8s(tmp->pInfo.maxbitsize); */

		tmp->pcmStream.bitrate = tmp->pInfo.params->modes.bitrate;
  		tmp->maxbitsize = tmp->pInfo.maxbitsize;
  		USC_CODEC_Fxns->std.Init(tmp->pInfo.params, tmp->pBanks, &(tmp->codec));

#ifndef IPPCORE_NO_SSE
  		ippCoreSetFlushToZero( 1, NULL );
#endif
  		tmp->pcmStream.pcmType.bitPerSample = tmp->pInfo.pcmType->bitPerSample;
  		tmp->pcmStream.pcmType.sample_frequency = tmp->pInfo.pcmType->sample_frequency;
  		tmp->bitStream.nbytes  = tmp->maxbitsize;
  		tmp->bitStream.pBuffer = tmp->bitstream_buf;

		USC_CODEC_Fxns->std.Reinit(&((USC_Option*)tmp->pInfo.params)->modes, tmp->codec);

		tmp->tail = 0;
		localusecnt++;
		ast_update_use_count();
	}
	
	return 0;
}

static int g729tolin_new(struct ast_trans_pvt *pvt) {
	int i;
	struct g729_coder_pvt *tmp = pvt->pvt;
	//tmp = malloc(sizeof(struct g729_coder_pvt));
	if(tmp) {
		USC_CODEC_Fxns->std.GetInfo((USC_Handle)NULL, &(tmp->pInfo));	
		((USC_Option*)tmp->pInfo.params)->modes.bitrate = 0;
  		((USC_Option*)tmp->pInfo.params)->modes.truncate = 1;
  		((USC_Option*)tmp->pInfo.params)->direction = 1;

		/* tmp->bitstream_buf = ippsMalloc_8s(size); */
		
		USC_CODEC_Fxns->std.NumAlloc(tmp->pInfo.params, &tmp->nbanks);
		if(!(tmp->pBanks = (USC_MemBank*)ippsMalloc_8u(sizeof(USC_MemBank)*(tmp->nbanks)))) {
			ast_log(LOG_WARNING, "ippsMalloc_8u failed allocating %d bytes", sizeof(USC_MemBank)*(tmp->nbanks));
			return 1;
		}
		USC_CODEC_Fxns->std.MemAlloc(tmp->pInfo.params, tmp->pBanks);
		for(i=0; i<tmp->nbanks;i++) {
			if(!(tmp->pBanks[i].pMem = ippsMalloc_8u(tmp->pBanks->nbytes))) {
				ast_log(LOG_WARNING, "ippsMalloc_8u failed allocating %d bytes", sizeof(USC_MemBank)*(tmp->nbanks));
				/* printf("\nLow memory: %d bytes not allocated\n", tmp->pBanks->nbytes); */
				return 1;
			}
		}

		tmp->outFrameSize = getOutFrameSize();

		/* pcm_buf ippsMalloc_8s(getOutFrameSize()); */

		tmp->maxbitsize = tmp->pInfo.maxbitsize;
		USC_CODEC_Fxns->std.Init(tmp->pInfo.params, tmp->pBanks, &(tmp->codec));

#ifndef IPPCORE_NO_SSE
		ippCoreSetFlushToZero( 1, NULL );
#endif

		tmp->bitStream.nbytes  = tmp->maxbitsize;
	  	tmp->bitStream.bitrate = 0;
	        tmp->bitStream.frametype = 3;
	        tmp->pcmStream.pBuffer = tmp->pcm_buf;
		tmp->pcmStream.pcmType.bitPerSample = 0;
	        tmp->pcmStream.pcmType.sample_frequency = 0;

	        USC_CODEC_Fxns->std.Reinit(&((USC_Option*)tmp->pInfo.params)->modes, tmp->codec);

		tmp->tail = 0;
		localusecnt++;
		ast_update_use_count();
	}
	return 0;
}

static struct ast_frame *lintog729_sample(void) {
	static struct ast_frame f;
	f.frametype = AST_FRAME_VOICE;
	f.subclass = AST_FORMAT_SLINEAR;
	f.datalen = sizeof(slin_g729_ex);
	f.samples = sizeof(slin_g729_ex) / 2;
	f.mallocd = 0;
	f.offset = 0;
	f.src = __PRETTY_FUNCTION__;
	f.data = slin_g729_ex;
	return &f;
}

static struct ast_frame *g729tolin_sample(void) {
	static struct ast_frame f;
	f.frametype = AST_FRAME_VOICE;
	f.subclass = AST_FORMAT_G729A;
	f.datalen = sizeof(g729_slin_ex);
	f.samples = 240;
	f.mallocd = 0;
	f.offset = 0;
	f.src = __PRETTY_FUNCTION__;
	f.data = g729_slin_ex;
	return &f;
}

/**
 * Retrieve a frame that has already been decompressed
 */
static struct ast_frame *g729tolin_frameout(struct ast_trans_pvt *pvt) {
	struct g729_coder_pvt *tmp = pvt->pvt;
	if(!tmp->tail)
		return NULL;
	tmp->f.frametype = AST_FRAME_VOICE;
	tmp->f.subclass = AST_FORMAT_SLINEAR;
	tmp->f.datalen = tmp->tail * 2;
	tmp->f.samples = tmp->tail;
	tmp->f.mallocd = 0;
	tmp->f.offset = AST_FRIENDLY_OFFSET;
	tmp->f.src = __PRETTY_FUNCTION__;
	tmp->f.data = tmp->pcm_buf;
	tmp->tail = 0;
	return &tmp->f;
}

/**
 * Accept a frame and decode it at the end of the current buffer
 */
static int g729tolin_framein(struct ast_trans_pvt *pvt, struct ast_frame *f) {
	struct g729_coder_pvt *tmp = pvt->pvt;
	int x, i;
	int frameSize = 0;
	
/*	if(f->datalen % 10) {
		ast_log(LOG_WARNING, "Received a G.729 frame that was %d bytes from %s\n", f->datalen, f->src);
		return -1;
	} */

	for(x = 0; x < f->datalen; x += frameSize) {
		if((f->datalen - x) == 2)
			frameSize = 2;   /* VAD frame */
		else
			frameSize = 10;  /* Regular frame */
		if(tmp->tail + 80 < sizeof(tmp->pcm_buf) / 2) {
			/* decode the frame */
			tmp->bitStream.pBuffer = f->data + x;
			tmp->pcmStream.pBuffer = tmp->pcm_buf + tmp->tail;
			USC_CODEC_Fxns->std.Decode (tmp->codec, &tmp->bitStream, &tmp->pcmStream);
			pvt->samples += 80;

			tmp->tail += 80;
		} else {
			ast_log(LOG_WARNING, "Out of G.729 buffer space\n");
			return -1;
		}
	}
	return 0;
}

static int lintog729_framein(struct ast_trans_pvt *pvt, struct ast_frame *f) {
	struct g729_coder_pvt *tmp = pvt->pvt;
	if(tmp->tail + f->datalen/2 < sizeof(tmp->pcm_buf) / 2) {
		memcpy((tmp->pcm_buf + tmp->tail), f->data, f->datalen);
		tmp->tail += f->datalen/2;
		pvt->samples += f->samples;
	} else {
		ast_log(LOG_WARNING, "Out of buffer space\n");
		return -1;
	}
	return 0;
}

static struct ast_frame *lintog729_frameout(struct ast_trans_pvt *pvt) {
	struct g729_coder_pvt *tmp = pvt->pvt;


	int x = 0, i;
	if(tmp->tail < 80)
		return NULL;
	tmp->f.frametype = AST_FRAME_VOICE;
	tmp->f.subclass = AST_FORMAT_G729A;
	tmp->f.mallocd = 0;
	tmp->f.offset = AST_FRIENDLY_OFFSET;
	tmp->f.src = __PRETTY_FUNCTION__;
	tmp->f.data = tmp->bitstream_buf;
	while(tmp->tail >= 80) {
		if((x+1) * 10 >= sizeof(tmp->bitstream_buf)) {
			ast_log(LOG_WARNING, "Out of buffer space\n");
			break;
		}
		/* Copy the frame to workspace, then encode it */
		tmp->pcmStream.pBuffer = tmp->pcm_buf;
		tmp->bitStream.pBuffer = tmp->bitstream_buf + (x * 10);
		USC_CODEC_Fxns->std.Encode (tmp->codec, &tmp->pcmStream, &tmp->bitStream);

		tmp->tail -= 80;
		if(tmp->tail)
			memmove(tmp->pcm_buf, tmp->pcm_buf + 80, tmp->tail * 2);
		x++;
	}
	tmp->f.datalen = x * 10;
	tmp->f.samples = x * 80;


	return &(tmp->f);
}

static void g729_release(struct ast_trans_pvt *pvt) {
	struct g729_coder_pvt *tmp = pvt->pvt;

	int i;
	for(i = 0; i < tmp->nbanks; i++) {
		if(tmp->pBanks[i].pMem)
			ippsFree(tmp->pBanks[i].pMem);	
		tmp->pBanks[i].pMem=NULL;
	}
	if(tmp->pBanks)
		ippsFree(tmp->pBanks);
	//free(tmp);
	localusecnt--;
	ast_update_use_count();
}

static struct ast_translator g729tolin = {
	.name = "g729tolin",
	.srcfmt = AST_FORMAT_G729A, 
	.dstfmt = AST_FORMAT_SLINEAR,
	.newpvt = g729tolin_new,
	.framein = g729tolin_framein,
	.frameout = g729tolin_frameout,
	.destroy = g729_release,
	.sample = g729tolin_sample,
	.desc_size = sizeof(struct g729_coder_pvt),
	.buf_size = 8000 * 2};

static struct ast_translator lintog729 = {
	.name = "lintog729",
	.srcfmt = AST_FORMAT_SLINEAR, 
	.dstfmt = AST_FORMAT_G729A,
	.newpvt = lintog729_new,
	.framein = lintog729_framein,
	.frameout = lintog729_frameout,
	.destroy = g729_release,
	.sample = lintog729_sample,
	.desc_size = sizeof(struct g729_coder_pvt),
	.buf_size = 1000 };

/* int reload(void) {
	// do nothing for now
	return 0;
} */

static int load_module(void) {
	USC_CODEC_Fxns = USC_GetCodecByName ();
	int res;
	res = ast_register_translator(&g729tolin);
	if(!res)
		res = ast_register_translator(&lintog729);
	else
		ast_unregister_translator(&g729tolin);
	return res;
}

static int unload_module(void) {
	int res;
	ast_mutex_lock(&localuser_lock);
	res = ast_unregister_translator(&lintog729);
	if(!res)
		res = ast_unregister_translator(&g729tolin);
	if(localusecnt)
		res = -1;
	ast_mutex_unlock(&localuser_lock);
	return res;
}

/* char *description(void) {
	return tdesc;
} */

/* int usecount(void) {
	int res;
	STANDARD_USECOUNT(res);
	return res;
} */

/* char *key() {
	return ASTERISK_GPL_KEY;
} */

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "G.729 Coder/Decoder");

