#ifndef _PTHREAD_H
#define _PTHREAD_H

#include "c/stdinc.h"

typedef stduint pthread_t;
typedef struct { int detachstate; } pthread_attr_t;

typedef struct {
	volatile uint32 lock_value;
} pthread_mutex_t;

typedef int pthread_mutexattr_t;

#ifdef __cplusplus
extern "C" {
#endif

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
void pthread_exit(void* retval);
int pthread_join(pthread_t thread, void** retval);
pthread_t pthread_self(void);
int pthread_detach(pthread_t thread);

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);

#ifdef __cplusplus
}
#endif

#endif
