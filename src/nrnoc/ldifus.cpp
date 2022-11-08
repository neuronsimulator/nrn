#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "section.h"
#include "membfunc.h"
#include "neuron.h"
#include "nrniv_mf.h"
#include "parse.hpp"


#define nt_t  nrn_threads->_t
#define nt_dt nrn_threads->_dt

struct LongDifus {
    int dchange;
    int* mindex; /* index into memb_list[m] */
    int* pindex; /* parent in this struct */
    double** state;
    double* a;
    double* b;
    double* d;
    double* rhs;
    double* af;  /* efficiency benefit is very low, rall/dx^2 */
    double* bf;  /* 1/dx^2 */
    double* vol; /* volatile volume from COMPARTMENT */
    double* dc;  /* volatile diffusion constant * cross sectional
                area from LONGITUDINAL_DIFFUSION */
};

struct LongDifusThreadData {
    int nthread;
    LongDifus** ldifus;
    Memb_list** ml;
};

static int ldifusfunccnt;
static ldifusfunc_t* ldifusfunc;

static ldifusfunc2_t stagger, ode, matsol, overall_setup;

void hoc_register_ldifus1(ldifusfunc_t f) {
    ldifusfunc = (ldifusfunc_t*) erealloc(ldifusfunc, (ldifusfunccnt + 1) * sizeof(ldifusfunc_t));
    ldifusfunc[ldifusfunccnt] = f;
    ++ldifusfunccnt;
}

extern "C" void nrn_tree_solve(double* a, double* d, double* b, double* rhs, int* pindex, int n) {
    /*
        treesolver

        a      - above the diagonal
        d      - diagonal
        b      - below the diagonal
        rhs    - right hand side, which is changed to the result
        pindex - parent indices
        n      - number of states
    */

    int i;

    /* triang */
    for (i = n - 1; i > 0; --i) {
        int pin = pindex[i];
        if (pin > -1) {
            double p;
            p = a[i] / d[i];
            d[pin] -= p * b[i];
            rhs[pin] -= p * rhs[i];
        }
    }
    /* bksub */
    for (i = 0; i < n; ++i) {
        int pin = pindex[i];
        if (pin > -1) {
            rhs[i] -= b[i] * rhs[pin];
        }
        rhs[i] /= d[i];
    }
}


void long_difus_solve(int method, NrnThread* nt) {
    ldifusfunc2_t* f{};
    if (ldifusfunc) {
        switch (method) {
        case 0: /* normal staggered time step */
            f = stagger;
            break;
        case 1: /* dstate = f(state) */
            f = ode;
            break;
        case 2: /* solve (1 + dt*jacobian)*x = b */
            f = matsol;
            break;
        case 3: /* setup only called by thread 0 */
            f = overall_setup;
            break;
        }
        assert(f);
        for (int i = 0; i < ldifusfunccnt; ++i) {
            ldifusfunc[i](f, nt);
        }
    }
}

static void longdifusfree(LongDifus** ppld) {
    if (*ppld) {
        LongDifus* pld = *ppld;
#if 0
printf("free longdifus structure_change=%d %d\n", pld->schange, structure_change_cnt);
#endif
        free(pld->mindex);
        free(pld->pindex);
        free(pld->state);
        free(pld->a);
        free(pld->b);
        free(pld->d);
        free(pld->rhs);
        free(pld->af);
        free(pld->bf);
        free(pld->vol);
        free(pld->dc);
        free(pld);
    }
    *ppld = (LongDifus*) 0;
}

