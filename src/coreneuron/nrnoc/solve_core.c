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

    double *vec_a = &(VEC_A(0));
    double *vec_b = &(VEC_B(0));
    double *vec_d = &(VEC_D(0));
    double *vec_rhs = &(VEC_RHS(0));
    int *parent_index = _nt->_v_parent_index;

    /** @todo: just for benchmarking, otherwise produces wrong results */
	#pragma acc parallel loop present(vec_a[0:i3], vec_b[0:i3], vec_d[0:i3], vec_rhs[0:i3], parent_index[0:i3]) if(_nt->compute_gpu)
	for (i = i3 - 1; i >= i2; --i) {
		p = vec_a[i] / vec_d[i];
		vec_d[parent_index[i]] -= p * vec_b[i];
		vec_rhs[parent_index[i]] -= p * vec_rhs[i];
	}
}

/* back substitution to finish solving the matrix equations */
static void bksub(NrnThread* _nt) {
	int i, i1, i2, i3;
	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;

    double *vec_b = &(VEC_B(0));
    double *vec_d = &(VEC_D(0));
    double *vec_rhs = &(VEC_RHS(0));
    int *parent_index = _nt->_v_parent_index;

    /** @todo: just for benchmarking, otherwise produces wrong results */
	#pragma acc parallel loop present(vec_d[0:i2], vec_rhs[0:i2]) if(_nt->compute_gpu)
	for (i = i1; i < i2; ++i) {
		vec_rhs[i] /= vec_d[i];
	}

    /** @todo: just for benchmarking, otherwise produces wrong results */
	#pragma acc parallel loop present(vec_b[0:i3], vec_d[0:i3], vec_rhs[0:i3], parent_index[0:i3]) if(_nt->compute_gpu)
	for (i = i2; i < i3; ++i) {
		vec_rhs[i] -= vec_b[i] * vec_rhs[parent_index[i]];
		vec_rhs[i] /= vec_d[i];
    }
}

