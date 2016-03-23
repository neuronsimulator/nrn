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

#ifndef nrniv_mf_h
#define nrniv_mf_h

#include "classreg.h"
#include "membfunc.h"
struct NrnThread;

typedef void(*Pvmi)(NrnThread*, Memb_list*, int);

#if defined(__cplusplus)
extern "C" {
#endif

extern void register_mech(char**, void(*)(Prop*), Pvmi, Pvmi, Pvmi, Pvmi, int, int);
extern int point_register_mech(char**, void(*)(Prop*), Pvmi, Pvmi, Pvmi, Pvmi, int,
	void*(*)(Object*), void(*)(void*), Member_func*, int);
extern void hoc_register_cvode(int, int(*)(int),
	int(*)(int, double**, double**, double*, Datum*, double*, int),
	int(*)(NrnThread*, Memb_list*, int),
	int(*)(NrnThread*, Memb_list*, int)
);

extern int nrn_get_mechtype(const char*);
extern int v_structure_change;
extern void ion_reg(const char*, double);
extern Prop* need_memb(Symbol*);
extern Prop* prop_alloc(Prop**, int, Node*);
extern double nrn_ion_charge(Symbol*);
extern void* create_point_process(int, Object*);
extern void destroy_point_process(void*);
extern double has_loc_point(void*);
extern double get_loc_point_process(void*);
extern double loc_point_process(int, void*);
extern Prop* nrn_point_prop_;
extern Point_process* ob2pntproc(Object*);
extern Point_process* ob2pntproc_0(Object*);
#if defined(__cplusplus)
}
#endif

#endif /* nrniv_mf_h */
