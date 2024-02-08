#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/treeset.cpp,v 1.39 1999/07/08 14:25:07 hines Exp */

#include "cvodeobj.h"
#include "membfunc.h"
#include "multisplit.h"
#include "nrn_ansi.h"
#include "neuron.h"
#include "neuron/cache/mechanism_range.hpp"
#include "neuron/cache/model_data.hpp"
#include "neuron/container/soa_container.hpp"
#include "nonvintblock.h"
#include "nrndae_c.h"
#include "nrniv_mf.h"
#include "nrnmpi.h"
#include "ocnotify.h"
#include "partrans.h"
#include "section.h"
#include "spmatrix.h"
#include "utils/profile/profiler_interface.h"
#include "multicore.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <algorithm>
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

#if 1 || NRNMPI
void (*nrn_multisplit_setup_)();
#endif

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

/*
 * Update actual_rhs based on _sp13_rhs used for sparse13 solver
 */
void update_actual_rhs_based_on_sp13_rhs(NrnThread* nt) {
    for (int i = 0; i < nt->end; i++) {
        nt->actual_rhs(i) = nt->_sp13_rhs[nt->_v_node[i]->eqn_index_];
    }
}

/*
 * Update _sp13_rhs used for sparse13 solver based on changes on actual_rhs
 */
void update_sp13_rhs_based_on_actual_rhs(NrnThread* nt) {
    for (int i = 0; i < nt->end; i++) {
        nt->_sp13_rhs[nt->_v_node[i]->eqn_index_] = nt->actual_rhs(i);
    }
}

/*
 * Update the SoA storage for node matrix diagonals from the sparse13 matrix.
 */
void update_actual_d_based_on_sp13_mat(NrnThread* nt) {
    for (int i = 0; i < nt->end; ++i) {
        nt->actual_d(i) = *nt->_v_node[i]->_d_matelm;
    }
}

/*
 * Update the SoA storage for node matrix diagonals from the sparse13 matrix.
 */
void update_sp13_mat_based_on_actual_d(NrnThread* nt) {
    for (int i = 0; i < nt->end; ++i) {
        *nt->_v_node[i]->_d_matelm = nt->actual_d(i);
    }
}

/* calculate right hand side of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
This is a common operation for fixed step, cvode, and daspk methods
*/

