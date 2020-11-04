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

#pragma once

#include <cstddef>
#include "coreneuron/mechanism/mechanism.hpp"

namespace coreneuron {

extern int v_structure_change;
extern int diam_changed;
extern int structure_change_cnt;

extern char* pnt_name(Point_process* pnt);

extern void nrn_exit(int);

extern void* emalloc(size_t size);
extern void* ecalloc(size_t n, size_t size);
extern void* erealloc(void* ptr, size_t size);

extern double* makevector(size_t size); /* size in bytes */
extern double** makematrix(size_t nrow, size_t ncol);
void freevector(double*);
void freematrix(double**);

extern void hoc_execerror(const char*, const char*); /* print and abort */
extern void hoc_warning(const char*, const char*);

extern double hoc_Exp(double x);

// defined in eion.cpp and this file included in translated mod files.
extern double nrn_nernst(double ci, double co, double z, double celsius);
extern double nrn_ghk(double v, double ci, double co, double z);

}  // namespace coreneuron
