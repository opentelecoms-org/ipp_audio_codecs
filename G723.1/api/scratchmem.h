/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2002-2004 Intel Corporation. All Rights Reserved.
//
//
//  Purpose: scratch/stack memory managment header file
//
*/

#ifndef __SCRATCHMEM_H__
#define __SCRATCHMEM_H__

#if (defined( __ICL ) || defined ( __ECL )) && (defined( _WIN32 ) && !defined( _WIN64 ))
__INLINE
Ipp32s IPP_INT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp32s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE
Ipp32u IPP_UINT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp32u  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

#elif (defined( __ICL ) || defined ( __ECL )) && defined( _WIN64 )
__INLINE
Ipp64s IPP_INT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp64s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE
Ipp64u IPP_UINT_PTR( const void* ptr )  {
    union {
        void*    Ptr;
        Ipp64u   Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

#elif defined( __ICC ) && defined( linux32 )
__INLINE
Ipp32s IPP_INT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp32s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE
Ipp32u IPP_UINT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp32u  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

#elif defined ( __ECC ) && defined( linux64 )
__INLINE
Ipp64s IPP_INT_PTR( const void* ptr )  {
    union {
        void*   Ptr;
        Ipp64s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE
Ipp64u IPP_UINT_PTR( const void* ptr )  {
    union {
        void*    Ptr;
        Ipp64u   Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

#else
  #define IPP_INT_PTR( ptr )  ( (long)(ptr) )
  #define IPP_UINT_PTR( ptr ) ( (unsigned long)(ptr) )
#endif

#define IPP_BYTES_TO_ALIGN(ptr, align) ((-(IPP_INT_PTR(ptr)&((align)-1)))&((align)-1))

#define IPP_ALIGNED_PTR(ptr, align) (void*)( (char*)(ptr) + (IPP_BYTES_TO_ALIGN( ptr, align )) )

#define IPP_MALLOC_ALIGNED_BYTES  32

#define IPP_MALLOC_ALIGNED_0BYTES   0
#define IPP_MALLOC_ALIGNED_1BYTES   1
#define IPP_MALLOC_ALIGNED_8BYTES   8
#define IPP_MALLOC_ALIGNED_16BYTES 16
#define IPP_MALLOC_ALIGNED_32BYTES 32

#define IPP_ALIGNED_ARRAY(align,arrtype,arrname,arrlength)\
 arrtype arrname##AlignedArrBuff[(arrlength)+IPP_MALLOC_ALIGNED_##align##BYTES/sizeof(arrtype)];\
 arrtype *arrname = (arrtype*)IPP_ALIGNED_PTR(arrname##AlignedArrBuff,align)

__INLINE void* GetMemory(int arrlen, int sizeOfElem, char **CurPtr)
{
   void *ret;

   ret = (void*)IPP_ALIGNED_PTR(*CurPtr,sizeOfElem);
   *CurPtr += (arrlen+1)*sizeOfElem;

   return ret;
}

__INLINE void* GetAlignMemory(int align, int arrlen, int sizeOfElem, char **CurPtr)
{
   void *ret;

   ret = (void*)IPP_ALIGNED_PTR(*CurPtr,align);
   *CurPtr += (arrlen+align/sizeOfElem)*sizeOfElem;

   return ret;
}

#if !defined (SCRATCH_OLD)
#if !defined (NO_SCRATCH_MEMORY_USED)

   #define LOCAL_ALIGN_ARRAY(align,arrtype,arrname,arrlength,obj)\
      arrtype *arrname = (arrtype *)GetAlignMemory(align,arrlength,sizeof(arrtype),&(obj)->Mem.CurPtr)

   #define LOCAL_ARRAY(arrtype,arrname,arrlength,obj)\
      arrtype *arrname = (arrtype *)GetMemory(arrlength,sizeof(arrtype),&(obj)->Mem.CurPtr)

#else

   #define LOCAL_ALIGN_ARRAY(align,arrtype,arrname,arrlength,obj)\
      IPP_ALIGNED_ARRAY(align,arrtype,arrname,arrlength)

   #define LOCAL_ARRAY(arrtype,arrname,arrlength,obj)\
      arrtype arrname[arrlength]


#endif
#else
#if !defined (NO_SCRATCH_MEMORY_USED)
   #define PRE_ALLOC_ARRAY(align,arrtype,arrname,arrlength)\
      arrtype *arrname

   #define LOCAL_ALIGN_ARRAY(align,arrtype,arrname,arrlength,obj)\
      arrname = (arrtype*)IPP_ALIGNED_PTR((obj)->Mem.CurPtr,align);\
      (obj)->Mem.CurPtr += ((arrlength)+IPP_MALLOC_ALIGNED_##align##BYTES/sizeof(arrtype))*sizeof(arrtype)

   #define LOCAL_ARRAY(arrtype,arrname,arrlength,obj)\
      arrname = (arrtype*)IPP_ALIGNED_PTR((obj)->Mem.CurPtr,sizeof(arrtype));\
      (obj)->Mem.CurPtr += ((arrlength)+1)*sizeof(arrtype)

#else
   #define PRE_ALLOC_ARRAY(align,arrtype,arrname,arrlength)\
      IPP_ALIGNED_ARRAY(align,arrtype,arrname,arrlength)

   #define LOCAL_ALIGN_ARRAY(align,arrtype,arrname,arrlength,obj)

   #define LOCAL_ARRAY(arrtype,arrname,arrlength,obj)


#endif
#endif

#if !defined (NO_SCRATCH_MEMORY_USED)
#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
   #pragma message("Scratch memory enabled")
#endif
   #define LOCAL_ARRAY_FREE(arrtype,arrname,arrlength,obj)\
      arrname=NULL;\
      (obj)->Mem.CurPtr -= ((arrlength)+1)*sizeof(arrtype)

   #define LOCAL_ALIGN_ARRAY_FREE(align,arrtype,arrname,arrlength,obj)\
      arrname=NULL;\
      (obj)->Mem.CurPtr -= ((arrlength)+IPP_MALLOC_ALIGNED_##align##BYTES/sizeof(arrtype))*sizeof(arrtype)

   #define CLEAR_SCRATCH_MEMORY(obj)\
      (obj)->Mem.CurPtr = (obj)->Mem.base

   #define OPEN_SCRATCH_BLOCK(obj)\
      (obj)->Mem.VecPtr[(obj)->Mem.offset] = IPP_INT_PTR((obj)->Mem.CurPtr);\
      (obj)->Mem.offset++

   #define CLOSE_SCRATCH_BLOCK(obj)\
      (obj)->Mem.offset--;\
      (obj)->Mem.CurPtr = (char *)(obj)->Mem.VecPtr[(obj)->Mem.offset]
#else
   #define LOCAL_ARRAY_FREE(arrtype,arrname,arrlength,obj)

   #define LOCAL_ALIGN_ARRAY_FREE(align,arrtype,arrname,arrlength,obj)

   #define CLEAR_SCRATCH_MEMORY(obj)

   #define OPEN_SCRATCH_BLOCK(obj)

   #define CLOSE_SCRATCH_BLOCK(obj)
#endif

#endif /*__SCRATCHMEM_H__*/
