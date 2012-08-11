
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "usc.h"
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "encoder.h"


int main(int argc, char *argv[]) {

  char *buf_in;
  char *bitstream_buf;
  char *p;
  char *f;
  long file_size;
  int i;

  if(argc != 3) {
    fprintf(stderr, "Usage: my_enc audio.raw audio.g729\n");
    exit(-1);
  }
  
  fprintf(stderr, "Converting %s to %s\n", argv[1], argv[2]);

  USC_Fxns *USC_CODEC_Fxns;
  USC_CodecInfo pInfo;
  USC_Handle encoder;

  USC_PCMStream in;
  USC_Bitstream out;
  USC_MemBank* pBanks;

  int nbanks = 0;
  int maxbitsize;
  int inFrameSize;
  int outFrameSize = 0;

  USC_CODEC_Fxns = USC_GetCodecByName_xx2 ();
  USC_CODEC_Fxns->std.GetInfo((USC_Handle)NULL, &pInfo); /* codec info */
  ((USC_Option*)pInfo.params)->modes.truncate = 1;
  ((USC_Option*)pInfo.params)->direction = 0;
  ((USC_Option*)pInfo.params)->modes.vad = 0;

  FILE *f_in = fopen(argv[1], "r");
  FILE *f_out = fopen(argv[2], "w");

  fseek(f_in, 0, SEEK_END);
  file_size = ftell(f_in);
  fseek(f_in, 0, SEEK_SET);

  buf_in = ippsMalloc_8s(file_size);
  fread(buf_in, file_size, 1, f_in);
  p = buf_in;
  f = buf_in + file_size;

  USC_CODEC_Fxns->std.NumAlloc(pInfo.params, &nbanks);
  pBanks = (USC_MemBank*)ippsMalloc_8u(sizeof(USC_MemBank)*nbanks);
  USC_CODEC_Fxns->std.MemAlloc(pInfo.params, pBanks);
  for(i=0; i<nbanks;i++) {
    if(!(pBanks[i].pMem = ippsMalloc_8u(pBanks->nbytes))) {
      printf("\nLow memory: %d bytes not allocated\n",pBanks->nbytes);
      return -1;
    }
  }

  inFrameSize = getInFrameSize();
  outFrameSize = getOutFrameSize_xx3();

  bitstream_buf = ippsMalloc_8s(pInfo.maxbitsize);

  in.bitrate = pInfo.params->modes.bitrate;
  maxbitsize = pInfo.maxbitsize;
  USC_CODEC_Fxns->std.Init(pInfo.params, pBanks, &encoder);

  ippCoreSetFlushToZero( 1, NULL );
  in.pcmType.bitPerSample = pInfo.pcmType->bitPerSample;
  in.pcmType.sample_frequency = pInfo.pcmType->sample_frequency;
  out.nbytes  = maxbitsize;
  out.pBuffer = bitstream_buf;

  USC_CODEC_Fxns->std.Reinit(&((USC_Option*)pInfo.params)->modes, encoder);

  while(p < f) {
    in.pBuffer = p;
    USC_CODEC_Fxns->std.Encode (encoder, &in, &out);
    fwrite(out.pBuffer, 10, 1, f_out);
    p += inFrameSize; 
  }

  fclose(f_out);
  fclose(f_in);

  return 0;
}