static void longdifus_diamchange(LongDifus* pld, int m, int sindex, Memb_list* ml, NrnThread* _nt) {
    int i, n, mi, mpi, j, index, pindex, vnodecount;
    Node *nd, *pnd;
    double rall, dxp, dxc;

    if (pld->dchange == diam_change_cnt) {
        return;
    }
    /*printf("longdifus_diamchange %d %d\n", pld->dchange, diam_change_cnt);*/
    vnodecount = _nt->end;
    n = ml->nodecount;
    for (i = 0; i < n; ++i) {
        /* For every child with a parent having this mechanism */
        /* Also child may butte end to end with parent or attach to middle */
        mi = pld->mindex[i];
        if (sindex < 0) {
            pld->state[i] = ml->pdata[mi][-sindex - 1].get<double*>();
        } else {
            // With the new SOA format it's not possible to easily navigate from
            // the nth array element for a particular index to the mth array
            // element. That was trivial in AOS and led to usage like
            // pld->state[i][ai], but that should now be avoided.
            pld->state[i] = &ml->data(mi, sindex);
        }
        nd = ml->nodelist[mi];
        pindex = pld->pindex[i];
        if (pindex > -1) {
            mpi = pld->mindex[pindex];
            pnd = ml->nodelist[mpi];
            if (nd->sec_node_index_ == 0) {
                rall = nd->sec->prop->dparam[4].get<double>();
            } else {
                rall = 1.;
            }
            dxc = section_length(nd->sec) / ((double) (nd->sec->nnode - 1));
            dxp = section_length(pnd->sec) / ((double) (pnd->sec->nnode - 1));
            pld->af[i] = 2 * rall / dxp / (dxc + dxp);
            pld->bf[i] = 2 / dxc / (dxc + dxp);
        }
    }
    pld->dchange = diam_change_cnt;
}

static void longdifusalloc(LongDifus** ppld, int m, int sindex, Memb_list* ml, NrnThread* _nt) {
    LongDifus* pld;
    int i, n, mi, mpi, j, index, pindex, vnodecount;
    int *map, *omap;
    Node *nd, *pnd;
    hoc_Item* qsec;

    vnodecount = _nt->end;
    *ppld = pld = (LongDifus*) emalloc(sizeof(LongDifus));
    n = ml->nodecount;

    pld->dchange = 0;
    pld->mindex = (int*) ecalloc(n, sizeof(int));
    pld->pindex = (int*) ecalloc(n, sizeof(int));
    pld->state = (double**) ecalloc(n, sizeof(double*));
    pld->a = (double*) ecalloc(n, sizeof(double));
    pld->b = (double*) ecalloc(n, sizeof(double));
    pld->d = (double*) ecalloc(n, sizeof(double));
    pld->rhs = (double*) ecalloc(n, sizeof(double));
    pld->af = (double*) ecalloc(n, sizeof(double));
    pld->bf = (double*) ecalloc(n, sizeof(double));
    pld->vol = (double*) ecalloc(n, sizeof(double));
    pld->dc = (double*) ecalloc(n, sizeof(double));

    /* make a map from node_index to memb_list index. -1 means no exist*/
    map = (int*) ecalloc(vnodecount, sizeof(int));
    omap = (int*) ecalloc(n, sizeof(int));
    for (i = 0; i < vnodecount; ++i) {
        map[i] = -1;
    }
    for (i = 0; i < n; ++i) {
        map[ml->nodelist[i]->v_node_index] = i;
    }
#if 0
for (i=0; i < vnodecount; ++i) {
	printf("%d index=%d\n", i, map[i]);
}
#endif
    /* order the indices for efficient gaussian elimination */
    /* But watch out for 0 area nodes. Use the parent of parent */
    /* But if parent of parent does not have diffusion mechanism
       check the parent section */
    /* And watch out for root. Use first node of root section */
    for (i = 0, j = 0; i < vnodecount; ++i) {
        if (map[i] > -1) {
            pld->mindex[j] = map[i];
            omap[map[i]] = j; /* from memb list index to order */
            pnd = _nt->_v_parent[i];
            pindex = map[pnd->v_node_index];
            if (pindex == -1) { /* maybe this was zero area node */
                pnd = _nt->_v_parent[pnd->v_node_index];
                if (pnd) {
                    pindex = map[pnd->v_node_index];
                    if (pindex < 0) { /* but what about the	parent section */
                        Section* psec = _nt->_v_node[i]->sec->parentsec;
                        if (psec) {
                            pnd = psec->pnode[0];
                            pindex = map[pnd->v_node_index];
                        }
                    }
                } else { /* maybe this section is not the root */
                    Section* psec = _nt->_v_node[i]->sec->parentsec;
                    if (psec) {
                        pnd = psec->pnode[0];
                        pindex = map[pnd->v_node_index];
                    }
                }
            }
            if (pindex > -1) {
                pld->pindex[j] = omap[pindex];
            } else {
                pld->pindex[j] = -1;
            }
            ++j;
        }
    }
    longdifus_diamchange(pld, m, sindex, ml, _nt);
#if 0
	for (i=0; i < n; ++i) {
printf("i=%d pin=%d mi=%d :%s node %d state[(%i)]=%g\n", i, pld->pindex[i],
	pld->mindex[i], secname(ml->nodelist[pld->mindex[i]]->sec),
	ml->nodelist[pld->mindex[i]]->sec_node_index_
	, sindex, *pld->state[i]);
	}
#endif
    free(map);
    free(omap);
}

