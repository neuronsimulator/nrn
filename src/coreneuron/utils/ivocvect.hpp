/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/utils/offload.hpp"

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
        : n_{vec.n_}
        , data_{nullptr} {
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

    nrn_pragma_acc(routine seq)
    const T* data(void) const {
        return data_;
    }

    nrn_pragma_acc(routine seq)
    T* data(void) {
        return data_;
    }

    nrn_pragma_acc(routine seq)
    size_t size() const {
        return n_;
    }
};

using IvocVect = fixed_vector<double>;

extern IvocVect* vector_new(int n);
extern int vector_capacity(IvocVect* v);
extern double* vector_vec(IvocVect* v);

// retro-compatibility API
extern IvocVect* vector_new1(int n);
nrn_pragma_acc(routine seq)
extern int vector_capacity(void* v);
nrn_pragma_acc(routine seq)
extern double* vector_vec(void* v);

}  // namespace coreneuron
