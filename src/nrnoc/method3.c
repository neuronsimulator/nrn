#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/method3.c,v 1.5 1999/07/08 14:25:03 hines Exp */
/*
method3.c,v
 * Revision 1.5  1999/07/08  14:25:03  hines
 * Uniformly use section_length(sec) instead of sec->prop->dparam[2].val
 * and make sure it never returns <= 0
 * Also avoid recursive error when error due to shape updating.
 *
 * Revision 1.4  1998/02/19  20:20:03  hines
 * several fields moved from memb_list to memb_func.
 * This was more convenient for local variable time step method
 *
 * Revision 1.3  1997/04/07  19:47:26  hines
 * PI now same everywhere
 *
 * Revision 1.2  1996/05/21  17:09:21  hines
 * Section now holds list of node pointers instead of vector of nodes.
 * This will be used later to allow changing nseg to not throw away node
 * information. This has been checked up through the level of the nrn/examples
 * files to verify that it produces the same results as the previous structure.
 *
 * Revision 1.1.1.1  1994/10/12  17:22:33  hines
 * NEURON 3.0 distribution
 *
 * Revision 4.25  1993/04/20  08:58:42  hines
 * rootsection no longer exists. section structure changed so
 * sec->parentnode is a Node*. sec->parentsec no longer changed but
 * kept the way user specified it. The function Section* nrn_trueparent(sec)
 * returns the section that used to be in parentsec after setup_topology()
 * was executed. no more inode_exact(Section** psec, double x)---has been
 * replaced by Node* node_exact(Section* sec, double x)
 * There is now a rootnode list. parentsec of a section which was not
 * connect'ed to anything has a parentsec of nil. One can now disconnect
 * and delete sections without otherwise changing user spec (except
 * point processes at ends may be moved or changed -- will be fixed later)
 *
 * Revision 4.4  93/01/06  09:44:45  hines
 * minor adjustments for cray c90
 * 
 * Revision 3.43  92/12/16  14:24:33  hines
 * Ra switched from global variable to section variable in which it can
 * take on different values in different sections. Similar to L.
 * No need to set diam_changed when Ra changed.
 * 
 * Revision 3.35  92/10/27  12:09:50  hines
 * list.c list.h moved from nrnoc  to oc
 * 
 * Revision 3.21  92/10/08  10:24:42  hines
 * third order correct with _method3 = 3 and when more than 1 segment
 * per section and interior segments do not have point processes.
 * when not active then third order correct no matter what.
 * 
 * Revision 3.17  92/09/30  16:52:59  hines
 * strength of stimuli requires area=100 for _method3
 * 
 * Revision 3.15  92/09/27  19:30:12  hines
 * set up for testing with 1-d cables. only first node sees G' and E'
 * dg and de set to 0 for all other nodes. Doesn't appear to be a difference
 * whether G' and E' are second order correct or not for HH cables.
 * 
 * Revision 3.13  92/09/27  17:45:11  hines
 * non vectorized model descriptions with _method3 fill thisnode.GC,EC
 * instead of d,rhs when models are channel densities.
 * HH doesn't work too well with G' and E' as first order differences so
 * now it is zeroed.
 * 
 * Revision 3.12  92/09/25  18:03:22  hines
 * for method3. now tested and working for transient passive cables.
 * but perhaps the equations could be written with better roundoff
 * error when dt is small
 * 
 * Revision 3.11  92/09/24  17:03:42  hines
 * METHOD3 option. use with spatial_method(i) with i=0-3
 * 0 is the default and is the normal neuron method with zero area nodes
 * at boundaries and branch points.
 * 1 is the standard method with half area nodes at boundaries
 * 2 is a modified second order method which weights by 2/12, 8/12, 2/12
 * 3 is a third order correct method which weights by 1/12 10/12 1/12
 * with the last three methods x = i/nnode with i=0 to nnode
 * methods 1 and 2 are at present quite inefficient since they use the
 * data structures of 3.
 * Works in steady state. not tested with dynamic simulations
 * At this time, discontinuities in parameters (except diameter) is
 * not handled well across nodes and all parameters are assumed to
 * be constant in interval between node and parent node.
 * Good deal of difficulty remains in managing values that change discontinuously
 * 
 * Revision 1.1  92/09/24  11:26:31  hines
 * Initial revision
 * 
*/
/* started from version 3.5 of treeset.c */


#include	<stdio.h>
#include	<errno.h>
#include	"section.h"
#if METHOD3 && VECTORIZE
#include	"membfunc.h"
#include	"neuron.h"
#include	"parse.h"

extern int	diam_changed;
extern int	tree_changed;
extern double	chkarg();
extern double	nrn_ra();

int _method3;

