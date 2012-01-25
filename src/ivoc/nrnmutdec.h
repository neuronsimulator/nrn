#ifndef nrnmutdec_h
#define nrnmutdec_h

#include <nrnpthread.h>
#if USE_PTHREAD

#include <pthread.h>

#ifdef MINGW
#undef DELETE
#undef near
#endif

#define MUTDEC pthread_mutex_t* mut_;
#define MUTCONSTRUCTED (mut_ != (pthread_mutex_t*)0)
#if defined(__cplusplus)
#define MUTCONSTRUCT(mkmut) {if (mkmut) {mut_ = new pthread_mutex_t; pthread_mutex_init(mut_, 0);}else{mut_ = 0;}}
#define MUTDESTRUCT {if (mut_){pthread_mutex_destroy(mut_); delete mut_; mut_ = (pthread_mutex_t*)0;}}
#else
#define MUTCONSTRUCT(mkmut) {if (mkmut) {mut_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)); pthread_mutex_init(mut_, 0);}else{mut_ = 0;}}
#define MUTDESTRUCT {if (mut_){pthread_mutex_destroy(mut_); free((char*)mut_); mut_ = (pthread_mutex_t*)0;}}
#endif
#define MUTLOCK {if (mut_) {pthread_mutex_lock(mut_);}}
#define MUTUNLOCK {if (mut_) {pthread_mutex_unlock(mut_);}}
/*#define MUTLOCK {if (mut_) {printf("lock %lx\n", mut_); pthread_mutex_lock(mut_);}}*/
/*#define MUTUNLOCK {if (mut_) {printf("unlock %lx\n", mut_); pthread_mutex_unlock(mut_);}}*/
#else
#define MUTDEC /**/
#define MUTCONSTRUCTED (0)
#define MUTCONSTRUCT(mkmut) /**/
#define MUTDESTRUCT /**/
#define MUTLOCK /**/
#define MUTUNLOCK /**/
#endif

#endif
