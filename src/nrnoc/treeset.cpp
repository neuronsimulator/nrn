#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/treeset.cpp,v 1.39 1999/07/08 14:25:07 hines Exp */

#include "cvodeobj.h"
#include "membfunc.h"
#include "multisplit.h"
#include "nrn_ansi.h"
#include "neuron.h"
#include "neuron/cache/model_data.hpp"
#include "neuron/container/soa_container_impl.hpp"
#include "nonvintblock.h"
#include "nrndae_c.h"
#include "nrniv_mf.h"
#include "nrnmpi.h"
#include "ocnotify.h"
#include "section.h"
#include "spmatrix.h"
#include "treeset.h"
#include "utils/profile/profiler_interface.h"
#include "multicore.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>
#include <math.h>
#include <string>

extern spREAL* spGetElement(char*, int, int);

int nrn_shape_changed_; /* for notifying Shape class in nrniv */
double* nrn_mech_wtime_;

extern double chkarg(int, double low, double high);
extern double nrn_ra(Section*);
#if !defined(NRNMPI) || NRNMPI == 0
extern double nrnmpi_wtime();
#endif
extern Symlist* hoc_built_in_symlist;
extern int* nrn_prop_dparam_size_;
extern int* nrn_dparam_ptr_start_;
extern int* nrn_dparam_ptr_end_;
extern void nrn_define_shape();
extern void nrn_partrans_update_ptrs();

#if 1 || PARANEURON
void (*nrn_multisplit_setup_)();
#endif

#if CACHEVEC


/* a, b, d and rhs are, from now on, all stored in extra arrays, to improve
 * cache efficiency in nrn_lhs() and nrn_rhs(). Formerly, three levels of
 * indirection were necessary for accessing these elements, leading to lots
 * of L2 misses.  2006-07-05/Hubert Eichner */
/* these are now thread instance arrays */
static void nrn_recalc_node_ptrs();
#define UPDATE_VEC_AREA(nd)                                       \
    if (nd->_nt && nd->_nt->_actual_area) {                       \
        nd->_nt->_actual_area[(nd)->v_node_index] = NODEAREA(nd); \
    }
#endif /* CACHEVEC */
int use_cachevec;

/*
Do not use unless necessary (loops in tree structure) since overhead
(for gaussian elimination) is
about a factor of 3 in space and 2 in time even for a tree.
*/
int nrn_matrix_cnt_ = 0;
int use_sparse13 = 0;
int nrn_use_daspk_ = 0;

/*
When properties are allocated to nodes or freed, v_structure_change is
set to 1. This means that the mechanism vectors need to be re-determined.
*/
int v_structure_change;
int structure_change_cnt;
int diam_change_cnt;
int nrn_node_ptr_change_cnt_;

extern int section_count;
extern Section** secorder;
#if 1 /* if 0 then handled directly to save space : see finitialize*/
extern short* nrn_is_artificial_;
extern cTemplate** nrn_pnt_template_;
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
(see nrn_cur of passive.cpp) It does NOT add anything to the extracellular
current balance row.

An electrode current (convention positive current increases the internal potential)
such as a voltage clamp with small resistance, is=g*(vc - (v+vext))
(see nrn_cur of svclmp.cpp)
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

