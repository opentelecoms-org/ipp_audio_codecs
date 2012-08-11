/*////////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 Intel Corporation. All Rights Reserved.
//
//  Intel(R) Integrated Performance Primitives Speech Codec Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  Speech Codec is a international standard promoted by ITU-T and
//  other organizations. Implementations of this standard, or the standard
//  enabled platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: USC encoder main program
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usc.h"
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "encoder.h"

#ifdef _DEBUG
	#include <CRTDBG.H>
#endif

#define MAX_THREADS 2048

/* This object and functions are used to measure codec speed */
typedef struct {
   Ipp64u p_start;
   Ipp64u p_end;
   float speed_in_mhz;
}MeasureIt;

static Ipp64s diffclocks(Ipp64u s, Ipp64u e){
   if(s < e) return (e-s);
   return (IPP_MAX_64S-e+s);
}

static float getUSec(Ipp64u s, Ipp64u e) {
   return (float) (diffclocks(s,e)/1000000.0);
}

static void measure_start(MeasureIt *m)
{
   m->p_start = ippGetCpuClocks();
}

static void measure_end(MeasureIt *m)
{
   m->p_end = ippGetCpuClocks();
}

static void measure_output(MeasureIt *m, float speech_sec, int nTh, int optionreports){
   int MHz;
   m->speed_in_mhz = getUSec(m->p_start,m->p_end)/(nTh*speech_sec);
   if(optionreports){
      ippGetCpuFreqMhz(&MHz);
	   printf("%4.2f MHz per channel on CPU runs at %d MHz\n",m->speed_in_mhz,MHz);
   }else{
	   printf("%4.2f MHz per channel\n",m->speed_in_mhz);
   }
}
/**/

static USC_Fxns *USC_CODEC_Fxns;
static USC_CodecInfo pInfo;

static USC_Handle encoder[MAX_THREADS];
static vm_thread ThreadHandle[MAX_THREADS];
static char *in_buff = NULL;
static char *out_buff[MAX_THREADS];
static int in_len;
static int out_buff_len[MAX_THREADS];
static int inFrameSize;

static char *rat_buff = NULL;
static int nRates=0;

static int ReadSpeech(char **pSpeech, char **in_buff_cur, int *in_len_cur);
static void ippsCompare_8u(Ipp8u *buff1, Ipp8u *buff2, int len, int *pResult);

typedef struct _THREADARG {
    int iTh;
    int totalBytes;
    int maxbitsize;
    int n_repeat;
    int WrkRate;
    char* bitstream_buf;
    USC_MemBank* pBanks;
} ThreadArgType;

static ThreadArgType ThArg[MAX_THREADS];

static void* thread_proc(void* pArg)
{
   ThreadArgType* pThArg = (ThreadArgType*)pArg;
   int iTh = pThArg->iTh;
   int totalRead = 0;
   int in_len_cur = 0;
   char *in_buff_cur = NULL;
   char *out_buff_cur = NULL;
   char *bitstream_buf = pThArg->bitstream_buf;
   int outFrameSize = 0;
   USC_PCMStream in;
   USC_Bitstream out;
   int iRate = 0;

   ippCoreSetFlushToZero( 1, NULL );

   pThArg->totalBytes = 0;
   in.pcmType.bitPerSample = pInfo.pcmType->bitPerSample;
   in.pcmType.sample_frequency = pInfo.pcmType->sample_frequency;
   out.nbytes  = pThArg->maxbitsize;
   out.pBuffer = bitstream_buf;

   while(pThArg->n_repeat--){

      USC_CODEC_Fxns->std.Reinit(&((USC_Option*)pInfo.params)->modes, encoder[iTh]);

      out_buff_cur = out_buff[iTh];
      in_len_cur = in_len;
      in_buff_cur = in_buff;
      out_buff_len[iTh] = 0;
      totalRead = 0;
      iRate = 0;

      while(ReadSpeech(&in.pBuffer, &in_buff_cur, &in_len_cur)){
         totalRead += inFrameSize;

         if(nRates > 1) {
             if (iRate < nRates) {
                 pThArg->WrkRate = rat_buff[iRate];
                 iRate++;
             } else {
                 printf("\nEnd of rate control file reached\n");
             }
         }

         in.bitrate = pThArg->WrkRate;

         USC_CODEC_Fxns->std.Encode (encoder[iTh], &in, &out);

         /* Parameters to serial bits */
         outFrameSize = Bits2Ref(in, out, out_buff_cur);

         out_buff_len[iTh] += outFrameSize;
         out_buff_cur += outFrameSize;

      }  
      pThArg->totalBytes += totalRead;
   }  
   return 0;
}

