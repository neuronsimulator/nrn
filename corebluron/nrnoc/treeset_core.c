#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrnoc/nrnoc_decl.h"

/*
Fixed step method with threads and cache efficiency. No extracellular,
sparse matrix, multisplit, or legacy features.
*/

static void nrn_rhs(NrnThread* _nt) {
	int i, i1, i2, i3;
	NrnThreadMembList* tml;
	
	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;
	
	for (i = i1; i < i3; ++i) {
		VEC_RHS(i) = 0.;
	}

	nrn_ba(_nt, BEFORE_BREAKPOINT);
	/* note that CAP has no current */
	for (tml = _nt->tml; tml; tml = tml->next) if (memb_func[tml->index].current) {
		mod_f_t s = memb_func[tml->index].current;
		(*s)(_nt, tml->ml, tml->index);
		if (errno) {
hoc_warning("errno set during calculation of currents", (char*)0);
		}
	}
	/* now the internal axial currents.
	The extracellular mechanism contribution is already done.
		rhs += ai_j*(vi_j - vi)
	*/
	for (i = i2; i < i3; ++i) {
		double dv = VEC_V(_nt->_v_parent_index[i]) - VEC_V(i);
		/* our connection coefficients are negative so */
		VEC_RHS(i) -= VEC_B(i)*dv;
		VEC_RHS(_nt->_v_parent_index[i]) += VEC_A(i)*dv;
	}
}

/* calculate left hand side of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
with a matrix so that the solution is of the form [dvm+dvx,dvx] on the right
hand side after solving.
This is a common operation for fixed step, cvode, and daspk methods
*/

static void nrn_lhs(NrnThread* _nt) {
	int i, i1, i2, i3;
	NrnThreadMembList* tml;

	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;

	for (i = i1; i < i3; ++i) {
		VEC_D(i) = 0.;
	}

	/* note that CAP has no jacob */
	for (tml = _nt->tml; tml; tml = tml->next) if (memb_func[tml->index].jacob) {
		mod_f_t s = memb_func[tml->index].jacob;
		(*s)(_nt, tml->ml, tml->index);
		if (errno) {
hoc_warning("errno set during calculation of jacobian", (char*)0);
		}
	}
/* now the cap current can be computed because any change to cm by another model
has taken effect
*/
	/* note, the first is CAP */
	if (_nt->tml) {
		assert(_nt->tml->index == CAP);
		nrn_cap_jacob(_nt, _nt->tml->ml);
	}

	/* now add the axial currents */
        for (i=i2; i < i3; ++i) {
		VEC_D(i) -= VEC_B(i);
		VEC_D(_nt->_v_parent_index[i]) -= VEC_A(i);
        }
}

/* for the fixed step method */
void* setup_tree_matrix_minimal(NrnThread* _nt){
	nrn_rhs(_nt);
	nrn_lhs(_nt);
	return (void*)0;
}