void nrn_rhs(NrnThread* _nt) {
    int i, i1, i2, i3;
    double w;
    int measure = 0;
    NrnThreadMembList* tml;

    i1 = 0;
    i2 = i1 + _nt->ncell;
    i3 = _nt->end;
    if (_nt->id == 0 && nrn_mech_wtime_) {
        measure = 1;
    }

    if (diam_changed) {
        nrn_thread_error("need recalc_diam()");
        recalc_diam();
    }
    if (use_sparse13) {
        int i, neqn;
        nrn_thread_error("nrn_rhs use_sparse13");
        neqn = spGetSize(_nt->_sp13mat, 0);
        for (i = 1; i <= neqn; ++i) {
            _nt->_actual_rhs[i] = 0.;
        }
    } else {
#if CACHEVEC
        if (use_cachevec) {
            for (i = i1; i < i3; ++i) {
                VEC_RHS(i) = 0.;
            }
        } else
#endif /* CACHEVEC */
        {
            for (i = i1; i < i3; ++i) {
                NODERHS(_nt->_v_node[i]) = 0.;
            }
        }
    }
    if (_nt->_nrn_fast_imem) {
        for (i = i1; i < i3; ++i) {
            _nt->_nrn_fast_imem->_nrn_sav_rhs[i] = 0.;
        }
    }

    nrn_ba(_nt, BEFORE_BREAKPOINT);
    /* note that CAP has no current */
    for (tml = _nt->tml; tml; tml = tml->next)
        if (memb_func[tml->index].current) {
            Pvmi s = memb_func[tml->index].current;
            std::string mechname("cur-");
            mechname += memb_func[tml->index].sym->name;
            if (measure) {
                w = nrnmpi_wtime();
            }
            nrn::Instrumentor::phase_begin(mechname.c_str());
            (*s)(_nt, tml->ml, tml->index);
            nrn::Instrumentor::phase_end(mechname.c_str());
            if (measure) {
                nrn_mech_wtime_[tml->index] += nrnmpi_wtime() - w;
            }
            if (errno) {
                if (nrn_errno_check(tml->index)) {
                    hoc_warning("errno set during calculation of currents", (char*) 0);
                }
            }
        }
    activsynapse_rhs();

    if (_nt->_nrn_fast_imem) {
        /* _nrn_save_rhs has only the contribution of electrode current
           so here we transform so it only has membrane current contribution
        */
        double* p = _nt->_nrn_fast_imem->_nrn_sav_rhs;
        if (use_cachevec) {
            for (i = i1; i < i3; ++i) {
                p[i] -= VEC_RHS(i);
            }
        } else {
            for (i = i1; i < i3; ++i) {
                Node* nd = _nt->_v_node[i];
                p[i] -= NODERHS(nd);
            }
        }
    }
#if EXTRACELLULAR
    /* Cannot have any axial terms yet so that i(vm) can be calculated from
    i(vm)+is(vi) and is(vi) which are stored in rhs vector. */
    nrn_rhs_ext(_nt);
    /* nrn_rhs_ext has also computed the the internal axial current
    for those nodes containing the extracellular mechanism
    */
#endif
    if (use_sparse13) {
        /* must be after nrn_rhs_ext so that whatever is put in
        nd->_rhs does not get added to nde->rhs */
        nrndae_rhs();
    }

    activstim_rhs();
    activclamp_rhs();
    /* now the internal axial currents.
    The extracellular mechanism contribution is already done.
        rhs += ai_j*(vi_j - vi)
    */
#if CACHEVEC
    if (use_cachevec) {
        for (i = i2; i < i3; ++i) {
            double dv = VEC_V(_nt->_v_parent_index[i]) - VEC_V(i);
            /* our connection coefficients are negative so */
            VEC_RHS(i) -= VEC_B(i) * dv;
            VEC_RHS(_nt->_v_parent_index[i]) += VEC_A(i) * dv;
        }
    } else
#endif /* CACHEVEC */
    {
        for (i = i2; i < i3; ++i) {
            Node* nd = _nt->_v_node[i];
            Node* pnd = _nt->_v_parent[i];
            double dv = NODEV(pnd) - NODEV(nd);
            /* our connection coefficients are negative so */
            NODERHS(nd) -= NODEB(nd) * dv;
            NODERHS(pnd) += NODEA(nd) * dv;
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

void nrn_lhs(NrnThread* _nt) {
    int i, i1, i2, i3;
    NrnThreadMembList* tml;

    i1 = 0;
    i2 = i1 + _nt->ncell;
    i3 = _nt->end;

    if (diam_changed) {
        nrn_thread_error("need recalc_diam()");
    }

    if (use_sparse13) {
        int i, neqn;
        neqn = spGetSize(_nt->_sp13mat, 0);
        spClear(_nt->_sp13mat);
    } else {
#if CACHEVEC
        if (use_cachevec) {
            for (i = i1; i < i3; ++i) {
                VEC_D(i) = 0.;
            }
        } else
#endif /* CACHEVEC */
        {
            for (i = i1; i < i3; ++i) {
                NODED(_nt->_v_node[i]) = 0.;
            }
        }
    }

    if (_nt->_nrn_fast_imem) {
        for (i = i1; i < i3; ++i) {
            _nt->_nrn_fast_imem->_nrn_sav_d[i] = 0.;
        }
    }

    /* note that CAP has no jacob */
    for (tml = _nt->tml; tml; tml = tml->next)
        if (memb_func[tml->index].jacob) {
            Pvmi s = memb_func[tml->index].jacob;
            std::string mechname("cur-");
            mechname += memb_func[tml->index].sym->name;
            nrn::Instrumentor::phase_begin(mechname.c_str());
            (*s)(_nt, tml->ml, tml->index);
            nrn::Instrumentor::phase_end(mechname.c_str());
            if (errno) {
                if (nrn_errno_check(tml->index)) {
                    hoc_warning("errno set during calculation of jacobian", (char*) 0);
                }
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

    activsynapse_lhs();


    if (_nt->_nrn_fast_imem) {
        /* _nrn_save_d has only the contribution of electrode current
           so here we transform so it only has membrane current contribution
        */
        double* p = _nt->_nrn_fast_imem->_nrn_sav_d;
        if (use_sparse13) {
            for (i = i1; i < i3; ++i) {
                Node* nd = _nt->_v_node[i];
                p[i] += NODED(nd);
            }
        } else if (use_cachevec) {
            for (i = i1; i < i3; ++i) {
                p[i] += VEC_D(i);
            }
        } else {
            for (i = i1; i < i3; ++i) {
                Node* nd = _nt->_v_node[i];
                p[i] += NODED(nd);
            }
        }
    }
#if EXTRACELLULAR
    /* nde->_d[0] contains the -ELECTRODE_CURRENT contribution to nd->_d */
    nrn_setup_ext(_nt);
#endif
    if (use_sparse13) {
        /* must be after nrn_setup_ext so that whatever is put in
        nd->_d does not get added to nde->d */
        nrndae_lhs();
    }

    activclamp_lhs();

    /* at this point d contains all the membrane conductances */


    /* now add the axial currents */

    if (use_sparse13) {
        for (i = i2; i < i3; ++i) {
            Node* nd = _nt->_v_node[i];
            *(nd->_a_matelm) += NODEA(nd);
            *(nd->_b_matelm) += NODEB(nd); /* b may have value from lincir */
            NODED(nd) -= NODEB(nd);
        }
        for (i = i2; i < i3; ++i) {
            NODED(_nt->_v_parent[i]) -= NODEA(_nt->_v_node[i]);
        }
    } else {
#if CACHEVEC
        if (use_cachevec) {
            for (i = i2; i < i3; ++i) {
                VEC_D(i) -= VEC_B(i);
                VEC_D(_nt->_v_parent_index[i]) -= VEC_A(i);
            }
        } else
#endif /* CACHEVEC */
        {
            for (i = i2; i < i3; ++i) {
                NODED(_nt->_v_node[i]) -= NODEB(_nt->_v_node[i]);
                NODED(_nt->_v_parent[i]) -= NODEA(_nt->_v_node[i]);
            }
        }
    }
}

/* for the fixed step method */
void* setup_tree_matrix(NrnThread* _nt) {
    nrn::Instrumentor::phase p_setup_tree_matrix("setup-tree-matrix");
    nrn_rhs(_nt);
    nrn_lhs(_nt);
    nrn_nonvint_block_current(_nt->end, _nt->_actual_rhs, _nt->id);
    nrn_nonvint_block_conductance(_nt->end, _nt->_actual_d, _nt->id);
    return nullptr;
}

/* membrane mechanisms needed by other mechanisms (such as Eion by HH)
may require that the needed mechanism appear before it in the list.
(because ina must be initialized before it is incremented by HH)
With current implementation, a chain of "needs" may not be in list order.
*/
static Prop** current_prop_list;  /* the one prop_alloc is working on
                     when need_memb is called */
static int disallow_needmemb = 0; /* point processes cannot use need_memb
    when inserted at locations 0 or 1 */

Section* nrn_pnt_sec_for_need_;

extern Prop* prop_alloc(Prop**, int, Node*);


Prop* need_memb(Symbol* sym) {
    int type;
    Prop *mprev, *m;
    if (disallow_needmemb) {
        fprintf(stderr,
                "You can not locate a point process at\n\
 position 0 or 1 if it needs an ion\n");
        hoc_execerror(sym->name, "can't be inserted in this node");
    }
    type = sym->subtype;
    mprev = (Prop*) 0; /* may need to relink m */
    for (m = *current_prop_list; m; mprev = m, m = m->next) {
        if (m->_type == type) {
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
    } else if (nrn_pnt_sec_for_need_) {
        /* The caller was a POINT_PROCESS and we need to make sure
        that all segments of this section have the ion in order to
        prevent the possibility of multiple instances of this ion
        if a density mechanism that needs it is subsequently inserted
        or if the ion mechanism itself is inserted. Any earlier
        insertions of the latter or locating this kind of POINT_PROCESS
        in this section will mean that we no longer get to this arm
        of the if statement because m above is not nil.
        */
        Section* sec = nrn_pnt_sec_for_need_;
        Prop** cpl = current_prop_list;
        nrn_pnt_sec_for_need_ = (Section*) 0;
        mech_insert1(sec, type);
        current_prop_list = cpl;
        m = need_memb(sym);
    } else {
        m = prop_alloc(current_prop_list, type, (Node*) 0);
    }
    return m;
}


Node* nrn_alloc_node_; /* needed by models that use area */

Prop* prop_alloc(Prop** pp, int type, Node* nd) {
    /* link in new property at head of list */
    /* returning *Prop because allocation may */
    /* cause other properties to be linked ahead */
    /* some models need the node (to find area) */
    Prop* p;

    if (nd) {
        nrn_alloc_node_ = nd;
    }
    v_structure_change = 1;
    current_prop_list = pp;
    p = (Prop*) emalloc(sizeof(Prop));
    p->_type = type;
    p->next = *pp;
    p->ob = nullptr;
    p->_alloc_seq = -1;
    *pp = p;
    assert(memb_func[type].alloc);
    p->dparam = (Datum*) 0;
    p->param = nullptr;
    p->param_size = 0;
    (memb_func[type].alloc)(p);
    return p;
}

Prop* prop_alloc_disallow(Prop** pp, short type, Node* nd) {
    Prop* p;
    disallow_needmemb = 1;
    p = prop_alloc(pp, type, nd);
    disallow_needmemb = 0;
    return p;
}

void prop_free(Prop** pp) /* free an entire property list */
{
    Prop *p, *pn;
    p = *pp;
    *pp = (Prop*) 0;
    while (p) {
        pn = p->next;
        single_prop_free(p);
        p = pn;
    }
}

void single_prop_free(Prop* p) {
    extern char* pnt_map;
    v_structure_change = 1;
    if (pnt_map[p->_type]) {
        clear_point_process_struct(p);
        return;
    }
    if (p->param) {
        notify_freed_val_array(p->param, p->param_size);
        nrn_prop_data_free(p->_type, p->param);
    }
    if (p->dparam) {
        if (p->_type == CABLESECTION) {
            notify_freed_val_array(&p->dparam[2].val, 6);
        }
        nrn_prop_datum_free(p->_type, p->dparam);
    }
    if (p->ob) {
        hoc_obj_unref(p->ob);
    }
    free((char*) p);
}


/* For now there is always one more node in a section than there are
segments */
/* except in section 0 in which all nodes serve as x=0 to connecting
sections */

#undef PI
#define PI 3.14159265358979323846

static double diam_from_list(Section* sec, int inode, Prop* p, double rparent);

int recalc_diam_count_, nrn_area_ri_nocount_, nrn_area_ri_count_;
void nrn_area_ri(Section* sec) {
    int j;
    double ra, dx, diam, rright, rleft;
    Prop* p;
    Node* nd;
    if (nrn_area_ri_nocount_ == 0) {
        ++nrn_area_ri_count_;
    }
#if DIAMLIST
    if (sec->npt3d) {
        sec->prop->dparam[2].val = sec->pt3d[sec->npt3d - 1].arc;
    }
#endif
    ra = nrn_ra(sec);
    dx = section_length(sec) / ((double) (sec->nnode - 1));
    rright = 0.;
    for (j = 0; j < sec->nnode - 1; j++) {
        nd = sec->pnode[j];
        for (p = nd->prop; p; p = p->next) {
            if (p->_type == MORPHOLOGY) {
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
            rright = diam_from_list(sec, j, p, rright);
        } else
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
            NODEAREA(nd) = PI * diam * dx; /* um^2 */
            UPDATE_VEC_AREA(nd);
            rleft = 1e-2 * ra * (dx / 2) / (PI * diam * diam / 4.); /*left half segment Megohms*/
            NODERINV(nd) = 1. / (rleft + rright);                   /*uS*/
            rright = rleft;
        }
    }
    /* last segment has 0 length. area is 1e2
    in dimensionless units */
    NODEAREA(sec->pnode[j]) = 1.e2;
    UPDATE_VEC_AREA(sec->pnode[j]);
    NODERINV(sec->pnode[j]) = 1. / rright;
    sec->recalc_area_ = 0;
    diam_changed = 1;
}

Node* nrn_parent_node(Node* nd) {
    return nd->_classical_parent;
}

void connection_coef(void) /* setup a and b */
{
    int j;
    double dx, diam, area, ra;
    hoc_Item* qsec;
    Node* nd;
    Prop* p;
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
        hoc_warning("Don't forget to set Ra in every section", "eg. forall Ra=35.4");
    }
#endif
    ++recalc_diam_count_;
    nrn_area_ri_nocount_ = 1;
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
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
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
#if 1 /* unnecessary because they are unused, but help when looking at fmatrix */
        if (!sec->parentsec) {
            if (nrn_classicalNodeA(sec->parentnode)) {
                ClassicalNODEA(sec->parentnode) = 0.0;
            }
            if (nrn_classicalNodeB(sec->parentnode)) {
                ClassicalNODEB(sec->parentnode) = 0.0;
            }
        }
#endif
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
        ClassicalNODEA(nd) = -1.e2 * sec->prop->dparam[4].val * NODERINV(nd) / area;
        for (j = 1; j < sec->nnode; j++) {
            nd = sec->pnode[j];
            area = NODEAREA(sec->pnode[j - 1]);
            ClassicalNODEA(nd) = -1.e2 * NODERINV(nd) / area;
        }
    }
    /* now the effect of parent on node equation. */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        for (j = 0; j < sec->nnode; j++) {
            nd = sec->pnode[j];
            ClassicalNODEB(nd) = -1.e2 * NODERINV(nd) / NODEAREA(nd);
        }
    }
#if EXTRACELLULAR
    ext_con_coef();
#endif
}

extern "C" void nrn_shape_update_always(void) {
    static int updating;
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

extern "C" void nrn_shape_update(void) {
    if (section_list->next != section_list) {
        nrn_shape_update_always();
    }
}

static void nrn_matrix_node_alloc(void);

void recalc_diam(void) {
    v_setup_vectors();
    nrn_matrix_node_alloc();
    connection_coef();
    diam_changed = 0;
    ++diam_change_cnt;
    stim_prepare();
    synapse_prepare();
    clamp_prepare();
}


void area(void) { /* returns area (um^2) of segment containing x */
    double x;
    Section* sec;
    x = *getarg(1);
    if (x == 0. || x == 1.) {
        hoc_retpushx(0.);
    } else {
        sec = chk_access();
        if (sec->recalc_area_) {
            nrn_area_ri(sec);
        }
        hoc_retpushx(NODEAREA(sec->pnode[node_index(sec, x)]));
    }
}


void ri(void) { /* returns resistance (Mohm) between center of segment containing x
        and the center of the parent segment */
    double area;
    Node* np;
    np = node_ptr(chk_access(), *getarg(1), &area);
    if (NODERINV(np)) {
        hoc_retpushx(1. / NODERINV(np)); /* Megohms */
    } else {
        hoc_retpushx(1.e30);
    }
}


#if DIAMLIST

static int pt3dconst_;

void pt3dconst(void) {
    int i = pt3dconst_;
    if (ifarg(1)) {
        pt3dconst_ = (int) chkarg(1, 0., 1.);
    }
    hoc_retpushx((double) i);
}

void nrn_pt3dstyle0(Section* sec) {
    if (sec->logical_connection) {
        free(sec->logical_connection);
        sec->logical_connection = (Pt3d*) 0;
        ++nrn_shape_changed_;
        diam_changed = 1;
    }
}

void nrn_pt3dstyle1(Section* sec, double x, double y, double z) {
    Pt3d* p = sec->logical_connection;
    if (!p) {
        p = sec->logical_connection = (Pt3d*) ecalloc(1, sizeof(Pt3d));
    }
    p->x = x;
    p->y = y;
    p->z = z;
    ++nrn_shape_changed_;
    diam_changed = 1;
}

void pt3dstyle(void) {
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
        if ((int) chkarg(1, 0., 1.) == 1) {
            if (hoc_is_pdouble_arg(2)) {
                Pt3d* p = sec->logical_connection;
                if (p) {
                    double* px;
                    px = hoc_pgetarg(2);
                    *px = p->x;
                    px = hoc_pgetarg(3);
                    *px = p->y;
                    px = hoc_pgetarg(4);
                    *px = p->z;
                }
            } else {
                nrn_pt3dstyle1(sec, *getarg(2), *getarg(3), *getarg(4));
            }
        } else {
            nrn_pt3dstyle0(sec);
        }
    }
    hoc_retpushx((double) (sec->logical_connection ? 1 : 0));
}

void pt3dclear(void) { /*destroys space in current section for 3d points.*/
    Section* sec = chk_access();
    int req;
    if (ifarg(1)) {
        req = chkarg(1, 0., 30000.);
    } else {
        req = 0;
    }
    nrn_pt3dclear(sec, req);
    hoc_retpushx((double) sec->pt3d_bsize);
}

static void nrn_pt3dbufchk(Section* sec, int n) {
    if (n > sec->pt3d_bsize) {
        sec->pt3d_bsize = n;
        if ((sec->pt3d = (Pt3d*) hoc_Erealloc((char*) sec->pt3d, n * sizeof(Pt3d))) == (Pt3d*) 0) {
            sec->npt3d = 0;
            sec->pt3d_bsize = 0;
            hoc_malchk();
        }
    }
}

static void nrn_pt3dmodified(Section* sec, int i0) {
    int n, i;

    ++nrn_shape_changed_;
    diam_changed = 1;
    sec->recalc_area_ = 1;
    n = sec->npt3d;
#if NTS_SPINE
#else
    if (sec->pt3d[i0].d < 0.) {
        hoc_execerror("Diameter less than 0", (char*) 0);
    }
#endif
    if (i0 == 0) {
        sec->pt3d[0].arc = 0.;
        i0 = 1;
    }
    for (i = i0; i < n; ++i) {
        Pt3d* p = sec->pt3d + i - 1;
        double t1, t2, t3;
        t1 = sec->pt3d[i].x - p->x;
        t2 = sec->pt3d[i].y - p->y;
        t3 = sec->pt3d[i].z - p->z;
        sec->pt3d[i].arc = p->arc + sqrt(t1 * t1 + t2 * t2 + t3 * t3);
    }
    sec->prop->dparam[2].val = sec->pt3d[n - 1].arc;
}

void nrn_pt3dclear(Section* sec, int req) {
    ++nrn_shape_changed_;
    if (req != sec->pt3d_bsize) {
        if (sec->pt3d) {
            free((char*) (sec->pt3d));
            sec->pt3d = (Pt3d*) 0;
            sec->pt3d_bsize = 0;
        }
        if (req > 0) {
            sec->pt3d = (Pt3d*) ecalloc(req, sizeof(Pt3d));
            sec->pt3d_bsize = req;
        }
    }
    sec->npt3d = 0;
}


void nrn_pt3dinsert(Section* sec, int i0, double x, double y, double z, double d) {
    int i, n;
    n = sec->npt3d;
    nrn_pt3dbufchk(sec, n + 1);
    ++sec->npt3d;
    for (i = n - 1; i >= i0; --i) {
        Pt3d* p = sec->pt3d + i + 1;
        p->x = sec->pt3d[i].x;
        p->y = sec->pt3d[i].y;
        p->z = sec->pt3d[i].z;
        p->d = sec->pt3d[i].d;
    }
    sec->pt3d[i0].x = x;
    sec->pt3d[i0].y = y;
    sec->pt3d[i0].z = z;
    sec->pt3d[i0].d = d;
    nrn_pt3dmodified(sec, i0);
}

void pt3dinsert(void) {
    Section* sec;
    int i, n, i0;
    sec = chk_access();
    n = sec->npt3d;
    i0 = (int) chkarg(1, 0., (double) (n));
    nrn_pt3dinsert(sec, i0, *getarg(2), *getarg(3), *getarg(4), *getarg(5));
    hoc_retpushx(0.);
}

void nrn_pt3dchange1(Section* sec, int i, double d) {
    sec->pt3d[i].d = d;
    ++nrn_shape_changed_;
    diam_changed = 1;
    sec->recalc_area_ = 1;
}

void nrn_pt3dchange2(Section* sec, int i, double x, double y, double z, double diam) {
    sec->pt3d[i].x = x;
    sec->pt3d[i].y = y;
    sec->pt3d[i].z = z;
    sec->pt3d[i].d = diam;
    nrn_pt3dmodified(sec, i);
}

void pt3dchange(void) {
    int i, n;
    Section* sec = chk_access();
    n = sec->npt3d;
    i = (int) chkarg(1, 0., (double) (n - 1));
    if (ifarg(5)) {
        nrn_pt3dchange2(sec, i, *getarg(2), *getarg(3), *getarg(4), *getarg(5));
    } else {
        nrn_pt3dchange1(sec, i, *getarg(2));
    }
    hoc_retpushx(0.);
}

void nrn_pt3dremove(Section* sec, int i0) {
    int i, n;
    n = sec->npt3d;
    for (i = i0 + 1; i < n; ++i) {
        Pt3d* p = sec->pt3d + i - 1;
        p->x = sec->pt3d[i].x;
        p->y = sec->pt3d[i].y;
        p->z = sec->pt3d[i].z;
        p->d = sec->pt3d[i].d;
    }
    --sec->npt3d;
    nrn_pt3dmodified(sec, i0);
}

void pt3dremove(void) {
    int i, i0, n;
    Section* sec = chk_access();
    n = sec->npt3d;
    i0 = (int) chkarg(1, 0., (double) (n - 1));
    nrn_pt3dremove(sec, i0);

    hoc_retpushx(0.);
}

void nrn_diam_change(Section* sec) {
    if (!pt3dconst_ && sec->npt3d) { /* fill 3dpoints as though constant diam segments */
        int i;
        double x, L;
        L = section_length(sec);
        if (fabs(L - sec->pt3d[sec->npt3d - 1].arc) > .001) {
            nrn_length_change(sec, L);
        }
        for (i = 0; i < sec->npt3d; ++i) {
            x = sec->pt3d[i].arc / L;
            if (x > 1.0) {
                x = 1.0;
            }
            node_index(sec, x);
            sec->pt3d[i].d = nrn_diameter(sec->pnode[node_index(sec, x)]);
        }
        ++nrn_shape_changed_;
    }
}

void nrn_length_change(Section* sec, double d) {
    if (!pt3dconst_ && sec->npt3d) {
        int i;
        double x0, y0, z0;
        double fac;
        double l;
        x0 = sec->pt3d[0].x;
        y0 = sec->pt3d[0].y;
        z0 = sec->pt3d[0].z;
        l = sec->pt3d[sec->npt3d - 1].arc;
        fac = d / l;
        /*if (fac != 1) { printf("nrn_length_change from %g to %g\n", l, d);}*/
        for (i = 0; i < sec->npt3d; ++i) {
            sec->pt3d[i].arc = sec->pt3d[i].arc * fac;
            sec->pt3d[i].x = x0 + (sec->pt3d[i].x - x0) * fac;
            sec->pt3d[i].y = y0 + (sec->pt3d[i].y - y0) * fac;
            sec->pt3d[i].z = z0 + (sec->pt3d[i].z - z0) * fac;
        }
        ++nrn_shape_changed_;
    }
}

/*ARGSUSED*/
int can_change_morph(Section* sec) {
    return pt3dconst_ == 0;
}

static void stor_pt3d_vec(Section* sec, IvocVect* xv, IvocVect* yv, IvocVect* zv, IvocVect* dv) {
    int i;
    int n = vector_capacity(xv);
    double* x = vector_vec(xv);
    double* y = vector_vec(yv);
    double* z = vector_vec(zv);
    double* d = vector_vec(dv);
    nrn_pt3dbufchk(sec, n);
    sec->npt3d = n;
    for (i = 0; i < n; i++) {
        sec->pt3d[i].x = x[i];
        sec->pt3d[i].y = y[i];
        sec->pt3d[i].z = z[i];
        sec->pt3d[i].d = d[i];
    }
    nrn_pt3dmodified(sec, 0);
}

void pt3dadd(void) {
    /*pt3add(x,y,z, d) stores 3d point at end of current pt3d list.
      first point assumed to be at arc length position 0. Last point
      at 1. arc length increases monotonically.
    */
    if (hoc_is_object_arg(1)) {
        stor_pt3d_vec(chk_access(), vector_arg(1), vector_arg(2), vector_arg(3), vector_arg(4));
    } else {
        stor_pt3d(chk_access(), *getarg(1), *getarg(2), *getarg(3), *getarg(4));
    }
    hoc_retpushx(1.);
}


void n3d(void) { /* returns number of 3d points in section */
    Section* sec;
    sec = chk_access();
    hoc_retpushx((double) sec->npt3d);
}

void x3d(void) { /* returns x value at index of 3d list  */
    Section* sec;
    int n, i;
    sec = chk_access();
    n = sec->npt3d - 1;
    i = chkarg(1, 0., (double) n);
    hoc_retpushx((double) sec->pt3d[i].x);
}

void y3d(void) { /* returns x value at index of 3d list  */
    Section* sec;
    int n, i;
    sec = chk_access();
    n = sec->npt3d - 1;
    i = chkarg(1, 0., (double) n);
    hoc_retpushx((double) sec->pt3d[i].y);
}

void z3d(void) { /* returns x value at index of 3d list  */
    Section* sec;
    int n, i;
    sec = chk_access();
    n = sec->npt3d - 1;
    i = chkarg(1, 0., (double) n);
    hoc_retpushx((double) sec->pt3d[i].z);
}

void arc3d(void) { /* returns x value at index of 3d list  */
    Section* sec;
    int n, i;
    sec = chk_access();
    n = sec->npt3d - 1;
    i = chkarg(1, 0., (double) n);
    hoc_retpushx((double) sec->pt3d[i].arc);
}

void diam3d(void) { /* returns x value at index of 3d list  */
    Section* sec;
    double d;
    int n, i;
    sec = chk_access();
    n = sec->npt3d - 1;
    i = chkarg(1, 0., (double) n);
    d = (double) sec->pt3d[i].d;
    hoc_retpushx(fabs(d));
}

void spine3d(void) { /* returns x value at index of 3d list  */
    Section* sec;
    int n, i;
    double d;
    sec = chk_access();
    n = sec->npt3d - 1;
    i = chkarg(1, 0., (double) n);
    d = (double) sec->pt3d[i].d;
    if (d < 0) {
        hoc_retpushx(1.);
    } else {
        hoc_retpushx(0.);
    }
}

void stor_pt3d(Section* sec, double x, double y, double z, double d) {
    int n;

    n = sec->npt3d;
    nrn_pt3dbufchk(sec, n + 1);
    sec->npt3d++;
    sec->pt3d[n].x = x;
    sec->pt3d[n].y = y;
    sec->pt3d[n].z = z;
    sec->pt3d[n].d = d;
    nrn_pt3dmodified(sec, n);
}

static double spinearea = 0.;

void setSpineArea(void) {
    spinearea = *getarg(1);
    diam_changed = 1;
    hoc_retpushx(spinearea);
}

void getSpineArea(void) {
    hoc_retpushx(spinearea);
}

void define_shape(void) {
    nrn_define_shape();
    hoc_retpushx(1.);
}

static void nrn_translate_shape(Section* sec, float x, float y, float z) {
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
    } else {
        dx = x - sec->pt3d[0].x;
        dy = y - sec->pt3d[0].y;
        dz = z - sec->pt3d[0].z;
    }
    /*	if (dx*dx + dy*dy + dz*dz < 10.)*/
    for (i = 0; i < sec->npt3d; ++i) {
        sec->pt3d[i].x += dx;
        sec->pt3d[i].y += dy;
        sec->pt3d[i].z += dz;
    }
}

void nrn_define_shape(void) {
    static int changed_;
    int i, j;
    Section *sec, *psec, *ch;
    float x, y, z, dz, x1, y1;
    float nch, ich = 0.0, angle;
    double arc, len;
    if (changed_ == nrn_shape_changed_ && !diam_changed && !tree_changed) {
        return;
    }
    recalc_diam();
    dz = 100.;
    for (i = 0; i < section_count; ++i) {
        sec = secorder[i];
        arc = nrn_connection_position(sec);
        if ((psec = sec->parentsec) == (Section*) 0) {
            /* section has no parent */
            x = 0;
            y = 0;
            z = i * dz;
            x1 = 1;
            y1 = 0;
        } else {
            double arc1 = arc;
            x1 = psec->pt3d[psec->npt3d - 1].x - psec->pt3d[0].x;
            y1 = psec->pt3d[psec->npt3d - 1].y - psec->pt3d[0].y;
            if (!arc0at0(psec)) {
                arc1 = 1. - arc;
            }
            if (arc1 < .5) {
                x1 = -x1;
                y1 = -y1;
            }
            x = psec->pt3d[psec->npt3d - 1].x * arc1 + psec->pt3d[0].x * (1 - arc1);
            y = psec->pt3d[psec->npt3d - 1].y * arc1 + psec->pt3d[0].y * (1 - arc1);
            z = psec->pt3d[psec->npt3d - 1].z * arc1 + psec->pt3d[0].z * (1 - arc1);
        }
        if (sec->npt3d) {
            if (psec) {
                nrn_translate_shape(sec, x, y, z);
            }
            continue;
        }
        if (fabs(y1) < 1e-6 && fabs(x1) < 1e-6) {
            Printf("nrn_define_shape: %s first and last 3-d point at same (x,y)\n", secname(psec));
            angle = 0.;
        } else {
            angle = atan2(y1, x1);
        }
        if (arc > 0. && arc < 1.) {
            angle += 3.14159265358979323846 / 2.;
        }
        nch = 0.;
        if (psec)
            for (ch = psec->child; ch; ch = ch->sibling) {
                if (ch == sec) {
                    ich = nch;
                }
                if (arc == nrn_connection_position(ch)) {
                    ++nch;
                }
            }
        if (nch > 1) {
            angle += ich / (nch - 1.) * .8 - .4;
        }
        len = section_length(sec);
        x1 = x + len * cos(angle);
        y1 = y + len * sin(angle);
        stor_pt3d(sec, x, y, z, nrn_diameter(sec->pnode[0]));
        for (j = 0; j < sec->nnode - 1; ++j) {
            double frac = ((double) j + .5) / (double) (sec->nnode - 1);
            stor_pt3d(sec,
                      x * (1 - frac) + x1 * frac,
                      y * (1 - frac) + y1 * frac,
                      z,
                      nrn_diameter(sec->pnode[j]));
        }
        stor_pt3d(sec, x1, y1, z, nrn_diameter(sec->pnode[sec->nnode - 2]));
        /* don't let above change length due to round-off errors*/
        sec->pt3d[sec->npt3d - 1].arc = len;
        sec->prop->dparam[2].val = len;
    }
    changed_ = nrn_shape_changed_;
}

static double diam_from_list(Section* sec, int inode, Prop* p, double rparent)
/* p->param[0] is diam of inode in sec.*/
/* rparent right half resistance of the parent segment*/
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
    double diam, delta, temp, ri, area, ra, rleft = 0.0;
    int npt, nspine;

    if (inode == 0) {
        j = 0;
        si = 0.;
        x1 = sec->pt3d[j].arc;
        y1 = fabs(sec->pt3d[j].d);
        ds = sec->pt3d[sec->npt3d - 1].arc / ((double) (sec->nnode - 1));
    }
    si = (double) inode * ds;
    npt = sec->npt3d;
    diam = 0.;
    area = 0.;
    nspine = 0;
    ra = nrn_ra(sec);
    for (ihalf = 0; ihalf < 2; ihalf++) {
        ri = 0.;
        sip = si + ds / 2.;
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
                } else {
                    frac = (sip - xj) / (xjp - xj);
                }
                x2 = sip;
                y2 = (1. - frac) * fabs(sec->pt3d[j].d) + frac * fabs(sec->pt3d[jp].d);
                jnext = j;
            } else {
                x2 = xjp;
                y2 = fabs(sec->pt3d[jp].d);
                jnext = jp;
            }

            /* f += integrate(x1,y1, x2, y2) */
            delta = (x2 - x1);
            diam += (y2 + y1) * delta;
            if (delta < 1e-15) {
                delta = 1e-15;
            }
            if ((temp = y2 * y1 / delta) == 0) {
                temp = 1e-15;
            }
            ri += 1 / temp;
#if 1 /*taper is very seldom taken into account*/
            temp = .5 * (y2 - y1);
            temp = sqrt(delta * delta + temp * temp);
#else
            temp = delta;
#endif

            area += (y2 + y1) * temp;
            /* end of integration */

            x1 = x2;
            y1 = y2;
            if (j == jnext) {
                break;
            }
            j = jnext;
        }
        if (ihalf == 0) {
            rleft = ri * ra / PI * (4. * .01); /*left seg resistance*/
        } else {
            ri = ri * ra / PI * (4. * .01); /* MegOhms */
                                            /* above is right half segment resistance */
        }
        si = sip;
    }
    /* answer for inode is here */
    NODERINV(sec->pnode[inode]) = 1. / (rparent + rleft);
    diam *= .5 / ds;
    if (fabs(diam - p->param[0]) > 1e-9 || diam < 1e-5) {
        p->param[0] = diam; /* microns */
    }
    NODEAREA(sec->pnode[inode]) = area * .5 * PI; /* microns^2 */
    UPDATE_VEC_AREA(sec->pnode[inode]);
