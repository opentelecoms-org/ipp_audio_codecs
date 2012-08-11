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
//  Purpose: G.728 encoder API demo
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ippcore.h>
#include <ipps.h>
#include "g728encoder.h"
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


typedef struct {
   int thrdNumber;
   int nRepeat;
   int nFrames;
   Ipp32u totalSamples;
} ThreadArgType;
static ThreadArgType ThArg[MAX_THREADS];

typedef struct {
   G728_Type type;
   G728_Rate rate;
   G728_PST pst;
   G728_PCM_Law law;
} SEncoderControl;

static SEncoderControl encCtrl;

static G728Encoder_Obj *encObjVec[MAX_THREADS];
static vm_thread ThreadHandle[MAX_THREADS];

static Ipp16s *in_buff=NULL;
static Ipp16s *out_buff[MAX_THREADS];

static void bits2prm(const unsigned char* bitstream, short* prm, G728_Rate rate);
static int  PrepareInput(FILE* pFile, char* comment, Ipp16s** pBuff, Ipp32u* dstLen);

static void* thread_proc(void* pArg);

static void ippsCompare_8u(Ipp8u *pSrc1, Ipp8u *pSrc2, unsigned int len, int *pResult);

int main(int argc,char *argv[]){
   char* appName=argv[0];
   char* inputFileName=NULL;
   char* outputFileName=NULL;
   FILE* fpIn = NULL;
   FILE* fpOut = NULL;
   int optionreports=0, usage=0, puttocsv=0;
   int nRepeat=1, nThreads=1;
   int rateVal=16000;
   FILE *f_csv=NULL; /* csv File */ 

   int i, nFrames;
   Ipp32u encoderSize, srcLen, dstLen;
   int result, thrdErr;

   MeasureIt measure;
   double speech_sec ; 
   const  IppLibraryVersion *ver = ippscGetLibVersion(); 

   encCtrl.law = G728_PCM_Linear; // default linear 
   encCtrl.rate = G728_Rate_16000;

   argc--;
   while(argc-- > 0){
      argv++;
      if ('-' == (*argv)[0]) { 
         if (!strncmp("-r",*argv,2)){
            if (!strcmp("128",*argv+2)){
                encCtrl.rate = G728_Rate_12800;
                rateVal = 12800;
            } else if(!strcmp("96",*argv+2)){
                encCtrl.rate = G728_Rate_9600;
                rateVal = 9600;
            }
            continue;
         } else if (!strcmp(*argv,"-s")){
            if(argc-->0){
               int rep = atoi(*++argv);
               if(0 == rep) rep=1; 
               if(rep > nRepeat) nRepeat = rep;
            }
            continue;
         } else if (!strcmp(*argv,"-t")){
            if(argc-->0){
               int thread = atoi(*++argv);
               if(0 == thread) thread = 1; 
               if(thread > MAX_THREADS) nThreads = MAX_THREADS;
               if(thread > nThreads) nThreads = thread;
            }
            continue;
         }
         if (strchr(*argv,'a')){
            if(encCtrl.law == G728_PCM_MuLaw){
               printf(" -a and -u modes should not be both defined.\n");
               usage = 1;
            }
            encCtrl.law = G728_PCM_ALaw;
         }
         if (strchr(*argv,'u')){
            if(encCtrl.law == G728_PCM_ALaw){
               printf(" -a and -u modes should not be both defined.\n");
               usage = 1;
            }
            encCtrl.law = G728_PCM_MuLaw;
         }
         if (strchr(*argv,'n')){
            optionreports = 1;
         }
         if (strchr(*argv,'c')){
            if(0 == nRepeat) nRepeat = 1;
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
         printf("\nEncoder: error opening %s.\n",inputFileName);
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
         printf("\nEncoder: error opening %s.\n",outputFileName);
         return FOPEN_FAIL;
      }
   } else {
      printf("Unknown output file.\n");
      usage=1;
   }
   if ( usage ){
      printf("Usage: %s <options> <input-audio-file> <output-bitstream-file> \n", appName);
      printf("   [-rXX]       where XX =( 96, 16, 128) - bit rate: 9.6, 16 or 12.8 Kbit/s\n");
      printf("   [-a|u]       a - audio file is A-Law compressed PCM\n");
      printf("                u - audio file is Mu-Law compressed PCM\n");
      printf("                otherwize 8 KHz linear 16-bit PCM input supposed");
      printf("   [-pack]      G.728 ASCII packed test vector input \n");
      printf("   [-n]         option reports (optional) . \n");
      printf("   [-c]         write performance line to codecspeed.csv file (optional). \n");
      printf("   [-s [<num>]] repeater, how many times to repeat an operation (optional).\n");
      printf("   [-t [<num>]] number of threads , default 1 (optional).\n");
      return USAGE;
   }
   if(optionreports){
      char* tmpStr=NULL;
      printf("Intel LD-CELP speech encoder bit-to-bit compatible with G.728/H.\n");
      printf("IPP library used:  %d.%d.%d Build %d, name %s\n",
         ver->major,ver->minor,ver->majorBuild,ver->build,ver->Name);
      printf("framesize = %d samples\n",G728_FRAME);
      switch(encCtrl.law){
      case G728_PCM_MuLaw : tmpStr="Mu-law"; break;
      case G728_PCM_ALaw  : tmpStr="A-law";  break;
      default : tmpStr="8 KHz linear";
      }
      printf("Input %s PCM audio file: %s\n",  tmpStr, inputFileName);
      printf("Output bitstream file:  %s\n", outputFileName);
      printf("Encode ... ");
   }
   if (puttocsv) {
      if ( (f_csv = fopen("codecspeed.csv", "a")) == NULL) {
         printf("\nFile codecspeed.csv could not be open.\n");
         return FOPEN_FAIL;
      }
   }

   if (result = PrepareInput(fpIn, "input",&in_buff, &srcLen))
      return result;
   fclose(fpIn);
   /* Allocate [nThreads] out buffers */ 
   nFrames = srcLen/G728_VECTOR;
   dstLen = nFrames*sizeof(short); /* total bytes in encoded bitstream */ 
   for (i=0; i<nThreads; i++){
      if(!(out_buff[i]=(Ipp16s*)ippsMalloc_8u(dstLen + G728_FRAME*sizeof(short)))){
         printf("\nLow memory: %d bytes not allocated (out_buff, thread N:%d)\n",
            dstLen, i);
         return MEMORY_FAIL;
      }
   }
   /* Allocate [nThreads] encoder objects */ 
   apiG728Encoder_Alloc(&encoderSize);
   for (i=0; i<nThreads; i++){
      if (!(encObjVec[i] = (G728Encoder_Obj*)ippsMalloc_8u(encoderSize))){
         printf("\nLow memory: %d bytes not allocated (decObj, thread N:%d)\n", 
            encoderSize, i);
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
      ThArg[i].nFrames = nFrames;
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
      ippsFree(encObjVec[i]);
      ippsFree(out_buff[i]);
   }
   /* Write output PCM to the output file */ 
   if (!thrdErr)
      fwrite(out_buff[0], 1, dstLen, fpOut);
   fclose(fpOut);

   ippsFree(encObjVec[0]);
   ippsFree(out_buff[0]);

   speech_sec = ThArg[0].totalSamples/8000.0;
   printf("\nDone %d samples of 8 KHz PCM wave file (%g sec)\n",
      ThArg[0].totalSamples, speech_sec);
   
   measure_output(&measure,speech_sec,nThreads,optionreports);
   if (puttocsv) {
      fprintf(f_csv,"%s,%s,%s,%4.2f,%s,%4.2f,%4.2f,%d\n",
         "G728","encode","", rateVal/1000.0,inputFileName,speech_sec,measure.speed_in_mhz,nThreads);
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

int  PrepareInput(FILE* pFile, char* comment, Ipp16s** pBuff, Ipp32u* dstLen)
{
   int lenFile;
   Ipp8u *buff = NULL;
   fseek(pFile,0,SEEK_END);
   lenFile = ftell(pFile); /*  size of file */ 
   /* allocate buffer for input file loading with some room */ 
   if(!(buff=ippsMalloc_8u(lenFile + G728_VECTOR*sizeof(short)))){
      printf("\nNo memory for load of %d bytes from %s file",
         lenFile + G728_VECTOR*sizeof(short), comment);
      return MEMORY_FAIL;
   }

   rewind(pFile);
   lenFile = fread(buff,1,lenFile,pFile); 

   if(encCtrl.law == G728_PCM_ALaw || encCtrl.law == G728_PCM_MuLaw){
      Ipp8u *cvt_buff = (Ipp8u*)buff; /* 8bit PCM input data */ 
      /* allocate another one for expanded 16-bit PCM input data with some room */ 
      if(!(buff = ippsMalloc_8u((lenFile + G728_VECTOR)*sizeof(short)))){ 
         printf("\nNo %d bytes memory for expansion of %s-law 8bit PCM to 16bit uniform PCM.\n",
            (lenFile + G728_VECTOR)*sizeof(short), (encCtrl.law == G728_PCM_MuLaw)? "Mu":"A");
         return MEMORY_FAIL;
      } 
      /* G.728 4.2.1 EXPAND */ 
      if (encCtrl.law == G728_PCM_ALaw) {
         ippsALawToLin_8u16s(cvt_buff, (short*) buff, lenFile);
      }else{
         ippsMuLawToLin_8u16s(cvt_buff, (short*) buff, lenFile);
      }
      ippsRShiftC_16u_I(1,(Ipp16u*)buff,lenFile);
      ippsFree(cvt_buff);
      *pBuff = (Ipp16s *)buff; 
      *dstLen = lenFile-1;
      return 0;
   }
   /* 16-bit full-range linear PCM */ 
   ippsRShiftC_16s_I(1,(Ipp16s*)buff,lenFile/2); /* scale down to 14-bit uniform PCM*/ 
   *pBuff = (Ipp16s *)buff; 
   *dstLen = lenFile/2;
   return 0;
}

static void bits2prm(const unsigned char* bitstream, short* prm, G728_Rate rate)
{
   if(rate==G728_Rate_12800)     { 
      prm[0] = bitstream[0];   
      prm[1] = bitstream[1]; 
      prm[2] = bitstream[2]; 
      prm[3] = bitstream[3]; 
   }
   else if(rate==G728_Rate_9600) { 
      prm[0] = (bitstream[0] >> 2);                               
      prm[1] = ( (bitstream[0] & 0x3) << 4 ) | (bitstream[1] >> 4); 
      prm[2] = ( (bitstream[1] & 0xf) << 2 ) | (bitstream[2] >> 6);  
      prm[3] = (bitstream[2] & 0x3f);                             
   }
   else if(rate==G728_Rate_16000){ 
      prm[0] = (bitstream[0] << 2) | (bitstream[1] >> 6);            
      prm[1] = ( (bitstream[1] & 0x3f) << 4 ) | (bitstream[2] >> 4); 
      prm[2] = ( (bitstream[2] & 0xf) << 6 ) | (bitstream[3] >> 2);  
      prm[3] = ( (bitstream[3] & 0x3) << 8 ) | bitstream[4];         
   }
}

static void* thread_proc(void* pArg)
{
   ThreadArgType* pThrdArg = (ThreadArgType*)pArg;
   short* pInBuff, *pOutBuff;
   unsigned char bitstream[5];
   int nThrd = pThrdArg->thrdNumber;
   int i;

   pThrdArg->totalSamples = 0;
   while(pThrdArg->nRepeat--){
      pInBuff = in_buff;
      pOutBuff = out_buff[nThrd];
      /* initialize the encoder */ 
      apiG728Encoder_Init(encObjVec[nThrd], encCtrl.rate);
      for(i=0; i <= pThrdArg->nFrames/G728_VEC_PER_FRAME; i++){
         /* encode file at once */;
         apiG728Encode(encObjVec[nThrd], pInBuff, bitstream);

         bits2prm(bitstream, pOutBuff, encCtrl.rate);
         
         pOutBuff += G728_VEC_PER_FRAME;
         pInBuff += G728_FRAME;
         pThrdArg->totalSamples += G728_FRAME; 
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
