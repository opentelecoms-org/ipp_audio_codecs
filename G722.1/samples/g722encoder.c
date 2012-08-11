/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.722.1 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.722.1 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.722.1 encoder demo
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "g722encoder.h"
#ifdef _DEBUG
	#include <CRTDBG.H>
#endif

#define USAGE          1
#define FOPEN_FAIL     2
#define MEMORY_FAIL    3
#define UNKNOWN_FORMAT 4
#define THREAD_FAIL    5
#define MT_SAFETY      6

#define MAX_THREADS 2048
/* This object and functions are used to measure codec speed */
typedef struct {
   Ipp64u p_start;
   Ipp64u p_end;
   double speed_in_mhz;
}MeasureIt;

Ipp64s diffclocks(Ipp64u s, Ipp64u e){
   if(s < e) return (e-s);
   return (IPP_MAX_64S-e+s);
}


double getUSec(Ipp64u s, Ipp64u e) {
   return diffclocks(s,e)/1000000.0;
}

void measure_start(MeasureIt *m)
{
   m->p_start = ippGetCpuClocks();
}
void measure_end(MeasureIt *m)
{
   m->p_end = ippGetCpuClocks();
}
static void measure_output(MeasureIt *m, float speech_sec, int nTh, int optionreports){
   int MHz;
   m->speed_in_mhz = getUSec(m->p_start,m->p_end)/speech_sec;
   if(optionreports){
      ippGetCpuFreqMhz(&MHz);
	   printf("%4.2f MHz per channel on CPU runs at %d MHz\n",m->speed_in_mhz,MHz);
   }else{
	   printf("%4.2f MHz per channel\n",m->speed_in_mhz);
   }
}
/**/


/* This object is used to control the command line input */ 
typedef struct {
   short pack;
   short bitRate;
   short bitFrameSize;
   short wordFrameSize;
   FILE* fpIn;
   FILE* fpOut;
} SEncoderControl;

static G722Encoder_Obj *encObjVec[MAX_THREADS];
static vm_thread ThreadHandle[MAX_THREADS];
static Ipp8u *in_buff=NULL;
static Ipp8u *out_buff[MAX_THREADS];

typedef struct {
   int thrdNumber;
   int n_repeat;
   int nFrames;
   int totalBytes;
   SEncoderControl* pEncCtrl;
} ThreadArgType;
static ThreadArgType ThArg[MAX_THREADS];

static void write_ITU_format(short *out_buff, short nBitsPerFrame,
               short nWordsPerFrame, FILE* fp_bitstream, int nFrames);
static void* thread_proc(void* pArg);
static void ippsCompare_8u(Ipp8u *pSrc1, Ipp8u *pSrc2, int len, int *pResult);