#if NTS_SPINE
    /* if last point has a spine then increment spine count for last node */
    if (inode == sec->nnode - 2 && sec->pt3d[npt - 1].d < 0.) {
        nspine += 1;
    }
    NODEAREA(sec->pnode[inode]) += nspine * spinearea;
    UPDATE_VEC_AREA(sec->pnode[inode]);
#endif
    return ri;
}

#endif /*DIAMLIST*/

void v_setup_vectors(void) {
    int inode, i;
    int isec;
    Section* sec;
    Node* nd;
    Prop* p;
    NrnThread* _nt;

    if (tree_changed) {
        setup_topology(); /* now classical secorder */
        v_structure_change = 1;
        ++nrn_shape_changed_;
    }
    /* get rid of the old */
    if (!v_structure_change) {
        return;
    }

    nrn_threads_free();

    for (i = 0; i < n_memb_func; ++i)
        if (nrn_is_artificial_[i] && memb_func[i].initialize) {
            if (memb_list[i].nodecount) {
                memb_list[i].nodecount = 0;
                free(memb_list[i].nodelist);
#if CACHEVEC
                free((void*) memb_list[i].nodeindices);
#endif /* CACHEVEC */
                if (memb_func[i].hoc_mech) {
                    free(memb_list[i].prop);
                } else {
                    free(memb_list[i]._data);
                    free(memb_list[i].pdata);
                }
            }
        }

#if 1 /* see finitialize */
    /* and count the artificial cells */
    for (i = 0; i < n_memb_func; ++i)
        if (nrn_is_artificial_[i] && memb_func[i].initialize) {
            cTemplate* tmp = nrn_pnt_template_[i];
            memb_list[i].nodecount = tmp->count;
        }
#endif

    /* allocate it*/

    for (i = 0; i < n_memb_func; ++i)
        if (nrn_is_artificial_[i] && memb_func[i].initialize) {
            if (memb_list[i].nodecount) {
                memb_list[i].nodelist = (Node**) emalloc(memb_list[i].nodecount * sizeof(Node*));
#if CACHEVEC
                memb_list[i].nodeindices = (int*) emalloc(memb_list[i].nodecount * sizeof(int));
#endif /* CACHEVEC */
                if (memb_func[i].hoc_mech) {
                    memb_list[i].prop = (Prop**) emalloc(memb_list[i].nodecount * sizeof(Prop*));
                } else {
                    memb_list[i]._data = (double**) emalloc(memb_list[i].nodecount *
                                                            sizeof(double*));
                    memb_list[i].pdata = (Datum**) emalloc(memb_list[i].nodecount * sizeof(Datum*));
                }
                memb_list[i].nodecount = 0; /* counted again below */
            }
        }

#if MULTICORE
    if (!nrn_user_partition()) {
        /* does not depend on memb_list */
        int ith, j;
        NrnThread* _nt;
        section_order(); /* could be already reordered */
        /* round robin distribution */
        for (ith = 0; ith < nrn_nthread; ++ith) {
            _nt = nrn_threads + ith;
            _nt->roots = hoc_l_newlist();
            _nt->ncell = 0;
        }
        j = 0;
        for (ith = 0; ith < nrn_nthread; ++ith) {
            _nt = nrn_threads + ith;
            for (i = ith; i < nrn_global_ncell; i += nrn_nthread) {
                hoc_l_lappendsec(_nt->roots, secorder[j]);
                ++_nt->ncell;
                ++j;
            }
        }
    }
    /* reorder. also fill NrnThread node indices, v_node, and v_parent */
    reorder_secorder();
#endif

#if CACHEVEC
    FOR_THREADS(_nt) {
        for (inode = 0; inode < _nt->end; inode++) {
            if (_nt->_v_parent[inode] != NULL) {
                _nt->_v_parent_index[inode] = _nt->_v_parent[inode]->v_node_index;
            }
        }
    }

#endif /* CACHEVEC */

    nrn_thread_memblist_setup();

    /* fill in artificial cell info */
    for (i = 0; i < n_memb_func; ++i) {
        if (nrn_is_artificial_[i] && memb_func[i].initialize) {
            hoc_Item* q;
            hoc_List* list;
            int j, nti;
            cTemplate* tmp = nrn_pnt_template_[i];
            memb_list[i].nodecount = tmp->count;
            nti = 0;
            j = 0;
            list = tmp->olist;
            ITERATE(q, list) {
                Object* obj = OBJ(q);
                auto* pnt = static_cast<Point_process*>(obj->u.this_pointer);
                p = pnt->prop;
                memb_list[i].nodelist[j] = nullptr;
                memb_list[i]._data[j] = p->param;
                memb_list[i].pdata[j] = p->dparam;
                /* for now, round robin all the artificial cells */
                /* but put the non-threadsafe ones in thread 0 */
                /*
                 Note that artficial cells are assumed not to need a thread specific
                 Memb_list. But this implies that they do not have thread specific
                 data. For this reason, for now, an otherwise thread-safe artificial
                 cell model is declared by nmodl as thread-unsafe.
                */
                NrnThread *nt{};
                if (memb_func[i].vectorized == 0) {
                    nt = nrn_threads;
                } else {
                    nt = nrn_threads + nti;
                    nti = (nti + 1) % nrn_nthread;
                }
                pnt->_vnt = nt;
                pnt->_i_instance = j;
                // Multiple NrnThreads have _ml_list[i] pointing at the same
                // Memb_list*
                nt->_ml_list[i] = std::next(memb_list, i);
                ++j;
            }
        }
    }
    nrn_recalc_node_ptrs();
    // The cache might contain pointers to data that were just reallocated.
    neuron::cache::invalidate();
    v_structure_change = 0;
    nrn_update_ps2nt();
    ++structure_change_cnt;
    long_difus_solve(3, nrn_threads);
    nrn_nonvint_block_setup();
    diam_changed = 1;
}


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

