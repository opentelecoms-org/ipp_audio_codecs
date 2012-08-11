/*
 * G723.1 codec for Asterisk
 *
 * Based on sample applications from Intel Performance Primitives (IPP)
 * libraries.
 *
 * For Intel IPP licensing, see http://www.intel.com
 *
 * For G.723.1 royalty payments, see http://www.google.com 
 *   The G.723.1 patent only exists in some countries, so you may not
 *   need to pay for a license to use this mechanism.
 *
 * This source file is Copyright (C) 2004 Ready Technology Limited
 * This code is provided for educational purposes and is not warranted
 * to be fit for commercial use.  There is no warranty of any kind.
 * 
 * Author: daniel@readytechnology.co.uk
 */

#define _GNU_SOURCE

#include <asterisk/lock.h>
#include <asterisk/translate.h>
#include <asterisk/module.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "g723encoder.h"
#include "g723decoder.h"

#include "slin_g723_ex.h"
#include "g723_slin_ex.h"


/**
 * Each 24 byte G.723.1 frame is 30ms of 16bit signed linear sampled at 8kHz
 */
#define MY_G723_1_FRAME_SIZE 24
#define MY_G723_1_SAMPLE_SIZE 240		

#define G723_RATE_63 0
#define G723_RATE_53 1

#define DEFAULT_SEND_RATE G723_RATE_53

AST_MUTEX_DEFINE_STATIC(localuser_lock);

static int localusecnt=0;

static int defaultSendRate;

static char *tdesc = "G723.1/PCM16 (signed linear) Codec Translator, based on IPP";

struct ast_translator_pvt {

	struct ast_frame f;

	G723Encoder_Obj *encoder;
	G723Decoder_Obj *decoder;

	int sendRate;

	int maxbitsize;
  	int inFrameSize;
   	int outFrameSize;

	short pcm_buf[8000];
	unsigned char bitstream_buf[1000];

	int tail;
};

#define g723_coder_pvt ast_translator_pvt

static struct ast_translator_pvt *lintog723_new(void) {
	int i;
	struct g723_coder_pvt *tmp;
	int eSize;

	tmp = malloc(sizeof(struct g723_coder_pvt));
	if(tmp) {

		apiG723Encoder_Alloc(&eSize);
		tmp->encoder = (G723Encoder_Obj*)ippsMalloc_8u(eSize);		
		tmp->decoder = NULL;
		tmp->sendRate = defaultSendRate;

		// Init, no VAD or silence compression
		apiG723Encoder_Init(tmp->encoder, 0);

		tmp->tail = 0;
		localusecnt++;
		ast_update_use_count();
	}
	return tmp;
}

static struct ast_translator_pvt *g723tolin_new(void) {
	int i;
	struct g723_coder_pvt *tmp;
	int dSize;

	tmp = malloc(sizeof(struct g723_coder_pvt));
	if(tmp) {

		apiG723Decoder_Alloc(&dSize);
		tmp->encoder = NULL;
		tmp->decoder = (G723Decoder_Obj *)ippsMalloc_8u(dSize);

		apiG723Decoder_Init(tmp->decoder, 0);

		tmp->tail = 0;
		localusecnt++;
		ast_update_use_count();
	}
	return tmp;
}

static struct ast_frame *lintog723_sample(void) {
	static struct ast_frame f;
	f.frametype = AST_FRAME_VOICE;
	f.subclass = AST_FORMAT_SLINEAR;
	f.datalen = sizeof(slin_g723_ex);
	f.samples = sizeof(slin_g723_ex) / 2;
	f.mallocd = 0;
	f.offset = 0;
	f.src = __PRETTY_FUNCTION__;
	f.data = slin_g723_ex;
	return &f;
}

static struct ast_frame *g723tolin_sample(void) {
	static struct ast_frame f;
	f.frametype = AST_FRAME_VOICE;
	f.subclass = AST_FORMAT_G723_1;
	f.datalen = sizeof(g723_slin_ex);
	f.samples = 240;
	f.mallocd = 0;
	f.offset = 0;
	f.src = __PRETTY_FUNCTION__;
	f.data = g723_slin_ex;
	return &f;
}

/**
 * Retrieve a frame that has already been decompressed
 */
