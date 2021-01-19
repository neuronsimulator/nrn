/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
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
