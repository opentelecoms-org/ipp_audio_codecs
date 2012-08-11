/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2004 Intel Corporation. All Rights Reserved.
//
//     Cross-architecture support tool. 
//     Threading primitives for Linux. 
*/
#ifdef LINUX32
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include "vm_thread.h"

/* Set the thread handler an invalid value */
void vm_thread_set_invalid(vm_thread *thread) 
{
	thread->is_valid=0;
}

/* Verify if the thread handler is valid */
int  vm_thread_is_valid(vm_thread *thread)
{
	return thread->is_valid;
}

/* Create a thread. Return 1 if success */
int vm_thread_create(vm_thread *thread,
				void *(*vm_thread_func)(void *),void *arg)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr,geteuid()?SCHED_OTHER:SCHED_RR);
	thread->is_valid=!pthread_create(&thread->handle,&attr,vm_thread_func,arg);
	pthread_attr_destroy(&attr);
	vm_thread_set_priority(thread, VM_THREAD_PRIORITY_LOWEST);
	return thread->is_valid;
}

/* Terminate the current thread */
void vm_thread_exit(void)
{
	pthread_exit(0);
}

/* Set thread priority, return 1 if successful */
int  vm_thread_set_priority(vm_thread *thread, vm_thread_priority priority)
{
	int policy, pmin, pmax, pmean;
	struct sched_param param;

	if (!thread->is_valid) return 0;
	pthread_getschedparam(thread->handle,&policy,&param);

	pmin=sched_get_priority_min(policy);
	pmax=sched_get_priority_max(policy);
	pmean=(pmin+pmax)/2;

	switch (priority) {
	case VM_THREAD_PRIORITY_HIGHEST:
		param.sched_priority=pmax; break;
	case VM_THREAD_PRIORITY_LOWEST:
		param.sched_priority=pmin; break;
	case VM_THREAD_PRIORITY_NORMAL:
		param.sched_priority=pmean; break;
	case VM_THREAD_PRIORITY_HIGH:
		param.sched_priority=(pmax+pmean)/2; break;
	case VM_THREAD_PRIORITY_LOW:
		param.sched_priority=(pmin+pmean)/2; break;
	}

	return !pthread_setschedparam(thread->handle,policy,&param);
}

/* Wait until a thread exists */
void vm_thread_wait(vm_thread *thread)
{
	if (thread->is_valid) {
		pthread_join(thread->handle,NULL);
		thread->is_valid=0;
	}
}

/* Terminate a thread */
void vm_thread_terminate(vm_thread *thread)
{
	if (thread->is_valid) {
		pthread_cancel(thread->handle);
		vm_thread_wait(thread);
	}
}

#endif
