#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/treeset.c,v 1.39 1999/07/08 14:25:07 hines Exp */

#include	<stdio.h>
#if HAVE_STDLIB_H
#include	<stdlib.h>
#endif
#include	<errno.h>
#include	<math.h>
#include	"section.h"
#include	"membfunc.h"
#include	"neuron.h"
#include	"parse.h"
#include	"nrnmpiuse.h"
#include	"multisplit.h"
#include "spmatrix.h"

#if CVODE
extern int cvode_active_;
int nrn_cvode_; /* used by some models in case READandWRITE currents need updating*/
#endif

int nrn_shape_changed_;	/* for notifying Shape class in nrniv */
double* nrn_mech_wtime_;

extern int	diam_changed;
extern int	tree_changed;
extern double	chkarg();
extern double	nrn_ra();
extern double   nrnmpi_wtime();
extern char* secname();
extern double nrn_arc_position();
extern Symlist* hoc_built_in_symlist;
extern Symbol* hoc_table_lookup();
extern int* nrn_prop_dparam_size_;
extern int* nrn_dparam_ptr_start_;
extern int* nrn_dparam_ptr_end_;

#if PARANEURON
void (*nrn_multisplit_setup_)();
#endif

double* actual_d;
double *actual_rhs;

#if CACHEVEC
/* a, b, d and rhs are, from now on, all stored in extra arrays, to improve
 * cache efficiency in nrn_lhs() and nrn_rhs(). Formerly, three levels of
 * indirection were necessary for accessing these elements, leading to lots
 * of L2 misses.  2006-07-05/Hubert Eichner */
double *actual_a;
double *actual_b;
double *actual_v;
double *actual_area;
static int actual_v_size;
static void nrn_recalc_node_ptrs();
#define UPDATE_VEC_AREA(nd) if (actual_area) { actual_area[(nd)->v_node_index] = NODEAREA(nd);}
#endif /* CACHEVEC */
int use_cachevec;
extern void nrn_cachevec();

/*
Do not use unless necessary (loops in tree structure) since overhead
(for gaussian elimination) is
about a factor of 3 in space and 2 in time even for a tree.
*/
int nrn_matrix_cnt_ = 0;
int use_sparse13 = 0;
int nrn_use_daspk_ = 0;
char* sp13mat_;			/* handle to the general sparse matrix */
extern int linmod_extra_eqn_count();
extern void linmod_alloc(), linmod_rhs(), linmod_lhs();

#if VECTORIZE
/*
When properties are allocated to nodes or freed, v_structure_change is
set to 1. This means that the mechanism vectors need to be re-determined.
*/
int v_structure_change;
int structure_change_cnt;
int diam_change_cnt;
int v_node_count;
Node** v_node;
Node** v_parent;
#if CACHEVEC
int *v_parent_index;
#endif /* CACHEVEC */

#if _CRAY
/* in case several instance of point process at same node */
static struct CrayNodes {
	int cnt;
	Node** pnd1;
	Node* nd2;
} cray_nodes;

Node*** v_node_depth_lists;
Node*** v_parent_depth_lists;
int* v_node_depth_count;
int v_node_depth;
#endif

#endif
extern int section_count;
extern Section** secorder;
#if 1 /* if 0 then handled directly to save space : see finitialize*/
extern short* nrn_is_artificial_;
extern Template** nrn_pnt_template_;
#endif

/*
(2002-04-06)
One reason for going into the following in depth is to allow
comparison of the fixed step implicit method
with the implementation of the corresponding
cvode (vext not allowed) and daspk (vext allowed) versions.

In the fixed step method the result of a setup is a linear matrix
equation of the form
	G*(vinew,vxnew) = i(vm,vx)
which is solved and followed by an update step
	vx=vxnew
	vm = vinew - vxnew

When cvode was added, as much as possible of the existing setup was re-used
through the addition of flags. Cvode solves a system of the form
	dy/dt = f(y)
and this is not obtainable in the presence of extracellular or LinearMechanism
instances. And even in the cases it can handle, it
required a good deal of manipulation to remove the zero area compartments
from the state vector and compute the algebraic equations separately.
In fact, as of 2002-04-06, the CONSERVE statement for kinetic schemes
in model descriptions is still ignored.
At any rate, the issues involved are
1) How to compute f(y). Any algebraic equations are solved inside f(y)
so that the algebraic states are consistent with y.
2) How to setup the jacobian (I - gamma*df/dy)*y = b

When daspk was added which solves the form
	0 = f(y, y')
having one implementation serving the setup for
three very distinct methods became too cumbersome. Therefore setup_tree_matrix
was copied twice and modified separately so that each method
had its own setup code derived from this file.

The issues that have
arisen with daspk are
1) How to actually implement a residual function res = f(y, y')
2) How to actually implement a preconditioner setup of the form
	(df/dy + cj*df/dy')*y = b
3) how to initialize so that 0 = f(y(0), y'(0))
when it is not possible to know which are the algebraic and which
are the differential equations. Also, it is often the case that
a linear combination
of apparently differential equations yields an algebraic equation and we
don't know the linearly dependent subsets.
DASPK itself will only handle initialization if one can mark some states
as differential and the remaining states as algebraic.

With that, here's what is going on in this file with respect to setup of
	G*(vinew, vxnew) = i(vm,vx)

The current balance equations are sum of outward currents = sum of inward currents
For an internal node
 cm*dvm/dt + i(vm) = is(vi) + ai_j*(vi_j - vi)
For an external node (one layer)
 cx*dvx/dt + gx*(vx - ex) = cm*dvm/dt + i(vm) + ax_j*(vx_j - vx)
where vm = vi - vx
and every term has the dimensions mA/cm2
(the last terms in each equation represent the second derivative cable properties)
The chosen convention that determines the side of the terms and their sign
is that
positive membrane current is outward but electrode stimulus
currents are toward the node. We place the terms involving
currents leaving a node on the left, and currents entering a node
on the right. i.e.
	    |is
	ia `.'
	---> <---
	    |im
	ie `.'
	---> <---
	    |gx*vx
	   `.'
	   ---
	   \ /
	    -

At the hoc level, v refers to vm and vext refers to vx.
It should also be realized that LinearMechanism instances overlay equations of
the form
c*dy/dt + g*y = b
onto the current balance equation set thus adding new states,
equations, and terms to the above equations. However,
note that when y refers to a voltage of a segment in those equations
it always means vi or vx and never vm.

At this point it is up to us whether we setup and solve the form
G*(vm,vx) = i or G*(vi,vx) = i
since any particular choice is resolved back to (vm,vx) in the update step.
At present, particularly for the implicit fixed step method whose setup
implementation resides in this file, we've chosen G*(vi,vx) = i,
because there are fewer operations in setting up second derivatives and it
makes life easy for the LinearMechanism. i.e. on setup each element of the
linear mechanism gets added to only one element of the complete matrix equations
and on update we merely copy the
vi solution to the proper LinearMechanism states and update v=vi-vext
for the compartment membrane potentials.

The implicit linear form for the compartment internal node is
cm/dt*(vmnew - vmold) + (di/dvm*(vmnew - vmold) + i(vmold))
	= (dis/dvi*(vinew - viold) + is(viold)) + ai_j*(vinew_j - vinew)
and for the compartment external node is
cx/dt*(vxnew - vxold) - gx(vxnew - ex)
	= cm/dt*(vmnew - vmold) + (di/dvm*(vmnew - vmold) + i(vmold))
	+ ax_j*(vxnew_j - vxnew) = 0
	
and this forms the matrix equation row (for an internal node)
	(cm/dt + di/dvm - dis/dvi + ai_j)*vinew
	- ai_j*vinew_j
	-(cm/dt + di/dvm)*vxnew
	=
	cm/dt*vmold + di/dvm*vmold - i(vmold) - dis/dvi*viold + is(viold)

where old is present value of vm, vi, or vx and new will be found by solving
this matrix equation (which is of the form G*v = rhs)
The matrix equation row for the extracellular node is
	(cx/dt + gx + ax_j + cm/dt + di/dvm)*vext
	-ax_j*vxnew_j
	-(cm/dt + di/dvm)*vinew
	= cx/dt*vxold - (cm/dt*vmold + di/dv*vmold - i(vmold)) - gx*ex

Node.v is always interpreted as membrane potential and it is important to
distinguish this from internal potential (Node.v + Node.extnode.v)
when the extracellular mechanism is present.

In dealing with these current balance equations, all three methods
use the same mechanism calls to calculate di/dvm, i(vm), dis/dvi, is(vi)
and add them to the matrix equation.
Thus the passive channel, i=g*(v - e), is a transmembrane current
(convention is positive outward) and the mechanism adds g to the
diagonal element of the matrix and subtracts i - g*v from the right hand side.
(see nrn_cur of passive.c) It does NOT add anything to the extracellular
current balance row.

An electrode current (convention positive current increases the internal potential)
such as a voltage clamp with small resistance, is=g*(vc - (v+vext))
(see nrn_cur of svclmp.c)
subtracts  -g from the matrix diagonal (since dis/dvi = -g)
and adds i - g*(v+vext) to the
right hand side. In the presence of an extracellular mechanism, such electrode
mechanisms add the dis/dvi term to the extracellular equation diagonal and also add
is - dis/dvi*(viold) to the extracellular equation right hand side.

This is where a trick comes in. Note that in the equations it is the
membrane currents that appear in both rows whereas the stimulus appears
in the internal row only. But the membrane mechanisms add membrane currents
only to the internal row and stimuli to both rows. Perhaps the additional
confusion was not worth the mechanism efficiency gain ( since there are many
more membrane (and synaptic) currents than stimulus currents) but at any rate
the matrix filling is ordered so that membrane mechanisms are done before
the axial currents. This means that only membrane and stimulus currents are present
on the internal diagonal and right hand side. However, only stimulus current
is present on the external diagonal and right hand side. The difference
of course is the membrane current elements by themselves and the extracellular
code fills the elements appropriately.
So, in the code below, the order for doing the extracellular setup is very
important with respect to filling the matrix and mechanisms must be first,
then the obsolete activsynapse, then the extracellular setup, then
linear models and the obsolete activstim and activclamp. I.e it is crucial
to realize that before extracellular has been called that the internal
current balance matrix rows are with respect to vm whereas (except for special
handling of stimulus terms by keeping separate track of those currents)
and after extracellular has been called the current balance matrix rows are
with respect to vi and vext.
Finally at the end it is  possible to add the internal axial terms.
*/

