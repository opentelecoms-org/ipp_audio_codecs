/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2004 Intel Corporation. All Rights Reserved.
//
//     Cross-architecture support tool. 
//     Common types header. 
*/
#ifndef __VM_TYPES_H__
#define __VM_TYPES_H__
#ifdef WIN32
#include "sys/vm_types_win32.h"
#elif LINUX32
#include "sys/vm_types_linux32.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef vm_var64 vm_sizet;

#define VM_ALIGN16_DECL(X) VM_ALIGN_DECL(16,X)
#define VM_ALIGN32_DECL(X) VM_ALIGN_DECL(32,X)

#ifdef __cplusplus
};
#endif

#endif