/*********************************************************************************
//  USC encoder main program :
//     Purpose: process command line and run multiple encode threads 
*/
int main(int argc, char *argv[])
{
   int n_repeat = 1;
   int n_thread = 1;
   int totalWritten = 0;
   MeasureIt measure;
   double speech_sec; 
   char *in_buff_cur = NULL;
   int in_len_cur = 0;
   FILE *f_in = NULL;         /* input File  */
   FILE *f_out = NULL;        /* output File */
   FILE *f_csv = NULL;        /* csv File    */
   int usage=0;
   int optionreports = 0;
   int puttocsv = 0;
   char* inputFileName = NULL;
   char* outputFileName = NULL;
   unsigned char *cvt_buff = NULL;
   char *speech_buf = NULL;
   char* appName=argv[0];
   int nFrame = 0;
   int alaw = 0;
   int mulaw = 0;
   int trunc = 1;
   int i, iTh;
   int pResult, errTh = 0;
   const  IppLibraryVersion *ver = ippscGetLibVersion(); 
   int nbanks = 0;
   int outFrameSize = 0;
   int emode = 0;
   
   USC_CODEC_Fxns = USC_GetCodecByName ();
   USC_CODEC_Fxns->std.GetInfo((USC_Handle)NULL, &pInfo); /* codec info */
   
   
   argc--;
   while(argc-- > 0){ /*process command line */
      argv++;
      if ('-' == (*argv)[0]) { 
         if (!strncmp("-r",*argv,2)){ /* check if rate is specified */
            if (text2rate(*argv+2, &rat_buff, &nRates, &pInfo)) {
               continue;
            }
         }else if (checkVad(*argv, &emode)) { /* check if vad is specified */
            continue ;
         }else if (!strcmp(*argv,"-s")){ /* check repetition number */
            if(argc-->0){
               int rep = atoi(*++argv);
               if(0 == rep) rep = 1; 
               if(rep > n_repeat) n_repeat=rep;
            }
            continue;
         }else if (!strcmp(*argv,"-t")){ /* check how many threads are specified */
            if(argc-->0){
               int thread = atoi(*++argv);
               if(0 == thread) thread = 1; 
               if(thread > MAX_THREADS) thread = MAX_THREADS;
               if(thread > n_thread) n_thread = thread;
            }
            continue;
         }
         if (strchr(*argv,'n')){ /* check if options are to be reported */
            optionreports = 1;
         }
         if (strchr(*argv,'a')){ /* check for a-Law audio type */
            if(mulaw){
               mulaw=0;
               printf(" -a and -u modes should not be both defined.\n");
            }
            alaw=1;
         }
         if (strchr(*argv,'u')){ /* check for mu-Law audio type */
            if(alaw){
               alaw=0;
               printf(" -a and -u modes should not be both defined.\n");
            }
            mulaw=1;
         }
         if (strchr(*argv,'T')){ /* check if truncation is specified */
            trunc=0;
         }
         if (strchr(*argv,'c')){ /* check if csv output is specified */
            puttocsv=1;
         }
      }else{ /* unknown option */
         argc++; /* skip */
         argv--;
         break;
      }
   }
   if(argc-->0){ /* check if input file name is specified */
      argv++; 
      inputFileName = *argv; /* input file name */
      if ( (f_in = fopen(inputFileName, "rb")) == NULL) {
         printf("File %s could not be open.\n", inputFileName);
         return FOPEN_FAIL;
      }
   }
   if(argc-->0){ /* check if output file name is specified */
      argv++; 
      outputFileName = *argv;
   }
   if (!usage && !inputFileName){ /* input file must be specified */
      printf("Unknown input file.\n");
      usage=1;
   }
   if(!usage && !outputFileName){ /* output file must be specified */
      printf("Unknown output file.\n");
      usage=1;
   }
   if ( usage ){ /* return with help info if incorrect command line */
      printf("Usage : %s <options> <inFile> <outFile>\n", appName);
      printf(" encode 16bit %d Hz PCM speech input file to bitstream output file\n", ((USC_PCMType*)pInfo.pcmType)->sample_frequency);
      printf(" contains octets written in transmitting order.\n");
      printf("  options:\n");
      printf("   [-rXXX]      XXX - one of the modes.\n");
      printf("   [-v|v1|v2]     enables DTX mode VAD, VAD1 or VAD2 (optional).\n");
      printf("   [-a]         convert from A-Law to linear PCM value (optional).\n");
      printf("   [-u]         convert from Mu-Law to linear PCM value (optional).\n");
      printf("   [-T]         don`t truncate the LSBs (optional).\n");
      printf("   [-n]         option reports (optional) .\n");
      printf("   [-c]         write performance line to codecspeed.csv file (optional).\n");
      printf("   [-s [<num>]] repeater, how many times to repeat an operation (optional).\n");
      printf("   [-t [<num>]] number of threads , default 1 (optional).\n");
      return USAGE;
   }

   ((USC_Option*)pInfo.params)->modes.truncate = trunc;
   ((USC_Option*)pInfo.params)->direction = 0;
   ((USC_Option*)pInfo.params)->modes.vad = emode;

   if(optionreports){ /* report options info */
      printf("Intel speech codec bit-to-bit compatible with %s\n", pInfo.name);
      printf("      Dual-rate speech coder for multimedia communications\n");
      printf("IPP library used:  %d.%d.%d Build %d, name %s\n",
       ver->major,ver->minor,ver->majorBuild,ver->build,ver->Name);
      printf(" Input %s file%s:  %s\n",  "speech", "    ",inputFileName);
      printf(" Output %s file%s:  %s\n", "bitstream", "",outputFileName);
      printf("%s",(emode == 1)?" VAD enabled. \n":" VAD disabled. \n");
      printf(" Transmitting rate %d kbit/s.\n",((USC_Option*)pInfo.params)->modes.bitrate);
      printf("Encode ... ");
   }
   /* check the input file size */
   fseek(f_in,0,SEEK_END);
   in_len = ftell(f_in);
   if(in_len <= 0){
      printf("\nEmpty input file.\n");
      return UNKNOWN_FORMAT;
   }
   /* load input file */
   if(!(in_buff = ippsMalloc_8s(in_len))){
      printf("\nNo memory for load of %d bytes from input file",in_len);
      return MEMORY_FAIL;
   }
   fseek(f_in,0,SEEK_SET);
   fread(in_buff,1,in_len,f_in);

   /* unpack to linear PCM if compound input */
   if (alaw) { /* A-Law input */
       if(!(cvt_buff=ippsMalloc_8u(in_len/2))){
            printf("\nNo memory for load of %d bytes convert from linear PCM to A-Law value",in_len/2);
            return MEMORY_FAIL;
       } 
       for (i=0;i<in_len/2;i++) {
            cvt_buff[i]=in_buff[i*2];
       } 
       ippsALawToLin_8u16s(cvt_buff, (short*) in_buff, in_len/2);
       ippsFree(cvt_buff);
   }
   if (mulaw) { /* Mu-Law input */
       if(!(cvt_buff=ippsMalloc_8u(in_len/2))){
            printf("\nNo memory for load of %d bytes convert from linear PCM to A-Law value",in_len/2);
            return MEMORY_FAIL;
       } 
       for (i=0;i<in_len/2;i++) {
            cvt_buff[i]=in_buff[i*2];
       } 
       ippsMuLawToLin_8u16s(cvt_buff, (short*) in_buff, in_len/2);
       ippsFree(cvt_buff);
   }

   if (puttocsv) { /* open the csv file if any */
      if ( (f_csv = fopen("codecspeed.csv", "a")) == NULL) {
         printf("\nFile codecspeed.csv could not be open.\n");
         return FOPEN_FAIL;
      }
   }

   USC_CODEC_Fxns->std.NumAlloc(pInfo.params, &nbanks);
   for (iTh=0; iTh < n_thread; iTh++) {
      if(!(ThArg[iTh].pBanks = (USC_MemBank*)ippsMalloc_8u(sizeof(USC_MemBank)*nbanks))) { 
          printf("\nLow memory: %d bytes not allocated\n",sizeof(USC_MemBank)*nbanks);
          return MEMORY_FAIL;
      }
      USC_CODEC_Fxns->std.MemAlloc(pInfo.params, ThArg[iTh].pBanks);
      for(i=0; i<nbanks;i++){
         if(!(ThArg[iTh].pBanks[i].pMem = ippsMalloc_8u(ThArg[iTh].pBanks->nbytes))) { 
             printf("\nLow memory: %d bytes not allocated\n",ThArg[iTh].pBanks->nbytes);
             return MEMORY_FAIL;
         } 
      }
   }

   /* compute the maximum size of the output */
   inFrameSize = getInFrameSize();
   outFrameSize = getOutFrameSize();

   in_len_cur = in_len;
   in_buff_cur = in_buff;
   while(ReadSpeech(&speech_buf, &in_buff_cur, &in_len_cur)){
      nFrame++;
      totalWritten += outFrameSize;
   }  

   if(0 == totalWritten){
      printf("\nInput file shell contain at least one full speech frame\n");
      return UNKNOWN_FORMAT;
   } 

   for (iTh=0; iTh < n_thread; iTh++) 
   {
      if(!(out_buff[iTh]=ippsMalloc_8s(totalWritten))){ /* allocate output buffer */
         printf("\nNo memory for buffering of %d output bytes",totalWritten);
         return MEMORY_FAIL;
      }   
      ThArg[iTh].bitstream_buf = ippsMalloc_8s(pInfo.maxbitsize);
      if(NULL == ThArg[iTh].bitstream_buf){
         printf("\nNo memory for buffering of %d output bytes",pInfo.maxbitsize);
         return MEMORY_FAIL;
      }
   }

   /* time stamp prior to threads creation, creation and running time may overlap. */
   measure_start(&measure); 
   
   /* Creating of the threads */ 
   for (iTh = 0; iTh < n_thread; iTh++) 
   { 
     int err_crth;
     ThArg[iTh].n_repeat = n_repeat;
     ThArg[iTh].iTh = iTh;
     ThArg[iTh].WrkRate = pInfo.params->modes.bitrate;
     ThArg[iTh].maxbitsize = pInfo.maxbitsize;
     USC_CODEC_Fxns->std.Init(pInfo.params, ThArg[iTh].pBanks, &encoder[iTh]);
     if ( (err_crth = vm_thread_create(&ThreadHandle[iTh],(vm_thread_callback)thread_proc,&ThArg[iTh])) == 0 )
     { 
         printf("CreateThread fail !! [err=%d] \n",err_crth);
         return THREAD_FAIL;
     } 
   } 

   /* Waiting for the end of threads */ 
   for (iTh=0; iTh<n_thread; iTh++){
      vm_thread_wait(&ThreadHandle[iTh]);
   }

   /* time stamp after the end of all encoding */
   measure_end(&measure);
   
   /* Threads termination and result checking */ 
   vm_thread_terminate(&ThreadHandle[0]);
   for (iTh = 1; iTh < n_thread; iTh++){
     vm_thread_terminate(&ThreadHandle[iTh]);
     if(out_buff_len[0] == out_buff_len[iTh]){ /* of the same length? */
        /* all bitstreams checked against first */
        ippsCompare_8u((Ipp8u*)out_buff[iTh], (Ipp8u*)out_buff[0], out_buff_len[0], &pResult);
        if (pResult) errTh++;
     }else errTh++;
   } 

   if(!errTh){
      if ( (f_out = fopen(outputFileName, "wb")) == NULL) {
          printf("Output file %s could not be open.\n", outputFileName);
          return FOPEN_FAIL;
      }
      fwrite(out_buff[0], 1, out_buff_len[0], f_out);
      if(f_out) fclose(f_out);
   }
   for (iTh=0; iTh < n_thread; iTh++) {
      for(i=0; i<nbanks;i++){
         if(!(ThArg[iTh].pBanks[i].pMem)) ippsFree(ThArg[iTh].pBanks[i].pMem);
         ThArg[iTh].pBanks[i].pMem = NULL;
      }
      if(!(ThArg[iTh].pBanks)) ippsFree(ThArg[iTh].pBanks);
      ThArg[iTh].pBanks = NULL;
      ippsFree(out_buff[iTh]);
      ippsFree(ThArg[iTh].bitstream_buf);
   }

   speech_sec = (ThArg[0].totalBytes/(int)(sizeof(short)))/(float)pInfo.pcmType->sample_frequency;
   measure_output(&measure,(float)speech_sec,n_thread,optionreports);
   printf("Done %d samples of %d Hz PCM wave file (%g sec)\n",
      ThArg[0].totalBytes/(int)(sizeof(short)),pInfo.pcmType->sample_frequency, speech_sec);

   if (puttocsv) {
      fprintf(f_csv,"%s,%s,%s,%d,%s,%4.2f,%4.2f,%d\n",pInfo.name,"encode",
         (((USC_Option*)pInfo.params)->modes.vad == 1)?"VAD1":(((USC_Option*)pInfo.params)->modes.vad == 2)?"VAD2":"    ",
         ((USC_Option*)pInfo.params)->modes.bitrate,inputFileName,speech_sec,measure.speed_in_mhz,n_thread);
      fclose(f_csv);
   }  

   printf ("\n%d frame(s) processed\n", nFrame);
   if (in_buff) ippsFree(in_buff);
   if (rat_buff) ippsFree(rat_buff);

#ifdef _DEBUG
   _CrtCheckMemory ();
   _CrtDumpMemoryLeaks ();
#endif
   if (f_in) fclose(f_in);
   if (errTh) {
       printf("Output of %d threads differs from the output of first thread of %d total.\n", errTh, n_thread);
       printf("Failed !\n");
       return MT_SAFETY;
   }
   printf("Completed !\n");
   return 0;
}

/* proc: shift pointer to next input speech frame */
static int ReadSpeech(char **pSpeech, char **in_buff_cur, int *in_len_cur)
{
    if(*in_len_cur < inFrameSize) return 0;
    *pSpeech = *in_buff_cur;
    *in_len_cur -= inFrameSize; 
    *in_buff_cur += inFrameSize;
    return inFrameSize;
}

static void ippsCompare_8u(Ipp8u *buff1, Ipp8u *buff2, int len, int *pResult)
{
    int i;
    *pResult = 0;
    for (i=0; i < len; i++) {
        if (buff1[i] != buff2[i]) {
            *pResult = 1;
        }
    }
    return;
}
