/*
Copyright 2010-2011, D. E. Shaw Research.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions, and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions, and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of D. E. Shaw Research nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __r123_nvcc_features_dot_h__
#define __r123_nvcc_features_dot_h__

// nvcc falls through to gcc or msvc.  So first define
// a couple of things and then include either gccfeatures.h
// or msvcfeatures.h

#ifndef R123_CUDA_DEVICE
#define R123_CUDA_DEVICE __device__
#endif

#ifndef R123_USE_MULHILO64_CUDA_INTRIN
#define R123_USE_MULHILO64_CUDA_INTRIN 1
#endif

#ifndef R123_ASSERT
#define R123_ASSERT(x) if((x)) ; else asm("trap;")
#endif

#ifndef R123_BUILTIN_EXPECT
#define R123_BUILTIN_EXPECT(expr,likely) expr
#endif

#ifndef R123_USE_AES_NI
#define R123_USE_AES_NI 0
#endif

#ifndef R123_USE_SSE4_2
#define R123_USE_SSE4_2 0
#endif

#ifndef R123_USE_SSE4_1
#define R123_USE_SSE4_1 0
#endif

#ifndef R123_USE_SSE
#define R123_USE_SSE 0
#endif

#ifndef R123_USE_GNU_UINT128
#define R123_USE_GNU_UINT128 0
#endif

#ifndef R123_THROW
// No exceptions in CUDA, at least upto 4.0
#define R123_THROW(x)    R123_ASSERT(0)
#endif

#if defined(__GNUC__)
#include "gccfeatures.h"
#elif defined(_MSC_FULL_VER)
#include "msvcfeatures.h"
#endif

#endif