/*
2002-04-06
General comments toward a reorganization of the implementation to
promote code sharing of the fixed, cvode, and daspk methods and with
regard to the efficiency of cvode and daspk.

The existing fixed step setup stategy is almost identical to that needed
by the current balance portion jacobian solvers for cvode and daspk. 
However it is wasteful to calculate the right hand side since that is
thrown away because those solvers demand a calculation of M*x=b where b
is provided and the answer ends up in b. Also, for the function evaluation
portion of those solvers, the reuse of the setup is wastful because
after setting up the matrix, we essentially calculate rhs - M*x
and then throw away M.

So, without reducing the efficiency of the fixed step it would be best
to re-implement the setup functions to separate the calculation of M
from that of b.  Those pieces could then be used by all three methods as
needed.  Also at this level of strategy we can properly take into
account cvode's and daspk's requirement of separating the dy/dt terms
from the y dependent terms. Note that the problem we are dealing with
is entirely confined to the current balance equations and little or
no change is required for the mechanism ode's.

One other detail must be considered.
The computation that memb_func[i].current presently
performs can be split into two parts only if the jacabian calculation is done
first. This is because the first BREAKPOINT evaluation is for
v+.001 and the second is for v. We can't turn that around. The best that
can be done (apart from analytic calculation of g) is to make use
of the simultaneous calculation of g for the fixed step, store it for
use when the jacobian is evalulated and eventually
have faster version of current only for use by cvode and daspk.

A further remark. LinearMechanisms allow much greater generality in
the structure of the current balance equations, especially with regard
to extracellular coupling and nonlinear gap junctions. In both these
cases, the extracellular equivalent circuit apparatus is entirely
superfluous and only the idea of a floating extracellular node is
needed. Perhaps this, as well as the elimination of axial resistors
for single segment sections will be forthcoming. Therefore it
is also useful to keep a clear separation between the computation of
membrane currents intra- and extracellular contribution
and the calculation of intracellular axial and extracellular currents.


The most reusable fixed step strategy I can think of is to compute
M*(dvi,dvx) = i on the basis of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
It has advantage of not adding capacitance terms to the
right hand side but requires the calculation of axial currents
for addition to the right hand side, which would have to be done anyway
for cvode and daspk. The choice is re-usability. Note that the dvm/dt term
in the external equation is why cvode can't work with the extracellular
mechanism. Also the axial coupling terms are linear so both the matrix
and righthand side are easy to setup separately without much loss of
efficiency.

1)Put -i(vm)+is(vi) into right hand side for internal node and,
if an extracellular node exists, store is(vi) as well so that we
can later carry out the trick of putting i(vm) into the external right
hand side. This is common also for cvode and daspk.
2)For extracellular nodes perform the trick and deal with gx*ex and
add the extracellular axial terms to the right hand side.
Common to daspk and cvode.
3) Add the internal axial terms to the right hand side. Common to
daspk and cvode
The above accomplishes a function evaluation for cvode. For daspk it would
remain only to use y' to calculate the residual from the derivative terms.

4)Start setting up the matrix by calculating di/dvm from the mechanisms
and put it directly into the rhs. For stimulus currents, store dis/dvi
for later use of the trick. Common also for cvode and daspk
5) For extracellular nodes perform the trick and deal with gx and extracellular
axial terms to the matrix. Common to cvode and daspk
6) Add the internal axial terms to the matrix.Commond to cvode and daspk.

At this point the detailed handling of capacitance may be different for the
three methods. For the fixed step, what is required is
7) add cm/dt to the internal diagonal
8) for external nodes fill in the cv/dt and cx/dt contributions.
This completes the setup of the matrix equations.
We then solve for dvi and dvx and update using (for implicit method)
vx += dvx
vm += dvi-dvx

*/

/* calculate right hand side of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
This is a common operation for fixed step, cvode, and daspk methods
*/

void nrn_rhs() {
	int i;
	double w;
	int measure = 0;

	if (nrn_mech_wtime_) { measure = 1; }
	if (diam_changed) {
		recalc_diam();
	}
	if (use_sparse13) {
		int i, neqn;
		neqn = spGetSize(sp13mat_, 0);
		for (i=1; i <= neqn; ++i) {
			actual_rhs[i] = 0.;
		}
	}else{
#if CACHEVEC
	    if (use_cachevec) {
		for (i = 0; i < v_node_count; ++i) {
			VEC_RHS(i) = 0.;
		}
	    }else
#endif /* CACHEVEC */
	    {
		for (i = 0; i < v_node_count; ++i) {
			NODERHS(v_node[i]) = 0.;
		}
	    }
	}

	nrn_ba(BEFORE_BREAKPOINT);
	for (i=CAP+1; i < n_memb_func; ++i) if (memb_func[i].current && memb_list[i].nodecount) {
		Pfri s = memb_func[i].current;
		if (measure) { w = nrnmpi_wtime(); }
		(*s)(memb_list + i, i);
		if (measure) { nrn_mech_wtime_[i] += nrnmpi_wtime() - w; }
		if (errno) {
			if (nrn_errno_check(i)) {
hoc_warning("errno set during calculation of currents", (char*)0);
			}
		}
	}
	activsynapse_rhs();

#if EXTRACELLULAR
	/* Cannot have any axial terms yet so that i(vm) can be calculated from
	i(vm)+is(vi) and is(vi) which are stored in rhs vector. */
	nrn_rhs_ext();
	/* nrn_rhs_ext has also computed the the internal axial current
	for those nodes containing the extracellular mechanism
	*/
#endif
	if (use_sparse13) {
		 /* must be after nrn_rhs_ext so that whatever is put in
		 nd->_rhs does not get added to nde->rhs */
		linmod_rhs();
	}

	activstim_rhs();
	activclamp_rhs();
	/* now the internal axial currents.
	The extracellular mechanism contribution is already done.
		rhs += ai_j*(vi_j - vi)
	*/
#if CACHEVEC
    if (use_cachevec) {
	for (i = rootnodecount; i < v_node_count; ++i) {
		double dv = VEC_V(v_parent_index[i]) - VEC_V(i);
		/* our connection coefficients are negative so */
		VEC_RHS(i) -= VEC_B(i)*dv;
		VEC_RHS(v_parent_index[i]) += VEC_A(i)*dv;
	}
    }else
#endif /* CACHEVEC */
    {
	for (i = rootnodecount; i < v_node_count; ++i) {
		Node* nd = v_node[i];
		Node* pnd = v_parent[i];
		double dv = NODEV(pnd) - NODEV(nd);
		/* our connection coefficients are negative so */
		NODERHS(nd) -= NODEB(nd)*dv;
		NODERHS(pnd) += NODEA(nd)*dv;
	}
    }
}

/* calculate left hand side of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
with a matrix so that the solution is of the form [dvm+dvx,dvx] on the right
hand side after solving.
This is a common operation for fixed step, cvode, and daspk methods
*/

void nrn_lhs() {
	int i;

	if (diam_changed) {
		recalc_diam();
	}

	if (use_sparse13) {
		int i, neqn;
		neqn = spGetSize(sp13mat_, 0);
		spClear(sp13mat_);
	}else{
#if CACHEVEC
	    if (use_cachevec) {
		for (i = 0; i < v_node_count; ++i) {
			VEC_D(i) = 0.;
		}
	    }else
#endif /* CACHEVEC */
	    {
		for (i = 0; i < v_node_count; ++i) {
			NODED(v_node[i]) = 0.;
		}
	    }
	}

	for (i=CAP+1; i < n_memb_func; ++i) if (memb_func[i].jacob && memb_list[i].nodecount) {
		int j, count;
		Pfri s = memb_func[i].jacob;
		(*s)(memb_list + i, i);
		if (errno) {
			if (nrn_errno_check(i)) {
hoc_warning("errno set during calculation of jacobian", (char*)0);
			}
		}
	}
/* now the cap current can be computed because any change to cm by another model
has taken effect
*/
	i = CAP;
	nrn_cap_jacob(memb_list + i);

	activsynapse_lhs();

#if EXTRACELLULAR
	 /* nde->_d[0] contains the -ELECTRODE_CURRENT contribution to nd->_d */
	nrn_setup_ext();
#endif
	if (use_sparse13) {
		 /* must be after nrn_setup_ext so that whatever is put in
		 nd->_d does not get added to nde->d */
		linmod_lhs();
	}

	activclamp_lhs();

	/* at this point d contains all the membrane conductances */


	/* now add the axial currents */

	if (use_sparse13) {
		for (i = rootnodecount; i < v_node_count; ++i) {
			Node* nd = v_node[i];
			*(nd->_a_matelm) += NODEA(nd);
			*(nd->_b_matelm) += NODEB(nd); /* b may have value from lincir */
			NODED(nd) -= NODEB(nd);
		}
        	for (i=rootnodecount; i < v_node_count; ++i) {
			NODED(v_parent[i]) -= NODEA(v_node[i]);
 		}
	} else {
#if CACHEVEC
	    if (use_cachevec) {
		for (i = 0; i < v_node_count; ++i) {
			VEC_D(i) -= VEC_B(i);
		}
	        for (i=rootnodecount; i < v_node_count; ++i) {
			VEC_D(v_parent_index[i]) -= VEC_A(i);
	        }
	    }else
#endif /* CACHEVEC */
	    {
		for (i = 0; i < v_node_count; ++i) {
			NODED(v_node[i]) -= NODEB(v_node[i]);
		}
	        for (i=rootnodecount; i < v_node_count; ++i) {
        	        NODED(v_parent[i]) -= NODEA(v_node[i]);
	        }
	    }
	}
}

