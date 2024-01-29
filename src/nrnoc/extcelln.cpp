#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/extcell.cpp,v 1.4 1996/05/21 17:09:19 hines Exp */

#include <stdio.h>
#include <math.h>
#include "section.h"
#include "nrniv_mf.h"
#include "hocassrt.h"
#include "parse.hpp"


extern int nrn_use_daspk_;

#if EXTRACELLULAR

int nrn_nlayer_extracellular = EXTRACELLULAR;

/* the N index is a keyword in the following. See init.cpp for implementation.*/
/* if nlayer is changed the symbol table arayinfo->sub[0] must be updated
   for xraxial, xg, xc, and vext */
static const char* mechanism[] = {"0",
                                  "extracellular",
                                  "xraxial[N]",
                                  "xg[N]",
                                  "xc[N]",
                                  "e_extracellular",
                                  nullptr,
#if I_MEMBRANE
                                  "i_membrane",
                                  "sav_g",
                                  "sav_rhs",
#endif
                                  nullptr,
                                  "vext[N]",  // not handled with other mech data
                                  nullptr};
static HocParmLimits limits[] = {{"xraxial", {1e-9, 1e15}},
                                 {"xg", {0., 1e15}},
                                 {"xc", {0., 1e15}},
                                 {nullptr, {0., 0.}}};

static HocParmUnits units[] = {{"xraxial", "MOhm/cm"},
                               {"xg", "S/cm2"},
                               {"xc", "uF/cm2"},
                               {"e_extracellular", "mV"},
                               {"vext", "mV"},
                               {"i_membrane", "mA/cm2"},
                               {0, 0}};

static void extcell_alloc(Prop*);
static void extcell_init(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type);
#if 0
static void printnode(const char* s);
#endif

static int _ode_count(int type) {
    /*	hoc_execerror("extracellular", "cannot be used with CVODE");*/
    /* but can be used with daspk. However everything is handled
    in analogy with intracellular nodes instead of in analogy with a
    membrane mechanism.
    Thus the count really comes from the size of sp13mat.
*/
    return 0;
}

// 4: xraxial[nlayer], xg[nlayer], xc[nlayer], e_extracellular
constexpr static auto nparm = 4
#if I_MEMBRANE
                              + 3  // i_membrane, sav_g, and sav_rhs
#endif
    ;
// it seems that one past the end (i.e. index nparm) is used to refer to vext, but that is
// allocated/managed separately; see neuron::extracellular::vext_pseudoindex() and its usage

static void update_parmsize() {
    using neuron::mechanism::field;
    // clang-format off
    neuron::mechanism::register_data_fields(EXTRACELL
        , field<double>{"xraxial", nrn_nlayer_extracellular}
        , field<double>{"xg", nrn_nlayer_extracellular}
        , field<double>{"xc", nrn_nlayer_extracellular}
        , field<double>{"e_extracellular"}
#if I_MEMBRANE
        , field<double>{"i_membrane"}
        , field<double>{"sav_g"}
        , field<double>{"sav_rhs"}
#endif
    );
    // clang-format on
    hoc_register_prop_size(EXTRACELL, nparm, 0);
}

extern "C" void extracell_reg_(void) {
    register_mech(mechanism, extcell_alloc, nullptr, nullptr, nullptr, extcell_init, -1, 1);
    int const i = nrn_get_mechtype(mechanism[1]);
    assert(i == EXTRACELL);
    hoc_register_cvode(i, _ode_count, nullptr, nullptr, nullptr);
    hoc_register_limits(i, limits);
    hoc_register_units(i, units);
    update_parmsize();
}


/* solving is done with sparse13 */

/* interface between hoc and extcell */
using namespace neuron::extracellular;