int spatial_method() {
	int new_method;
	
	if (ifarg(1)) {
		new_method = (int)chkarg(1, 0., 3.);
		if (_method3 == 0 && new_method) { /* don't use last node */
			tree_changed = 1;
		}else if (_method3 && new_method == 0) { /* need the last node */
			tree_changed = 1;
		}
		if (_method3 != new_method) {
			_method3 = new_method;
			tree_changed = 1;
			diam_changed = 1;
		}
		_method3 = new_method;
	}
	hoc_retpushx((double) _method3);
}
		
/*
When properties are allocated to nodes or freed, v_structure_change is
set to 1. This means that the mechanism vectors need to be re-determined.
*/
extern int v_structure_change;
extern int v_node_count;
extern Node** v_node;
extern Node** v_parent;

extern int section_count;
extern Section** secorder;

method3_setup_tree_matrix() /* construct diagonal elements */
{
	int i;
	if (diam_changed) {
		recalc_diam();
	}
#if _CRAY
#pragma _CRI ivdep
#endif
	for (i = 0; i < v_node_count; ++i) {
		Node* nd = v_node[i];
		NODED(nd) = 0.;
		NODERHS(nd) = 0.;
		nd->thisnode.GC = 0.;
		nd->thisnode.EC = 0.;
	}
	for (i=0; i < n_memb_func; ++i) if (memb_func[i].current && memb_list[i].nodecount) {
		if (memb_func[i].vectorized) {
			memb_func[i].current(
			   memb_list[i].nodecount,
			   memb_list[i].nodelist,
			   memb_list[i].data,
			   memb_list[i].pdata
			);
		}else{
			int j, count;
			Pfrd s = memb_func[i].current;
			Memb_list* m = memb_list + i;
			count = m->nodecount;
		   if (memb_func[i].is_point) {
			for (j = 0; j < count; ++j) {
				Node* nd = m->nodelist[j];
				NODERHS(nd) -= (*s)(m->data[j], m->pdata[j],
				  		&NODED(nd),nd->v);
			};
		   }else{
			for (j = 0; j < count; ++j) {
				Node* nd = m->nodelist[j];
				nd->thisnode.EC -= (*s)(m->data[j], m->pdata[j],
				  		&nd->thisnode.GC,nd->v);
			};
		   }
		}
		if (errno) {
			if (nrn_errno_check(i)) {
hoc_warning("errno set during calculation of currents", (char*)0);
			}
		}
	}
#if 0 && _CRAY
#pragma _CRI ivdep
#endif
	for (i=rootnodecount; i < v_node_count; ++i) {
		Node* nd2;
		Node* nd = v_node[i];
		Node* pnd = v_parent[nd->v_node_index];
		double dg, de, dgp, dep, fac;

#if 0
if (i == rootnodecount) {
	printf("v0 %g  vn %g  jstim %g  jleft %g jright %g\n",
	nd->v, pnd->v, nd->fromparent.current, nd->toparent.current,
	nd[1].fromparent.current);
}
#endif

		/* dg and de must be second order when used */
		if ((nd2 = nd->toparent.nd2) != (Node*)0) {
			dgp = -(3*(pnd->thisnode.GC - pnd->thisnode.Cdt)
				- 4*(nd->thisnode.GC - nd->thisnode.Cdt)
				+(nd2->thisnode.GC - nd2->thisnode.Cdt))/2
			;
			dep = -(3*(pnd->thisnode.EC - pnd->thisnode.Cdt * pnd->v)
				- 4*(nd->thisnode.EC - nd->thisnode.Cdt * nd->v)
				+(nd2->thisnode.EC - nd2->thisnode.Cdt * nd2->v))/2
			;
		}else{
			dgp = 0.;
			dep = 0.;
		}

		if ((nd2 = pnd->fromparent.nd2) != (Node*)0) {
			dg = -(3*(nd->thisnode.GC - nd->thisnode.Cdt)
				- 4*(pnd->thisnode.GC - pnd->thisnode.Cdt)
				+(nd2->thisnode.GC - nd2->thisnode.Cdt))/2
			;
			de = -(3*(nd->thisnode.EC - nd->thisnode.Cdt * nd->v)
				- 4*(pnd->thisnode.EC - pnd->thisnode.Cdt * pnd->v)
				+(nd2->thisnode.EC - nd2->thisnode.Cdt * nd2->v))/2
			;
		}else{
			dg = 0.;
			de = 0.;
		}

		fac = 1. + nd->toparent.coefjdot * nd->thisnode.GC;
		nd->toparent.djdv0 = (
			nd->toparent.coefj
			+ nd->toparent.coef0 * nd->thisnode.GC
			+ nd->toparent.coefdg * dg
			)/fac;
		NODED(nd) += nd->toparent.djdv0;
		nd->toparent.current = (
			- nd->toparent.coef0 * nd->thisnode.EC
			- nd->toparent.coefn * pnd->thisnode.EC
			+ nd->toparent.coefjdot * 
				nd->thisnode.Cdt * nd->toparent.current
			- nd->toparent.coefdg * de
			)/fac;
		NODERHS(nd) -= nd->toparent.current;
		NODEB(nd) = (
			- nd->toparent.coefj
			+ nd->toparent.coefn * pnd->thisnode.GC
			)/fac;

		/* this can break cray vectors */
		fac = 1. + nd->fromparent.coefjdot * pnd->thisnode.GC;
		nd->fromparent.djdv0 = (
			nd->fromparent.coefj
			+ nd->fromparent.coef0 * pnd->thisnode.GC
			+ nd->fromparent.coefdg * dgp
			)/fac;
		pNODED(nd) += nd->fromparent.djdv0;
		nd->fromparent.current = (
			- nd->fromparent.coef0 * pnd->thisnode.EC
			- nd->fromparent.coefn * nd->thisnode.EC
			+ nd->fromparent.coefjdot * 
				nd->thisnode.Cdt * nd->fromparent.current
			- nd->fromparent.coefdg * dep
			)/fac;
		pNODERHS(nd) -= nd->fromparent.current;
		NODEA(nd) = (
			- nd->fromparent.coefj
			+ nd->fromparent.coefn * nd->thisnode.GC
			)/fac;
	}
	activstim();
	activsynapse();
#if SEJNOWSKI
	activconnect();
#endif
	activclamp();
}