/* for the fixed step method */
void setup_tree_matrix(){
	if (secondorder == 0) {
		nrn_set_cj(1./dt);
	}else{
		nrn_set_cj(2./dt);
	}
	nrn_rhs();
	nrn_lhs();
}

/* membrane mechanisms needed by other mechanisms (such as Eion by HH)
may require that the needed mechanism appear before it in the list.
(because ina must be initialized before it is incremented by HH)
With current implementation, a chain of "needs" may not be in list order.
*/
static Prop **current_prop_list; /* the one prop_alloc is working on
					when need_memb is called */
static int disallow_needmemb = 0; /* point processes cannot use need_memb
	when inserted at locations 0 or 1 */

Prop *
need_memb(sym)
	Symbol* sym;
{
	int type;
	Prop *mprev, *m, *prop_alloc();

	if (disallow_needmemb) {
		fprintf(stderr, "You can not locate a point process at\n\
 position 0 or 1 if it needs an ion\n");
		hoc_execerror(sym->name, "can't be inserted in this node");
	}
	type = sym->subtype;
	mprev = (Prop *)0; /* may need to relink m */
	for (m = *current_prop_list; m; mprev = m, m = m->next) {
		if (m->type == type) {
			break;
		}
	}
	if (m) {
		/* a chain of "needs" will not be in list order
		so it is important that order only important for Eion */
		if (mprev) {
			mprev->next = m->next;
			m->next = *current_prop_list;
		}
		*current_prop_list = m;
	} else {
		m = prop_alloc(current_prop_list, type, (Node*)0);
	}
	return m;
}
	
Node* nrn_alloc_node_; /* needed by models that use area */

Prop *
prop_alloc(pp, type, nd)	/* link in new property at head of list */
	Prop **pp;	/* returning *Prop because allocation may */
	int type;	/* cause other properties to be linked ahead */
	Node* nd;	/* some models need the node (to find area) */
{
	Prop *p;

	if (nd) {
		nrn_alloc_node_ = nd;
	}
#if VECTORIZE
	v_structure_change = 1;
#endif
	current_prop_list = pp;
	p = (Prop *)emalloc(sizeof(Prop));
	p->type = type;
	p->next = *pp;
	p->ob = (Object*)0;
	*pp = p;
	assert(memb_func[type].alloc);
	p->dparam = (Datum *)0;
	p->param = (double *)0;
	p->param_size = 0;
	(memb_func[type].alloc)(p);
	return p;
}

Prop*
prop_alloc_disallow(pp, type, nd)
	Prop** pp;
	short type;
	Node* nd;
{
	Prop *p;
	disallow_needmemb = 1;
	p = prop_alloc(pp, type, nd);
	disallow_needmemb = 0;
	return p;
}

void
prop_free(pp)	/* free an entire property list */
	Prop **pp;
{
	Prop *p, *pn;
	p = *pp;
	*pp = (Prop *)0;
	while (p) {
		pn = p->next;
		single_prop_free(p);
		p = pn;
	}
}

single_prop_free(p)
	Prop* p;
{
	extern char* pnt_map;
#if VECTORIZE
	v_structure_change = 1;
#endif
	if (pnt_map[p->type]) {
		clear_point_process_struct(p);
		return;
	}
	if (p->param) {
		notify_freed_val_array(p->param, p->param_size);
		nrn_prop_data_free(p->type, p->param);
	}
	if (p->dparam) {
		if (p->type == CABLESECTION) {
			notify_freed_val_array(&p->dparam[2].val, 6);
		}
		nrn_prop_datum_free(p->type, p->dparam);
	}
	if (p->ob) {
		hoc_obj_unref(p->ob);
	}
	free((char *)p);
}

/* For now there is always one more node in a section than there are
segments */
/* except in section 0 in which all nodes serve as x=0 to connecting
sections */

#undef PI
#define PI	3.14159265358979323846

static double diam_from_list();

int recalc_diam_count_, nrn_area_ri_nocount_, nrn_area_ri_count_;
void nrn_area_ri(Section* sec) {
	int j;
	double ra, dx, diam, rright, rleft;
	Prop* p;
	Node* nd;
	if (nrn_area_ri_nocount_ == 0) { ++nrn_area_ri_count_; }
#if DIAMLIST
	if (sec->npt3d) {
		sec->prop->dparam[2].val = sec->pt3d[sec->npt3d - 1].arc;
	}
#endif
	ra = nrn_ra(sec);
	dx = section_length(sec)/((double)(sec->nnode - 1));
	rright = 0.;
	for (j = 0; j < sec->nnode-1; j++) {
		nd = sec->pnode[j];
		for (p = nd->prop; p; p = p->next) {
			if (p->type == MORPHOLOGY) {
				break;
			}
		}
		assert(p);
#if DIAMLIST
		if (sec->npt3d > 1) {
  /* trapezoidal integration of diam, area, and resistance useing pt3d
     the integration is over the span of the segment j.
     and NODEAREA and NODERINV are filled in. The right half of the segment
     resistance (MOhm) is returned.
  */
			rright = diam_from_list(sec, j,  p, rright);
		}else
#endif
		{
  /* area for right circular cylinders. Ri as right half of parent + left half
     of this
  */
			diam = p->param[0];
			if (diam <= 0.) {
				p->param[0] = 1e-6;
hoc_execerror(secname(sec), "diameter diam = 0. Setting to 1e-6");
			}
			NODEAREA(nd) = PI*diam*dx;	/* um^2 */
			UPDATE_VEC_AREA(nd);
			rleft = 1e-2*ra*(dx/2)/(PI*diam*diam/4.); /*left half segment Megohms*/
			NODERINV(nd) = 1./(rleft + rright); /*uS*/
			rright = rleft;
		}
	}
	/* last segment has 0 length. area is 1e2 
	in dimensionless units */
	NODEAREA(sec->pnode[j]) = 1.e2;
	UPDATE_VEC_AREA(sec->pnode[j]);
	NODERINV(sec->pnode[j]) = 1./rright;
	sec->recalc_area_ = 0;
	diam_changed = 1;
}

connection_coef()	/* setup a and b */
{
	int j;
	double dx, diam, area, ra;
	hoc_Item* qsec;
	Node *nd;
	Prop *p;
#if RA_WARNING
	extern int nrn_ra_set;
#endif
	
#if 1
	/* now only called from recalc_diam */
	assert(!tree_changed);
#else
	if (tree_changed) {
		setup_topology();
	}
#endif

#if RA_WARNING
	if (nrn_ra_set > 0 && nrn_ra_set < section_count - 1) {
		hoc_warning("Don't forget to set Ra in every section",
			"eg. forall Ra=35.4");
	}
#endif
	++recalc_diam_count_;
	nrn_area_ri_nocount_ = 1;
	ForAllSections(sec)
		nrn_area_ri(sec);
	}
	nrn_area_ri_nocount_ = 0;
	/* assume that if only one connection at x=1, then they butte
	together, if several connections at x=1
	then last point is at x=1, has 0 area and other points are at
	centers of nnode-1 segments.
	If interior connection then child half
	section connects straight to the point*/
	/* for the near future we always have a last node at x=1 with
	no properties */
	ForAllSections(sec)
		/* convert to siemens/cm^2 for all nodes except last
		and microsiemens for last.  This means that a*V = mamps/cm2
		and a*v in last node = nanoamps. Note that last node
		has no membrane properties and no area. It may perhaps receive
		current stimulus later */
		/* first the effect of node on parent equation. Note That
		last nodes have area = 1.e2 in dimensionless units so that
		last nodes have units of microsiemens */
		nd = sec->pnode[0];
		area = NODEAREA(sec->parentnode);
		/* dparam[4] is rall_branch */
		ClassicalNODEA(nd) = -1.e2*sec->prop->dparam[4].val * NODERINV(nd) / area;
		for (j = 1; j < sec->nnode; j++) {
			nd = sec->pnode[j];
			area = NODEAREA(sec->pnode[j - 1]);
			ClassicalNODEA(nd) = -1.e2 * NODERINV(nd) / area;
		}
	}
	/* now the effect of parent on node equation. */
	ForAllSections(sec)
		for (j=0; j < sec->nnode; j++) {
			nd = sec->pnode[j];
			ClassicalNODEB(nd) = -1.e2 * NODERINV(nd) / NODEAREA(nd);
		}
	}
#if EXTRACELLULAR
	ext_con_coef();
#endif
}