/* based on update() in fadvance.cpp */
/* update has already been called so modify nd->v based on dvi
we only need to update extracellular
nodes and base the corresponding nd->v on dvm (dvm = dvi - dvx)
*/
void nrn_update_2d(NrnThread* nt) {
    int i, cnt, il;
    extern int secondorder;
    Node *nd, **ndlist;
    Extnode* nde;
    double cfac;
    Memb_list* ml = nt->_ecell_memb_list;
    if (!ml) {
        return;
    }
    cfac = .001 * nt->cj;
    cnt = ml->nodecount;
    ndlist = ml->nodelist;

    for (i = 0; i < cnt; ++i) {
        nd = ndlist[i];
        nde = nd->extnode;
        for (il = 0; il < nlayer; ++il) {
            nde->v[il] += *nde->_rhs[il];
        }
        nd->v() -= *nde->_rhs[0];
    }

#if I_MEMBRANE
    for (i = 0; i < cnt; ++i) {
        nd = ndlist[i];
        NODERHS(nd) -= *nd->extnode->_rhs[0];
        ml->data(i, i_membrane_index) = ml->data(i, sav_g_index) * (NODERHS(nd)) +
                                        ml->data(i, sav_rhs_index);
#if 1
        /* i_membrane is a current density (mA/cm2). However
            it contains contributions from Non-ELECTRODE_CURRENT
            point processes. i_membrane(0) and i_membrane(1) will
            return the membrane current density at the points
            .5/nseg and 1-.5/nseg respectively. This can cause
            confusion if non-ELECTRODE_CURRENT point processes
            are located at these 0-area nodes since 1) not only
            is the true current density infinite, but 2) the
            correct absolute current is being computed here
            at the x=1 point but is not available, and 3) the
            correct absolute current at x=0 is not computed
            if the parent is a rootnode or there is no
            extracellular mechanism for the parent of this
            section. Thus, if non-ELECTRODE_CURRENT point processesm,
            eg synapses, are being used it is not a good idea to
            insert them at the points x=0 or x=1
        */
#else
        ml->data(i, i_membrane_index) *= NODEAREA(nd);
        /* i_membrane is nA for every segment. This is different
            from all other continuous mechanism currents and
            same as PointProcess currents since it contains
            non-ELECTRODE_CURRENT point processes and may
            be non-zero for the zero area nodes.
        */
#endif
    }
#endif
}

static void extcell_alloc(Prop* p) {
    assert(p->param_size() == (nparm - 3) + 3 * nrn_nlayer_extracellular);
    assert(p->param_num_vars() == nparm);
    for (auto i = 0; i < nrn_nlayer_extracellular; ++i) {
        p->param(xraxial_index, i) = 1e9;
        p->param(xg_index, i) = 1e9;
        p->param(xc_index, i) = 0.0;
    }
    p->param(e_extracellular_index) = 0.0;
}

/*ARGSUSED*/
static void extcell_init(neuron::model_sorted_token const&,
                         NrnThread* nt,
                         Memb_list* ml,
                         int type) {
    int ndcount = ml->nodecount;
    Node** ndlist = ml->nodelist;
    if ((cvode_active_ > 0) && (nrn_use_daspk_ == 0)) {
        hoc_execerror("Extracellular mechanism only works with fixed step methods and daspk", 0);
    }
    for (int i = 0; i < ndcount; ++i) {
        for (int j = 0; j < nrn_nlayer_extracellular; ++j) {
            ndlist[i]->extnode->v[j] = 0.;
        }
#if I_MEMBRANE
        ml->data(i, i_membrane_index) = 0.0;
#endif
    }
}

void extnode_free_elements(Extnode* nde) {
    if (nde->v) {
        free(nde->v);  /* along with _a and _b */
        free(nde->_d); /* along with _rhs, _a_matelm, _b_matelm, _x12, and _x21 */
        nde->v = NULL;
        nde->_a = NULL;
        nde->_b = NULL;
        nde->_d = NULL;
        nde->_rhs = NULL;
        nde->_a_matelm = NULL;
        nde->_b_matelm = NULL;
        nde->_x12 = NULL;
        nde->_x21 = NULL;
    }
}

