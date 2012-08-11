/*
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004 Intel Corporation. All Rights Reserved.
//
//                 Unified Speech Codec sample interface main header.
//                     Structures and functions declarations.
*/
#ifndef __USC_H__
#define __USC_H__

#if defined( USC_W32DLL )
  #if defined( _MSC_VER ) || defined( __ICL ) || defined ( __ECL )
    #define USCFUN  __declspec(dllexport)
  #else
    #define USCFUN  extern
  #endif 
#else
  #define USCFUN  extern
#endif

/* USC error code */
typedef enum {
   /* errors: negative response */
   USC_NotInitialized       = -8,   
   USC_InvalidHandler       = -7,   
   USC_NoOperation          = -6,   
   USC_UnsupportedPCMType   = -5,   
   USC_UnsupportedBitRate   = -4,   
   USC_UnsupportedFrameType = -3,   
   USC_UnsupportedVADType   = -2,   
   USC_BadDataPointer       = -1,   
   USC_NoError              =  0,
   /* warnings: positive response */
   USC_StateNotChanged      =  1
}USC_Status;

/* USC codec modes, (may be alternated) */
typedef struct {
   int bitrate;    /* codec dependent bitrate index */
   int truncate;   /* 0 - no truncate */
   int vad;        /* 0 - disabled, otherwize vad type (1,2, more if any) */
   int hpf;        /* high pass filter: 1- on, 0- off */
   int pf;         /* post filter: 1- on, 0- off */
}USC_Modes;


/* USC codec option */
typedef struct {
   int       direction;  /* 0 - encode only, 1 - decode only, 2 - both */
   int       law;        /* 0 - pcm, 1 - aLaw, 2 -muLaw */
   USC_Modes modes;
}USC_Option;

/* USC PCM stream type */
typedef struct {
   int  sample_frequency; /* sample rate in Hz */
   int  bitPerSample;     /* bit per sample */  
}USC_PCMType;

/* USC codec information */
typedef struct {
   const char        *name;       /* codec name */  
   int                framesize;  /* PCM frame size in bytes */
   int                maxbitsize; /* bitstream max frame size in bytes */
   USC_Option        *params;     /* what is supported */ 
   USC_PCMType       *pcmType;    /* codec audio source */ 
}USC_CodecInfo;


/* USC memory banks */
typedef struct {
   char *pMem; 
   int   nbytes; 
}USC_MemBank;

/* USC PCM stream */
typedef struct {
   char        *pBuffer;
   int          nbytes;     /* pcm data size in byte */
   USC_PCMType  pcmType;                 
   int          bitrate;    /* codec dependent bitrate index */
}USC_PCMStream;

/* USC compressed bits */
typedef struct {
   char        *pBuffer;
   int          nbytes;     /* bitstream size in byte */
   int          frametype;  /* codec specific frame type (transmission frame type) */
   int          bitrate;    /* codec dependent bitrate index */
}USC_Bitstream;

typedef void* USC_Handle; 

/* USC functions table.
   Each codec should supply a function table structure, which is derived from this base table.
   Codec table is to be specified for each codec by name as follows:
       USC_<codec-name>_Fxns, for example USC_g729_Fxns.
   The typical usage model:
    - Questing a codec about memory requirement using  MemAlloc() function 
      which returns a memory banks description table with required bank sizes.
    - Use Init() function to create codec instance according to modes (vad, rate etc) requested.
      Codec handle is returned. Thus different instances of particular codec may be created
      and used in parallel. 
    - Encode(), Decode() - compression/decompression functions.
    - GetInfo() - inquire codec state or codec requirement. 
*/

typedef struct {

	/*	Get_Info() - quest codec information 
		General inquiry is possible without initialization when handle==NULL.
	*/
	USC_Status (*GetInfo)(USC_Handle handle, USC_CodecInfo *pInfo);

	/* NumAlloc() - inquiring number of memory buffers 
		   options - codec control. 
		   nbanks  - number of table entries (size of pBanks table). 
   */
	USC_Status (*NumAlloc)(const USC_Option *options, int *nbanks);
   
   /*   MemAlloc() - inquiring information about memory requirement
                     (buffers to be allocated)
		   options - codec control. 
		   pBanks  - input table of size nbanks to be filled with memory requirement
                   (pMem=NULL if to be allocated )
	*/
	USC_Status (*MemAlloc)(const USC_Option *options, USC_MemBank *pBanks);

	/*	Init() - create codec handle and set it to initial state 
      options - codec control
		pBanks  - allocated memory banks of number as after MemAlloc 
      handle - pointer to the output codec instance pointer 
   */
   USC_Status (*Init)(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle );

	/*	Reinit() - set codec to initial state 
      handle - pointer to the input codec instance pointer 
   */
   USC_Status (*Reinit)(USC_Modes *modes, USC_Handle handle );

   /*	Control() - alternate codec modes 
                  The only modes were set on Init() may be alternated.   
      modes - what modes to alternate 
      handle - pointer to the input codec instance pointer 
   */
   USC_Status (*Control)(USC_Handle handle, USC_Modes *modes);

   /*	Encode()
      	in - input audio stream (pcm) pointer, 
      	out - output bitstream pointer , 
	*/
	USC_Status (*Encode)(USC_Handle handle, const USC_PCMStream *in, USC_Bitstream *out);
	
	/*	Decode()
      	in -  input bitstream pointer, 
      	out - output audio stream (pcm) pointer, 
	*/
	USC_Status (*Decode)(USC_Handle handle, const USC_Bitstream *in, USC_PCMStream *out);

} USC_stdFxns;

/*
  USC extended function table
*/
  
typedef struct {
   USC_stdFxns std;
   /* function extension, any */
}USC_Fxns;
  
#endif /* __USC_H__ */