void nrn_shape_update() {
	static int updating;
	if (section_list->next == section_list) {
		return;
	}
    if (!updating || updating != diam_change_cnt) {
	updating = diam_change_cnt;
	if (tree_changed) {
		setup_topology();
	}
	if (v_structure_change) {
		v_setup_vectors();
	}
	if (diam_changed) {
		recalc_diam();
	}
	updating = 0;
    }
}

int recalc_diam() {
#if METHOD3
	if (_method3) {
		method3_connection_coef();
	}else
#endif
	v_setup_vectors();
	nrn_matrix_node_alloc();
	connection_coef();
	diam_changed = 0;
	++diam_change_cnt;
	stim_prepare();
	synapse_prepare();
#if SEJNOWSKI
	connect_prepare();
#endif
	clamp_prepare();
}

int
area() { /* returns area (um^2) of segment containing x */
	double x;
	Section *sec;
	Node *node_ptr();
	x = *getarg(1);
	if (x == 0. || x == 1.) {
		ret(0.);
	}else{
		sec = chk_access();
		if (sec->recalc_area_) {
			nrn_area_ri(sec);
		}
		ret(NODEAREA(sec->pnode[node_index(sec, x)]));
	}
}

int
ri() { /* returns resistance (Mohm) between center of segment containing x
		and the center of the parent segment */
	double area;
	Node *np, *node_ptr();
	np = node_ptr(chk_access(), *getarg(1), &area);
	if (NODERINV(np)) {
		ret(1./NODERINV(np)); /* Megohms */
	}else{
		ret(1.e30);
	}
}


#if DIAMLIST

static int pt3dconst_;

pt3dconst() {
	int i = pt3dconst_;
	if (ifarg(1)) {
		pt3dconst_ = (int)chkarg(1, 0., 1.);	
	}
	ret((double)i);
}

pt3dstyle() {
	Section* sec = chk_access();
	if (ifarg(1)) {
		/* Classical, Logical connection */
		/*
		Logical connection is used to turn off snapping to the
		centroid of the parent which sometimes ruins
		the 3-d shape rendering due to branches connected to a
		large diameter soma getting translated to the middle of
		the soma. Of course, the question then arises of what
		to do when the soma is moved to a new location or
		an ancestor branch length is changed (i.e. exactly what
		snapping to the parent deals with automatically)
		Our answer is that a pt3dstyle==1 branch gets translated
		exactly by the amount of the parent translation
		instead of snapping to a specific absolute x,y,z position.
		*/
		if ((int)chkarg(1, 0., 1.) == 1) {
			Pt3d* p = sec->logical_connection;
			if (hoc_is_pdouble_arg(2)) {
				if (p) {
					double* px, *hoc_pgetarg();
					px = hoc_pgetarg(2); *px = p->x;
					px = hoc_pgetarg(3); *px = p->y;
					px = hoc_pgetarg(4); *px = p->z;
				}
			}else{
				if (!p) {
					p = sec->logical_connection = (Pt3d*)ecalloc(1, sizeof(Pt3d));
				}
				p->x = *getarg(2);
				p->y = *getarg(3);
				p->z = *getarg(4);
				++nrn_shape_changed_;
				diam_changed = 1;
			}
		}else{
			if (sec->logical_connection) {
				free((char*)sec->logical_connection);
				sec->logical_connection = (Pt3d*)0;
				++nrn_shape_changed_;
				diam_changed = 1;
			}
		}
	}
	ret((double)(sec->logical_connection ? 1 : 0));
}

pt3dclear() { /*destroys space in current section for 3d points.*/
	Section* sec = chk_access();
	nrn_pt3dclear(sec);
	ret((double)sec->pt3d_bsize);
}

static nrn_pt3dbufchk(sec, n) Section* sec; int n; {
	if (n > sec->pt3d_bsize) {
		sec->pt3d_bsize = n;
		if ((sec->pt3d =
		(Pt3d *)hoc_Erealloc((char *)sec->pt3d, n*sizeof(Pt3d)))
		== (Pt3d *)0) {
			sec->npt3d = 0;
			sec->pt3d_bsize = 0;
			hoc_malchk();
		}
	}
}

static nrn_pt3dmodified(sec, i0) Section* sec; int i0; {
	int n, i;
	
	++nrn_shape_changed_;
	diam_changed = 1;
	sec->recalc_area_ = 1;
	n = sec->npt3d;
#if NTS_SPINE
#else
	if (sec->pt3d[i0].d < 0.) {
		hoc_execerror("Diameter less than 0", (char *)0);
	}
#endif
	if (i0 == 0) {
		sec->pt3d[0].arc = 0.;
		i0 = 1;
	}
	for (i = i0; i < n; ++i) {
		Pt3d* p = sec->pt3d + i - 1;
		double t1,t2,t3;
		t1 = sec->pt3d[i].x - p->x;
		t2 = sec->pt3d[i].y - p->y;
		t3 = sec->pt3d[i].z - p->z;
		sec->pt3d[i].arc = p->arc + sqrt(t1*t1 + t2*t2 + t3*t3);
	}
	sec->prop->dparam[2].val = sec->pt3d[n-1].arc;
}

nrn_pt3dclear(sec)
	Section* sec;
{
	int req;
	if (ifarg(1)) {
		req = chkarg(1, 0., 30000.);
	}else{
		req = 0;
	}
	++nrn_shape_changed_;
	if (req != sec->pt3d_bsize) {
		if (sec->pt3d) {
			free((char *)(sec->pt3d));
			sec->pt3d = (Pt3d *)0;
			sec->pt3d_bsize = 0;
		}
		if (req > 0) {
			sec->pt3d = (Pt3d*)ecalloc(req, sizeof(Pt3d));
			sec->pt3d_bsize = req;
		}
	}
	sec->npt3d = 0;
}


pt3dinsert() {
	Section* sec;
	int i, n, i0;
	sec = chk_access();
	n = sec->npt3d;
	nrn_pt3dbufchk(sec, n+1);
	i0 = (int)chkarg(1, 0., (double)(n));
	++sec->npt3d;
	for (i = n-1; i >= i0; --i) {
		Pt3d* p = sec->pt3d + i + 1;
		p->x = sec->pt3d[i].x;
		p->y = sec->pt3d[i].y;
		p->z = sec->pt3d[i].z;
		p->d = sec->pt3d[i].d;
	}
	sec->pt3d[i0].x = *getarg(2);
	sec->pt3d[i0].y = *getarg(3);
	sec->pt3d[i0].z = *getarg(4);
	sec->pt3d[i0].d = *getarg(5);
	nrn_pt3dmodified(sec,i0);
	ret(0.);
}
pt3dchange() {
	int i, n;
	Section* sec = chk_access();
	n = sec->npt3d;
	i = (int)chkarg(1, 0., (double)(n-1));
	if (ifarg(5)) {
		sec->pt3d[i].x = *getarg(2);
		sec->pt3d[i].y = *getarg(3);
		sec->pt3d[i].z = *getarg(4);
		sec->pt3d[i].d = *getarg(5);
		nrn_pt3dmodified(sec, i);
	}else{
		sec->pt3d[i].d = *getarg(2);
		++nrn_shape_changed_;
		diam_changed = 1;
		sec->recalc_area_ = 1;
	}
	ret(0.);
}
pt3dremove() {
	int i, i0, n;
	Section* sec = chk_access();
	n = sec->npt3d;
	i0 = (int)chkarg(1, 0., (double)(n-1));
	for (i=i0+1; i < n; ++i) {
		Pt3d* p = sec->pt3d + i - 1;
		p->x = sec->pt3d[i].x;
		p->y = sec->pt3d[i].y;
		p->z = sec->pt3d[i].z;
		p->d = sec->pt3d[i].d;
	}
	--sec->npt3d;
	nrn_pt3dmodified(sec, i0);
	ret(0.);
}

nrn_diam_change(sec)
	Section* sec;
{
	if (!pt3dconst_ && sec->npt3d) { /* fill 3dpoints as though constant diam segments */
		int i;
		double x, L;
		L = section_length(sec);
		if (fabs(L - sec->pt3d[sec->npt3d - 1].arc) > .001) {
			nrn_length_change(sec, L);
		}
		for (i=0; i < sec->npt3d; ++i) {
			x = sec->pt3d[i].arc/L;
			if (x > 1.0) {x = 1.0;}
			node_index(sec, x);
sec->pt3d[i].d = nrn_diameter(sec->pnode[node_index(sec, x)]);
		}
		++nrn_shape_changed_;
	}
}

nrn_length_change(sec, d)
	Section* sec;
	double d;
{
	if (!pt3dconst_ && sec->npt3d) {
		int i;
		double x0,y0,z0;
		double fac;
		double l;
		x0 = sec->pt3d[0].x;
		y0 = sec->pt3d[0].y;
		z0 = sec->pt3d[0].z;
		l = sec->pt3d[sec->npt3d-1].arc;
		fac = d/l;
/*if (fac != 1) { printf("nrn_length_change from %g to %g\n", l, d);}*/
		for (i=0; i < sec->npt3d; ++i) {
			sec->pt3d[i].arc =     sec->pt3d[i].arc    *fac;
			sec->pt3d[i].x = x0 + (sec->pt3d[i].x - x0)*fac;
			sec->pt3d[i].y = y0 + (sec->pt3d[i].y - y0)*fac;
			sec->pt3d[i].z = z0 + (sec->pt3d[i].z - z0)*fac;
			
		}
		++nrn_shape_changed_;
	}
}

/*ARGSUSED*/
int
can_change_morph(sec)
	Section* sec;
{
	return pt3dconst_ == 0;
}
	
