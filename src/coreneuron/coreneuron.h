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

/***
 * Includes all headers required to communicate and run all methods
 * described in CoreNeuron, neurox, and mod2c C-generated mechanisms
 * functions.
 **/

#ifndef CORENEURON_H
#define CORENEURON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "coreneuron/scopmath_core/newton_struct.h"  //Newton Struct
#include "coreneuron/nrnoc/membdef.h"                //static definitions
#include "coreneuron/nrnoc/nrnoc_ml.h"               //Memb_list and mechs info
#include "coreneuron/utils/randoms/nrnran123.h"      //Random Number Generator

#if defined(__cplusplus)
#include "coreneuron/nrniv/memory.h"  //Memory alignemnts and padding

extern "C" {
#endif

// global variables required by mechanisms
// TODO requires #include coreneuron/coreneuron.h in all mechs
// extern double celsius; //from coreneuron/coreneuron/nrnconf.h
// extern int nrn_ion_global_map_size; //from coreneuron/nrnoc/membfunc.h
// extern double** nrn_ion_global_map; //from coreneuron/nrnoc/membfunc.h

#ifdef EXPORT_MECHS_FUNCTIONS
// from (auto-generated) mod_func_ptrs.c
extern mod_f_t get_init_function(const char* sym);
extern mod_f_t get_cur_function(const char* sym);
extern mod_f_t get_state_function(const char* sym);
extern mod_f_t get_BA_function(const char* sym, int BA_func_id);
#endif

// from nrnoc/capac.c
extern void nrn_init_capacitance(struct NrnThread*, struct Memb_list*, int);
extern void nrn_cur_capacitance(struct NrnThread* _nt, struct Memb_list* ml, int type);
extern void nrn_alloc_capacitance(double* data, Datum* pdata, int type);

// from nrnoc/eion.c
extern void nrn_init_ion(struct NrnThread*, struct Memb_list*, int);
extern void nrn_cur_ion(struct NrnThread* _nt, struct Memb_list* ml, int type);
extern void nrn_alloc_ion(double* data, Datum* pdata, int type);
extern void second_order_cur(NrnThread* _nt, int secondorder);

#if defined(__cplusplus)
}
#endif

#endif