static void check_if_extracellular_in_use() {
    hoc_Item* qsec;
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        if (sec->pnode[0]->extnode) {
            hoc_execerror("Cannot change nlayer_extracellular when instances exist", NULL);
        }
    }
}

static void update_existing_extnode(int old_nlayer) {
    /*
      This is a placeholder for a later extension to allow change
      of nlayer even if the extracellular mechanism has been instantiated
      in some sections. It is deferred for later because it is not clear
      that handling pointer updates to vext is straightforward.
      Hence the check_if_extracellular_in_use() function, which will
      be eliminated when this is filled in.
    */
}

static void update_extracellular_reg(int old_nlayer) {
    /* update hoc_symlist for arayinfo->sub[0] and u.rng.index. */
    int i, ix;
    Symbol* ecell = hoc_table_lookup("extracellular", hoc_built_in_symlist);
    assert(ecell);
    assert(ecell->type == MECHANISM);
    ix = 0;
    for (i = 0; i < ecell->s_varn; ++i) {
        Symbol* s = ecell->u.ppsym[i];
        if (s->type == RANGEVAR) {
            Arrayinfo* a = s->arayinfo;
            s->u.rng.index = ix;
            if (a && a->nsub == 1) {
                assert(a->sub[0] == old_nlayer);
                a->sub[0] = nlayer;
                ix += nlayer;
            } else {
                ix += 1;
            }
        }
    }
}

void nlayer_extracellular() {
    if (ifarg(1)) {
        /* Note in section.h: #define  nlayer (nrn_nlayer_extracellular) */
        int old = nlayer;
        nrn_nlayer_extracellular = (int) chkarg(1, 1., 1000.);
        if (nrn_nlayer_extracellular == old) {
            return;
        }
        check_if_extracellular_in_use();
        update_parmsize();
        /*global nlayer is the new value. Following needs to know the previous */
        update_extracellular_reg(old);
        update_existing_extnode(old);
    }
    hoc_retpushx((double) nlayer);
}

static void extnode_alloc_elements(Extnode* nde) {
    extnode_free_elements(nde);
    if (nlayer > 0) {
        nde->v = (double*) ecalloc(nlayer * 3, sizeof(double));
        nde->_a = nde->v + nlayer;
        nde->_b = nde->_a + nlayer;

        nde->_d = (double**) ecalloc(nlayer * 6, sizeof(double*));
        nde->_rhs = nde->_d + nlayer;
        nde->_a_matelm = nde->_rhs + nlayer;
        nde->_b_matelm = nde->_a_matelm + nlayer;
        nde->_x12 = nde->_b_matelm + nlayer;
        nde->_x21 = nde->_x12 + nlayer;
    }
}

void extcell_node_create(Node* nd) {
    int i, j;
    Extnode* nde;
    Prop* p;
    /* may be a nnode increase so some may already be allocated */
    if (!nd->extnode) {
        nde = new Extnode{};
        extnode_alloc_elements(nde);
        nd->extnode = nde;
        for (j = 0; j < nlayer; ++j) {
            nde->v[j] = 0.;
        }
        for (p = nd->prop; p; p = p->next) {
            if (p->_type == EXTRACELL) {
                for (auto i = 0; i < p->param_num_vars(); ++i) {
                    for (auto array_index = 0; array_index < p->param_array_dimension(i);
                         ++array_index) {
                        nde->param.push_back(p->param_handle(i, array_index));
                    }
                }
                break;
            }
        }
        assert(p && p->_type == EXTRACELL);
    }
}

void extcell_2d_alloc(Section* sec) {
    int i, j;
    Node* nd;
    Extnode* nde;
    Prop* p;
    /* may be a nnode increase so some may already be allocated */
    for (i = sec->nnode - 1; i >= 0; i--) {
        extcell_node_create(sec->pnode[i]);
    }
    /* if the rootnode is owned by this section then it gets extnode also*/
    if (!sec->parentsec && sec->parentnode) { /* last "if" clause unnecessary*/
        extcell_node_create(sec->parentnode);
    }
}