method3_axial_current() {
	int i;
#if _CRAY
#pragma _CRI ivdep
#endif
	for (i=rootnodecount; i < v_node_count; ++ i) {
		Node* nd = v_node[i];
		Node* pnd = v_parent[i];
		nd->toparent.current +=
			nd->toparent.djdv0 * nd->v
			+ NODEB(nd) * pnd->v;
			
		nd->fromparent.current +=
			nd->fromparent.djdv0 * pnd->v
			+ NODEA(nd) * nd->v;
		
	}	
#if 0
printf("cur0 %g curleft %g curright %g\n", 
	v_node[rootnodecount]->fromparent.current,
	v_node[rootnodecount]->toparent.current,
	v_node[rootnodecount+1]->fromparent.current
	);
#endif
}

/* For now there is always one more node in a section than there are
segments */
/* except in section 0 in which all nodes serve as x=0 to connecting
sections */
/* the above is obsolete with method3 */

#define PI	3.14159265358979323846

method3_connection_coef()	/* setup a and b */
{
	int j;
	double dx, diam, ra, coef;
	hoc_Item* qsec;
	Section* sec;
	Node *nd;
	Prop *p, *nrn_mechanism();
	float r, s, t;
	
	if (tree_changed) {
		setup_topology();
	}
/* for now assume diameter between node and parent is constant and located at the node */

	/* r = 6 is standard method, 5 is third order, 4 is modified second order */
	switch (_method3) {
	case 1:
		r = 6.;
		break;
	case 2:
		r = 4.;
		break;
	case 3:
		r = 5.;
		break;
	default:
		hoc_execerror(" invalid spatial method", (char*)0);
	}
	s = 6. - r;
	if (r == 5.) {
		t = 1.;
	}else{
		t = 0.;
	}

	ForAllSections(sec)
		dx = section_length(sec)/((double)(sec->nnode));
		for (j = 0; j < sec->nnode; ++j) {
			nd = sec->pnode[j];
			p = nrn_mechanism(MORPHOLOGY, nd);
			assert(p);
			diam = p->param[0];
			/* dv/(ra*dx) is nanoamps */
			ra = nrn_ra(sec)*4.e-2/(PI * diam*diam);
			/* coef*dx* mA/cm^2 should be nanoamps */
			coef = PI *1e-2* diam;

			nd->area = 100.;
			sec->parentnode->area = 100.;

			nd->toparent.coef0 = coef*r*dx/12.;
			nd->fromparent.coef0 = coef*r*dx/12.;
			nd->toparent.coefn = coef*s*dx/12.;
			nd->fromparent.coefn = coef*s*dx/12.;
			nd->toparent.coefjdot = t*ra*coef*dx*dx/12.;
			nd->fromparent.coefjdot = t*ra*coef*dx*dx/12.;
			nd->toparent.coefdg = t*coef*dx/12.;
			nd->fromparent.coefdg = t*coef*dx/12.;
			nd->toparent.coefj = 1./(ra*dx);
			nd->fromparent.coefj = 1./(ra*dx);
			nd->toparent.nd2 = 0;
			nd->fromparent.nd2 = 0;
		}
		if (sec->nnode > 1) {
			sec->pnode[0]->toparent.nd2 = sec->pnode[1];
			if (sec->nnode > 2) {
				sec->pnode[sec->nnode - 2]->fromparent.nd2 =
					sec->pnode[sec->nnode - 3];
			}else{
			   sec->pnode[sec->nnode - 2]->fromparent.nd2 =
				sec->parentsec->pnode[sec->parentsec->nnode - 1];
			}
		}
	}
}

#endif