static struct ast_frame *g723tolin_frameout(struct ast_translator_pvt *tmp) {
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
static int g723tolin_framein(struct ast_translator_pvt *tmp, struct ast_frame *f) {
	int x, i;
	int frameInfo;
	int frameSize;
	int sampleSize = MY_G723_1_SAMPLE_SIZE;
	char *frame = (char *)f->data;

	int f_offset = 0;

	while(f_offset < f->datalen) {

		/* Determine the frame type */
		frameInfo = frame[f_offset] & (short)0x0003;
		switch(frameInfo) {
		case 0:	/* Active frame, high rate, 24 bytes */
			frameSize = 24;

			break;
		case 1: /* Active frame, low rate, 20 bytes */
			frameSize = 20;

			break;
		case 2: /* Sid Frame, 4 bytes */
			frameSize = 4;
			break;

		default: /* untransmitted */
			frameSize = 1;
			break;

		}

		if((f_offset + frameSize) > f->datalen) {
			ast_log(LOG_WARNING, "Received a G.723.1 frame that was %d bytes from %s\n", f->datalen, f->src);
			return -1;
		}

		if(tmp->tail + sampleSize < sizeof(tmp->pcm_buf) / 2) {
			/* decode the frame */
			apiG723Decode(tmp->decoder, frame + f_offset, 0, tmp->pcm_buf + tmp->tail);

			tmp->tail += sampleSize;
		} else {
			ast_log(LOG_WARNING, "Out of G.723 buffer space\n");
			return -1;
		}
		f_offset += frameSize;
	}
	return 0;
}

static int lintog723_framein(struct ast_translator_pvt *tmp, struct ast_frame *f) {
	if(tmp->tail + f->datalen/2 < sizeof(tmp->pcm_buf) / 2) {
		memcpy((tmp->pcm_buf + tmp->tail), f->data, f->datalen);
		tmp->tail += f->datalen/2;
	} else {
		ast_log(LOG_WARNING, "Out of buffer space\n");
		return -1;
	}
	return 0;
}

static struct ast_frame *lintog723_frameout(struct ast_translator_pvt *tmp) {
	int x = 0, i;
	int frameSize, sampleSize;

	sampleSize = MY_G723_1_SAMPLE_SIZE;
	switch(tmp->sendRate) {
	case G723_RATE_63:
		frameSize = 24;
		break;
	case G723_RATE_53:
		frameSize = 20;
		break;
	default:
		break;
	}

	if(tmp->tail < sampleSize)
		return NULL;
	tmp->f.frametype = AST_FRAME_VOICE;
	tmp->f.subclass = AST_FORMAT_G723_1;
	tmp->f.mallocd = 0;
	tmp->f.offset = AST_FRIENDLY_OFFSET;
	tmp->f.src = __PRETTY_FUNCTION__;
	tmp->f.data = tmp->bitstream_buf;
	while(tmp->tail >= sampleSize) {
		if((x+1) * frameSize >= sizeof(tmp->bitstream_buf)) {
			ast_log(LOG_WARNING, "Out of buffer space\n");
			break;
		}
		/* Copy the frame to workspace, then encode it */
		apiG723Encode(tmp->encoder, tmp->pcm_buf, tmp->sendRate, tmp->bitstream_buf + (x * frameSize));

		tmp->tail -= sampleSize;
		if(tmp->tail)
			memmove(tmp->pcm_buf, tmp->pcm_buf + sampleSize, tmp->tail * 2);
		x++;
	}
	tmp->f.datalen = x * frameSize;
	tmp->f.samples = x * sampleSize;


	return &(tmp->f);
}

static void g723_release(struct ast_translator_pvt *pvt) {
	int i;
	if(pvt->encoder != NULL) {
		/* Free an encoder instance */
		ippsFree(pvt->encoder);

	} else {
		/* Free a decoder instance */
		ippsFree(pvt->decoder);
	}

	free(pvt);
	localusecnt--;
	ast_update_use_count();
}

static struct ast_translator g723tolin = {
	"g723tolin",
	AST_FORMAT_G723_1, AST_FORMAT_SLINEAR,
	g723tolin_new,
	g723tolin_framein,
	g723tolin_frameout,
	g723_release,
	g723tolin_sample };

static struct ast_translator lintog723 = {
	"lintog723",
	AST_FORMAT_SLINEAR, AST_FORMAT_G723_1,
	lintog723_new,
	lintog723_framein,
	lintog723_frameout,
	g723_release,
	lintog723_sample };

int reload(void) {
	// do nothing for now
	return 0;
}

int load_module(void) {
	int res;

	/* We should read this from a config file */
	defaultSendRate = DEFAULT_SEND_RATE;

	res = ast_register_translator(&g723tolin);
	if(!res)
		res = ast_register_translator(&lintog723);
	else
		ast_unregister_translator(&g723tolin);
	return res;
}

int unload_module(void) {
	int res;
	ast_mutex_lock(&localuser_lock);
	res = ast_unregister_translator(&lintog723);
	if(!res)
		res = ast_unregister_translator(&g723tolin);
	if(localusecnt)
		res = -1;
	ast_mutex_unlock(&localuser_lock);
	return res;
}

char *description(void) {
	return tdesc;
}

int usecount(void) {
	int res;
	STANDARD_USECOUNT(res);
	return res;
}

char *key() {
	return ASTERISK_GPL_KEY;
}