/* from treesetup.cpp */
void nrn_rhs_ext(NrnThread* _nt) {
    int i, j, cnt;
    Node *nd, *pnd, **ndlist;
    Extnode *nde, *pnde;
    Memb_list* ml = _nt->_ecell_memb_list;
    if (!ml) {
        return;
    }
    cnt = ml->nodecount;
    ndlist = ml->nodelist;

    /* nd rhs contains -membrane current + stim current */
    /* nde rhs contains stim current */
    for (i = 0; i < cnt; ++i) {
        nd = ndlist[i];
        nde = nd->extnode;
        *nde->_rhs[0] -= NODERHS(nd);
#if I_MEMBRANE
        ml->data(i, sav_rhs_index) = *nde->_rhs[0];
        /* and for daspk this is the ionic current which can be
           combined later with i_cap before return from solve. */
#endif
    }
    for (i = 0; i < cnt; ++i) {
        nd = ndlist[i];
        nde = nd->extnode;
        pnd = _nt->_v_parent[nd->v_node_index];
        if (pnd) {
            pnde = pnd->extnode;
            /* axial contributions */
            if (pnde) { /* parent sec may not be extracellular */
                for (j = 0; j < nrn_nlayer_extracellular; ++j) {
                    double dv = pnde->v[j] - nde->v[j];
                    *nde->_rhs[j] -= nde->_b[j] * dv;
                    *pnde->_rhs[j] += nde->_a[j] * dv;
                    /* for the internal balance equation
                    take care of vi = vm + vx */
                    if (j == 0) {
                        NODERHS(nd) -= NODEB(nd) * dv;
                        NODERHS(pnd) += NODEA(nd) * dv;
                    }
                }
            } else { /* no extracellular in parent but still need
                  to take care of vi = vm + vx in this node.
                  Note that we are NOT grounding the parent
                  side of xraxial so equivalent to sealed.
                  Also note that when children of this node
                  do not have extracellular, we deal with
                  that, at end of this function. */
                double dv = -nde->v[0];
                NODERHS(nd) -= NODEB(nd) * dv;
                NODERHS(pnd) += NODEA(nd) * dv;
            }

            /* series resistance and battery to ground */
            /* between nlayer-1 and ground */
            j = nrn_nlayer_extracellular - 1;
            *nde->_rhs[j] -= *nde->param[xg_index_ext(j)] *
                             (nde->v[j] - *nde->param[e_extracellular_index_ext()]);
            for (--j; j >= 0; --j) { /* between j and j+1 layer */
                double x = *nde->param[xg_index_ext(j)] * (nde->v[j] - nde->v[j + 1]);
                *nde->_rhs[j] -= x;
                *nde->_rhs[j + 1] += x;
            }
        }
    }
    cnt = _nt->_ecell_child_cnt;
    for (i = 0; i < cnt; ++i) {
        double dv;
        nd = _nt->_ecell_children[i];
        pnd = _nt->_v_parent[nd->v_node_index];
        dv = pnd->extnode->v[0];
        NODERHS(nd) -= NODEB(nd) * dv;
        NODERHS(pnd) += NODEA(nd) * dv;
    }
}

