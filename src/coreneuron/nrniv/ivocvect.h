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

#ifndef ivoc_vector_h
#define ivoc_vector_h

#include <stdio.h>
#include "coreneuron/nrniv/nrnmutdec.h"
namespace coreneuron {
template <typename T>
class fixed_vector {
    size_t n_;
    MUTDEC

  public:
    T* data_; /*making public for openacc copying */
    fixed_vector(size_t n) : n_(n) {
        data_ = new T[n_];
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

    const T* data(void) const {
        return data_;
    }
    T* data(void) {
        return data_;
    }

    size_t size() const {
        return n_;
    }

#if (USE_PTHREAD || defined(_OPENMP))
    void mutconstruct(int mkmut) {
        if (!mut_)
            MUTCONSTRUCT(mkmut)
    }
#else
    void mutconstruct(int) {
    }
#endif
    void lock() {
        MUTLOCK
    }
    void unlock() {
        MUTUNLOCK
    }
};

typedef fixed_vector<double> IvocVect;

extern IvocVect* vector_new(int n);
extern int vector_capacity(IvocVect* v);
extern double* vector_vec(IvocVect* v);

// retro-compatibility API
extern void* vector_new1(int n);
extern int vector_capacity(void* v);
extern double* vector_vec(void* v);

}  // namespace coreneuron

#endif