void node_data_scaffolding(void) {
    int i;
    Pd(n_memb_func);
    /*	P "Mechanism names (first two are nil) beginning with memb_func[2]\n");*/
    for (i = 2; i < n_memb_func; ++i) {
        P "%s", memb_func[i].sym->name);
        Pn;
    }
}

void node_data_structure(void) {
    int i, j;
    nrn_thread_error("node_data_structure");
    Pd(v_node_count);

    Pd(nrn_global_ncell);
    /*	P "Indices of node parents\n");*/
    for (i = 0; i < v_node_count; ++i) {
        Pd(v_parent[i]->v_node_index);
    }
    /*	P "node lists for the membrane mechanisms\n");*/
    for (i = 2; i < n_memb_func; ++i) {
        /*		P "count, node list for mechanism %s\n", memb_func[i].sym->name);*/
        Pd(memb_list[i].nodecount);
        for (j = 0; j < memb_list[i].nodecount; ++j) {
            Pd(memb_list[i].nodelist[j]->v_node_index);
        }
    }
}

void node_data_values(void) {
    int i, j, k;
    /*	P "data for nodes then for all mechanisms in order of the above structure\n");	*/
    for (i = 0; i < v_node_count; ++i) {
        Pg(NODEV(v_node[i]));
        Pg(NODEA(v_node[i]));
        Pg(NODEB(v_node[i]));
        Pg(NODEAREA(v_node[i]));
    }
    for (i = 2; i < n_memb_func; ++i) {
        Prop* prop;
        int cnt;
        double* pd;
        if (memb_list[i].nodecount) {
            assert(!memb_func[i].hoc_mech);
            prop = nrn_mechanism(i, memb_list[i].nodelist[0]);
            cnt = prop->param_size;
            Pd(cnt);
        }
        for (j = 0; j < memb_list[i].nodecount; ++j) {
            pd = memb_list[i]._data[j];
            for (k = 0; k < cnt; ++k) {
                Pg(pd[k]);
            }
        }
    }
}