void nrn_setup_ext(NrnThread* _nt) {
    int i, j, cnt;
    Node *nd, *pnd, **ndlist;
    double* pd;
    double d, cfac, mfac;
    Extnode *nde, *pnde;
    Memb_list* ml = _nt->_ecell_memb_list;
    if (!ml) {
        return;
    }
    /*printnode("begin setup");*/
    cnt = ml->nodecount;
    ndlist = ml->nodelist;
    cfac = .001 * _nt->cj;

    /* d contains all the membrane conductances (and capacitance) */
    /* i.e. (cm/dt + di/dvm - dis/dvi)*[dvi] and
        (dis/dvi)*[dvx] */
    for (i = 0; i < cnt; ++i) {
        nd = ndlist[i];
        nde = nd->extnode;
        d = NODED(nd);
        /* nde->_d only has -ELECTRODE_CURRENT contribution */
        d = (*nde->_d[0] += NODED(nd));
        /* now d is only the membrane current contribution */
        /* i.e. d =  cm/dt + di/dvm */
        *nde->_x12[0] -= d;
        *nde->_x21[0] -= d;
#if I_MEMBRANE
        ml->data(i, sav_g_index) = d;
#endif
    }
    /* series resistance, capacitance, and axial terms. */
    for (i = 0; i < cnt; ++i) {
        nd = ndlist[i];
        nde = nd->extnode;
        pnd = _nt->_v_parent[nd->v_node_index];
        if (pnd) {
            /* series resistance and capacitance to ground */
            j = 0;
            for (;;) { /* between j and j+1 layer */
                mfac = (*nde->param[xg_index_ext(j)] + *nde->param[xc_index_ext(j)] * cfac);
                *nde->_d[j] += mfac;
                ++j;
                if (j == nrn_nlayer_extracellular) {
                    break;
                }
                *nde->_d[j] += mfac;
                *nde->_x12[j] -= mfac;
                *nde->_x21[j] -= mfac;
            }
            pnde = pnd->extnode;
            /* axial connections */
            if (pnde) { /* parent sec may not be extracellular */
                for (j = 0; j < nrn_nlayer_extracellular; ++j) {
                    *nde->_d[j] -= nde->_b[j];
                    *pnde->_d[j] -= nde->_a[j];
                    *nde->_a_matelm[j] += nde->_a[j];
                    *nde->_b_matelm[j] += nde->_b[j];
                }
            }
        }
    }
    /*printnode("end setup_lhs");*/
}

/* based on treeset.cpp */
void ext_con_coef(void) /* setup a and b */
{
    int j, k;
    double dx, area;
    hoc_Item* qsec;
    Node *nd, **pnd;
    Extnode* nde;
    double* pd;

    /* temporarily store half segment resistances in rhs */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        if (sec->pnode[0]->extnode) {
            dx = section_length(sec) / ((double) (sec->nnode - 1));
            for (j = 0; j < sec->nnode - 1; j++) {
                nde = sec->pnode[j]->extnode;
                for (k = 0; k < nrn_nlayer_extracellular; ++k) {
                    *nde->_rhs[k] = 1e-4 * *nde->param[xraxial_index_ext(k)] *
                                    (dx / 2.); /*Megohms*/
                }
            }
            /* last segment has 0 length. */
            nde = sec->pnode[j]->extnode;
            for (k = 0; k < nrn_nlayer_extracellular; ++k) {
                *nde->_rhs[k] = 0.;
                *nde->param[xc_index_ext(k)] = 0.;
                *nde->param[xg_index_ext(k)] = 0.;
            }
            /* if owns a rootnode */
            if (!sec->parentsec) {
                nde = sec->parentnode->extnode;
                for (k = 0; k < nrn_nlayer_extracellular; ++k) {
                    *nde->_rhs[k] = 0.;
                    *nde->param[xc_index_ext(k)] = 0.;
                    *nde->param[xg_index_ext(k)] = 0.;
                }
            }
        }
    }
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
        if (sec->pnode[0]->extnode) {
            /* node half resistances in general get added to the
            node and to the node's "child node in the same section".
            child nodes in different sections don't involve parent
            node's resistance */
            nde = sec->pnode[0]->extnode;
            for (k = 0; k < nlayer; ++k) {
                nde->_b[k] = *nde->_rhs[k];
            }
            for (j = 1; j < sec->nnode; j++) {
                nde = sec->pnode[j]->extnode;
                for (k = 0; k < nlayer; ++k) {
                    nde->_b[k] = *nde->_rhs[k] + *(sec->pnode[j - 1]->extnode->_rhs[k]); /*megohms*/
                }
            }
        }
    }
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        if (sec->pnode[0]->extnode) {
            /* convert to siemens/cm^2 for all nodes except last
            and microsiemens for last.  This means that a*V = mamps/cm2
            and a*v in last node = nanoamps. Note that last node
            has no membrane properties and no area. It may perhaps recieve
            current stimulus later */
            /* first the effect of node on parent equation. Note That
            last nodes have area = 1.e2 in dimensionless units so that
            last nodes have units of microsiemens's */
            pnd = sec->pnode;
            nde = pnd[0]->extnode;
            area = NODEAREA(sec->parentnode);
            /* param[4] is rall_branch */
            for (k = 0; k < nlayer; ++k) {
                nde->_a[k] = -1.e2 * sec->prop->dparam[4].get<double>() / (nde->_b[k] * area);
            }
            for (j = 1; j < sec->nnode; j++) {
                nde = pnd[j]->extnode;
                area = NODEAREA(pnd[j - 1]);
                for (k = 0; k < nlayer; ++k) {
                    nde->_a[k] = -1.e2 / (nde->_b[k] * area);
                }
            }
        }
    }
    /* now the effect of parent on node equation. */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        if (sec->pnode[0]->extnode) {
            for (j = 0; j < sec->nnode; j++) {
                nd = sec->pnode[j];
                nde = nd->extnode;
                for (k = 0; k < nlayer; ++k) {
                    nde->_b[k] = -1.e2 / (nde->_b[k] * NODEAREA(nd));
                }
            }
        }
    }
}