int
pt3dadd() {
	/*pt3add(x,y,z, d) stores 3d point at end of current pt3d list.
	  first point assumed to be at arc length position 0. Last point
	  at 1. arc length increases monotonically.
	*/
	stor_pt3d(chk_access(), *getarg(1), *getarg(2),
		*getarg(3), *getarg(4));
	ret(1.);
}

int
n3d() { /* returns number of 3d points in section */
	Section* sec;
	sec = chk_access();
	ret ((double)sec->npt3d);
}

int
x3d() { /* returns x value at index of 3d list  */
	Section* sec;
	int n, i;
	sec = chk_access();
	n = sec->npt3d - 1;
	i = chkarg(1, 0., (double)n);
	ret((double)sec->pt3d[i].x);
}

int
y3d() { /* returns x value at index of 3d list  */
	Section* sec;
	int n, i;
	sec = chk_access();
	n = sec->npt3d - 1;
	i = chkarg(1, 0., (double)n);
	ret((double)sec->pt3d[i].y);
}
int
z3d() { /* returns x value at index of 3d list  */
	Section* sec;
	int n, i;
	sec = chk_access();
	n = sec->npt3d - 1;
	i = chkarg(1, 0., (double)n);
	ret((double)sec->pt3d[i].z);
}

int
arc3d() { /* returns x value at index of 3d list  */
	Section* sec;
	int n, i;
	sec = chk_access();
	n = sec->npt3d - 1;
	i = chkarg(1, 0., (double)n);
	ret((double)sec->pt3d[i].arc);
}

int
diam3d() { /* returns x value at index of 3d list  */
	Section* sec;
	double d;
	int n, i;
	sec = chk_access();
	n = sec->npt3d - 1;
	i = chkarg(1, 0., (double)n);
	d = (double)sec->pt3d[i].d;
	ret(fabs(d));
}

int
spine3d() { /* returns x value at index of 3d list  */
	Section* sec;
	int n, i;
	double d;
	sec = chk_access();
	n = sec->npt3d - 1;
	i = chkarg(1, 0., (double)n);
	d = (double)sec->pt3d[i].d;
	if (d < 0) {
		ret(1.);
	}else{
		ret(0.);
	}
}

stor_pt3d(sec, x,y,z, d)
	Section *sec;
	double x,y,z, d;
{
	int n;
	
	n = sec->npt3d;
	nrn_pt3dbufchk(sec, n+1);
	sec->npt3d++;			
	sec->pt3d[n].x = x;
	sec->pt3d[n].y = y;
	sec->pt3d[n].z = z;
	sec->pt3d[n].d = d;
	nrn_pt3dmodified(sec, n);
}

static double spinearea=0.;

setSpineArea() {
	spinearea = *getarg(1);
	diam_changed = 1;
	ret(spinearea);
}

getSpineArea() {
	ret(spinearea);
}

define_shape() {
	nrn_define_shape();
	ret(1.);
}

static nrn_translate_shape(sec, x, y, z)
	Section* sec;
	float x, y, z;
{
	int i;
	float dx, dy, dz;
	if (pt3dconst_) {
		return;
	}
	if (sec->logical_connection) { /* args are relative not absolute */
		Pt3d* p = sec->logical_connection;
		dx = x - p->x;
		dy = y - p->y;
		dz = z - p->z;
		p->x = x;
		p->y = y;
		p->z = z;
	}else{
		dx = x - sec->pt3d[0].x;
		dy = y - sec->pt3d[0].y;
		dz = z - sec->pt3d[0].z;
	}
/*	if (dx*dx + dy*dy + dz*dz < 10.)*/
	for (i=0; i < sec->npt3d; ++i) {
		sec->pt3d[i].x += dx;
		sec->pt3d[i].y += dy;
		sec->pt3d[i].z += dz;
	}
}

nrn_define_shape() {
	static changed_;
	int i, j;
	Section* sec, *psec, *ch, *nrn_trueparent();
	float x, y, z, dz, x1, y1;
	float nch, ich, angle;
	double arc, len;
	double nrn_connection_position();
	if (changed_ == nrn_shape_changed_ && !diam_changed && !tree_changed) {
		return;
	}
	recalc_diam();
	dz = 100.;
	for (i=0; i < section_count; ++i) {
		sec = secorder[i];
		arc = nrn_connection_position(sec);
		if ((psec = sec->parentsec) == (Section*)0) {
			/* section has no parent */
			x = 0;
			y = 0;
			z = i * dz;
			x1 = 1;
			y1 = 0;
		}else{
			double arc1 = arc;
			x1 = psec->pt3d[psec->npt3d-1].x - psec->pt3d[0].x;
			y1 = psec->pt3d[psec->npt3d-1].y - psec->pt3d[0].y;
			if (!arc0at0(psec)) {
				arc1 = 1. - arc;
			}				
			if (arc1 < .5) {
				x1 = -x1;
				y1 = -y1;
			}
x = psec->pt3d[psec->npt3d-1].x*arc1 + psec->pt3d[0].x*(1-arc1);
y = psec->pt3d[psec->npt3d-1].y*arc1 + psec->pt3d[0].y*(1-arc1);
z = psec->pt3d[psec->npt3d-1].z*arc1 + psec->pt3d[0].z*(1-arc1);
		}
		if (sec->npt3d) {
			if (psec) {
				nrn_translate_shape(sec, x, y, z);
			}
			continue;
		}
		if (fabs(y1) < 1e-6 && fabs(x1) < 1e-6) {
printf("nrn_define_shape: %s first and last 3-d point at same (x,y)\n", secname(psec));
			angle = 0.;
		}else{
			angle = atan2(y1, x1);
		}
		if (arc > 0. && arc < 1.) {
			angle += 3.14159265358979323846/2.;
		}
		nch = 0.;
		if (psec) for (ch=psec->child; ch; ch = ch->sibling) {
			if (ch == sec) {
				ich = nch;
			}
			if (arc == nrn_connection_position(ch)) {
				++nch;
			}
		}		
		if (nch > 1) {
			angle += ich/(nch - 1.) * .8 - .4;
		}
		len = section_length(sec);
		x1 = x + len*cos(angle);
		y1 = y + len*sin(angle);
		stor_pt3d(sec, x, y, z, nrn_diameter(sec->pnode[0]));
		for (j = 0; j < sec->nnode-1; ++j) {
			double frac = ((double)j+.5)/(double)(sec->nnode-1);
			stor_pt3d(sec,
				x*(1-frac)+x1*frac,
				y*(1-frac)+y1*frac,
				z,
				nrn_diameter(sec->pnode[j])
			);
		}
		stor_pt3d(sec, x1, y1, z, nrn_diameter(sec->pnode[sec->nnode-2]));
		/* don't let above change length due to round-off errors*/
		sec->pt3d[sec->npt3d-1].arc = len;		
		sec->prop->dparam[2].val = len;
	}
	changed_ = nrn_shape_changed_;
}

static double diam_from_list(sec, inode, p, rparent)
	Section *sec;
	int inode;
	Prop *p;  /* p->param[0] is diam of inode in sec.*/
	double rparent;  /* right half resistance of the parent segment*/
{
	/* Basic algorithm assumes a set of monotonic points on which a
	   function is defined. The extension is the piecewise continuous
	   function. y(x) for 0<=x<=x[n] (extrapolate for x>x[n]).
	   The problem is to form the integral of f(y(s)) over intervals
	   s[i] <= s <= s[i+1]. Note the intervals don't have to be equal.
	   This implementation is specific to a call to this function for
	   each interval in order.
	*/
	/* computes diam as average, area, and ri. Slightly weirder since
	   interval spit in two to compute right and left half values of ri
	*/
	/* fills NODEAREA and NODERINV and returns the right half resistance
	   (MOhms) of the segment.
	*/
	static int j;
	static double x1, y1, ds;
	int ihalf;
	double si, sip;
	double diam, delta, temp, ri, area, ra, rleft;
	int npt, nspine;

	if (inode == 0) {
		j = 0;
		si = 0.;
		x1 = sec->pt3d[j].arc;
		y1 = fabs(sec->pt3d[j].d);
		ds = sec->pt3d[sec->npt3d - 1].arc / ((double)(sec->nnode - 1));
	}
	si = (double)inode*ds;
	npt = sec->npt3d;
	diam = 0.;
	area = 0.;
	nspine = 0;
	ra = nrn_ra(sec);
    for (ihalf = 0; ihalf < 2; ihalf++) {
	ri = 0.;
    	sip = si + ds/2.;
	for (;;) {
		int jp, jnext;
		double x2, y2, xj, xjp;
		jp = j + 1;
		xj = sec->pt3d[j].arc;
#if NTS_SPINE
		if (sec->pt3d[j].d < 0 && xj >= si && xj < sip) {
			nspine++;
		}
#endif
		xjp = sec->pt3d[jp].arc;
		if (xjp > sip || jp == npt - 1) {
			double frac;
			if (fabs(xjp - xj) < 1e-10) {
				frac = 1;
			}else{
				frac = (sip - xj)/(xjp - xj);
			}
			x2 = sip;
			y2 = (1. - frac)*fabs(sec->pt3d[j].d) + frac*fabs(sec->pt3d[jp].d);
			jnext = j;
		}else{
			x2 = xjp;
			y2 = fabs(sec->pt3d[jp].d);
			jnext = jp;
		}

		/* f += integrate(x1,y1, x2, y2) */
		delta = (x2 - x1);
		diam += (y2 + y1)*delta;
		if (delta < 1e-15) {
			delta = 1e-15;
		}
		if ((temp = y2*y1/delta) == 0) {
			temp = 1e-15;
		}
		ri += 1/temp;
#if 1			/*taper is very seldom taken into account*/
			temp = .5*(y2 - y1);
			temp = sqrt(delta*delta + temp*temp);
#else
			temp = delta;
#endif

		area += (y2 + y1)*temp;
		/* end of integration */

		x1 = x2;
		y1 = y2;
		if (j == jnext) {
			break;
		}
		j = jnext;
	}
	if (ihalf == 0) {
		rleft = ri*ra/PI*(4.*.01); /*left seg resistance*/
	}else{
		ri = ri*ra/PI*(4.*.01);	/* MegOhms */
		/* above is right half segment resistance */
	}
	si = sip;
    }
	/* answer for inode is here */
	NODERINV(sec->pnode[inode]) = 1./(rparent + rleft);
	diam *= .5/ds;
	if (fabs(diam - p->param[0]) > 1e-9 || diam < 1e-5) {
		p->param[0] = diam;	/* microns */
	}
	NODEAREA(sec->pnode[inode]) = area*.5*PI;/* microns^2 */
	UPDATE_VEC_AREA(sec->pnode[inode]);
#if NTS_SPINE
	/* if last point has a spine then increment spine count for last node */
	if (inode == sec->nnode-2 && sec->pt3d[npt-1].d < 0.) {
		nspine += 1;
	}
	NODEAREA(sec->pnode[inode]) += nspine*spinearea;
	UPDATE_VEC_AREA(sec->pnode[inode]);
#endif
	return ri;
}

