/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2004 Intel Corporation. All Rights Reserved.
//
//     Cross-architecture support tool. 
//     Linux types header. 
*/
#ifdef LINUX32

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long vm_var32;
typedef unsigned long long vm_var64;
typedef char vm_char;

#define VM_ALIGN_DECL(X,Y) Y __attribute__ ((aligned(X)))

#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>

#include <asterisk/lock.h>

/* vm_thread.h */
typedef struct {
	pthread_t handle;
	int is_valid;
} vm_thread;

/* vm_event.h */
typedef struct {
	ast_cond_t cond;
	ast_mutex_t mutex;
	int manual;
	int state;
} vm_event;

/* vm_mmap.h */
typedef struct {
	int    fd;
	void  *address;
	size_t sizet;
} vm_mmap;

/* vm_mutex.h */
typedef struct {
	ast_mutex_t handle;
	int is_valid;
} vm_mutex;

/* vm_semaphore.h */
typedef struct {
    ast_cond_t cond;
    ast_mutex_t mutex;
    int count;
} vm_semaphore;

#ifdef __cplusplus
};
#endif

#endif
