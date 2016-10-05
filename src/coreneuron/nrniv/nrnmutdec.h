/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef nrnmutdec_h
#define nrnmutdec_h

#if defined(_OPENMP)

#include <omp.h>
#define USE_PTHREAD 0

#define MUTDEC omp_lock_t* mut_;
#define MUTCONSTRUCTED (mut_ != (omp_lock_t*)0)
#if defined(__cplusplus)
#define MUTCONSTRUCT(mkmut)        \
    {                              \
        if (mkmut) {               \
            mut_ = new omp_lock_t; \
            omp_init_lock(mut_);   \
        } else {                   \
            mut_ = 0;              \
        }                          \
    }
#define MUTDESTRUCT                 \
    {                               \
        if (mut_) {                 \
            omp_destroy_lock(mut_); \
            delete mut_;            \
            mut_ = (omp_lock_t*)0;  \
        }                           \
    }
#else
#define MUTCONSTRUCT(mkmut)                                 \
    {                                                       \
        if (mkmut) {                                        \
            mut_ = (omp_lock_t*)malloc(sizeof(omp_lock_t)); \
            omp_init_lock(mut_);                            \
        } else {                                            \
            mut_ = 0;                                       \
        }                                                   \
    }
#define MUTDESTRUCT                 \
    {                               \
        if (mut_) {                 \
            omp_destroy_lock(mut_); \
            free((char*)mut_);      \
            mut_ = (omp_lock_t*)0;  \
        }                           \
    }
#endif
#define MUTLOCK                 \
    {                           \
        if (mut_) {             \
            omp_set_lock(mut_); \
        }                       \
    }
#define MUTUNLOCK                 \
    {                             \
        if (mut_) {               \
            omp_unset_lock(mut_); \
        }                         \
    }

#else /* _OPENMP */

#define USE_PTHREAD 1
#if USE_PTHREAD

#include <pthread.h>

#ifdef MINGW
#undef DELETE
#undef near
#endif

#define MUTDEC pthread_mutex_t* mut_;
#define MUTCONSTRUCTED (mut_ != (pthread_mutex_t*)0)
#if defined(__cplusplus)
#define MUTCONSTRUCT(mkmut)              \
    {                                    \
        if (mkmut) {                     \
            mut_ = new pthread_mutex_t;  \
            pthread_mutex_init(mut_, 0); \
        } else {                         \
            mut_ = 0;                    \
        }                                \
    }
#define MUTDESTRUCT                      \
    {                                    \
        if (mut_) {                      \
            pthread_mutex_destroy(mut_); \
            delete mut_;                 \
            mut_ = (pthread_mutex_t*)0;  \
        }                                \
    }
#else
#define MUTCONSTRUCT(mkmut)                                           \
    {                                                                 \
        if (mkmut) {                                                  \
            mut_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)); \
            pthread_mutex_init(mut_, 0);                              \
        } else {                                                      \
            mut_ = 0;                                                 \
        }                                                             \
    }
#define MUTDESTRUCT                      \
    {                                    \
        if (mut_) {                      \
            pthread_mutex_destroy(mut_); \
            free((char*)mut_);           \
            mut_ = (pthread_mutex_t*)0;  \
        }                                \
    }
#endif
#define MUTLOCK                       \
    {                                 \
        if (mut_) {                   \
            pthread_mutex_lock(mut_); \
        }                             \
    }
#define MUTUNLOCK                       \
    {                                   \
        if (mut_) {                     \
            pthread_mutex_unlock(mut_); \
        }                               \
    }
/*#define MUTLOCK {if (mut_) {printf("lock %lx\n", mut_); pthread_mutex_lock(mut_);}}*/
/*#define MUTUNLOCK {if (mut_) {printf("unlock %lx\n", mut_); pthread_mutex_unlock(mut_);}}*/
#else
#define MUTDEC /**/
#define MUTCONSTRUCTED (0)
#define MUTCONSTRUCT(mkmut) /**/
#define MUTDESTRUCT         /**/
#define MUTLOCK             /**/
#define MUTUNLOCK           /**/
#endif

#endif /* USE_PTHREAD */

#endif