#endif /*DIAMLIST*/

#if VECTORIZE
v_setup_vectors() {
	int inode, i;
	int isec;
	Section* sec;
	Node* nd;
	Prop* p;
	
	if (tree_changed) {
		setup_topology();
		v_structure_change = 1;
		++nrn_shape_changed_;
	}
	/* get rid of the old */
	if (!v_structure_change) {
		return;
	}
	if (v_node) {
		free((char*) v_node);
		free((char*) v_parent);
		v_node = 0;
		v_parent = 0;
		v_node_count = 0;
	}
#if CACHEVEC
	if (v_parent_index) {
		free((void*)v_parent_index);
	}
#endif /* CACHEVEC */
	for (i=0; i < n_memb_func; ++i)
	  if (memb_func[i].current || memb_func[i].state || memb_func[i].initialize) {
		if (memb_list[i].nodecount) {
			memb_list[i].nodecount = 0;
			free((char*)memb_list[i].nodelist);
#if CACHEVEC
			free((void *)memb_list[i].nodeindices);
#endif /* CACHEVEC */
			if (memb_func[i].hoc_mech) {
				free((char*)memb_list[i].prop);
			}else{
				free((char*)memb_list[i].data);
				free((char*)memb_list[i].pdata);
			}
		}
	}
		
	/* count up what we need */

	for (isec = 0; isec < section_count; ++isec) {
		sec = secorder[isec];
		for (inode = -1; inode < sec->nnode; ++inode) {
			if (inode == -1) {
				if (sec->parentsec) {
					continue;
				}else{
					nd = sec->parentnode;
				}
			}else {
				nd = sec->pnode[inode];
			}
			++v_node_count;
			for (p = nd->prop; p; p = p->next) {
				if (memb_func[p->type].current
				   || memb_func[p->type].state
				   || memb_func[p->type].initialize
				   ) {
					++memb_list[p->type].nodecount;
				}
			}
		}
	}
#if 1 /* see finitialize */
	/* and count the artificial cells */
	for (i=0; i < n_memb_func; ++i) if (nrn_is_artificial_[i] && memb_func[i].initialize) {
		Template* tmp = nrn_pnt_template_[i];
		memb_list[i].nodecount = tmp->count;
	}
#endif
	
	/* allocate it*/

	if (v_node_count) {
		v_node = (Node**) emalloc(v_node_count*sizeof(Node*));
		v_parent = (Node**) emalloc(v_node_count*sizeof(Node*));
		v_node_count = 0; /* counted again below */
	}
	for (i=0; i < n_memb_func; ++i)
	  if (memb_func[i].current || memb_func[i].state || memb_func[i].initialize) {
		if (memb_list[i].nodecount) {
			memb_list[i].nodelist = (Node**) emalloc(memb_list[i].nodecount * sizeof(Node*));
#if CACHEVEC
			memb_list[i].nodeindices = (int *) emalloc(memb_list[i].nodecount * sizeof(int));
#endif /* CACHEVEC */
			if (memb_func[i].hoc_mech) {
				memb_list[i].prop = (Prop**) emalloc(memb_list[i].nodecount * sizeof(Prop*));
			}else{
				memb_list[i].data = (double**) emalloc(memb_list[i].nodecount * sizeof(double*));
				memb_list[i].pdata = (Datum**) emalloc(memb_list[i].nodecount * sizeof(Datum*));
			}
			memb_list[i].nodecount = 0; /* counted again below */
		}
	}

	/* setup the pointers */
	/* setup the node order (classical follows the section order) */
	/* put the nodes in an order suitable for vectorizing the elimination */
	for (isec = 0; isec < rootnodecount; ++isec) {
		sec = secorder[isec];
		assert(!sec->parentsec);
		nd = sec->parentnode;
		v_node[isec] = nd;
		v_parent[isec] = (Node*)0;
		v_node[v_node_count]->v_node_index = v_node_count;
		++v_node_count;
	}
	for (isec = 0; isec < section_count; ++isec) {
		sec = secorder[isec];
		for (inode = 0; inode < sec->nnode; ++inode) {
			nd = sec->pnode[inode];
			v_node[v_node_count] = nd;
			if (inode) {
				v_parent[v_node_count] = sec->pnode[inode-1];
			}else{
				v_parent[v_node_count] = sec->parentnode;
			}
			v_node[v_node_count]->v_node_index = v_node_count;
			++v_node_count;
		}
	}

#if PARANEURON
	for (inode=0; inode < v_node_count; ++inode) {
		v_node[inode]->_classical_parent = v_parent[inode];	
	}
	if (nrn_multisplit_setup_) {
		/* classical order abandoned */
		(*nrn_multisplit_setup_)();
	}
#endif

#if CACHEVEC
	v_parent_index = (int *)malloc(sizeof(int)*v_node_count);
	for (inode=0; inode<v_node_count; inode++) {
		if (v_parent[inode]!=NULL) {
			v_parent_index[inode]=v_parent[inode]->v_node_index;
		}
	}
	
#endif /* CACHEVEC */

	/* setup node prop vectors */
	for (inode=0; inode < v_node_count; inode++) {
		nd = v_node[inode];
		for (p = nd->prop; p; p = p->next) {
			i = p->type;
			if (memb_func[i].current
			   || memb_func[i].state
			   || memb_func[i].initialize
			   ) {
				if (memb_func[i].is_point) {
					Point_process* pnt = (Point_process*)p->dparam[1]._pvoid;
					pnt->iml_ = memb_list[i].nodecount;
				}
				memb_list[i].nodelist[memb_list[i].nodecount] = nd;
#if CACHEVEC
				memb_list[i].nodeindices[memb_list[i].nodecount] = inode;
#endif /* CACHEVEC */
				if (memb_func[i].hoc_mech) {
					memb_list[i].prop[memb_list[i].nodecount] = p;
				}else{
					memb_list[i].data[memb_list[i].nodecount] = p->param;
					memb_list[i].pdata[memb_list[i].nodecount] = p->dparam;
				}
				++memb_list[i].nodecount;
			}
		}
	}

#if 1 /* see finitialize */
	/* fill in artificial cell info */
	for (i=0; i < n_memb_func; ++i) if (nrn_is_artificial_[i] && memb_func[i].initialize) {
		hoc_Item* q;
		hoc_List* list;
		int j;
		Template* tmp = nrn_pnt_template_[i];
		memb_list[i].nodecount = tmp->count;
		j = 0;
		list = tmp->olist;
		ITERATE(q, list) {
			Object* obj = OBJ(q);
			Point_process* pnt = (Point_process*)obj->u.this_pointer;
			pnt->iml_ = j;
			p = pnt->prop;
			memb_list[i].nodelist[j] = (Node*)0;
			memb_list[i].data[j] = p->param;
			memb_list[i].pdata[j] = p->dparam;
			++j;			
		}
	}
#endif
	v_structure_change = 0;
	++structure_change_cnt;
#if _CRAY
	cray_solve();
	cray_node_setup();
#endif
}

#if _CRAY

