/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once

#if defined(_OPENMP)
#include <omp.h>

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

#else

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