void nrn_rhs(neuron::model_sorted_token const& cache_token, NrnThread& nt) {
    auto* const _nt = &nt;
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
    auto* const vec_rhs = nt.node_rhs_storage();
    if (use_sparse13) {
        int i, neqn;
        nrn_thread_error("nrn_rhs use_sparse13");
        neqn = spGetSize(_nt->_sp13mat, 0);
        for (i = 1; i <= neqn; ++i) {
            _nt->_sp13_rhs[i] = 0.;
        }
        for (i = i1; i < i3; ++i) {
            NODERHS(_nt->_v_node[i]) = 0.;
        }
    } else {
        for (i = i1; i < i3; ++i) {
            vec_rhs[i] = 0.;
        }
    }
    auto const vec_sav_rhs = _nt->node_sav_rhs_storage();
    if (vec_sav_rhs) {
        for (i = i1; i < i3; ++i) {
            vec_sav_rhs[i] = 0.;
        }
    }

    nrn_ba(cache_token, nt, BEFORE_BREAKPOINT);
    /* note that CAP has no current */
    for (tml = _nt->tml; tml; tml = tml->next)
        if (auto const current = memb_func[tml->index].current; current) {
            std::string mechname("cur-");
            mechname += memb_func[tml->index].sym->name;
            if (measure) {
                w = nrnmpi_wtime();
            }
            nrn::Instrumentor::phase_begin(mechname.c_str());
            current(cache_token, _nt, tml->ml, tml->index);
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

    if (vec_sav_rhs) {
        /* _nrn_save_rhs has only the contribution of electrode current
           so here we transform so it only has membrane current contribution
        */
        for (i = i1; i < i3; ++i) {
            vec_sav_rhs[i] -= vec_rhs[i];
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
        nrndae_rhs(_nt);
    }

    activstim_rhs();
    activclamp_rhs();
    /* now the internal axial currents.
    The extracellular mechanism contribution is already done.
        rhs += ai_j*(vi_j - vi)
    */
    auto* const vec_a = nt.node_a_storage();
    auto* const vec_b = nt.node_b_storage();
    auto* const vec_v = nt.node_voltage_storage();
    auto* const parent_i = nt._v_parent_index;
    for (i = i2; i < i3; ++i) {
        auto const pi = parent_i[i];
        auto const dv = vec_v[pi] - vec_v[i];
        // our connection coefficients are negative so
        vec_rhs[i] -= vec_b[i] * dv;
        vec_rhs[pi] += vec_a[i] * dv;
    }
}

/* calculate left hand side of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
with a matrix so that the solution is of the form [dvm+dvx,dvx] on the right
hand side after solving.
This is a common operation for fixed step, cvode, and daspk methods
*/

void nrn_lhs(neuron::model_sorted_token const& sorted_token, NrnThread& nt) {
    auto* const _nt = &nt;
    int i, i1, i2, i3;
    NrnThreadMembList* tml;

    i1 = 0;
    i2 = i1 + _nt->ncell;
    i3 = _nt->end;

    if (diam_changed) {
        nrn_thread_error("need recalc_diam()");
    }

    if (use_sparse13) {
        // Zero the sparse13 matrix
        spClear(_nt->_sp13mat);
    }

    // Make sure the SoA node diagonals are also zeroed (is this needed?)
    auto* const vec_d = _nt->node_d_storage();
    for (int i = i1; i < i3; ++i) {
        vec_d[i] = 0.;
    }

    auto const vec_sav_d = _nt->node_sav_d_storage();
    if (vec_sav_d) {
        for (i = i1; i < i3; ++i) {
            vec_sav_d[i] = 0.;
        }
    }

    /* note that CAP has no jacob */
    for (tml = _nt->tml; tml; tml = tml->next)
        if (auto const jacob = memb_func[tml->index].jacob; jacob) {
            std::string mechname("cur-");
            mechname += memb_func[tml->index].sym->name;
            nrn::Instrumentor::phase_begin(mechname.c_str());
            jacob(sorted_token, _nt, tml->ml, tml->index);
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
        nrn_cap_jacob(sorted_token, _nt, _nt->tml->ml);
    }

    activsynapse_lhs();

    if (vec_sav_d) {
        /* _nrn_save_d has only the contribution of electrode current
           so here we transform so it only has membrane current contribution
        */
        for (i = i1; i < i3; ++i) {
            vec_sav_d[i] += vec_d[i];
        }
    }
#if EXTRACELLULAR
    /* nde->_d[0] contains the -ELECTRODE_CURRENT contribution to nd->_d */
    nrn_setup_ext(_nt);
#endif
    if (use_sparse13) {
        /* must be after nrn_setup_ext so that whatever is put in
        nd->_d does not get added to nde->d */
        update_sp13_mat_based_on_actual_d(_nt);
        nrndae_lhs();
        update_actual_d_based_on_sp13_mat(_nt);  // because nrndae_lhs writes to sp13_mat
    }

    activclamp_lhs();

    /* at this point d contains all the membrane conductances */


    /* now add the axial currents */
    if (use_sparse13) {
        update_sp13_mat_based_on_actual_d(_nt);  // just because of activclamp_lhs
        for (i = i2; i < i3; ++i) {              // note i2
            Node* nd = _nt->_v_node[i];
            auto const parent_i = _nt->_v_parent_index[i];
            auto* const parent_nd = _nt->_v_node[parent_i];
            auto const nd_a = NODEA(nd);
            auto const nd_b = NODEB(nd);
            // Update entries in sp13_mat
            *nd->_a_matelm += nd_a;
            *nd->_b_matelm += nd_b; /* b may have value from lincir */
            *nd->_d_matelm -= nd_b;
            // used to update NODED (sparse13 matrix) using NODEA and NODEB ("SoA")
            *parent_nd->_d_matelm -= nd_a;
            // Also update the Node's d value in the SoA storage (is this needed?)
            vec_d[i] -= nd_b;
            vec_d[parent_i] -= nd_a;
        }
    } else {
        auto* const vec_a = _nt->node_a_storage();
        auto* const vec_b = _nt->node_b_storage();
        for (i = i2; i < i3; ++i) {
            vec_d[i] -= vec_b[i];
            vec_d[_nt->_v_parent_index[i]] -= vec_a[i];
        }
    }
}

/* for the fixed step method */
void setup_tree_matrix(neuron::model_sorted_token const& cache_token, NrnThread& nt) {
    nrn::Instrumentor::phase _{"setup-tree-matrix"};
    nrn_rhs(cache_token, nt);
    nrn_lhs(cache_token, nt);
    nrn_nonvint_block_current(nt.end, nt.node_rhs_storage(), nt.id);
    nrn_nonvint_block_conductance(nt.end, nt.node_d_storage(), nt.id);
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
        of the if statement because m above is not nullptr.
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
    nrn_alloc_node_ = nd;  // this might be null
    v_structure_change = 1;
    current_prop_list = pp;
    auto* p = new Prop{static_cast<short>(type)};
    p->next = *pp;
    p->ob = nullptr;
    p->_alloc_seq = -1;
    *pp = p;
    assert(memb_func[type].alloc);
    p->dparam = nullptr;
    (memb_func[type].alloc)(p);
    return p;
}

void prop_update_ion_variables(Prop* prop, Node* node) {
    nrn_alloc_node_ = node;
    nrn_point_prop_ = prop;
    current_prop_list = &node->prop;
    auto const type = prop->_type;
    assert(memb_func[type].alloc);
    memb_func[type].alloc(prop);
    current_prop_list = nullptr;
    nrn_point_prop_ = nullptr;
    nrn_alloc_node_ = nullptr;
}

Prop* prop_alloc_disallow(Prop** pp, short type, Node* nd) {
    disallow_needmemb = 1;
    auto* p = prop_alloc(pp, type, nd);
    disallow_needmemb = 0;
    return p;
}

// free an entire property list
void prop_free(Prop** pp) {
    Prop* p = *pp;
    *pp = nullptr;
    while (p) {
        single_prop_free(std::exchange(p, p->next));
    }
}

void single_prop_free(Prop* p) {
    extern char* pnt_map;
    v_structure_change = 1;
    if (pnt_map[p->_type]) {
        clear_point_process_struct(p);
        return;
    }
    if (auto got = nrn_mech_inst_destruct.find(p->_type); got != nrn_mech_inst_destruct.end()) {
        (got->second)(p);
    }
    if (p->dparam) {
        if (p->_type == CABLESECTION) {
            notify_freed_val_array(&(p->dparam[2].literal_value<double>()), 6);
        }
        nrn_prop_datum_free(p->_type, p->dparam);
    }
    if (p->ob) {
        hoc_obj_unref(p->ob);
    }
    delete p;
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
    double ra, dx, rright, rleft;
    Prop* p;
    Node* nd;
    if (nrn_area_ri_nocount_ == 0) {
        ++nrn_area_ri_count_;
    }
#if DIAMLIST
    if (sec->npt3d) {
        sec->prop->dparam[2] = sec->pt3d[sec->npt3d - 1].arc;
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
            auto& diam = p->param(0);
            if (diam <= 0.) {
                diam = 1e-6;
                hoc_execerror(secname(sec), "diameter diam = 0. Setting to 1e-6");
            }
            nd->area() = PI * diam * dx;                            // um^2
            rleft = 1e-2 * ra * (dx / 2) / (PI * diam * diam / 4.); /*left half segment Megohms*/
            NODERINV(nd) = 1. / (rleft + rright);                   /*uS*/
            rright = rleft;
        }
    }
    /* last segment has 0 length. area is 1e2
    in dimensionless units */
    sec->pnode[j]->area() = 1.e2;
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

    // To match legacy behaviour, make sure that the SoA storage for "a" and "b" is zeroed before
    // the initilisation code below is run.
    for (auto tid = 0; tid < nrn_nthread; ++tid) {
        auto& nt = nrn_threads[tid];
        std::fill_n(nt.node_a_storage(), nt.end, 0.0);
        std::fill_n(nt.node_b_storage(), nt.end, 0.0);
    }
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        // Unnecessary because they are unused, but help when looking at fmatrix.
        if (!sec->parentsec) {
            if (auto* const ptr = nrn_classicalNodeA(sec->parentnode)) {
                *ptr = 0.0;
            }
            if (auto* const ptr = nrn_classicalNodeB(sec->parentnode)) {
                *ptr = 0.0;
            }
        }
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
        *nrn_classicalNodeA(nd) = -1.e2 * sec->prop->dparam[4].get<double>() * NODERINV(nd) / area;
        for (j = 1; j < sec->nnode; j++) {
            nd = sec->pnode[j];
            area = NODEAREA(sec->pnode[j - 1]);
            *nrn_classicalNodeA(nd) = -1.e2 * NODERINV(nd) / area;
        }
    }
    /* now the effect of parent on node equation. */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        for (j = 0; j < sec->nnode; j++) {
            nd = sec->pnode[j];
            *nrn_classicalNodeB(nd) = -1.e2 * NODERINV(nd) / NODEAREA(nd);
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
    sec->prop->dparam[2] = sec->pt3d[n - 1].arc;
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
        sec->prop->dparam[2] = len;
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
    if (fabs(diam - p->param(0)) > 1e-9 || diam < 1e-5) {
        p->param(0) = diam; /* microns */
    }
    sec->pnode[inode]->area() = area * .5 * PI; /* microns^2 */
#if NTS_SPINE
    /* if last point has a spine then increment spine count for last node */
    if (inode == sec->nnode - 2 && sec->pt3d[npt - 1].d < 0.) {
        nspine += 1;
    }
    sec->pnode[inode]->area() = sec->pnode[inode]->area() + nspine * spinearea;
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
        if (nrn_is_artificial_[i] && memb_func[i].has_initialize()) {
            if (memb_list[i].nodecount) {
                memb_list[i].nodecount = 0;
                free(memb_list[i].nodelist);
                free(memb_list[i].nodeindices);
                delete[] memb_list[i].prop;
                if (!memb_func[i].hoc_mech) {
                    // free(memb_list[i]._data);
                    free(memb_list[i].pdata);
                }
            }
        }

#if 1 /* see finitialize */
    /* and count the artificial cells */
    for (i = 0; i < n_memb_func; ++i)
        if (nrn_is_artificial_[i] && memb_func[i].has_initialize()) {
            cTemplate* tmp = nrn_pnt_template_[i];
            memb_list[i].nodecount = tmp->count;
        }
#endif

    /* allocate it*/

    for (i = 0; i < n_memb_func; ++i)
        if (nrn_is_artificial_[i] && memb_func[i].has_initialize()) {
            if (memb_list[i].nodecount) {
                memb_list[i].nodelist = (Node**) emalloc(memb_list[i].nodecount * sizeof(Node*));
                memb_list[i].nodeindices = (int*) emalloc(memb_list[i].nodecount * sizeof(int));
                // Prop used by ode_map even when hoc_mech is false
                memb_list[i].prop = new Prop*[memb_list[i].nodecount];
                if (!memb_func[i].hoc_mech) {
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

    FOR_THREADS(_nt) {
        for (inode = 0; inode < _nt->end; inode++) {
            if (_nt->_v_parent[inode] != NULL) {
                _nt->_v_parent_index[inode] = _nt->_v_parent[inode]->v_node_index;
            }
        }
    }

    nrn_thread_memblist_setup();

    /* fill in artificial cell info */
    for (i = 0; i < n_memb_func; ++i) {
        if (nrn_is_artificial_[i] && memb_func[i].has_initialize()) {
            hoc_Item* q;
            cTemplate* tmp = nrn_pnt_template_[i];
            memb_list[i].nodecount = tmp->count;
            int nti{}, j{};
            hoc_List* list = tmp->olist;
            std::vector<std::size_t> thread_counts(nrn_nthread);
            ITERATE(q, list) {
                Object* obj = OBJ(q);
                auto* pnt = static_cast<Point_process*>(obj->u.this_pointer);
                p = pnt->prop;
                memb_list[i].nodelist[j] = nullptr;
                /* for now, round robin all the artificial cells */
                /* but put the non-threadsafe ones in thread 0 */
                /*
                 Note that artficial cells are assumed not to need a thread specific
                 Memb_list. But this implies that they do not have thread specific
                 data. For this reason, for now, an otherwise thread-safe artificial
                 cell model is declared by nmodl as thread-unsafe.
                */
                if (memb_func[i].vectorized == 0) {
                    pnt->_vnt = nrn_threads;
                } else {
                    pnt->_vnt = nrn_threads + nti;
                    nti = (nti + 1) % nrn_nthread;
                }
                auto const tid = static_cast<NrnThread*>(pnt->_vnt)->id;
                ++thread_counts[tid];
                // pnt->_i_instance = j;
                ++j;
            }
            assert(j == memb_list[i].nodecount);
            // The following is a transition measure while data are SOA-backed
            // using the new neuron::container::soa scheme but pdata are not.
            // data get permuted so that artificial cells are blocked according
            // to the NrnThread they are assigned to, but without this change
            // then the pdata order encoded in the global non-thread-specific
            // memb_list[i] structure was different, with threads interleaved.
            // This was a problem when we wanted to e.g. run the initialisation
            // kernels in finitialize using that global structure, as the i-th
            // rows of data and pdata did not refer to the same mechanism
            // instance. The temporary solution here is to manually organise
            // pdata to match the data order, with all the instances associated
            // with thread 0 followed by all the instances associated with
            // thread 1, and so on. See CellGroup::mk_tml_with_art for another
            // side of this story and why it is useful to have artificial cell
            // data blocked by thread.
            std::vector<std::size_t> thread_offsets(nrn_nthread);
            for (auto j = 1; j < nrn_nthread; ++j) {
                thread_offsets[j] = std::exchange(thread_counts[j - 1], 0) + thread_offsets[j - 1];
            }
            thread_counts[nrn_nthread - 1] = 0;
            ITERATE(q, list) {
                auto* const pnt = static_cast<Point_process*>(OBJ(q)->u.this_pointer);
                auto const tid = static_cast<NrnThread*>(pnt->_vnt)->id;
                memb_list[i].pdata[thread_offsets[tid] + thread_counts[tid]++] = pnt->prop->dparam;
            }
        }
    }
    neuron::model().node_data().mark_as_unsorted();
    v_structure_change = 0;
    nrn_update_ps2nt();
    ++structure_change_cnt;
    long_difus_solve(nrn_ensure_model_data_are_sorted(), 3, *nrn_threads);  // !!!
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
    /*	P "Mechanism names (first two are nullptr) beginning with memb_func[2]\n");*/
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

void nrn_matrix_node_free() {
    NrnThread* nt;
    FOR_THREADS(nt) {
        if (nt->_sp13_rhs) {
            free(std::exchange(nt->_sp13_rhs, nullptr));
        }
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
            // used to return here if the cache-efficient structures for a/b/... were non-null
        }
    }
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
        nt->_sp13_rhs = (double*) ecalloc(neqn + 1, sizeof(double));
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
            nt->_sp13_rhs[i] = nt->actual_rhs(in);
            nd->_d_matelm = spGetElement(nt->_sp13mat, i, i);
            if (nde) {
                for (ie = 0; ie < nlayer; ++ie) {
                    k = i + ie + 1;
                    nde->_d[ie] = spGetElement(nt->_sp13mat, k, k);
                    nde->_rhs[ie] = nt->_sp13_rhs + k;
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
                nd->_a_matelm = nullptr;
                nd->_b_matelm = nullptr;
            }
        }
        nrndae_alloc();
    } else {
        FOR_THREADS(nt) {
            assert(nrndae_extra_eqn_count() == 0);
            assert(!nt->_ecell_memb_list || nt->_ecell_memb_list->nodecount == 0);
        }
    }
}

/** @brief Sort the underlying storage for a particular mechanism.
 *
 *  After model building is complete the storage vectors backing all Mechanism
 *  instances can be permuted to ensure that preconditions are met for the
 *  computations performed while time-stepping.
 *
 *  This method ensures that the Mechanism data is ready for this compute phase.
 *  It is guaranteed to remain "ready" until the returned tokens are destroyed.
 */
static void nrn_sort_mech_data(
    neuron::container::Mechanism::storage::frozen_token_type& sorted_token,
    neuron::cache::Model& cache,
    neuron::container::Mechanism::storage& mech_data) {
    // Do the actual sorting here. For now the algorithm is just to ensure that
    // the mechanism instances are partitioned by NrnThread.
    auto const type = mech_data.type();
    // Some special types are not "really" mechanisms and don't need to be
    // sorted
    if (type != MORPHOLOGY) {
        std::size_t const mech_data_size{mech_data.size()};
        std::vector<short> pdata_fields_to_cache{};
        neuron::cache::indices_to_cache(type,
                                        [mech_data_size,
                                         &pdata_fields_to_cache,
                                         &pdata_hack = cache.mechanism.at(type).pdata_hack](
                                            auto field) {
                                            if (field >= pdata_hack.size()) {
                                                // we get called with the largest field first
                                                pdata_hack.resize(field + 1);
                                            }
                                            pdata_hack.at(field).reserve(mech_data_size);
                                            pdata_fields_to_cache.push_back(field);
                                        });
        std::size_t global_i{}, trivial_counter{};
        std::vector<std::size_t> mech_data_permutation(mech_data_size,
                                                       std::numeric_limits<std::size_t>::max());
        NrnThread* nt{};
        FOR_THREADS(nt) {
            // the Memb_list for this mechanism in this thread, this might be
            // null if there are no entries, or if it's an artificial cell type(?)
            auto* const ml = nt->_ml_list[type];
            if (ml) {
                // Tell the Memb_list what global offset its values start at
                ml->set_storage_offset(global_i);
            }
            // Record where in the global storage this NrnThread's instances of
            // the mechanism start
            cache.thread.at(nt->id).mechanism_offset.at(type) = global_i;
            // Count how many times we see this mechanism in this NrnThread
            auto nt_mech_count = 0;
            // Loop through the Nodes in this NrnThread
            for (auto i = 0; i < nt->end; ++i) {
                auto* const nd = nt->_v_node[i];
                // See if this Node has a mechanism of this type
                for (Prop* p = nd->prop; p; p = p->next) {
                    if (p->_type != type) {
                        continue;
                    }
                    // this condition comes from thread_memblist_setup(...)
                    if (!memb_func[type].current && !memb_func[type].state &&
                        !memb_func[type].has_initialize()) {
                        continue;
                    }
                    // OK, p is an instance of the mechanism we're currently
                    // considering.
                    auto const current_global_row = p->id().current_row();
                    trivial_counter += (current_global_row == global_i);
                    mech_data_permutation.at(current_global_row) = global_i++;
                    for (auto const field: pdata_fields_to_cache) {
                        cache.mechanism.at(type).pdata_hack.at(field).push_back(p->dparam + field);
                    }
                    // Checks
                    assert(ml->nodelist[nt_mech_count] == nd);
                    assert(ml->nodeindices[nt_mech_count] == nd->v_node_index);
                    ++nt_mech_count;
                }
            }
            assert(!ml || ml->nodecount == nt_mech_count);
            // Look for any artificial cells attached to this NrnThread
            if (nrn_is_artificial_[type]) {
                cTemplate* tmp = nrn_pnt_template_[type];
                hoc_Item* q;
                ITERATE(q, tmp->olist) {
                    Object* obj = OBJ(q);
                    auto* pnt = static_cast<Point_process*>(obj->u.this_pointer);
                    assert(pnt->prop->_type == type);
                    if (nt == pnt->_vnt) {
                        auto const current_global_row = pnt->prop->id().current_row();
                        trivial_counter += (current_global_row == global_i);
                        mech_data_permutation.at(current_global_row) = global_i++;
                        for (auto const field: pdata_fields_to_cache) {
                            cache.mechanism.at(type).pdata_hack.at(field).push_back(
                                pnt->prop->dparam + field);
                        }
                    }
                }
            }
        }
        if (global_i != mech_data_size) {
            // This means that we did not "positively" find all the instances of
            // this mechanism by traversing the model structure. This can happen
            // if HOC (or probably Python) scripts create instances and then do
            // not attach them anywhere, or do not explicitly destroy
            // interpreter variables that are preventing reference counts from
            // reaching zero. In this case we can figure out which the missing
            // entries are and permute them to the end of the vector.
            auto missing_elements = mech_data_size - global_i;
            // There are `missing_elements` integers from the range [0 ..
            // mech_data_size-1] whose values in `mech_data_permutation` are
            // still std::numeric_limits<std::size_t>::max().
            for (auto global_row = 0ul; global_row < mech_data_size; ++global_row) {
                if (mech_data_permutation[global_row] == std::numeric_limits<std::size_t>::max()) {
                    trivial_counter += (global_row == global_i);
                    mech_data_permutation[global_row] = global_i++;
                    --missing_elements;
                    if (missing_elements == 0) {
                        break;
                    }
                }
            }
            if (global_i != mech_data_size) {
                std::ostringstream oss;
                oss << "(global_i = " << global_i << ") != (mech_data_size = " << mech_data_size
                    << ") for " << mech_data.name();
                throw std::runtime_error(oss.str());
            }
        }
        assert(trivial_counter <= mech_data_size);
        if (trivial_counter < mech_data_size) {
            // The `mech_data_permutation` vector is not a unit transformation
            mech_data.apply_reverse_permutation(std::move(mech_data_permutation), sorted_token);
        }
    }
    // Make sure that everything ends up flagged as sorted, even morphologies,
    // mechanism types with no instances, and cases where the permutation
    // vector calculated was found to be trivial.
    mech_data.mark_as_sorted(sorted_token);
}

void nrn_fill_mech_data_caches(neuron::cache::Model& cache,
                               neuron::container::Mechanism::storage& mech_data) {
    // Generate some temporary "flattened" vectors from pdata
    // For example, when a mechanism uses an ion then one of its pdata fields holds Datum
    // (=generic_data_handle) objects that wrap data_handles to ion (RANGE) variables.
    // Dereferencing those fields to access the relevant double values can be indirect and
    // expensive, so here we can generate a std::vector<double*> that can be used directly in
    // hot loops. This is partitioned for the threads in the same way as the other data.
    // Note that this needs to come after *all* of the mechanism types' data have been permuted, not
    // just the type that we are filling the cache for.
    // TODO could identify the case that the pointers are all monotonically increasing and optimise
    // further?
    auto const type = mech_data.type();
    // Some special types are not "really" mechanisms and don't need to be
    // sorted
    if (type != MORPHOLOGY) {
        auto& mech_cache = cache.mechanism.at(type);
        // Transform the vector<Datum> in pdata_hack into vector<double*> in pdata
        std::transform(mech_cache.pdata_hack.begin(),
                       mech_cache.pdata_hack.end(),
                       std::back_inserter(mech_cache.pdata),
                       [](std::vector<Datum*>& pdata_hack) {
                           std::vector<double*> tmp{};
                           std::transform(pdata_hack.begin(),
                                          pdata_hack.end(),
                                          std::back_inserter(tmp),
                                          [](Datum* datum) { return datum->get<double*>(); });
                           pdata_hack.clear();
                           pdata_hack.shrink_to_fit();
                           return tmp;
                       });
        mech_cache.pdata_hack.clear();
        // Create a flat list of pointers we can use inside generated code
        std::transform(mech_cache.pdata.begin(),
                       mech_cache.pdata.end(),
                       std::back_inserter(mech_cache.pdata_ptr_cache),
                       [](auto const& pdata) { return pdata.empty() ? nullptr : pdata.data(); });
    }
}

/** @brief Sort the underlying storage for Nodes.
 *
 *  After model building is complete the storage vectors backing all Node
 *  objects can be permuted to ensure that preconditions are met for the
 *  computations performed while time-stepping.
 *
 *  This method ensures that the Node data is ready for this compute phase.
 */
static void nrn_sort_node_data(neuron::container::Node::storage::frozen_token_type& sorted_token,
                               neuron::cache::Model& cache) {
    // Make sure the voltage storage follows the order encoded in _v_node.
    // Generate the permutation vector to update the underlying storage for
    // Nodes. This must come after nrn_multisplit_setup_, which can change the
    // Node order.
    auto& node_data = neuron::model().node_data();
    std::size_t const node_data_size{node_data.size()};
    std::size_t global_i{};
    std::vector<std::size_t> node_data_permutation(node_data_size,
                                                   std::numeric_limits<std::size_t>::max());
    // Process threads one at a time -- this means that the data for each
    // NrnThread will be contiguous.
    NrnThread* nt{};
    FOR_THREADS(nt) {
        // What offset in the global node data structure do the values for this thread
        // start at
        nt->_node_data_offset = global_i;
        cache.thread.at(nt - nrn_threads).node_data_offset = global_i;
        for (int i = 0; i < nt->end; ++i, ++global_i) {
            Node* nd = nt->_v_node[i];
            auto const current_node_row = nd->_node_handle.current_row();
            assert(current_node_row < node_data_size);
            assert(global_i < node_data_size);
            node_data_permutation.at(current_node_row) = global_i;
        }
    }
    if (global_i != node_data_size) {
        // This means that we did not "positively" find all the Nodes by traversing the NrnThread
        // objects. In this case we can figure out which the missing entries are and permute them to
        // the end of the global vectors.
        auto missing_elements = node_data_size - global_i;
        std::cout << "permuting " << missing_elements << " 'lost' Nodes to the end\n";
        // There are `missing_elements` integers from the range [0 .. node_data_size-1] whose values
        // in `node_data_permutation` are still std::numeric_limits<std::size_t>::max().
        for (auto global_row = 0ul; global_row < node_data_size; ++global_row) {
            if (node_data_permutation[global_row] == std::numeric_limits<std::size_t>::max()) {
                node_data_permutation[global_row] = global_i++;
                --missing_elements;
                if (missing_elements == 0) {
                    break;
                }
            }
        }
        if (global_i != node_data_size) {
            std::ostringstream oss;
            oss << "(global_i = " << global_i << ") != (node_data_size = " << node_data_size << ')';
            throw std::runtime_error(oss.str());
        }
    }
    // Apply the permutation, using `sorted_token` to authorise this operation
    // despite the container being frozen.
    node_data.apply_reverse_permutation(std::move(node_data_permutation), sorted_token);
}

/**
 * @brief Ensure neuron::container::* data are sorted.
 *
 * Set all of the containers to be in read-only mode, until the returned token
 * is destroyed. This method can be called from multi-threaded regions.
 */
neuron::model_sorted_token nrn_ensure_model_data_are_sorted() {
    // Rather than a more complicated lower-level solution, just serialise all
    // calls to this method. The more complicated lower-level solution would
    // presumably involve a more fully-fledged std::shared_mutex-type model,
    // where the soa containers can be locked by many readers (clients that do
    // not do anything that invalidates pointers/caches) or a single writer
    // (who *is* authorised to perform those operations), with a deadlock
    // avoiding algorithm used to acquire those two types of lock for all the
    // different soa containers at once.
    static std::mutex s_mut{};
    std::unique_lock _{s_mut};
    // Two scenarii:
    // - model is already sorted, in which case we just assemble the return
    //   value but don't mutate anything or do any real work
    // - something is not already sorted, and by extension the cache is not
    //   valid.
    // In both cases, we want to start by acquiring tokens from all of the
    // data containers in the model. Once we have locked the whole model in
    // this way, we can trigger permuting the model (by loaning out the tokens
    // one by one) to mark it as sorted.
    auto& model = neuron::model();
    auto& node_data = model.node_data();
    // Get tokens for the whole model
    auto node_token = node_data.issue_frozen_token();
    auto already_sorted = node_data.is_sorted();
    // How big does an array have to be to be indexed by mechanism type?
    auto const mech_storage_size = model.mechanism_storage_size();
    std::vector<neuron::container::Mechanism::storage::frozen_token_type> mech_tokens{};
    mech_tokens.reserve(mech_storage_size);
    model.apply_to_mechanisms([&already_sorted, &mech_tokens](auto& mech_data) {
        mech_tokens.push_back(mech_data.issue_frozen_token());
        already_sorted = already_sorted && mech_data.is_sorted();
    });
    // Now the whole model is marked frozen/read-only, but it may or may not be
    // marked sorted (if it is, the cache should be valid, otherwise it should
    // not be).
    if (already_sorted) {
        // If everything was already sorted, the cache should already be valid.
        assert(neuron::cache::model);
        // There isn't any more work to be done, really.
    } else {
        // Some part of the model data is not already marked sorted. In this
        // case we expect that the cache is *not* valid, because whatever
        // caused something to not be sorted should also have invalidated the
        // cache.
        assert(!neuron::cache::model);
        // Build a new cache (*not* in situ, so it doesn't get invalidated
        // under our feet while we're in the middle of the job) and populate it
        // by calling the various methods that sort the model data.
        neuron::cache::Model cache{};
        cache.thread.resize(nrn_nthread);
        for (auto& thread_cache: cache.thread) {
            thread_cache.mechanism_offset.resize(mech_storage_size);
        }
        cache.mechanism.resize(mech_storage_size);
        // The cache is initialised enough to be populated by the various data
        // sorting algorithms. The small "problem" here is that all of the
        // model data structures are already marked as frozen via the tokens
        // that we acquired above. The way around this is to transfer those
        // tokens back to the relevant containers, so they can check that the
        // only active token is the one that has been provided back to them. In
        // other words, any token is a "read lock" but a non-const token that
        // refers to a container with token reference count of exactly one has
        // an elevated "write lock" status.
        nrn_sort_node_data(node_token, cache);
        assert(node_data.is_sorted());
        // TODO: maybe we should separate out cache population from sorting.
        std::size_t n{};  // eww
        model.apply_to_mechanisms([&cache, &n, &mech_tokens](auto& mech_data) {
            // TODO do we need to pass `node_token` to `nrn_sort_mech_data`?
            nrn_sort_mech_data(mech_tokens[n], cache, mech_data);
            assert(mech_data.is_sorted());
            ++n;
        });
        // Now that all the mechanism data is sorted we can fill in pdata caches
        model.apply_to_mechanisms(
            [&cache](auto& mech_data) { nrn_fill_mech_data_caches(cache, mech_data); });
        // Move our working cache into the global storage.
        neuron::cache::model = std::move(cache);
    }
    // Move our tokens into the return value and be done with it.
    neuron::model_sorted_token ret{*neuron::cache::model, std::move(node_token)};
    ret.mech_data_tokens = std::move(mech_tokens);
    return ret;
}
