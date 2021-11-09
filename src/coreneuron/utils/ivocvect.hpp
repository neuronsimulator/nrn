/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef ivoc_vector_h
#define ivoc_vector_h

#include <cstdio>
#include <utility>

namespace coreneuron {
template <typename T>
class fixed_vector {
    size_t n_;

  public:
    T* data_; /*making public for openacc copying */

    fixed_vector() = default;

    fixed_vector(size_t n)
        : n_(n) {
        data_ = new T[n_];
    }

    fixed_vector(const fixed_vector& vec) = delete;
    fixed_vector& operator=(const fixed_vector& vec) = delete;
    fixed_vector(fixed_vector&& vec)
        : data_(nullptr)
        , n_(vec.n_) {
        std::swap(data_, vec.data_);
    }
    fixed_vector& operator=(fixed_vector&& vec) {
        data_ = nullptr;
        std::swap(data_, vec.data_);
        n_ = vec.n_;
        return *this;
    }

    ~fixed_vector() {
        delete[] data_;
    }

    const T& operator[](int i) const {
        return data_[i];
    }
    T& operator[](int i) {
        return data_[i];
    }

#pragma acc routine seq
    const T* data(void) const {
        return data_;
    }

#pragma acc routine seq
    T* data(void) {
        return data_;
    }

#pragma acc routine seq
    size_t size() const {
        return n_;
    }
};

using IvocVect = fixed_vector<double>;

extern IvocVect* vector_new(int n);
extern int vector_capacity(IvocVect* v);
extern double* vector_vec(IvocVect* v);

// retro-compatibility API
extern void* vector_new1(int n);
#pragma acc routine seq
extern int vector_capacity(void* v);
#pragma acc routine seq
extern double* vector_vec(void* v);

}  // namespace coreneuron

#endif
