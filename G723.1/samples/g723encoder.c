/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2004 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives G.723.1 Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  G.723.1 is an international standard promoted by ITU-T and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
//
//  Purpose: G.723.1 encoder demo
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include "g723encoder.h"

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


#define Rate63 0
#define Rate53 1

static int emode=0;
static int optionreports=0;

static vm_thread ThreadHandle[MAX_THREADS];
static G723Encoder_Obj *encoder[MAX_THREADS];
static int out_buff_len[MAX_THREADS];
static int inFrameSize;

static char *in_buff=NULL;
static int in_len;
static char *out_buff[MAX_THREADS];
static char *rat_buff = NULL;
static int numRate=0;

static int    UseHp = 1 ;
static int    UseVx = 0 ;

typedef struct _THREADARG {
    int iTh;
    int totalBytes;
    int n_repeat;
    char WrkRate;
} ThreadArgType;
static ThreadArgType ThArg[MAX_THREADS];

static void* thread_proc(void* pArg)
{
   ThreadArgType* pThArg = (ThreadArgType*)pArg;
   int iTh = pThArg->iTh;
   int totalRead, outSize, in_len_cur;
   short *speech_buf;
   char *in_buff_cur;
   char *out_buff_cur;
   int iRate;
   pThArg->totalBytes = 0;
   while(pThArg->n_repeat--){
        apiG723Encoder_Init(encoder[iTh], emode);

        out_buff_cur = out_buff[iTh];
        in_len_cur = in_len;
        in_buff_cur = in_buff;
        out_buff_len[iTh] = 0;
        totalRead=0;
        iRate = 0;
        while(ReadSpeech(&speech_buf, &in_buff_cur, &in_len_cur)){
            totalRead+=inFrameSize;
            if(numRate) {
                if (iRate < numRate) {
                    pThArg->WrkRate = rat_buff[iRate];
                    iRate++;
                } else {
                    printf("\nend of rate control file reached\n");
                }
            }
            apiG723Encode(encoder[iTh],speech_buf,pThArg->WrkRate,out_buff_cur);
            outSize=WriteFrameSise(out_buff_cur);
            out_buff_cur += outSize;
            out_buff_len[iTh]+=outSize;
        }
        pThArg->totalBytes += totalRead;
   } 
   return 0;
}

/* proc: get length of decoded frame to be write */
int  WriteFrameSise(char *prm )
{
    short  Info ;

    Info = prm[0] & (short)0x0003 ;
    /* Check frame type and rate informations */
    switch (Info) {
        case 0x0002 : {   /* SID frame */
            return 4;
        }
        case 0x0003 : {  /* untransmitted silence frame */
            return 1;
        }
        case 0x0001 : {   /* active frame, low rate */
            return 20;
        }
        default : {  /* active frame, high rate */
            break;
        }
    }
    return 24; /* active frame, high rate */
}

/* proc: shift poiter to next input speech frame */
int  ReadSpeech(short **pSpeech, char **in_buff_cur, int *in_len_cur)
{
    int Fr_size = G723_FrameSize*sizeof(short);

    if(*in_len_cur < Fr_size) return 0;
    *pSpeech = (short*)*in_buff_cur;
    *in_len_cur -= Fr_size; 
    *in_buff_cur += Fr_size;
    return Fr_size;
}