#if 0
/* needs to be fixed to deal with rootnodes having this property */

static void printnode(const char* s) {
	int in, i, j, k;
	hoc_Item* qsec;
	Section* sec;
	Node* nd;
	Extnode* nde;	
	double *pd;
	NrnThread* _nt;
	FOR_THREADS(_nt) for (in=0; in < _nt->end; ++in) {
		nd = _nt->_v_node[in];
		if (nd->extnode) {
			sec = nd->sec;
			j = nd->sec_node_index_;
			nde = nd->extnode;
			pd = nde->param;
			for (k=0; k < nlayer; ++k) {
printf("%s %s nd%d layer%d v=%g rhs=%g:\n", s, secname(sec), j, k, nde->v[k], *nde->_rhs[k]);
printf("xraxial=%g xg=%g xc=%g e=%g\n", xraxial[k], xg[k], xc[k], e_extracellular);
			}
		}
	}
}
#if 0
static int cntndsave;
static Extnode* ndesave;

void save2mat(void) {
	int i, j, k, im, ipm;
	register Node *nd, *pnd;
	register Extnode *nde, *pnde;
	
	if (cntndsave < v_node_count) {
		if (ndesave) {
			free(ndesave);
		}
		cntndsave = v_node_count;
		ndesave = (Extnode*)ecalloc(cntndsave, sizeof(Extnode));
	}
	for (i=0; i < v_node_count; ++i) {
		nd = v_node[i];
		nde = nd->extnode;
		if (nde) {
			for (j=0; j < nlayer; ++j) {
				ndesave[i].v[j] = nde->v[j];
				ndesave[i].rhs[j] = nde->_rhs[j];
				for (k=0; k < 2*(nlayer)+1; ++k) {
					ndesave[i].m[j][k] = nde->_m[j][k];
				}
				if (!v_parent[i]->extnode && j > 0) {
					ndesave[i].m[j][0] = 0.;
					ndesave[i].m[j][2*(nlayer)] = 0.;
				}
			}
		}else{
			for (j=0; j < nlayer; ++j) {
				ndesave[i].m[j][nlayer] = 1;
			}
			ndesave[i].v[0] = nd->v;
			ndesave[i].m[0][nlayer] = NODED(nd);
			ndesave[i].rhs[0] = NODERHS(nd);
			ndesave[i].m[0][0] = NODEB(nd);
			ndesave[i].m[0][2*(nlayer)] = NODEA(nd);
		}			
	}
}

