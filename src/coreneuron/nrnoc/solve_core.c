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

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"

int use_solve_interleave;

static void triang(NrnThread*), bksub(NrnThread*);

/* solve the matrix equation */
void nrn_solve_minimal(NrnThread* _nt) {
    if (use_solve_interleave) {
   	solve_interleaved(_nt->id);
    }else{
	triang(_nt);
	bksub(_nt);
    }
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