cray_solve() {
	int i, id, max_depth;
	int* nchild, *depth;
	/* free the old stuff */
	if (v_node_depth) {
		free((char*)v_node_depth_count);
		for (i = 0; i < v_node_depth; i++) {
			free((char*)v_node_depth_lists[i]);
			free((char*)v_parent_depth_lists[i]);
		}
		free((char*)v_node_depth_lists);
		free((char*)v_parent_depth_lists);
		v_node_depth = 0;
		v_node_depth_lists = 0;
		v_parent_depth_lists = 0;
		v_node_depth_count = 0;
	}
	
	/*
	  The bad thing is when we have one long
	  cable. Then the depth increases and there aren't many other nodes
	  that can share the depth.
	*/
	/*
	  We require that
	  no nodes at the same depth can share the same parent. Otherwise
	  during triangularizaton several nodes may try to change the same
	  parent at the same time. The curious fact is that a node may
	  increase its depth as long as it does not go to a depth that
	  has a node with the same parent and remains at a depth which is
	  less than any of its children.
	*/
  	depth = (int*)ecalloc(v_node_count, sizeof(int));
	nchild = (int*)ecalloc(v_node_count, sizeof(int));
	max_depth = 0;
	for (i=rootnodecount; i < v_node_count; ++i){
		int pindex = v_parent[i]->v_node_index;
		++nchild[pindex];
		depth[i] = depth[pindex] + nchild[pindex];
		if (depth[i] > max_depth) {
			max_depth = depth[i];
		}
	}
		
	/*allocate memory*/
	v_node_depth = max_depth + 1;
	v_node_depth_count = (int*)ecalloc(v_node_depth, sizeof(int));
	/* count the list sizes */
	for (i=0; i < v_node_count; ++i) {
		++v_node_depth_count[depth[i]];
	}
	v_node_depth_lists = (Node***)ecalloc(v_node_depth, sizeof(Node**));
	v_parent_depth_lists = (Node***)ecalloc(v_node_depth, sizeof(Node**));
	for (i=0; i < v_node_depth; ++i) {
v_node_depth_lists[i] = (Node**)ecalloc(v_node_depth_count[i], sizeof(Node*));
v_parent_depth_lists[i] = (Node**)ecalloc(v_node_depth_count[i], sizeof(Node*));
	}
	/* set up the lists */
	for (i=0; i < v_node_depth; ++i) {
		v_node_depth_count[i] = 0;
	}
	for (i = 0; i < v_node_count; ++i) {
		id = depth[i];
		v_node_depth_lists[id][v_node_depth_count[id]] = v_node[i];
		v_parent_depth_lists[id][v_node_depth_count[id]] = v_parent[i];
		++v_node_depth_count[id];
	}

#if 0
	/* for debugging so we can know which node is which place the
		section index in NODED(nd) */
	for (i=0; i < section_count; ++i) {
		sec = secorder[i];
		for (j=0; j < sec->nnode; ++j) {
			NODED(sec->pnode[j]) = (double)i;
		}
	}
	printf("v_node_depth = %d\n", v_node_depth);
	for (i=0; i < v_node_depth; ++i) {
		count = v_node_depth_count[i];
		printf("  depth = %d  count = %d\n", i, count);
		for (j = 0; j < count; ++j) {
			nd = v_node_depth_lists[i][j];
			pnd = v_parent_depth_lists[i][j];
			sec = secorder[(int)(NODED(nd))];
			psec = secorder[(int)(NODED(pnd))];
printf("    %d node %s[%d]", j, secname(sec), nd->sec_node_index_);
printf("  parent %s[%d]\n", secname(psec), pnd->sec_node_index_);
  		}
	}
#endif
	free((char*)depth);
	free((char*)nchild);
}

static cray_node_setup() {
	int i, j, k;
	cray_nodes.cnt = 0;
	if (cray_nodes.pnd1) {
		free((char*) cray_nodes.pnd1);
		free((char*) cray_nodes.nd2);
	}

	for (i=0; i < n_memb_func; ++i) if (
	   memb_func[i].is_point
	   && memb_func[i].vectorized
	   && memb_func[i].current
	   && memb_list[i].nodecount > 1)
	{
		for (j =  memb_list[i].nodecount - 1; j; --j) {
			if (memb_list[i].nodelist[j] == memb_list[i].nodelist[j-1]) {
				++cray_nodes.cnt;
			}
		}
	}

	if (cray_nodes.cnt) {
		cray_nodes.pnd1 = (Node**)ecalloc(cray_nodes.cnt, sizeof(Node*));
		cray_nodes.nd2 = (Node*)ecalloc(cray_nodes.cnt, sizeof(Node));
		k = 0;
		for (i=0; i < n_memb_func; ++i) if (
		   memb_func[i].is_point
		   && memb_func[i].vectorized
		   && memb_func[i].current
		   && memb_list[i].nodecount > 1)
		{
			for (j =  memb_list[i].nodecount - 1; j; --j) {
				if (memb_list[i].nodelist[j] == memb_list[i].nodelist[j-1]) {
cray_nodes.pnd1[k] = memb_list[i].nodelist[j];
cray_nodes.nd2[k].area = memb_list[i].nodelist[j]->area;
#if 1
/* the following should not be necessary */
cray_nodes.nd2[k].prop = memb_list[i].nodelist[j]->prop;
cray_nodes.nd2[k].child = memb_list[i].nodelist[j]->child;
#endif
memb_list[i].nodelist[j] = cray_nodes.nd2 + k;
					++k;
				}
			}
		}
	}
}

cray_node_init() {
	if (cray_nodes.cnt){
		int i;
		Node* nd1, *nd2;
#pragma _CRI ivdep
		for (i=0; i < cray_nodes.cnt; ++i) {
			nd1 = cray_nodes.pnd1[i];
			nd2 = cray_nodes.nd2 + i;
			nd2->v = nd1->v;
		}
	}
		
}

#endif

#endif

#define NODE_DATA 0
#if NODE_DATA
static FILE* fnd;

#undef P
#undef Pn
#undef Pd
#undef Pg

#define P fprintf(fnd,
#define Pn P "\n")
#define Pd(arg) P "%d\n", arg)
#define Pg(arg) P "%g\n", arg)

node_data_scaffolding() {
	int i;
	Pd(n_memb_func);
/*	P "Mechanism names (first two are nil) beginning with memb_func[2]\n");*/
	for (i=2; i < n_memb_func; ++ i){
		P "%s", memb_func[i].sym->name); Pn;
	}
}

node_data_structure() {
	int i, j;
	Pd(v_node_count);
	
	Pd(rootnodecount);
/*	P "Indices of node parents\n");*/
	for (i=0; i < v_node_count; ++i) {
		Pd(v_parent[i]->v_node_index);
	}
/*	P "node lists for the membrane mechanisms\n");*/
	for (i=2; i < n_memb_func; ++ i){
/*		P "count, node list for mechanism %s\n", memb_func[i].sym->name);*/
		Pd(memb_list[i].nodecount);
		for (j=0; j < memb_list[i].nodecount; ++j) {
			Pd(memb_list[i].nodelist[j]->v_node_index);
		}
	}
}

node_data_values() {
	int i, j, k;
/*	P "data for nodes then for all mechanisms in order of the above structure\n");	*/
	for (i=0; i < v_node_count; ++ i) {
		Pg(NODEV(v_node[i]));
		Pg(NODEA(v_node[i]));
		Pg(NODEB(v_node[i]));
		Pg(NODEAREA(v_node[i]));
	}
	for (i=2; i < n_memb_func; ++ i){
		Prop* prop, *nrn_mechanism();
		int cnt;
		double* pd;
		if (memb_list[i].nodecount) {
			assert(!memb_func[i].hoc_mech);
			prop = nrn_mechanism(i, memb_list[i].nodelist[0]);
			cnt = prop->param_size;
			Pd(cnt);
		}
		for (j=0; j < memb_list[i].nodecount; ++j) {
			pd = memb_list[i].data[j];
			for (k = 0; k < cnt; ++k) {
				Pg(pd[k]);
			}
		}
	}
}

node_data() {
	fnd = fopen(gargstr(1), "w");
	if (!fnd) {
		hoc_execerror("node_data: can't open", gargstr(1));
	}
	if (tree_changed) {
		setup_topology();
	}
	if (v_structure_change) {
		v_setup_vectors();
	}
	if (diam_changed) {
		recalc_diam();
	}
	node_data_scaffolding();
	node_data_structure();
	node_data_values();
	fclose(fnd);
	ret(1.);
}

#else
node_data() {
	printf("recalc_diam=%d nrn_area_ri=%d\n", recalc_diam_count_, nrn_area_ri_count_);
	ret(0.);
}

#endif

nrn_complain(pp) double* pp; {
	/* print location for this param on the standard error */
	Node* nd;
	hoc_Item* qsec;
	int j;
	Prop* p;
	ForAllSections(sec)
		for (j = 0; j < sec->nnode; ++j) {
			nd = sec->pnode[j];
			for (p = nd->prop; p; p = p->next) {
				if (p->param == pp) {
					
fprintf(stderr, "Error at section location %s(%g)\n", secname(sec),
  nrn_arc_position(sec, nd));
					return;
				}
			}
		}
	}
	fprintf(stderr, "Don't know the location of params at %lx\n", (long)pp);
}

nrn_matrix_node_free() {
	if (actual_rhs) {
		free((char*)actual_rhs);
		actual_rhs = (double*)0;
	}
	if (actual_d) {
		free((char*)actual_d);
		actual_d = (double*)0;
	}
#if CACHEVEC
	if (actual_a) {
		free((char*)actual_a);
		actual_a = (double*)0;
	}
	if (actual_b) {
		free((char*)actual_b);
		actual_b = (double*)0;
	}
	/* because actual_v and actual_area have pointers to them from many
	   places, defer the freeing until nrn_recalc_node_ptrs is called
	*/
#endif /* CACHEVEC */
	if (sp13mat_) {
		spDestroy(sp13mat_);
		sp13mat_ = (char*)0;
	}
	diam_changed = 1;
}

/* 0 means no model, 1 means ODE, 2 means DAE */
int nrn_modeltype() {
	static Template* lm = (Template*)0;
	int type;
	if (!lm) {
		Symbol* s = hoc_table_lookup("LinearMechanism", hoc_built_in_symlist);
		if (s) {
			lm = s->u.template;
		}
	}
	v_setup_vectors();
	type = 0;
	if (rootnodecount > 0) {
		type = 1;
		if ((lm && lm->count > 0) || memb_list[EXTRACELL].nodecount > 0) {
			type = 2;
		}
	}
	if (type == 0 && linmod_extra_eqn_count() > 0) {
		type = 2;
	}
	return type;
}

