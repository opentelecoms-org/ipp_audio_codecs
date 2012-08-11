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
//  Purpose: G.728 decoder API demo
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ippcore.h>
#include <ipps.h>
#include "g728decoder.h"
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
void measure_output(MeasureIt *m, double speech_sec, int nTh ){
   int MHz=500;
   m->speed_in_mhz = getUSec(m->p_start,m->p_end)/(nTh*speech_sec);
   ippGetCpuFreqMhz(&MHz);
   printf("%4.2f MHz per channel on CPU runs at %d MHz\n",m->speed_in_mhz,MHz);
}
/**/



typedef struct {
   int thrdNumber;
   int nRepeat;
   int srcLen;
   unsigned int totalSamples;
} ThreadArgType;
static ThreadArgType ThArg[MAX_THREADS];

typedef struct {
   G728_Type type;
   G728_Rate rate;
   G728_PST pst;
} SDecoderControl;

SDecoderControl decCtrl;

static G728Decoder_Obj *decObjVec[MAX_THREADS];
static vm_thread ThreadHandle[MAX_THREADS];

static Ipp8u *in_buff=NULL;
static short *out_buff[MAX_THREADS];

static int  PrepareInput(FILE* pFile, char* comment, Ipp8u** pBuff, int* out_len);

static void prm2bits_G728(const short* prm, unsigned char* prev_bufi,
               unsigned char* bitstream, G728_Rate rate, int* frame_counter);
static void prm2bits_G728I(const short* prm,  unsigned char* prev_bufi,
               unsigned char* bitstream, G728_Rate rate, int* frame_counter);
typedef void (*DecodedBitsPack_fun)(const short*, unsigned char*,
                                    unsigned char*, G728_Rate, int*);
DecodedBitsPack_fun prm2bits; 

static int GetFrameLen_G728(int frame_counter);
static int GetFrameLen_G728I(int frame_counter);
typedef int (*GetFrameLen_func)(int);
GetFrameLen_func GetFrameLen;

static void* thread_proc(void* pArg);

static void ippsCompare_8u(Ipp8u *pSrc1, Ipp8u *pSrc2, unsigned int len, int *pResult);

