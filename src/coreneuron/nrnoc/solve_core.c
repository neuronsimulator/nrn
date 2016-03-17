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

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"

static void triang(NrnThread*), bksub(NrnThread*);

/* solve the matrix equation */
void nrn_solve_minimal(NrnThread* _nt) {
	triang(_nt);
	bksub(_nt);
}

/* triangularization of the matrix equations */
static void triang(NrnThread* _nt) {
	double p;
	int i, i2, i3;
	i2 = _nt->ncell;
	i3 = _nt->end;
	for (i = i3 - 1; i >= i2; --i) {
		p = VEC_A(i) / VEC_D(i);
		VEC_D(_nt->_v_parent_index[i]) -= p * VEC_B(i);
		VEC_RHS(_nt->_v_parent_index[i]) -= p * VEC_RHS(i);
	}
}

/* back substitution to finish solving the matrix equations */
static void bksub(NrnThread* _nt) {
	int i, i1, i2, i3;
	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;
	for (i = i1; i < i2; ++i) {
		VEC_RHS(i) /= VEC_D(i);
	}
	for (i = i2; i < i3; ++i) {
		VEC_RHS(i) -= VEC_B(i) * VEC_RHS(_nt->_v_parent_index[i]);
		VEC_RHS(i) /= VEC_D(i);
	}	
}

