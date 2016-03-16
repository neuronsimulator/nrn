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

#ifndef ivoc_vector_h
#define ivoc_vector_h

#include <stdio.h>
#include "coreneuron/nrniv/nrnmutdec.h"

template <typename T>
class fixed_vector{
    size_t n_;
    MUTDEC

  public:
    T* data_; /*making public for openacc copying */
    fixed_vector(size_t n):n_(n) { data_ = new T[n_]; }
    ~fixed_vector() { delete [] data_; }

    const T& operator[] (int i) const { return data_[i]; }
    T& operator[] (int i) { return data_[i]; }

    const T* data(void) const { return data_; }
    T* data(void) { return data_; }

    size_t size() const { return n_; }

#if (USE_PTHREAD || defined(_OPENMP))
	void mutconstruct(int mkmut) {if (!mut_) MUTCONSTRUCT(mkmut)}
#else
	void mutconstruct(int) {}
#endif
	void lock() {MUTLOCK}
	void unlock() {MUTUNLOCK}
};

typedef fixed_vector<double> IvocVect;

extern "C" {
  extern IvocVect* vector_new(int n);
  extern int vector_capacity(IvocVect* v);
  extern double* vector_vec(IvocVect* v);
}

#endif