/* called at end of v_setup_vectors only for thread 0 */
/* only v makes sense and the purpose is to free the old, allocate space */
/* for the new, and setup the tml field */
/* the args used are m and v */
static void overall_setup(int m,
                          ldifusfunc3_t diffunc,
                          void** v,
                          int ai,
                          int sindex,
                          int dindex,
                          NrnThread* _nt) {
    int i;
    LongDifusThreadData** ppldtd = (LongDifusThreadData**) v;
    LongDifusThreadData* ldtd = *ppldtd;
    if (ldtd) { /* free the whole thing */
        free(ldtd->ml);
        for (i = 0; i < ldtd->nthread; ++i) {
            if (ldtd->ldifus[i]) {
                longdifusfree(ldtd->ldifus + i);
            }
        }
        free(ldtd->ldifus);
        free(ldtd);
        *ppldtd = (LongDifusThreadData*) 0;
    }
    /* new overall space */
    *ppldtd = (LongDifusThreadData*) emalloc(sizeof(LongDifusThreadData));
    ldtd = *ppldtd;
    ldtd->nthread = nrn_nthread;
    ldtd->ldifus = (LongDifus**) ecalloc(nrn_nthread, sizeof(LongDifus*));
    ldtd->ml = (Memb_list**) ecalloc(nrn_nthread, sizeof(Memb_list*));
    /* which have memb_list pointers */
    for (i = 0; i < nrn_nthread; ++i) {
        NrnThreadMembList* tml;
        for (tml = nrn_threads[i].tml; tml; tml = tml->next) {
            if (tml->index == m) {
                ldtd->ml[i] = tml->ml;
                longdifusalloc(ldtd->ldifus + i, m, sindex, tml->ml, nrn_threads + i);
                break;
            }
        }
    }
}

static LongDifus* v2ld(void** v, int tid) {
    LongDifusThreadData** ppldtd = (LongDifusThreadData**) v;
    return (*ppldtd)->ldifus[tid];
}

static Memb_list* v2ml(void** v, int tid) {
    LongDifusThreadData** ppldtd = (LongDifusThreadData**) v;
    return (*ppldtd)->ml[tid];
}

static void
stagger(int m, ldifusfunc3_t diffunc, void** v, int ai, int sindex, int dindex, NrnThread* _nt) {
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    LongDifus* const pld = v2ld(v, _nt->id);
    if (!pld) {
        return;
    }
    int const di = dindex + ai;
    auto* const ml = v2ml(v, _nt->id);
    int const n = ml->nodecount;
    Datum** const pdata = ml->pdata;
    Datum* const thread = ml->_thread;

    // with SOA data we can't get from the 0th element of an array variable
    // starting at sindex to the nth element, which used to be easy in AOS
    // format. try to mitigate that by baking in ai here and not applying it
    // again later
    longdifus_diamchange(pld, m, sindex > 0 ? sindex + ai : sindex, ml, _nt);
    /*flux and volume coefficients (if dc is constant this is too often)*/
    for (int i = 0; i < n; ++i) {
        int pin = pld->pindex[i];
        int mi = pld->mindex[i];
        double dfdi;
        pld->dc[i] = diffunc(ai, ml, mi, pdata[mi], pld->vol + i, &dfdi, thread, _nt);
        pld->d[i] = 0.;
        if (pin > -1) {
            /* D * area between compartments */
            double const dc = (pld->dc[i] + pld->dc[pin]) / 2.;
            pld->a[i] = -pld->af[i] * dc / pld->vol[pin];
            pld->b[i] = -pld->bf[i] * dc / pld->vol[i];
        }
    }
    /* setup matrix */
    for (int i = 0; i < n; ++i) {
        int pin = pld->pindex[i];
        int mi = pld->mindex[i];
        pld->d[i] += 1. / nt_dt;
        pld->rhs[i] = *pld->state[i] / nt_dt;
        if (pin > -1) {
            pld->d[i] -= pld->b[i];
            pld->d[pin] -= pld->a[i];
        }
    }

    /* we've set up the matrix; now solve it */
    nrn_tree_solve(pld->a, pld->d, pld->b, pld->rhs, pld->pindex, n);

    /* update answer */
    for (int i = 0; i < n; ++i) {
        *pld->state[i] = pld->rhs[i];
    }
}