int main(int argc,char *argv[]){
   char* appName=argv[0];
   char* inputFileName=NULL;
   char* outputFileName=NULL;
   FILE* fpIn=NULL;
   FILE* fpOut=NULL;
   int optionreports=0, usage=0, puttocsv=0;
   int nRepeat=1, nThreads=1;
   int rateVal=16000;
   FILE *f_csv=NULL; /* csv File */ 

   int i, nFrames, srcLen;
   Ipp32u decoderSize, dstLen;
   int result, thrdErr;

   MeasureIt measure;
   double speech_sec ; 
   
   const  IppLibraryVersion *ver = ippscGetLibVersion(); 

   decCtrl.pst = G728_PST_ON;
   decCtrl.rate = G728_Rate_16000;
   decCtrl.type = G728_Pure;

   argc--;
   while(argc-- > 0){
      argv++;
      if ('-' == (*argv)[0]) {
         if (!strncmp("-r",*argv,2)){
            if (!strcmp("128",*argv+2)){
               decCtrl.rate = G728_Rate_12800;
               rateVal = 12800;
            } else if(!strcmp("96",*argv+2)){
               decCtrl.rate = G728_Rate_9600;
               rateVal = 9600;
            }
            continue;
         } else if (!strncmp("-fe",*argv,3)){
            decCtrl.type = G728_Annex_I;
            continue;
         } else if (!strcmp(*argv,"-s")){
            if(argc-->0){
               int rep = atoi(*++argv);
               if(0 == rep) rep=1; 
               if(rep > nRepeat) nRepeat=rep;
            }
            continue;
         } else if(!strcmp("-Nop",*argv)) {
            decCtrl.pst = G728_PST_OFF;
            continue ;
         } else if (!strcmp(*argv,"-t")){
            if(argc-->0){
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
            if(0 == nRepeat) nRepeat=1;
            puttocsv=1;
         }
      }else{
         argc++;
         argv--;
         break;
      }
   }
   if(argc-->0){
      argv++;
      inputFileName = *argv;
      if ((fpIn = fopen(inputFileName,"rb")) == NULL){
         printf("\nDecoder: error opening %s.\n", inputFileName);
         return FOPEN_FAIL;
      }
   } else {
      printf("Unknown input file.\n");
      usage=1;
   }
   if(argc-->0){
      argv++; 
      outputFileName = *argv;
      if ((fpOut = fopen(outputFileName,"wb")) == NULL){ 
         printf("\nDecoder: error opening %s.\n",outputFileName);
         return FOPEN_FAIL;
      }
   } else {
      printf("Unknown output file.\n");
      usage=1;
   }
   if ( usage ){
      printf("Usage: %s <options> <input-bitstream-file> <output-audio-file> \n", appName);
      printf("   [-rXX]       where XX =( 96, 16, 128) - bit rate: 9.6, 16 or 12.8 Kbit/s\n");
      printf("   [-a|u]       a - ADPCM bitstream after A-Law PCM\n");
      printf("                u - ADPCM bitstream  after Mu-Law PCM\n");
      printf("                otherwize ADPCM after 8 KHz linear 16-bit PCM supposed");
      printf("   [-fe]        G.728 Frame or packet loss concealment (Annex I) \n");
      printf("   [-Nop]       Without postfilter (optional) \n");
      printf("   [-n]         option reports (optional) . \n");
      printf("   [-c]         write performance line to codecspeed.csv file (optional). \n");
      printf("   [-s [<num>]] repeater, how many times to repeat an operation (optional).\n");
      printf("   [-t [<num>]] number of threads , default 1 (optional).\n");
      return USAGE;
   }
   if(optionreports){
      printf("Intel LD-CELP speech decoder bit-to-bit compatible with G.728/H.\n");
      printf("IPP library used:  %d.%d.%d Build %d, name %s\n",
         ver->major,ver->minor,ver->majorBuild,ver->build,ver->Name);
      printf("sample_rate = 8000,    bit_rate = %d\n", rateVal);
      printf("Input ADPCM file: %s\n",  inputFileName);
      printf("Output 8 KHz PCM file:  %s\n", outputFileName);
      printf("Decode ... ");
   }
   if (puttocsv) { 
      if ( (f_csv = fopen("codecspeed.csv", "a")) == NULL) {
         printf("\nFile codecspeed.csv could not be open.\n");
         return FOPEN_FAIL;
      }
   }

   prm2bits = prm2bits_G728;
   GetFrameLen = GetFrameLen_G728;

   if(decCtrl.type == G728_Annex_I){
      decCtrl.pst = G728_PST_ON;
      prm2bits = prm2bits_G728I;
      GetFrameLen = GetFrameLen_G728I;
   }

   /* load input file */ 
   if (result = PrepareInput(fpIn, "input", &in_buff, &srcLen))
      return result;
   fclose(fpIn);
   /* Allocate [nThreads] out buffers */ 
   nFrames = (srcLen*16)/17+1;
   for (i=0; i<nThreads; i++){
      if(!(out_buff[i]=(short*)ippsMalloc_8u(G728_FRAME*nFrames*sizeof(short)))){
         printf("\nLow memory: %d bytes not allocated (out_buff, thread N:%d)\n",
            (int)(srcLen * sizeof(short)), i);
         return MEMORY_FAIL;
      }
   }
   /* Allocate [nThreads] decoder objects */ 
   apiG728Decoder_Alloc(&decoderSize);
   for (i=0; i<nThreads; i++){
      if (!(decObjVec[i] = (G728Decoder_Obj*)ippsMalloc_8u(decoderSize))){
         printf("\nLow memory: %d bytes not allocated (decObj, thread N:%d)\n", 
            decoderSize, i);
         return MEMORY_FAIL;
      }
   }
   /* time stamp prior to threads creation, creation and running time may overlap. */
   measure_start(&measure); 
   
   /* Create threads */ 
   for (i=0; i<nThreads; i++){
      int err_crth;
      ThArg[i].thrdNumber = i;    /* thread number */ 
      ThArg[i].nRepeat = nRepeat; /* number of repeatitions */ 
      ThArg[i].srcLen = srcLen;
      ThArg[i].totalSamples = 0;
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
   /* Check result vectors */ 
   if(decCtrl.type==G728_Pure){
      dstLen = srcLen*G728_VECTOR*sizeof(short);
   } else if (decCtrl.type==G728_Annex_I) {
      dstLen = (srcLen*16/17)*G728_VECTOR*sizeof(short);
   }
   thrdErr = 0;
   vm_thread_terminate(&ThreadHandle[0]);
   for (i=1; i<nThreads; i++){
      vm_thread_terminate(&ThreadHandle[i]);
      if (ThArg[0].totalSamples == ThArg[i].totalSamples){
         ippsCompare_8u((Ipp8u*)(out_buff[0]), (Ipp8u*)(out_buff[i]), dstLen, &result);
         if (result)
            thrdErr++;
      } else
         thrdErr++;
      ippsFree(decObjVec[i]);
      ippsFree(out_buff[i]);
   }
   /* Write output PCM to the output file */ 
   if (!thrdErr)
      fwrite(out_buff[0], 1, dstLen, fpOut);
   fclose(fpOut);

   ippsFree(decObjVec[0]);
   ippsFree(out_buff[0]);

   speech_sec = ThArg[0].totalSamples/8000.0;
   printf("\nDone %d samples of ADPCM file (%g sec)\n",
      ThArg[0].totalSamples, speech_sec);
   
   measure_output(&measure,speech_sec,nThreads);
   if (puttocsv) {
      fprintf(f_csv,"%s,%s,%s,%4.2f,%s,%4.2f,%4.2f,%d\n",
         "G728","decode","", rateVal/1000.0,inputFileName,speech_sec,measure.speed_in_mhz,nThreads);
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

int  PrepareInput(FILE* pFile, char* comment, Ipp8u** pBuff, int* out_len)
{
   int lenFile;
   Ipp8u *buff = NULL;
   fseek(pFile,0,SEEK_END);
   lenFile = ftell(pFile); // size of file
   if(!(buff=ippsMalloc_8u(lenFile))){
      printf("\nNo memory for load of %d bytes from %s file",lenFile,comment);
      return MEMORY_FAIL;
   }

   rewind(pFile);
   lenFile = fread(buff,1,lenFile,pFile); 
   
   *pBuff = buff; 
   *out_len = lenFile/2;
   return 0;
}

static void prm2bits_G728(const short* prm,  unsigned char* prev_bfi,
               unsigned char* bitstream, G728_Rate rate, int* frame_counter){
   if(rate==G728_Rate_12800)     { 
      bitstream[0] = (unsigned char) prm[0];                               
      bitstream[1] = (unsigned char) prm[1]; 
      bitstream[2] = (unsigned char) prm[2]; 
      bitstream[3] = (unsigned char) prm[3]; 
   }
   else if(rate==G728_Rate_9600) { 
      bitstream[0] = (unsigned char) (( prm[0] << 2 ) | ( prm[1] >> 4));       
      bitstream[1] = (unsigned char) (( (prm[1] & 0xf) << 4 ) | (prm[2] >> 2));
      bitstream[2] = (unsigned char) (( (prm[2] & 0x3) << 6 )| prm[3]);        
   }
   else if(rate==G728_Rate_16000){ 
      bitstream[0] = (unsigned char) ( prm[0] >> 2);                               
      bitstream[1] = (unsigned char) (( (prm[0] & 0x3) << 6 ) | (prm[1] >> 4)); 
      bitstream[2] = (unsigned char) (( (prm[1] & 0xf) << 4 ) | (prm[2] >> 6)); 
      bitstream[3] = (unsigned char) (( (prm[2] & 0x3f) << 2 )| (prm[3] >> 8)); 
      bitstream[4] = (unsigned char) (prm[3] & 0xff);                              
   }
}

static void prm2bits_G728I(const short* prm, unsigned char* prev_bfi,
               unsigned char* bitstream, G728_Rate rate, int* frame_counter){
   if(rate==G728_Rate_12800)     { 
      if(*frame_counter==4) {
         *frame_counter = 0;
         *prev_bfi = (unsigned char) prm[0];
         bitstream[0] = (unsigned char) prm[0];
         bitstream[1] = (unsigned char) prm[1];                               
         bitstream[2] = (unsigned char) prm[2]; 
         bitstream[3] = (unsigned char) prm[3]; 
         bitstream[4] = (unsigned char) prm[4]; 
      } else {
         bitstream[0] = *prev_bfi;
         bitstream[1] = (unsigned char) prm[0];                               
         bitstream[2] = (unsigned char) prm[1]; 
         bitstream[3] = (unsigned char) prm[2]; 
         bitstream[4] = (unsigned char) prm[3]; 
      }
   } else if(rate==G728_Rate_9600) {
      if(*frame_counter==4) {
         *frame_counter = 0;
         *prev_bfi = (unsigned char) prm[0];
         bitstream[0] = (unsigned char) prm[0];
         bitstream[1] = (unsigned char) (( prm[1] << 2 ) | ( prm[2] >> 4));       
         bitstream[2] = (unsigned char) (( (prm[2] & 0xf) << 4 ) | (prm[3] >> 2));
         bitstream[3] = (unsigned char) (( (prm[3] & 0x3) << 6 )| prm[4]);        
      } else {
         bitstream[0] = *prev_bfi;
         bitstream[1] = (unsigned char) (( prm[0] << 2 ) | ( prm[1] >> 4));       
         bitstream[2] = (unsigned char) (( (prm[1] & 0xf) << 4 ) | (prm[2] >> 2));
         bitstream[3] = (unsigned char) (( (prm[2] & 0x3) << 6 )| prm[3]);        
      }
   } else if(rate==G728_Rate_16000){
      if(*frame_counter==4) {
         *frame_counter = 0;
         *prev_bfi = (unsigned char) prm[0];
         bitstream[0] = (unsigned char) prm[0];
         bitstream[1] = (unsigned char) ( prm[1] >> 2);                               
         bitstream[2] = (unsigned char) (( (prm[1] & 0x3) << 6 ) | (prm[2] >> 4)); 
         bitstream[3] = (unsigned char) (( (prm[2] & 0xf) << 4 ) | (prm[3] >> 6)); 
         bitstream[4] = (unsigned char) (( (prm[3] & 0x3f) << 2 )| (prm[4] >> 8)); 
         bitstream[5] = (unsigned char) (prm[4] & 0xff);                              
      } else {
         bitstream[0] = *prev_bfi;
         bitstream[1] = (unsigned char) ( prm[0] >> 2);                               
         bitstream[2] = (unsigned char) (( (prm[0] & 0x3) << 6 ) | (prm[1] >> 4)); 
         bitstream[3] = (unsigned char) (( (prm[1] & 0xf) << 4 ) | (prm[2] >> 6)); 
         bitstream[4] = (unsigned char) (( (prm[2] & 0x3f) << 2 )| (prm[3] >> 8)); 
         bitstream[5] = (unsigned char) (prm[3] & 0xff);                              
      }
   }
   (*frame_counter)++;
}

static int GetFrameLen_G728(int frame_counter){
   return 4;
}

static int GetFrameLen_G728I(int frame_counter){
   if(frame_counter==4)
      return 5;
   else 
      return 4;
}

static void* thread_proc(void* pArg)
{
   ThreadArgType* pThArg = (ThreadArgType*)pArg;
   short* pInBuff, *pOutBuff;
   unsigned char bitstream[6], prev_bfi;
   int nThrd = pThArg->thrdNumber;
   int curLen, curFrameLen, frameCount;

   pThArg->totalSamples = 0;
   while(pThArg->nRepeat--){
      pInBuff = (short *)in_buff;
      pOutBuff = (short*)(out_buff[nThrd]);
      curLen = pThArg->srcLen;
      frameCount = 4;
      /* initialize the decoder */ 
      apiG728Decoder_Init(decObjVec[nThrd], decCtrl.type, decCtrl.rate, decCtrl.pst);
      /* decode file */ 
      while(curLen > 0){
         curFrameLen = GetFrameLen(frameCount);

         prm2bits(pInBuff, &prev_bfi, bitstream, decCtrl.rate, &frameCount);
         
         apiG728Decode(decObjVec[nThrd], bitstream, pOutBuff);
         
         curLen -= curFrameLen;
         pInBuff += curFrameLen;

         pOutBuff += G728_FRAME; 
         pThArg->totalSamples += G728_FRAME;
      }
   }
   return 0;
}

static void ippsCompare_8u(Ipp8u *pSrc1, Ipp8u *pSrc2, unsigned int len, int *pResult){
   unsigned int i;
   *pResult = 0;
   for (i=0; i<len; i++) {
      if (pSrc1[i] != pSrc2[i]) {
         *pResult = 1;
         return;
      }
   }
}
