#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/capac.c,v 1.6 1998/11/25 19:14:28 hines Exp */

#include	"section.h"
#include	"membdef.h"
#include	"membfunc.h"

static double cj; 
static char *mechanism[] = { "0", "capacitance", "cm",0, "i_cap", 0,0 };
static cap_alloc(), cap_init();

capac_reg_() {
	/* all methods deal with capacitance in special ways */
	register_mech(mechanism, cap_alloc, (Pfri)0, (Pfri)0, (Pfri)0, cap_init, -1, 1);
}

#define cm  vdata[i][0]
#define i_cap  vdata[i][1]

/*
cj is analogous to 1/dt for cvode and daspk
for fixed step second order it is 2/dt and
for pure implicit fixed step it is 1/dt
*/
void nrn_set_cj(x) double x; { /* x is 1/ms */
	cj = x;
}

double nrn_get_cj() { return cj; }

void nrn_cap_jacob(Memb_list* ml) {
	int count = ml->nodecount;
	Node **vnode = ml->nodelist;
	double **vdata = ml->data;
	int i;
	double cfac = .001*cj;
#if CACHEVEC
	if (use_cachevec) {
		int* ni = ml->nodeindices;
		for (i=0; i < count; i++) {
			VEC_D(ni[i]) += cfac*cm;
		}
	}else
#endif /* CACHEVEC */
	{
		for (i=0; i < count; ++i) {
			NODED(vnode[i]) += cfac*cm;
		}
	}
}

static cap_init(Memb_list* ml, int type ) {
	int count = ml->nodecount;
	double **vdata = ml->data;
	int i;
	for (i=0; i < count; ++i) {
		i_cap = 0;
	}
}

nrn_capacity_current(Memb_list* ml) {
	int count = ml->nodecount;
	Node **vnode = ml->nodelist;
	double **vdata = ml->data;
	int i;
	double cfac = .001*cj;
	/* since rhs is dvm for a full or half implicit step */
	/* (nrn_update_2d() replaces dvi by dvi-dvx) */
	/* no need to distinguish secondorder */
#if CACHEVEC
	if (use_cachevec) {
		int* ni = ml->nodeindices;
		for (i=0; i < count; i++) {
			i_cap = cfac*cm*VEC_RHS(ni[i]);
		}
	}else
#endif /* CACHEVEC */
	{
		for (i=0; i < count; ++i) {
			i_cap = cfac*cm*NODERHS(vnode[i]);
		}
	}
}

#if CVODE
nrn_mul_capacity(Memb_list* ml) {
	int count = ml->nodecount;
	Node **vnode = ml->nodelist;
	double **vdata = ml->data;
	int i;
	double cfac = .001*cj;
#if CACHEVEC
	if (use_cachevec) {
		int* ni = ml->nodeindices;
		for (i=0; i < count; i++) {
			VEC_RHS(ni[i]) *= cfac*cm;
		}
	}else
#endif /* CACHEVEC */
	{
		for (i=0; i < count; ++i) {
			NODERHS(vnode[i]) *= cfac*cm;
		}
	}
}
nrn_div_capacity(Memb_list* ml) {
	int count = ml->nodecount;
	Node **vnode = ml->nodelist;
	double **vdata = ml->data;
	int i;
#if CACHEVEC
	if (use_cachevec) {
		int* ni = ml->nodeindices;
		for (i=0; i < count; i++) {
			i_cap = VEC_RHS(ni[i]);
			VEC_RHS(ni[i]) /= 1.e-3*cm;
		}
	}else
#endif /* CACHEVEC */
	{
		for (i=0; i < count; ++i) {
			i_cap = NODERHS(vnode[i]);
			NODERHS(vnode[i]) /= 1.e-3*cm;
		}
	}
}
#endif /*CVODE*/

/* the rest can be constructed automattically from the above info*/

static cap_alloc(p)
	Prop *p;
{
	double *pd;
#define nparm 2
	pd = nrn_prop_data_alloc(CAP, nparm);
	pd[0] = DEF_cm;	/*default capacitance/cm^2*/
	p->param = pd;
	p->param_size = nparm;
}