/*
minimally update flags consistent with the model type and cvode_active_
return true if any flags changed.
If DAE then variable step method requires daspk
If DAE then must be sparse13
If daspk then must be sparse13
If cvode then must be classic tree method
*/

int nrn_method_consistent() {
	int consist;
	int type;
	consist = 0;
	type = nrn_modeltype();
	if (cvode_active_) {
		if (type == 2) {
			if (nrn_use_daspk_ == 0) {
				nrn_use_daspk(1);
				consist = 1;
			}
		}
		if (nrn_use_daspk_ != use_sparse13) {
			use_sparse13 = nrn_use_daspk_;
			consist = 1;
		}
	}else if (type == 2 && use_sparse13 == 0) {
		use_sparse13 = 1;
		consist = 1;
	}
	if (use_sparse13 != 0) {
		nrn_cachevec(0);
	}
	return consist;
}


/*
sparse13 uses the mathematical index scheme in which the rows and columns
range from 1...size instead of 0...size-1. Thus the calls to
spGetElement(i,j) have args that are 1 greater than a normal c style.
Also the actual_rhs_ uses this style, 1-neqn, when sparse13 is activated
and therefore is passed to spSolve as actual_rhs intead of actual_rhs-1.
*/

nrn_matrix_node_alloc() {
	int i;
	Node* nd;
	static int sp13_ = 0;
	
	nrn_method_consistent();
	if (sp13_ != use_sparse13) {
		nrn_matrix_node_free();
		sp13_ = use_sparse13;
	}
	if (actual_rhs != (double*)0) {
		return;
	}
#if CACHEVEC
	actual_a = (double*)ecalloc(v_node_count, sizeof(double));
	actual_b = (double*)ecalloc(v_node_count, sizeof(double));
	nrn_recalc_node_ptrs();
#endif /* CACHEVEC */

#if 0
printf("nrn_matrix_node_alloc use_sparse13=%d cvode_active_=%d nrn_use_daspk_=%d\n", use_sparse13, cvode_active_, nrn_use_daspk_);
#endif
	++nrn_matrix_cnt_;
	if (use_sparse13) {
		int in, err, extn, neqn, j;
		neqn = v_node_count + linmod_extra_eqn_count();
		extn =  memb_list[EXTRACELL].nodecount * nlayer;
/*printf(" %d extracellular nodes\n", extn);*/
		neqn += extn;
		actual_rhs = (double*)ecalloc(neqn+1, sizeof(double));
		sp13mat_ = spCreate(neqn, 0, &err);
		if (err != spOKAY) {
			hoc_execerror("Couldn't create sparse matrix", (char*)0);
		}
		for (in=0, i=1; in < v_node_count; ++in, ++i) {
			v_node[in]->eqn_index_ = i;
			if (v_node[in]->extnode) {
				i += nlayer;
			}
		}
		for (in = 0; in < v_node_count; ++in) {
			int ie, k;
			Node* nd, *pnd;
			Extnode* nde;
			nd = v_node[in];
			nde = nd->extnode;
			pnd = v_parent[in];
			i = nd->eqn_index_;
			nd->_rhs = actual_rhs + i;
			nd->_d = spGetElement(sp13mat_, i, i);
			if (nde) {
			    for (ie = 0; ie < nlayer; ++ie) {
				k = i + ie + 1;
				nde->_d[ie] = spGetElement(sp13mat_, k, k);
				nde->_rhs[ie] = actual_rhs + k;
				nde->_x21[ie] = spGetElement(sp13mat_, k, k-1);
				nde->_x12[ie] = spGetElement(sp13mat_, k-1, k);
			    }
			}
			if (pnd) {
				j = pnd->eqn_index_;
				nd->_a_matelm = spGetElement(sp13mat_, j, i);
				nd->_b_matelm = spGetElement(sp13mat_, i, j);
			    if (nde && pnd->extnode) for (ie = 0; ie < nlayer; ++ie) {
				int kp = j + ie + 1;
				k = i + ie + 1;
				nde->_a_matelm[ie] = spGetElement(sp13mat_, kp, k);
				nde->_b_matelm[ie] = spGetElement(sp13mat_, k, kp);
			    }
			}else{ /* not needed if index starts at 1 */
				nd->_a_matelm = (double*)0;
				nd->_b_matelm = (double*)0;
			}
		}
		linmod_alloc();
	}else{
		assert(linmod_extra_eqn_count() == 0);
		assert(memb_list[EXTRACELL].nodecount == 0);
		actual_d = (double*)ecalloc(v_node_count, sizeof(double));
		actual_rhs = (double*)ecalloc(v_node_count, sizeof(double));
		for (i = 0; i < v_node_count; ++i) {
			Node* nd = v_node[i];
			nd->_d = actual_d + i;
			nd->_rhs = actual_rhs + i;
		}
	}
}

void nrn_cachevec(b) int b; {
	if (use_sparse13) {
		use_cachevec = 0;
	}else{
		if (b && use_cachevec == 0) {
			tree_changed = 1;
		}
		use_cachevec = b;
	}
}

#if CACHEVEC
/*
Pointers that need to be updated are:
All Point process area pointers.
All mechanism POINTER variables that point to  v.
All Graph addvar pointers that plot v.
All Vector record and play pointers that deal with v.
All PreSyn threshold detectors that watch v.
*/

extern void nrniv_recalc_ptrs(int, double**, double*);
extern int nrn_isdouble(double*, double, double);
static int n_recalc_ptr_callback;
static void (*recalc_ptr_callback[20])();
static double *recalc_ptr_new_v_, **recalc_ptr_old_vp_;

double* nrn_recalc_ptr(double* old) {
	if (!recalc_ptr_old_vp_) { return old; }
	if (nrn_isdouble(old, 0.0, (double)v_node_count)) {
		int k = (int)(*old);
		if (old == recalc_ptr_old_vp_[k]) {
			return recalc_ptr_new_v_ + k;
		}
	}
	return old;
}

void nrn_register_recalc_ptr_callback(void (*f)()) {
	if (n_recalc_ptr_callback >= 20) {
		printf("More than 20 recalc_ptr_callback functions\n");
		exit(1);
	}
	recalc_ptr_callback[n_recalc_ptr_callback++] = f;
}

void nrn_recalc_node_ptrs() {
	int i, j, k;
	double *new_area;
	if (use_cachevec == 0) { return; }
	recalc_ptr_new_v_ = (double*)ecalloc(v_node_count, sizeof(double));
	recalc_ptr_old_vp_ = (double**)ecalloc(v_node_count, sizeof(double*));
	new_area = (double*)ecalloc(v_node_count, sizeof(double));
	/* first update the pointers without messing with the old NODEV,NODEAREA */
	/* to prepare for the update, copy all the v and area values into the */
	/* new arrays are replace the old values by index value. */
	/* a pointer dereference value of i allows us to easily check */
	/* if the pointer points to what v_node[i]->_v points to. */
	for (i = 0; i < v_node_count; ++i) {
		Node* nd = v_node[i];
		recalc_ptr_new_v_[i] = *nd->_v;
		recalc_ptr_old_vp_[i] = nd->_v;
		new_area[i] = nd->_area;
		*nd->_v = (double)i;
	}
	/* update POINT_PROCESS pointers to NODEAREA */
	/* and relevant POINTER pointers to NODEV */
	for (i=0; i < v_node_count; ++i) {
		Node* nd = v_node[i];
		Prop* p;
		Datum* d;
		int dpend;
		for (p = nd->prop; p; p = p->next) {
			if (memb_func[p->type].is_point && !nrn_is_artificial_[p->type]) {
				p->dparam[0].pval = new_area + i;
			}
			dpend = nrn_dparam_ptr_end_[p->type];
			for (j=nrn_dparam_ptr_start_[p->type]; j < dpend; ++j) {
				double* pval = p->dparam[j].pval;
				if (nrn_isdouble(pval, 0., (double)v_node_count)) {
					/* possible pointer to v */
					k = (int)(*pval);
					if (pval == v_node[k]->_v) {
					    p->dparam[j].pval = recalc_ptr_new_v_ + k;
					}
				}
			}
		}
	}

	/* update pointers managed by c++ */
	nrniv_recalc_ptrs(v_node_count, recalc_ptr_old_vp_, recalc_ptr_new_v_);

	/* user callbacks to update pointers */
	for (i=0; i < n_recalc_ptr_callback; ++i) {
		(*recalc_ptr_callback[i])();
	}

	/* now that all the pointers are updated we update the NODEV */
	for (i = 0; i < v_node_count; ++i) {
		Node* nd = v_node[i];
		nd->_v = recalc_ptr_new_v_ + i;
	}
	/* and free the old arrays */
	if (actual_v) {
		hoc_free_val_array(actual_v, actual_v_size);
		actual_v = (double*)0;
		actual_v_size = 0;
	}
	if (actual_area) {
		free((char*)actual_area);
		actual_area = (double*)0;
	}
	free((char*)recalc_ptr_old_vp_);
	actual_v_size = v_node_count;
	actual_v = recalc_ptr_new_v_;
	actual_area = new_area;
	recalc_ptr_old_vp_ = (double**)0;
	recalc_ptr_new_v_ = (double*)0;

	nrn_cache_prop_realloc();
}

#endif /* CACHEVEC */
