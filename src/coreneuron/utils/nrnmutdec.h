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

// This class respects the requirement *Mutex*
class OMP_Mutex {
    public:
        // Default constructible
        OMP_Mutex() {
            omp_init_lock(&mut_);
        }

        // Destructible
        ~OMP_Mutex() {
            omp_destroy_lock(&mut_);
        }

        // Not copyable
        OMP_Mutex(const OMP_Mutex&) = delete;
        OMP_Mutex& operator=(const OMP_Mutex&) = delete;

        // Not movable
        OMP_Mutex(const OMP_Mutex&&) = delete;
        OMP_Mutex& operator=(const OMP_Mutex&&) = delete;

        // Basic Lockable
        void lock() {
            omp_set_lock(&mut_);
        }

        void unlock() {
            omp_unset_lock(&mut_);
        }

        // Lockable
        bool try_lock() {
            return omp_test_lock(&mut_) != 0;
        }

    private:
        omp_lock_t mut_;
};

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
            mut_ = nullptr;         \
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
            mut_ = nullptr;         \
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
#else
#define MUTDEC /**/
#define MUTCONSTRUCTED (0)
#define MUTCONSTRUCT(mkmut) /**/
#define MUTDESTRUCT         /**/
#define MUTLOCK             /**/
#define MUTUNLOCK           /**/

// This class respects the requirement *Mutex*
class OMP_Mutex {
    public:
        // Default constructible
        OMP_Mutex() = default;

        // Destructible
        ~OMP_Mutex() = default;

        // Not copyable
        OMP_Mutex(const OMP_Mutex&) = delete;
        OMP_Mutex& operator=(const OMP_Mutex&) = delete;

        // Not movable
        OMP_Mutex(const OMP_Mutex&&) = delete;
        OMP_Mutex& operator=(const OMP_Mutex&&) = delete;

        // Basic Lockable
        void lock() {}

        void unlock() {}

        // Lockable
        bool try_lock() {
            return true;
        }
};
#endif

#endif