int main(int argc, char *argv[] )
{
   int n_repeat=1, n_thread = 1;
   int totalWritten =0;
   double speech_sec ; 
   char *in_buff_cur;
   int in_len_cur;
   FILE *f_in=NULL;         /* input File */
   FILE *f_out=NULL;        /* output File */
   FILE *f_csv=NULL;        /* csv File */
   FILE *f_rat = NULL;      /* rate File  */
   int usage=0;
   int puttocsv=0,vadcsv=0;
   char* inputFileName=NULL;
   char* rateFileName=NULL;
   char  WrkRate = Rate63 ;
   char* outputFileName=NULL;
   char* appName=argv[0];
   int nFrame=0;
   int eSize;
   int iTh;
   int pResult, errTh = 0;
   MeasureIt measure;
   const IppLibraryVersion *ver = ippscGetLibVersion(); 
   
   emode = 0;
   argc--;
   while(argc-- > 0){
      argv++;
      if ('-' == (*argv)[0]) { 
         if (!strncmp("-r",*argv,2)){
            if (!strcmp("63",*argv+2)){
                WrkRate = Rate63 ;
            }else if(!strcmp("53",*argv+2)){
                WrkRate = Rate53 ;
            }else {
                rateFileName = &(*argv)[2] ;
            }
            continue;
         }else if(!strcmp("-Noh",*argv)) {
            UseHp = 0;
            continue ;
         }else if (!strcmp(*argv,"-s")){
            if(argc-->0){
               int rep = atoi(*++argv);
               if(0 == rep) rep=1; 
               if(rep > n_repeat) n_repeat=rep;
            }
            continue;
         }else if (!strcmp(*argv,"-t")){
            if(argc-->0){
               int thread = atoi(*++argv);
               if(0 == thread) thread = 1; 
               if(thread > MAX_THREADS) n_thread = MAX_THREADS;
               if(thread > n_thread) n_thread = thread;
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
         if (strchr(*argv,'v')){
            UseVx=1;
         }
      }else{
         argc++;
         argv--;
         break;
      }
   }
   if (puttocsv) {
      vadcsv=UseVx;
   }
   if(argc-->0){
      argv++; 
      inputFileName = *argv;
      if ( (f_in = fopen(inputFileName, "rb")) == NULL) {
         printf("File %s could not be open.\n", inputFileName);
         return (2);
      }
   }
   if(argc-->0){
      argv++; 
      outputFileName = *argv;
   }
   if (!usage && !inputFileName){
      printf("Unknown input file.\n");
      usage=1;
   }
   if(!usage && !outputFileName){
      printf("Unknown output file.\n");
      usage=1;
   }
   if(rateFileName){
      int res, readRate;
      if ( (f_rat = fopen(rateFileName, "rb")) == NULL) {
         printf("Rate file %s could not be open.\n", rateFileName);
         return FOPEN_FAIL;
      }
      while (res = fread((char *)&readRate, sizeof(char), 1, f_rat)) {
          numRate++;
      }
      fseek(f_rat,0,SEEK_SET);
      if(!(rat_buff = ippsMalloc_8u(numRate))){
         printf("\nNo memory for load of %d bytes from Rate file\n",numRate);
         return MEMORY_FAIL;
      } 
      numRate = 0;
      while (res = fread((char *)&readRate, sizeof(char), 1, f_rat)) {
          rat_buff[numRate] = (char)readRate;
          numRate++;
      }
      if(f_rat) fclose(f_rat);
   }
   
   if ( usage ){
      printf("Usage : %s <options> <inFile> <outFile> \n", appName);
      printf("                encode 16bit 8000 Hz PCM speech input file\n");
      printf("                       to bitstream output file\n");
      printf("                       contains octets written in transmitting order.\n");
      printf("   [-r63|r53|r<filename>]   bit rate 6.3 or 5.3 kbit/s\n");
      printf("                            or given by file on bit rate per frame basis.\n");
      printf("   [-Noh]       highpass filter off (optional). \n");
      printf("   [-v]         VAD enabled (optional). \n");
      printf("   [-n]         option reports (optional) . \n");
      printf("   [-c]         write performance line to codecspeed.csv file (optional). \n");
      printf("   [-s [<num>]] repeater, how many times to repeat an operation (optional).\n");
      printf("   [-t [<num>]] number of threads , default 1 (optional).\n");
      return USAGE;
   }
   if(optionreports){
      printf("Intel speech codec bit-to-bit compatible with ITU-T G.723.1A\n");
      printf("IPP library used:  %d.%d.%d Build %d, name %s\n",
         ver->major,ver->minor,ver->majorBuild,ver->build,ver->Name);
      printf("               transmitting at 5.3 and 6.3 kbit/s.\n");
      printf(" Input %s file%s:  %s\n",  "speech", 
                                         "       ",inputFileName);
      printf(" Output %s file%s:  %s\n", "bitstream", 
                                         "   ",outputFileName);
      if(UseVx)
         printf("VAD enabled. ");
      printf("Highpass filter %s. ",(UseHp)?"on":"off");
      
      printf(" Transmitting rate %s kbit/s.\n",(Rate53 == WrkRate)?"5.3":"6.3");
      printf("Encode ... ");
   }

   fseek(f_in,0,SEEK_END);
   in_len = ftell(f_in);
   if(!(in_buff=(char*)ippsMalloc_8u(in_len))){
      printf("\nNo memory for load of %d bytes from input file",in_len);
      return MEMORY_FAIL;
   }
   rewind(f_in);
   fread(in_buff,1,in_len,f_in);


   if (puttocsv) {
      if ( (f_csv = fopen("codecspeed.csv", "a")) == NULL) {
         printf("\nFile codecspeed.csv could not be open.\n");
         return FOPEN_FAIL;
      }
   }
    if(UseVx)
       emode |= G723Encode_VAD_Enabled; 
    if(UseHp)
       emode |= G723Encode_HF_Enabled; 

   /*
       process input file 
   */
    {
      int outSize;
      short *speech_buf;

      apiG723Encoder_Alloc(&eSize);
      for (iTh = 0; iTh < n_thread; iTh++) {
        if(!(encoder[iTh] = (G723Encoder_Obj*)ippsMalloc_8u(eSize))){ 
            printf("\nLow memory: %d bytes not allocated\n",eSize);
            return MEMORY_FAIL;
        } 
      }

      inFrameSize = G723_FrameSize*sizeof(short);
      outSize = G723_BitStreamFrameSizeMax*sizeof(char);

      /* compute the maximum size of the output */
      in_len_cur = in_len;
      in_buff_cur = in_buff;
      while(ReadSpeech(&speech_buf, &in_buff_cur, &in_len_cur)){
         nFrame++;
         totalWritten+=outSize;
      }
      
      if(0 == totalWritten){
         printf("\nInput file shell contain at least one full speech frame\n");
         return UNKNOWN_FORMAT;
      } 
      for (iTh=0; iTh < n_thread; iTh++) {
        if(!(out_buff[iTh]=ippsMalloc_8u(totalWritten))){ /* allocate output buffer */
            printf("\nNo memory for buffering of %d output bytes",totalWritten);
            return MEMORY_FAIL;
        }  
      }

      /* time stamp prior to threads creation, creation and running time may overlap. */
      measure_start(&measure); 
   
      /* Create threads */ 
      for (iTh = 0; iTh < n_thread; iTh++){ 
        int err_crth;
        ThArg[iTh].n_repeat = n_repeat;
        ThArg[iTh].iTh = iTh;
        ThArg[iTh].WrkRate = WrkRate;
        if ( (err_crth = vm_thread_create(&ThreadHandle[iTh],thread_proc,&ThArg[iTh])) == 0 )
        { 
            printf("CreateThread fail !! [err=%d] \n",err_crth);
            return THREAD_FAIL;
        } 
      } 
 
      for (iTh=0; iTh<n_thread; iTh++){
         vm_thread_wait(&ThreadHandle[iTh]);
      }
      measure_end(&measure);

      vm_thread_terminate(&ThreadHandle[0]);
      for (iTh = 1; iTh < n_thread; iTh++){
         vm_thread_terminate(&ThreadHandle[iTh]);
         if(out_buff_len[0] == out_buff_len[iTh]){
            ippsCompare_8u((Ipp8u*)out_buff[iTh], (Ipp8u*)out_buff[0], out_buff_len[0], &pResult);
            if (pResult) errTh++;
         }else errTh++;
         ippsFree(encoder[iTh]);
         ippsFree(out_buff[iTh]);
      } 
   }
   if(!errTh){
      if ( (f_out = fopen(outputFileName, "wb")) == NULL) {
         printf("Output file %s could not be open.\n", outputFileName);
         return FOPEN_FAIL;
      }
      fwrite(out_buff[0], 1, out_buff_len[0], f_out);
      if(f_out) fclose(f_out);
   }
   ippsFree(encoder[0]);
   ippsFree(out_buff[0]);

   speech_sec = (ThArg[0].totalBytes/(int)(sizeof(short)))/8000.0;
   measure_output(&measure,speech_sec,n_thread,optionreports);
   printf("Done %d samples of 8000 Hz PCM wave file (%g sec)\n",
      ThArg[0].totalBytes/(int)(sizeof(short)),speech_sec);
   if (puttocsv) {
      fprintf(f_csv,"%s,%s,%s,%s,%s,%4.2f,%4.2f,%d\n",
         "G723.1A","encode",(vadcsv)?"VAD":"",
         (Rate63 == WrkRate)?"6.3":"5.3",inputFileName,speech_sec,measure.speed_in_mhz,n_thread);
      fclose(f_csv);
   }
   ippsFree(in_buff);
#ifdef _DEBUG
   _CrtCheckMemory ();
   _CrtDumpMemoryLeaks ();
#endif
   if(f_in) fclose(f_in);
   if (errTh) {
       printf("Failed !\n");
       return MT_SAFETY;
   }
   printf("Completed !\n");
   return 0;
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
}
