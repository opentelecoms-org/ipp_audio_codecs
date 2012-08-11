/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2004 Intel Corporation. All Rights Reserved.
//
//     Cross-architecture support tool. 
//     Threading header. 
*/
#ifndef __VM_THREAD_H__
#define __VM_THREAD_H__
#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* IMPORTANT:
 *    The thread return value is always saved for the calling thread to 
 *    collect. To avoid memory leak, always vm_thread_wait() for the 
 *    child thread to terminate in the calling thread.
 */

typedef void *(*vm_thread_callback)(void *);

typedef enum {
	VM_THREAD_PRIORITY_HIGHEST,
	VM_THREAD_PRIORITY_HIGH,
	VM_THREAD_PRIORITY_NORMAL,
	VM_THREAD_PRIORITY_LOW,
	VM_THREAD_PRIORITY_LOWEST
} vm_thread_priority;

/* Set the thread handler to an invalid value */
void vm_thread_set_invalid(vm_thread *thread);

/* Verify if the thread handler is valid */
int  vm_thread_is_valid(vm_thread *thread);

/* Create a thread. The thread is set to run at the call.
   Return 1 if successful */
int  vm_thread_create(vm_thread *thread,vm_thread_callback func,void *arg);

/* Wait until a thread exists. */
void vm_thread_wait(vm_thread *thread);

/* Terminate a thread with blocking */
void vm_thread_terminate(vm_thread *thread);

/* Terminate the current thread */
void vm_thread_exit(void);

/* Set thread priority. Return 1 if successful */
int  vm_thread_set_priority(vm_thread *thread, vm_thread_priority priority);

#ifdef __cplusplus
};
#endif

#endif
