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

#ifndef _H_NRNCONF_
#define _H_NRNCONF_

#include <cstdio>
#include <cmath>
#include <cassert>
#include <cerrno>
#include <cstdint>

namespace coreneuron {

#define NRNBBCORE 1

using Datum = int;
using Pfri = int(*)();
using Symbol = char;

#define VEC_A(i) (_nt->_actual_a[(i)])
#define VEC_B(i) (_nt->_actual_b[(i)])
#define VEC_D(i) (_nt->_actual_d[(i)])
#define VEC_RHS(i) (_nt->_actual_rhs[(i)])
#define VEC_V(i) (_nt->_actual_v[(i)])
#define VEC_AREA(i) (_nt->_actual_area[(i)])
#define VECTORIZE 1

extern double celsius;

extern double t, dt;
extern int rev_dt;
extern int secondorder;
extern bool stoprun;
extern const char* bbcore_write_version;
#define tstopbit (1 << 15)
#define tstopset stoprun |= tstopbit
#define tstopunset stoprun &= (~tstopbit)

extern void* nrn_cacheline_alloc(void** memptr, size_t size);
extern void* emalloc_align(size_t size, size_t alignment);
extern void* ecalloc_align(size_t n, size_t size, size_t alignment);
extern void check_bbcore_write_version(const char*);


}  // namespace coreneuron

#endif