void node_data(void) {
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
    hoc_retpushx(1.);
}

#else
void node_data(void) {
    Printf("recalc_diam=%d nrn_area_ri=%d\n", recalc_diam_count_, nrn_area_ri_count_);
    hoc_retpushx(0.);
}

#endif

void nrn_complain(double* pp) {
    /* print location for this param on the standard error */
    Node* nd;
    hoc_Item* qsec;
    int j;
    Prop* p;
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        for (j = 0; j < sec->nnode; ++j) {
            nd = sec->pnode[j];
            for (p = nd->prop; p; p = p->next) {
                if (p->param == pp) {
                    fprintf(stderr,
                            "Error at section location %s(%g)\n",
                            secname(sec),
                            nrn_arc_position(sec, nd));
                    return;
                }
            }
        }
    }
    fprintf(stderr, "Don't know the location of params at %p\n", pp);
}

void nrn_matrix_node_free() {
    NrnThread* nt;
    FOR_THREADS(nt) {
        if (nt->_actual_rhs) {
            free(nt->_actual_rhs);
            nt->_actual_rhs = (double*) 0;
        }
        if (nt->_actual_d) {
            free(nt->_actual_d);
            nt->_actual_d = (double*) 0;
        }
#if CACHEVEC
        if (nt->_actual_a) {
            free(nt->_actual_a);
            nt->_actual_a = (double*) 0;
        }
        if (nt->_actual_b) {
            free(nt->_actual_b);
            nt->_actual_b = (double*) 0;
        }
        /* because actual_v and actual_area have pointers to them from many
           places, defer the freeing until nrn_recalc_node_ptrs is called
        */
#endif /* CACHEVEC */
        if (nt->_sp13mat) {
            spDestroy(nt->_sp13mat);
            nt->_sp13mat = (char*) 0;
        }
    }
    diam_changed = 1;
}

