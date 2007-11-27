#include <../../nrnconf.h>
#include	<stdio.h>
#include	<errno.h>
#include	<math.h>
#include	<InterViews/resource.h>
#include	"nrnoc2iv.h"
#include	"cvodeobj.h"


typedef int (*Pfridot)(...);
extern "C" {

#include	"membfunc.h"
#include	"neuron.h"

extern int	diam_changed;
extern int	tree_changed;
extern char* secname(Section*);
extern void recalc_diam();
extern int nrn_errno_check(int);
extern void activstim_rhs(), activclamp_rhs(), activsynapse_rhs();
extern void activclamp_lhs(), activsynapse_lhs();
extern void nrn_cap_jacob(Memb_list*);
}

void Cvode::rhs() {
	int i;

	if (diam_changed) {
		recalc_diam();
	}
	for (i = 0; i < v_node_count_; ++i) {
		NODERHS(v_node_[i]) = 0.;
	}

	rhs_memb(cv_memb_list_);
	/* at this point d contains all the membrane conductances */
	/* now the internal axial currents.
		rhs += ai_j*(vi_j - vi)
	*/
	for (i = rootnodecount_; i < v_node_count_; ++i) {
		Node* nd = v_node_[i];
		Node* pnd = v_parent_[i];
		double dv = NODEV(pnd) - NODEV(nd);
		/* our connection coefficients are negative so */
		NODERHS(nd) -= NODEB(nd)*dv;
		NODERHS(pnd) += NODEA(nd)*dv;
	}
}

void Cvode::rhs_memb(CvMembList* cmlist) {
	CvMembList* cml;
	errno = 0;
	for (cml = cmlist; cml; cml = cml->next) {
		Memb_func* mf = memb_func + cml->index;
		Pfridot s = (Pfridot)mf->current;
		if (s) {
			Memb_list* ml = cml->ml;
			(*s)(ml, cml->index);
			if (errno) {
				if (nrn_errno_check(cml->index)) {
hoc_warning("errno set during calculation of currents", (char*)0);
				}
			}
		}
	}
	activsynapse_rhs();
	activstim_rhs();
	activclamp_rhs();
}

void Cvode::lhs() {
	int i;

	for (i = 0; i < v_node_count_; ++i) {
		NODED(v_node_[i]) = 0.;
	}

	lhs_memb(cv_memb_list_);
	nrn_cap_jacob(cmlcap_->ml);

	/* now add the axial currents */
	for (i = 0; i < v_node_count_; ++i) {
		NODED(v_node_[i]) -= NODEB(v_node_[i]);
	}
        for (i=rootnodecount_; i < v_node_count_; ++i) {
                NODED(v_parent_[i]) -= NODEA(v_node_[i]);
        }
}

void Cvode::lhs_memb(CvMembList* cmlist) {
	CvMembList* cml;
	for (cml = cmlist; cml; cml = cml->next) {
		Memb_func* mf = memb_func + cml->index;
		Memb_list* ml = cml->ml;
		Pfridot s = (Pfridot)mf->jacob;
		if (s) {
			Pfridot s = (Pfridot)mf->jacob;
			(*s)(ml, cml->index);
			if (errno) {
				if (nrn_errno_check(cml->index)) {
hoc_warning("errno set during calculation of di/dv", (char*)0);
				}
			}
		}
	}
	activsynapse_lhs();
	activclamp_lhs();
}

/* triangularization of the matrix equations */
void Cvode::triang() {
	register Node *nd, *pnd;
	double p;
	int i;

	for (i = v_node_count_ - 1; i >= rootnodecount_; --i) {
		nd = v_node_[i];
		pnd = v_parent_[i];
		p = NODEA(nd) / NODED(nd);
		NODED(pnd) -= p * NODEB(nd);
		NODERHS(pnd) -= p * NODERHS(nd);
	}
}

/* back substitution to finish solving the matrix equations */
void Cvode::bksub() {
	register Node *nd, *cnd;
	int i;

	for (i = 0; i < rootnodecount_; ++i) {
		NODERHS(v_node_[i]) /= NODED(v_node_[i]);
	}
	for (i = rootnodecount_; i < v_node_count_; ++i) {
		cnd = v_node_[i];
		nd = v_parent_[i];
		NODERHS(cnd) -= NODEB(cnd) * NODERHS(nd);
		NODERHS(cnd) /= NODED(cnd);
	}	
}