static void
ode(int m, ldifusfunc3_t diffunc, void** v, int ai, int sindex, int dindex, NrnThread* _nt) {
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    LongDifus* const pld = v2ld(v, _nt->id);
    if (!pld) {
        return;
    }
    auto* const ml = v2ml(v, _nt->id);
    int const di = dindex + ai;
    int const n = ml->nodecount;
    Datum** const pdata = ml->pdata;
    Datum* const thread = ml->_thread;
    longdifus_diamchange(pld, m, sindex, ml, _nt);
    /*flux and volume coefficients (if dc is constant this is too often)*/
    for (int i = 0; i < n; ++i) {
        int pin = pld->pindex[i];
        int mi = pld->mindex[i];
        double dfdi;
        pld->dc[i] = diffunc(ai, ml, mi, pdata[mi], pld->vol + i, &dfdi, thread, _nt);
        if (pin > -1) {
            /* D * area between compartments */
            double const dc = (pld->dc[i] + pld->dc[pin]) / 2.;
            pld->a[i] = pld->af[i] * dc / pld->vol[pin];
            pld->b[i] = pld->bf[i] * dc / pld->vol[i];
        }
    }
    /* add terms to diffeq */
    for (int i = 0; i < n; ++i) {
        double dif;
        int pin = pld->pindex[i];
        int mi = pld->mindex[i];
        if (pin > -1) {
            dif = (pld->state[pin][ai] - pld->state[i][ai]);
            ml->data(mi, di) += dif * pld->b[i];
            ml->data(pld->mindex[pin], di) -= dif * pld->a[i];
        }
    }
}

static void
matsol(int m, ldifusfunc3_t diffunc, void** v, int ai, int sindex, int dindex, NrnThread* _nt) {
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    LongDifus* const pld = v2ld(v, _nt->id);
    if (!pld) {
        return;
    }
    auto* const ml = v2ml(v, _nt->id);
    auto const di = dindex + ai;
    int const n = ml->nodecount;
    Datum** const pdata = ml->pdata;
    Datum* const thread = ml->_thread;

    /*flux and volume coefficients (if dc is constant this is too often)*/
    for (int i = 0; i < n; ++i) {
        int pin = pld->pindex[i];
        int mi = pld->mindex[i];
        double dfdi;
        pld->dc[i] = diffunc(ai, ml, mi, pdata[mi], pld->vol + i, &dfdi, thread, _nt);
        pld->d[i] = 0.;
        if (dfdi) {
            pld->d[i] += fabs(dfdi) / pld->vol[i] / pld->state[i][ai];
        }
        if (pin > -1) {
            /* D * area between compartments */
            auto const dc = (pld->dc[i] + pld->dc[pin]) / 2.;
            pld->a[i] = -pld->af[i] * dc / pld->vol[pin];
            pld->b[i] = -pld->bf[i] * dc / pld->vol[i];
        }
    }
    /* setup matrix */
    for (int i = 0; i < n; ++i) {
        int pin = pld->pindex[i];
        int mi = pld->mindex[i];
        pld->d[i] += 1. / nt_dt;
        pld->rhs[i] = ml->data(mi, di) / nt_dt;
        if (pin > -1) {
            pld->d[i] -= pld->b[i];
            pld->d[pin] -= pld->a[i];
        }
    }
    /* triang */
    for (int i = n - 1; i > 0; --i) {
        int pin = pld->pindex[i];
        if (pin > -1) {
            double p;
            p = pld->a[i] / pld->d[i];
            pld->d[pin] -= p * pld->b[i];
            pld->rhs[pin] -= p * pld->rhs[i];
        }
    }
    /* bksub */
    for (int i = 0; i < n; ++i) {
        int pin = pld->pindex[i];
        if (pin > -1) {
            pld->rhs[i] -= pld->b[i] * pld->rhs[pin];
        }
        pld->rhs[i] /= pld->d[i];
    }
    /* update answer */
    for (int i = 0; i < n; ++i) {
        int mi = pld->mindex[i];
        ml->data(mi, di) = pld->rhs[i];
    }
}
