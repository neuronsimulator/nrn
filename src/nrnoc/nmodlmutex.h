#ifndef nmodlmutex_h
#define nmodlmutex_h

#include <nrnpthread.h>
#if USE_PTHREAD
#include <pthread.h>
extern pthread_mutex_t* _nmodlmutex;
#define _NMODLMUTEXLOCK {if (_nmodlmutex) { pthread_mutex_lock(_nmodlmutex); }}
#define _NMODLMUTEXUNLOCK {if (_nmodlmutex) { pthread_mutex_unlock(_nmodlmutex); }}
#else
#define _NMODLMUTEXLOCK /**/
#define _NMODLMUTEXUNLOCK /**/
#endif

#endif