/* 0 means no model, 1 means ODE, 2 means DAE */
int nrn_modeltype(void) {
    NrnThread* nt;
    static cTemplate* lm = (cTemplate*) 0;
    int type;
    v_setup_vectors();

    if (!nrndae_list_is_empty()) {
        return 2;
    }

    type = 0;
    if (nrn_global_ncell > 0) {
        type = 1;
        FOR_THREADS(nt) if (nt->_ecell_memb_list) {
            type = 2;
        }
    }
    if (type == 0 && nrn_nonvint_block_ode_count(0, 0)) {
        type = 1;
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

int nrn_method_consistent(void) {
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
    } else if (type == 2 && use_sparse13 == 0) {
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

static void nrn_matrix_node_alloc(void) {
    int i, b;
    Node* nd;
    NrnThread* nt;

    nrn_method_consistent();
    nt = nrn_threads;
    /*printf("use_sparse13=%d sp13mat=%lx rhs=%lx\n", use_sparse13, (long)nt->_sp13mat,
     * (long)nt->_actual_rhs);*/
    if (use_sparse13) {
        if (nt->_sp13mat) {
            return;
        } else {
            nrn_matrix_node_free();
        }
    } else {
        if (nt->_sp13mat) {
            v_structure_change = 1;
            v_setup_vectors();
            return;
        } else {
            if (nt->_actual_rhs != (double*) 0) {
                return;
            }
        }
    }
/*printf("nrn_matrix_node_alloc does its work\n");*/
#if CACHEVEC
    FOR_THREADS(nt) {
        nt->_actual_a = (double*) ecalloc(nt->end, sizeof(double));
        nt->_actual_b = (double*) ecalloc(nt->end, sizeof(double));
    }
    nrn_recalc_node_ptrs();
#endif /* CACHEVEC */

#if 0
printf("nrn_matrix_node_alloc use_sparse13=%d cvode_active_=%d nrn_use_daspk_=%d\n", use_sparse13, cvode_active_, nrn_use_daspk_);
#endif
    ++nrn_matrix_cnt_;
    if (use_sparse13) {
        int in, err, extn, neqn, j;
        nt = nrn_threads;
        neqn = nt->end + nrndae_extra_eqn_count();
        extn = 0;
        if (nt->_ecell_memb_list) {
            extn = nt->_ecell_memb_list->nodecount * nlayer;
        }
        /*printf(" %d extracellular nodes\n", extn);*/
        neqn += extn;
        nt->_actual_rhs = (double*) ecalloc(neqn + 1, sizeof(double));
        nt->_sp13mat = spCreate(neqn, 0, &err);
        if (err != spOKAY) {
            hoc_execerror("Couldn't create sparse matrix", (char*) 0);
        }
        for (in = 0, i = 1; in < nt->end; ++in, ++i) {
            nt->_v_node[in]->eqn_index_ = i;
            if (nt->_v_node[in]->extnode) {
                i += nlayer;
            }
        }
        for (in = 0; in < nt->end; ++in) {
            int ie, k;
            Node *nd, *pnd;
            Extnode* nde;
            nd = nt->_v_node[in];
            nde = nd->extnode;
            pnd = nt->_v_parent[in];
            i = nd->eqn_index_;
            nd->_rhs = nt->_actual_rhs + i;
            nd->_d = spGetElement(nt->_sp13mat, i, i);
            if (nde) {
                for (ie = 0; ie < nlayer; ++ie) {
                    k = i + ie + 1;
                    nde->_d[ie] = spGetElement(nt->_sp13mat, k, k);
                    nde->_rhs[ie] = nt->_actual_rhs + k;
                    nde->_x21[ie] = spGetElement(nt->_sp13mat, k, k - 1);
                    nde->_x12[ie] = spGetElement(nt->_sp13mat, k - 1, k);
                }
            }
            if (pnd) {
                j = pnd->eqn_index_;
                nd->_a_matelm = spGetElement(nt->_sp13mat, j, i);
                nd->_b_matelm = spGetElement(nt->_sp13mat, i, j);
                if (nde && pnd->extnode)
                    for (ie = 0; ie < nlayer; ++ie) {
                        int kp = j + ie + 1;
                        k = i + ie + 1;
                        nde->_a_matelm[ie] = spGetElement(nt->_sp13mat, kp, k);
                        nde->_b_matelm[ie] = spGetElement(nt->_sp13mat, k, kp);
                    }
            } else { /* not needed if index starts at 1 */
                nd->_a_matelm = (double*) 0;
                nd->_b_matelm = (double*) 0;
            }
        }
        nrndae_alloc();
    } else {
        FOR_THREADS(nt) {
            assert(nrndae_extra_eqn_count() == 0);
            assert(!nt->_ecell_memb_list || nt->_ecell_memb_list->nodecount == 0);
            nt->_actual_d = (double*) ecalloc(nt->end, sizeof(double));
            nt->_actual_rhs = (double*) ecalloc(nt->end, sizeof(double));
            for (i = 0; i < nt->end; ++i) {
                Node* nd = nt->_v_node[i];
                nd->_d = nt->_actual_d + i;
                nd->_rhs = nt->_actual_rhs + i;
            }
        }
    }
}

void nrn_cachevec(int b) {
    if (use_sparse13) {
        use_cachevec = 0;
    } else {
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

static int n_recalc_ptr_callback;
static void (*recalc_ptr_callback[20])();
static int recalc_cnt_;
// static double **recalc_ptr_new_vp_, **recalc_ptr_old_vp_;
static int n_old_thread_;
// static int* old_actual_v_size_;
// static double** old_actual_v_;
static double** old_actual_area_;

/* defer freeing a few things which may have pointers to them
until ready to update those pointers */
void nrn_old_thread_save(void) {
    int i;
    int n = nrn_nthread;
    if (old_actual_area_) {
        return;
    } /* one is already outstanding */
    n_old_thread_ = n;
    // old_actual_v_size_ = (int*) ecalloc(n, sizeof(int));
    // old_actual_v_ = (double**) ecalloc(n, sizeof(double*));
    old_actual_area_ = (double**) ecalloc(n, sizeof(double*));
    for (i = 0; i < n; ++i) {
        NrnThread* nt = nrn_threads + i;
        // old_actual_v_size_[i] = nt->end;
        // old_actual_v_[i] = nt->_actual_v;
        old_actual_area_[i] = nt->_actual_area;
    }
}

static double* (*recalc_ptr_)(double*);

double* nrn_recalc_ptr(double* old) {
    if (recalc_ptr_) {
        return (*recalc_ptr_)(old);
    } else {
        return old;
    }
}

void nrn_register_recalc_ptr_callback(Pfrv f) {
    if (n_recalc_ptr_callback >= 20) {
        Printf("More than 20 recalc_ptr_callback functions\n");
        exit(1);
    }
    recalc_ptr_callback[n_recalc_ptr_callback++] = f;
}

void nrn_recalc_ptrs(double* (*r)(double*) ) {
    int i;

    recalc_ptr_ = r;

    /* update pointers managed by c++ */
    nrniv_recalc_ptrs();

    /* user callbacks to update pointers */
    for (i = 0; i < n_recalc_ptr_callback; ++i) {
        (*recalc_ptr_callback[i])();
    }
    recalc_ptr_ = nullptr;
}

/** @brief Sort the underlying storage for Nodes is sorted.
 *
 *  After model building is complete the storage vectors backing all Node
 *  objects can be permuted to ensure that preconditions are met for the
 *  computations performed while time-stepping.
 *
 *  This method ensures that the Node data is ready for this compute phase.
 *
 *  @todo Is it worth "freezing" the data before any of the current_row() calls?
 */
static neuron::model_sorted_token nrn_sort_node_data() {
    // Make sure the voltage storage follows the order encoded in _v_node.
    // Generate the permutation vector to update the underlying storage for
    // Nodes. This must come after nrn_multisplit_setup_, which can change the
    // Node order.
    auto& node_data = neuron::model().node_data();
    std::size_t const global_node_data_size{node_data.size()};
    std::size_t global_node_data_offset{}, global_i{};
    std::vector<std::size_t> global_node_data_permutation(global_node_data_size);
    // Process threads one at a time -- this means that the data for each
    // NrnThread will be contiguous.
    NrnThread* nt{};
    FOR_THREADS(nt) {
        // What offset in the global node data structure do the values for this thread
        // start at
        nt->_node_data_offset = global_i;
        for (int i = 0; i < nt->end; ++i, ++global_i) {
            Node* nd = nt->_v_node[i];
            auto const current_node_row = nd->_node_handle.id().current_row();
            assert(current_node_row < global_node_data_size);
            assert(global_i < global_node_data_size);
            global_node_data_permutation.at(global_i) = current_node_row;
        }
    }
    assert(global_i == global_node_data_size);
    // Should this and other permuting operations return a "sorted token"?
    node_data.apply_permutation(global_node_data_permutation);
    return node_data.get_sorted_token();
}

/** @brief Ensure neuron::container::* data are sorted.
 *
 *  Set the containers to be in read-only mode, until the returned token is
 *  destroyed.
 *
 *  So far this only affects Node voltages, which are use backing storage in
 *  neuron::model().node_data().
 */
neuron::model_sorted_token nrn_ensure_model_data_are_sorted() {
    auto& node_data = neuron::model().node_data();
    if (node_data.is_sorted()) {
        // Short-circuit
        return node_data.get_sorted_token();
    } else {
        return nrn_sort_node_data();
    }
}

void nrn_recalc_node_ptrs() {
    int i, ii, j, k;
    NrnThread* nt;
    if (use_cachevec == 0) {
        return;
    }
    /*printf("nrn_recalc_node_ptrs\n");*/
    recalc_cnt_ = 0;
    FOR_THREADS(nt) {
        recalc_cnt_ += nt->end;
    }
    // recalc_ptr_new_vp_ = (double**) ecalloc(recalc_cnt_, sizeof(double*));
    // recalc_ptr_old_vp_ = (double**) ecalloc(recalc_cnt_, sizeof(double*));

    /* first update the pointers without messing with the old NODEV,NODEAREA */
    /* to prepare for the update, copy all the v and area values into the */
    /* new arrays are replace the old values by index value. */
    /* a pointer dereference value of i allows us to easily check */
    /* if the pointer points to what v_node[i]->_v points to. */
    ii = 0;
    FOR_THREADS(nt) {
        // nt->_actual_v = (double*) ecalloc(nt->end, sizeof(double));
        nt->_actual_area = (double*) ecalloc(nt->end, sizeof(double));
    }
    FOR_THREADS(nt) for (i = 0; i < nt->end; ++i) {
        Node* nd = nt->_v_node[i];
        nt->_actual_area[i] = nd->_area;
        ++ii;
    }
    /* update POINT_PROCESS pointers to NODEAREA */
    /* and relevant POINTER pointers to NODEV */
    FOR_THREADS(nt) for (i = 0; i < nt->end; ++i) {
        Node* nd = nt->_v_node[i];
        for (Prop* p = nd->prop; p; p = p->next) {
            if (memb_func[p->_type].is_point && !nrn_is_artificial_[p->_type]) {
                nrn_set_pval(p->dparam[0], nt->_actual_area + i);
            }
        }
    }

    nrn_recalc_ptrs(nullptr);

    /* and free the old thread arrays if new ones were allocated */
    for (i = 0; i < n_old_thread_; ++i) {
        if (old_actual_area_[i])
            free(old_actual_area_[i]);
    }
    free(old_actual_area_);
    old_actual_area_ = 0;
    n_old_thread_ = 0;

    nrn_node_ptr_change_cnt_++;
    nrn_cache_prop_realloc();
    nrn_recalc_ptrvector();
    nrn_partrans_update_ptrs();
    nrn_imem_defer_free(nullptr);
}

#endif /* CACHEVEC */
