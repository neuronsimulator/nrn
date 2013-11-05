#include "nrnconf.h"

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