int main(short argc,char *argv[]){
   char* appName=argv[0];
   char* inputFileName=NULL;
   char* outputFileName=NULL;
   int optionreports=0, usage=0, puttocsv=0;
   int n_repeat=1, nThreads=1;
   FILE* f_csv=NULL;/* csv File */ 
   SEncoderControl control;
   int i, nFrames, in_len;
   Ipp32u encoderSize;
   int result, thrdErr;

   MeasureIt measure;
   double speech_sec;
   const IppLibraryVersion *ver = ippscGetLibVersion(); 

   /* parse the command line input into the control structure */ 
   control.pack = 1;
   control.bitRate = 32000; /* default */
   argc--;
   while(argc-- > 0){
      argv++;
      if ('-' == (*argv)[0]) { 
         if (!strncmp("-r",*argv,2)){
            control.bitRate = (short)(atoi(*argv+2));
            if ((control.bitRate < 16000) || (control.bitRate > 32000) ||
               ((control.bitRate/800)*800 != control.bitRate)) { 
               printf("codec: Error. bit-rate must be multiple of 800 between 16000 and 32000\n");
               return USAGE;
            } 
            continue;
         } else if(!strcmp("-pack",*argv)) {
            control.pack = 0;
            continue ;
         } else if (!strcmp(*argv,"-s")) {
            if(argc-->0) {
               int rep = atoi(*++argv);
               if(0 == rep) rep=1; 
               if(rep > n_repeat) n_repeat=rep;
            }
            continue;
         } else if (!strcmp(*argv,"-t")) {
            if(argc-->0) {
               int thread = atoi(*++argv);
               if(0 == thread) thread = 1; 
               if(thread > MAX_THREADS) nThreads = MAX_THREADS;
               if(thread > nThreads) nThreads = thread;
            }
            continue;
         }
         if (strchr(*argv,'n')){
            optionreports = 1;
         }
         if (strchr(*argv,'c')){
            if(0 == n_repeat) n_repeat=1;
            puttocsv=1;
         }
      } else {
         argc++;
         argv--;
         break;
      }
   }
   if(argc-->0){
      argv++; 
      inputFileName = *argv;
      if ((control.fpIn = fopen(inputFileName,"rb")) == NULL) {
         printf("codec: error opening %s.\n",inputFileName);
         return FOPEN_FAIL;
      } 
   }
   if(argc-->0){
      argv++; 
      outputFileName = *argv;
      if ((control.fpOut = fopen(outputFileName,"wb")) == NULL) { 
         printf("codec: error opening %s.\n",outputFileName);
         return FOPEN_FAIL;
      } 
   }
   if (!usage && !inputFileName) {
      printf("Unknown input file.\n");
      usage=1;
   }
   if(!usage && !outputFileName) {
      printf("Unknown output file.\n");
      usage=1;
   }

   if ( usage ) {
      printf("Usage: %s <options> <input-audio-file> <output-bitstream-file> \n", appName);
      printf("                  encode 16bit 16000 Hz PCM speech input file\n");
      printf("                       to bitstream output file\n");
      printf("   [-rXXXXX]      where XXXXX - bit rate\n");
      printf("   [-pack]        must be set if packed format (optional). \n");
      printf("   [-n]           option reports (optional) . \n");
      printf("   [-c]         write performance line to codecspeed.csv file (optional). \n");
      printf("   [-s [<num>]]   repeater, how many times to repeat an operation (optional).\n");
      return USAGE;
   }

   control.bitFrameSize = (short)((control.bitRate)/50);

   if(optionreports){
      printf("Intel speech codec bit-to-bit compatible with ITU-T G.722.1\n");
      printf("     Coding at 24 and 32 16 kbit/s for hand-free operation \n");
      printf("     in systems with low frame loss.\n");
      printf("IPP library used:  %d.%d.%d Build %d, name %s\n",
         ver->major,ver->minor,ver->majorBuild,ver->build,ver->Name);
      printf("bandwidth = 7 khz\n");
      printf("syntax = %d ",control.pack);

      if (control.pack == 0)
         printf(" packed bitstream\n");
      else if (control.pack == 1)
         printf(" ITU selection test bitstream\n");

      printf("sample rate = 16000    bit_rate = %d\n",control.bitRate);
      printf("framesize = 320 samples\n");
      printf("number of regions = 14\n");
      printf("number of bits per frame = %d bits\n",control.bitFrameSize);
      printf("Input %s file%s:  %s\n",  "speech", 
                                         "    ",inputFileName);
      printf("Output %s file%s:  %s\n", "bitstream", 
                                         "",outputFileName);
      printf("Encode ... ");
   }

   if (puttocsv) {
      if ( (f_csv = fopen("codecspeed.csv", "a")) == NULL) {
         printf("\nFile codecspeed.csv could not be open.\n");
         return FOPEN_FAIL;
      }
   }

   control.wordFrameSize = (short)(control.bitFrameSize/16);

   fseek(control.fpIn, 0, SEEK_END);
   in_len = ftell(control.fpIn);
   if(!(in_buff = ippsMalloc_8u(in_len))){
      printf("\nNo memory for load of %d bytes from input file", in_len);
      return MEMORY_FAIL;
   }
   rewind(control.fpIn);
   fread(in_buff, 1, in_len, control.fpIn);
   fclose(control.fpIn);

   nFrames = (int)(in_len / (G722_FRAMESIZE*sizeof(short)));
   /* Allocate [nThreads] out buffers */ 
   for (i=0; i<nThreads; i++){
      out_buff[i] = ippsMalloc_8u(nFrames*(G722_MAX_BITS_PER_FRAME/16)*sizeof(short));
      if(!out_buff[i]){
         printf("\nNo memory for load of %d bytes from input file",nFrames*(G722_MAX_BITS_PER_FRAME/16)*sizeof(short));
         return MEMORY_FAIL;
      }
   }
   /* Allocate [nThreads] encoder objects */ 
   apiG722Encoder_Alloc(&encoderSize);
   for (i=0; i<nThreads; i++){
      if (!(encObjVec[i] = (G722Encoder_Obj*)ippsMalloc_8u(encoderSize))){
         printf("\nLow memory: %d bytes not allocated\n", encoderSize);
         return MEMORY_FAIL;
      }
   }
   /* time stamp prior to threads creation, creation and running time may overlap. */
   measure_start(&measure); 
   
   /* Create threads */ 
   for (i=0; i<nThreads; i++){
      int err_crth;
      ThArg[i].thrdNumber = i; /* thread number */ 
      ThArg[i].n_repeat = n_repeat; /* number of repeatition */ 
      ThArg[i].nFrames = nFrames;
      ThArg[i].pEncCtrl = &control;
      if ( (err_crth = vm_thread_create(&ThreadHandle[i],thread_proc,&ThArg[i])) == 0 )
      { 
          printf("CreateThread fail !! [err=%d] \n",err_crth);
          return THREAD_FAIL;
      } 
  }


   for (i=0; i<nThreads; i++){
      vm_thread_wait(&ThreadHandle[i]);
   }

   measure_end(&measure);

   thrdErr = 0;
   vm_thread_terminate(&ThreadHandle[0]);
   for (i=1; i<nThreads; i++){
      vm_thread_terminate(&ThreadHandle[i]);
      if (ThArg[0].totalBytes == ThArg[i].totalBytes){
         ippsCompare_8u(out_buff[0], out_buff[i], 
            control.wordFrameSize*nFrames*sizeof(short), &result);
         if (result)
            thrdErr++;
      } else
         thrdErr++;
      ippsFree(encObjVec[i]);
      ippsFree(out_buff[i]);
   }
   
   if (!thrdErr){
      /* Write output bitstream to the output file */ 
      if (control.pack == 0)
         fwrite(out_buff[0], 2, (control.wordFrameSize*nFrames),control.fpOut);
      else
         write_ITU_format((short *)out_buff[0], control.bitFrameSize,
            control.wordFrameSize, control.fpOut,nFrames);
   }
   fclose(control.fpOut);

   ippsFree(encObjVec[0]);
   ippsFree(out_buff[0]);
   
   speech_sec = (ThArg[0].totalBytes/(int)(sizeof(short)))/16000.0;
   printf("Done %d samples of 16000 Hz PCM wave file (%g sec)\n",
      ThArg[0].totalBytes/(int)(sizeof(short)), speech_sec);
   measure_output(&measure,speech_sec,nThreads,optionreports);

   if (puttocsv) {
      fprintf(f_csv,"%s,%s,%s,%4.2f,%s,%4.2f,%4.2f,%d\n",
         "G722.1","encode","",control.bitRate/1000.0,inputFileName,speech_sec,measure.speed_in_mhz,nThreads);
      fclose(f_csv);
   }

   ippsFree(in_buff);

#ifdef _DEBUG
   _CrtCheckMemory ();
   _CrtDumpMemoryLeaks ();
#endif
   if (thrdErr) {
       printf("Failed !!!\n");
       return MT_SAFETY;
   }
   printf("Completed !!!\n");
   return 0;
}