#if 0
#define DBG printf
#else
DBG() {}
#endif

void check2mat(void) {
	int i, j, k, im, ip;
	Node* nd;
	Extnode* nde;
	double sum;

	/* copy solved v to saved v */
	for (i=0; i < v_node_count; ++ i) {
		nd = v_node[i];
		nde = nd->extnode;
		if (nde) {
			for (j=0; j < (nlayer); ++j) {
				ndesave[i].v[j] = nde->_rhs[j];
			}
		}else{
			ndesave[i].v[0] = NODERHS(nd);
		}
	}

#if 0
	printf("mat\n");
	for (i=0; i < v_node_count; ++i) {
		printf("node %d\n", i);
		for (j=0; j < (nlayer); ++j) {
			printf("%d %d  ", j, i);
			for (k=0; k <= 2*(nlayer); ++k) {
				printf(" %-8.3g", ndesave[i].m[j][k]);
			}
			printf(" %g  %g\n", ndesave[i].v[j], ndesave[i].rhs[j]);
		}
	}
#endif

	/* rhs - M*V accomplished by subtracting every term from rhs */
	for (i=0; i < v_node_count; ++i) {
DBG("work on node %d\n", i);
		if (v_parent[i]) {
			ip = v_parent[i]->v_node_index;
			/* effect of parent on node */
DBG(" effect of parent %d on node %d\n", ip, i);
			for (j=0; j < nlayer; ++j) {
DBG("  work on layer %d\n", j);
				for (k=j; k < nlayer; ++k) {
DBG("   nde[%d].rhs[%d] -= nde[%d].v[%d]*nde[%d].m[%d][%d]\n", i,j,ip,k,i,j,k-j);
DBG("                        %g * %g\n",ndesave[ip].v[k],ndesave[i].m[j][k-j]);
ndesave[i].rhs[j] -= ndesave[ip].v[k]*ndesave[i].m[j][k-j];
				}
			}
			/* effect of node on parent */
DBG(" effect of node %d on parent %d\n", i, ip);
			for (j=0; j < nlayer; ++j) {
DBG("  work on layer %d\n", j);
				for (k=0; k <= j; ++k) {
DBG("   nde[%d].rhs[%d] -= nde[%d].v[%d]*nde[%d].m[%d][%d]\n", ip,j,i,k,i,j,2*(nlayer)-j+k);
DBG("                        %g * %g\n",ndesave[i].v[k],ndesave[i].m[j][2*(nlayer)-j+k]);
ndesave[ip].rhs[j] -= ndesave[i].v[k]*ndesave[i].m[j][2*(nlayer)-j+k];
				}
			}
		}
		/* effect of node on node */
DBG(" effect of node %d on node %d\n", i, i);
		for (j=0; j < nlayer; ++j) {
DBG("  work on layer %d\n", j);
			for (k=0; k < nlayer; ++k) {
DBG("   nde[%d].rhs[%d] -= nde[%d].v[%d]*nde[%d].m[%d][%d]\n", i,j,i,k,i,j,(nlayer)+k-j);
DBG("                        %g * %g\n",ndesave[i].v[k],ndesave[i].m[j][(nlayer)+k-j]);
ndesave[i].rhs[j] -= ndesave[i].v[k]*ndesave[i].m[j][(nlayer)+k-j];
			}
		}
	}

	for (i=0; i < v_node_count; ++i) {
		for (j=0; j < (nlayer); ++j) {
			if (fabs(ndesave[i].rhs[j]) > 1e-5) {
				printf("bad soln of eq %d,%d rhs-M*V=%g\n",
				 i,j, ndesave[i].rhs[j]);
			}
		}
	}
}
#endif
#endif


#endif /*EXTRACELLULAR*/
