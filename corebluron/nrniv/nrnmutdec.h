/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef nrnmutdec_h
#define nrnmutdec_h

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