//************************************************************************************
//    Procedure/Function:     Write ITU format function 
//
//    Description:           Writes file output in PACKED ITU format
//************************************************************************************/
static void write_ITU_format(short *out_buff, short nBitsPerFrame,
               short nWordsPerFrame, FILE   *fp_bitstream, int nFrames) {
   short frame_start = 0x6b21;
   short one = 0x0081;
   short zero = 0x007f;

   short  i,j, k;
   short  packed_word;
   short  bit_count;
   short  bit;
   short  out_array[G722_MAX_BITS_PER_FRAME+2];
   short  *pOutBuff = out_buff;

   for(k = 0; k < nFrames;k++) {
      j = 0;
      out_array[j++] = frame_start;
      out_array[j++] = nBitsPerFrame;
      for (i=0; i<nWordsPerFrame; i++){ 
         packed_word = pOutBuff[i];
         bit_count = 15;
         while (bit_count >= 0){ 
            bit = (short)((packed_word >> bit_count) & 1);
            bit_count--;
            if (bit == 0)
                out_array[j++] = zero;
            else
                out_array[j++] = one;
         } 
      } 
      fwrite(out_array, 2, nBitsPerFrame+2, fp_bitstream);
      pOutBuff += nWordsPerFrame;
   }
}

static void* thread_proc(void* pArg)
{
   ThreadArgType* pThArg = (ThreadArgType*)pArg;
   Ipp8u *pInBuff, *pOutBuff;
   int nThrd = pThArg->thrdNumber;
   int curFrame;

   pThArg->totalBytes = 0;
   while(pThArg->n_repeat--){
      curFrame = pThArg->nFrames - 1;
      apiG722Encoder_Init(encObjVec[nThrd]);
      /* Read first frame of samples from buff. */ 
      pInBuff = in_buff;
      pOutBuff = out_buff[nThrd];
      while (curFrame>=0){ 
         pThArg->totalBytes+=G722_FRAMESIZE*sizeof(short);
         apiG722Encode(encObjVec[nThrd], pThArg->pEncCtrl->bitFrameSize, 
            (short*)pInBuff,(short*)pOutBuff);
         /* Read next frame of samples from buff. */ 
         pInBuff += G722_FRAMESIZE*sizeof(short);
         pOutBuff += pThArg->pEncCtrl->wordFrameSize*sizeof(short);
         curFrame--;
      }
   }
   return 0;
}

static void ippsCompare_8u(Ipp8u *pSrc1, Ipp8u *pSrc2, int len, int *pResult){
   int i;
   *pResult = 0;
   for (i=0; i<len; i++) {
      if (pSrc1[i] != pSrc2[i]) {
         *pResult = 1;
         return;
      }
   }
}


