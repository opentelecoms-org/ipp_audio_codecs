
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "usc.h"
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "decoder.h"


int main(int argc, char *argv[]) {

  char *bitstream_buf;
  char *buf_out;
  char *p;
  char *f;
  long file_size;
  int i;

  if(argc != 3) {
    fprintf(stderr, "Usage: my_dec <audio.g729> <audio.raw>\n");
    exit(-1);
  }

  fprintf(stderr, "Converting %s to %s\n", argv[1], argv[2]);


  USC_Fxns *USC_CODEC_Fxns;
  USC_CodecInfo pInfo;
  USC_Handle decoder;

  USC_Bitstream in;
  USC_PCMStream out;
  USC_MemBank* pBanks;

  int nbanks = 0;
  int maxbitsize;
  int inFrameSize = 10;
  int outFrameSize;

  USC_CODEC_Fxns = USC_GetCodecByName ();
  USC_CODEC_Fxns->std.GetInfo((USC_Handle)NULL, &pInfo); /* codec info */

  ((USC_Option*)pInfo.params)->modes.bitrate = 0;
  ((USC_Option*)pInfo.params)->modes.truncate = 1;
  ((USC_Option*)pInfo.params)->direction = 1;

  FILE *f_in = fopen(argv[1], "r");
  FILE *f_out = fopen(argv[2], "w");

  fseek(f_in, 0, SEEK_END);
  file_size = ftell(f_in);
  fseek(f_in, 0, SEEK_SET);

  bitstream_buf = ippsMalloc_8s(file_size);
  fread(bitstream_buf, file_size, 1, f_in);
  p = bitstream_buf;
  f = bitstream_buf + file_size;

  USC_CODEC_Fxns->std.NumAlloc(pInfo.params, &nbanks);
  pBanks = (USC_MemBank*)ippsMalloc_8u(sizeof(USC_MemBank)*nbanks);
  USC_CODEC_Fxns->std.MemAlloc(pInfo.params, pBanks);
  for(i=0; i<nbanks;i++) {
    if(!(pBanks[i].pMem = ippsMalloc_8u(pBanks->nbytes))) {
      printf("\nLow memory: %d bytes not allocated\n",pBanks->nbytes);
      return -1;
    }
  }

  outFrameSize = getOutFrameSize();

  buf_out = ippsMalloc_8s(getOutFrameSize());

  maxbitsize = pInfo.maxbitsize;
  USC_CODEC_Fxns->std.Init(pInfo.params, pBanks, &decoder);

  ippCoreSetFlushToZero( 1, NULL );

  in.nbytes  = maxbitsize;
  in.bitrate = 0;
  in.frametype = 3;
  out.pBuffer = buf_out;
  out.pcmType.bitPerSample = 0;
  out.pcmType.sample_frequency = 0;

  USC_CODEC_Fxns->std.Reinit(&((USC_Option*)pInfo.params)->modes, decoder);

  while(p < f) {
    in.pBuffer = p;
    USC_CODEC_Fxns->std.Decode (decoder, &in, &out);
    fwrite(out.pBuffer, outFrameSize, 1, f_out);

    p += inFrameSize;
  }
  
  fclose(f_out);
  fclose(f_in);

}

