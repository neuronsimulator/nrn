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

#ifndef usenrnmpi_h
#define usenrnmpi_h

/* define to 1 if you want MPI specific features activated
   (optionally provided by CMake option NRNMPI) */
#ifndef NRNMPI
#define NRNMPI 1
#endif

/* define to 1 if want multisend spike exchange available */
#ifndef NRN_MULTISEND
#define NRN_MULTISEND 1
#endif

/* define to 1 if you want parallel distributed cells (and gap junctions) */
#define PARANEURON 1

/* define to 1 if you want mpi dynamically loaded instead of linked normally */
#undef NRNMPI_DYNAMICLOAD

/* define to 1 if you want the MUSIC - MUlti SImulation Coordinator */
#undef NRN_MUSIC

/* define to the dll path if you want to load automatically */
#undef DLL_DEFAULT_FNAME

/* Number of times to retry a failed open */
#undef FILE_OPEN_RETRY

/* Define to 1 for possibility of rank 0 xopen/ropen a file and broadcast everywhere */
#undef USE_NRNFILEWRAP

#endif
