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
//  Purpose: USC decoder main program
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usc.h"
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "decoder.h"

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

static Ipp64s diffclocks(Ipp64u s, Ipp64u e){
   if(s < e) return (e-s);
   return (IPP_MAX_64S-e+s);
}

static float getUSec(Ipp64u s, Ipp64u e) {
   return diffclocks(s,e)/1000000.0;
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

static USC_Handle decoder[MAX_THREADS];
static vm_thread ThreadHandle[MAX_THREADS];
static int out_buff_len[MAX_THREADS];
static int inFrameSize;
static int outFrameSize;

static char *in_buff = NULL;
static char *out_buff[MAX_THREADS];
static int in_len;
static int vad=0;

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
static void ippsCompare_8u(Ipp8u *buff1, Ipp8u *buff2, int len, int *pResult);


static void* thread_proc(void* pArg)
{
   ThreadArgType* pThArg = (ThreadArgType*)pArg;
   int iTh = pThArg->iTh;
   int in_len_cur;
   char *in_buff_cur=NULL;
   char *bits_encoded=NULL;
   char *bitstream_buf = pThArg->bitstream_buf;
   USC_PCMStream out;
   USC_Bitstream in;

   ippCoreSetFlushToZero( 1, NULL );

   pThArg->totalBytes=0; 
   in.pBuffer = bitstream_buf;
   in.nbytes  = pThArg->maxbitsize;
   in.bitrate = 0;
   out.pcmType.bitPerSample = 0;
   out.pcmType.sample_frequency = 0;

   while(pThArg->n_repeat--){

        USC_CODEC_Fxns->std.Reinit(&((USC_Option*)pInfo.params)->modes, decoder[iTh]);

        in_len_cur = in_len;
        in_buff_cur = in_buff;
        out.pBuffer = out_buff[iTh];
        out_buff_len[iTh] = 0;

        while(read_frame(&bits_encoded, &in_buff_cur, &in_len_cur)){

            Ref2Bits(bits_encoded, &in); /* transfer to bitstream*/

            USC_CODEC_Fxns->std.Decode (decoder[iTh], &in, &out);

            out.pBuffer += outFrameSize;
            out_buff_len[iTh] += outFrameSize;
        }
        pThArg->totalBytes += out_buff_len[iTh];
   }
   return 0;
}

/*********************************************************************************
//  USC decoder main program :
//     Purpose: process command line and run multiple decode threads 
*/

int main(int argc, char *argv[])
{
   int n_repeat = 1;
   int n_thread = 1;
   int totalWritten = 0;
   MeasureIt measure;
   double speech_sec; 
   FILE *f_in = NULL;         /* input File */
   FILE *f_out = NULL;        /* output File */
   FILE *f_csv = NULL;        /* csv File */
   unsigned char *cvt_buff = NULL;
   char *in_buff_cur = NULL;
   int in_len_cur = 0;
   int usage = 0;
   int optionreports=0;
   int puttocsv = 0;
   char* inputFileName = NULL;
   char* outputFileName = NULL;
   char* appName = argv[0];
   char *bits_encoded = NULL;
   int nFrame = 0;
   int alaw = 0;
   int mulaw = 0;
   int trunc = 1;
   int dSize = 0;
   int WrkRate = 0;
   int i, iTh;
   int pResult = 0, errTh = 0;
   const  IppLibraryVersion *ver = ippscGetLibVersion(); 
   int nbanks = 0;
   
   USC_CODEC_Fxns = USC_GetCodecByName ();
   USC_CODEC_Fxns->std.GetInfo((USC_Handle)NULL, &pInfo); /* codec info */

   argc--;
   while(argc-- > 0){ /*process command line */
      argv++;
      if ('-' == (*argv)[0]) { 
         if (!strncmp("-r",*argv,2)){ /* check if rate is specified */
            if (text2rate(*argv+2, &WrkRate)) {
               continue;
            }
         }else if (!strcmp(*argv,"-s")){ /* check repetition number */
            if(argc-->0){
               int rep = atoi(*++argv);
               if(0 == rep) rep=1; 
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
            if(0 == n_repeat) n_repeat=1;
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
      inputFileName = *argv;
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
      printf("  decode bitstream input file to %d Hz PCM speech output file.\n",  ((USC_PCMType*)pInfo.pcmType)->sample_frequency);
      printf("  options:\n");
      printf("   [-a]         output file will be A-Law PCM (optional).\n");
      printf("   [-u]         output file will be Mu-Law PCM (optional).\n");
      printf("                default: output file will be linear PCM.\n");
      printf("   [-T]         don`t truncate to LSBs (optional).\n");
      printf("   [-n]         option reports (optional).\n");
      printf("   [-c]         write performance line to codecspeed.csv file (optional).\n");
      printf("   [-s [<num>]] repeater, how many times to repeat an operation (optional).\n");
      printf("   [-t [<num>]] number of threads , default 1 (optional).\n");
      return USAGE;
   }

   ((USC_Option*)pInfo.params)->modes.bitrate = WrkRate;
   ((USC_Option*)pInfo.params)->modes.truncate = trunc;
   ((USC_Option*)pInfo.params)->direction = 1;
//   ((USC_Option*)pInfo.params)->modes.vad = dmode;

   if(optionreports){ /* report options info */
      printf("Intel speech codec bit-to-bit compatible with %s\n", pInfo.name);
      printf("      Dual-rate speech coder for multimedia communications\n");
      printf("IPP library used:  %d.%d.%d Build %d, name %s\n",
         ver->major,ver->minor,ver->majorBuild,ver->build,ver->Name);
      printf(" Input %s file%s:  %s\n",  "bitstream", "    ",inputFileName);
      printf(" Output %s file%s:  %s\n", "speech", "",outputFileName);
      printf("Decode ... ");
   }

   /* check the input file size */
   fseek(f_in,0,SEEK_END);
   in_len = ftell(f_in);
   if(in_len<=0){
      printf("\nEmpty input file.\n");
      return UNKNOWN_FORMAT;
   }

   /* load input file */
   if(!(in_buff=ippsMalloc_8s(in_len))){
      printf("\nNo memory for load of %d bytes from input file",in_len);
      return MEMORY_FAIL;
   }
   fseek(f_in,0,SEEK_SET);
   fread(in_buff,1,in_len,f_in);

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
   outFrameSize = getOutFrameSize();

   in_len_cur = in_len;
   in_buff_cur = in_buff;
   while(read_frame(&bits_encoded, &in_buff_cur, &in_len_cur)){
      nFrame++;
      totalWritten += outFrameSize;
   } 

   if(0 == nFrame){
      printf("\nInput file shell contain at least one full speech frame\n");
      return UNKNOWN_FORMAT;
   } 

   for (iTh=0; iTh < n_thread; iTh++) {
      if(!(out_buff[iTh]=ippsMalloc_8s(totalWritten))){ /* allocate output buffer */
         printf("\nNo memory for buffering of %d output bytes",totalWritten);
         return MEMORY_FAIL;
      }  
      ThArg[iTh].bitstream_buf = ippsMalloc_8s(pInfo.maxbitsize);
      if(NULL == ThArg[iTh].bitstream_buf){
         printf("\nNo memory for buffering of %d output bytes",pInfo.maxbitsize);
         return MEMORY_FAIL;
      }
      ippsZero_8u(out_buff[iTh],totalWritten);
   }

   /* time stamp prior to threads creation, creation and running time may overlap. */
   measure_start(&measure);

   /* Creating of the threads */ 
   for (iTh = 0; iTh < n_thread; iTh++) 
   { 
      int err_crth;
      ThArg[iTh].n_repeat = n_repeat;
      ThArg[iTh].iTh = iTh;
      ThArg[iTh].maxbitsize = pInfo.maxbitsize;
      USC_CODEC_Fxns->std.Init(pInfo.params, ThArg[iTh].pBanks, &decoder[iTh]);
      if ( (err_crth = vm_thread_create(&ThreadHandle[iTh],thread_proc,&ThArg[iTh])) == 0 )
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
        /* all audio output checked against first */
        ippsCompare_8u((Ipp8u*)out_buff[iTh], (Ipp8u*)out_buff[0],  out_buff_len[0], &pResult);
        if (pResult) {
           errTh++;
           printf("err:byte N=%d %x!=%x\n",pResult,out_buff[0][pResult-1],out_buff[iTh][pResult-1]);
        }
     }else {
        errTh++;
        printf("err:length %d[0]!=%d[%d]\n",out_buff_len[0],out_buff_len[iTh],iTh);
     }
   }
   if(!errTh){
     if (alaw) {
         short *alawbuf = (short*)out_buff[0];
         if(!(cvt_buff=ippsMalloc_8u(out_buff_len[0]/2))){
             printf("\nNo memory for load of %d bytes convert from linear PCM to A-Law value",out_buff_len[0]/2);
             return MEMORY_FAIL;
         }  
         ippsLinToALaw_16s8u((const short*) out_buff[0], (Ipp8u*) cvt_buff, out_buff_len[0]/2);
         ippsZero_8u( (Ipp8u*)out_buff[0], out_buff_len[0] );
         for (i=0;i<out_buff_len[0]/2;i++) {
             alawbuf[i]=(short)cvt_buff[i] & 0xff;
         }  
         ippsFree(cvt_buff);
     }  
     if (mulaw) {
         short *mulawbuf = (short*)out_buff[0];
         if(!(cvt_buff=ippsMalloc_8u(out_buff_len[0]/2))){
             printf("\nNo memory for load of %d bytes convert from linear PCM to A-Law value",out_buff_len[0]/2);
             return MEMORY_FAIL;
         }
         ippsLinToMuLaw_16s8u((const short*) out_buff[0], (Ipp8u*) cvt_buff, out_buff_len[0]/2);
         ippsZero_8u( (Ipp8u*)out_buff[0], out_buff_len[0] );
         for (i=0;i<out_buff_len[0]/2;i++) {
             mulawbuf[i]=(short)cvt_buff[i] & 0xff;
         }  
         ippsFree(cvt_buff);
     }  
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
   measure_output(&measure,speech_sec,n_thread,optionreports);
   printf("Done %d samples of %d Hz PCM wave file (%g sec)\n",
      ThArg[0].totalBytes/(int)(sizeof(short)),pInfo.pcmType->sample_frequency,speech_sec);

   if(optionreports){
      printf("Transmitting rate %d kbit/s.\n", ((USC_Option*)pInfo.params)->modes.bitrate);
   }

   if (puttocsv) {
      fprintf(f_csv,"%s,%s,%s,%d,%s,%4.2f,%4.2f,%d\n",pInfo.name,"decode",
          (((USC_Option*)pInfo.params)->modes.vad == 1)?"VAD1":(((USC_Option*)pInfo.params)->modes.vad == 2)?"VAD2":"    ",
          ((USC_Option*)pInfo.params)->modes.bitrate,inputFileName,speech_sec,measure.speed_in_mhz,n_thread);
      fclose(f_csv);
   }

   printf ("\n%d frame(s) processed\n", nFrame);
   ippsFree(in_buff);

#ifdef _DEBUG
   _CrtCheckMemory ();
   _CrtDumpMemoryLeaks ();
#endif

   if(f_in) fclose(f_in);
   if (errTh) {
       printf("Output of %d threads differs from the output of first thread of %d total.\n", errTh, n_thread);
       printf("Failed !\n");
       return MT_SAFETY;
   }
   else
       printf("Completed !\n");
   return 0;
}

static void ippsCompare_8u(Ipp8u *buff1, Ipp8u *buff2, int len, int *pResult)
{
    int i;
    *pResult = 0;
    for (i=0; i < len; i++) {
        if (buff1[i] != buff2[i]) {
            *pResult = i+1;
        }
    }
}

