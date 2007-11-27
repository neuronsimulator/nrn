#include <../../nrnconf.h>
#include        <stdio.h>
#include        <errno.h>
#include        <math.h>
#include        "section.h"
#include        "membfunc.h"
#include        "neuron.h"
#include        "parse.h"

extern char* secname(), *hoc_Erealloc(); 
extern int diam_change_cnt;
extern int structure_change_cnt;

typedef struct LongDifus {
	int schange;
	int* mindex;	/* index into memb_list[m] */
	int* pindex;	/* parent in this struct */
	double** state;
	double* a;
	double* b;
	double* d;
	double* rhs;
	double* af;	/* efficiency benefit is very low, rall/dx^2 */
	double* bf;	/* 1/dx^2 */
	double* vol;	/* volatile volume from COMPARTMENT */
	double* dc;	/* volatile diffusion constant * cross sectional
			   area from LONGITUDINAL_DIFFUSION */
}LongDifus;

typedef void (*Pfrv)();

static int ldifusfunccnt;
static Pfrv* ldifusfunc;
static void stagger(), ode(), matsol();

hoc_register_ldifus1( f ) Pfrv f; {
	ldifusfunc = (Pfrv*)erealloc(ldifusfunc, (ldifusfunccnt + 1)*sizeof(Pfrv) );
	ldifusfunc[ldifusfunccnt] = f;	
	++ldifusfunccnt;
}

#if MAC
/* this avoids a missing _ptrgl12 in the mac library that was called by
the MrC compiled object
*/
void mac_difusfunc(f, m, diffunc, v, ai, sindex, dindex)
	Pfrv f;
	int m;
	double (*diffunc)();
	void** v;
	int ai, sindex, dindex;
{
	(*f)(m, diffunc, v, ai, sindex, dindex);
}
#endif

long_difus_solve(method) int method; {
	Pfrv f;
	int i;
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
		}
		assert(f);
		
		for (i=0; i < ldifusfunccnt; ++i) {
			(*ldifusfunc[i])(f);
		}
	}
}	

static longdifusfree(ppld) LongDifus** ppld; {
	LongDifus* pld;
	
	if (*ppld) {
		pld = *ppld;
#if 0
printf("free longdifus structure_change=%d %d\n", pld->schange, structure_change_cnt);
#endif
		free (pld->mindex);
		free (pld->pindex);
		free (pld->state);
		free (pld->a);
		free (pld->b);
		free (pld->d);
		free (pld->rhs);
		free (pld->af);
		free (pld->bf);
		free (pld->vol);
		free (pld->dc);
		free(pld);
	}
	*ppld = (LongDifus*)0;
}

static longdifusalloc(ppld, m, sindex)
 LongDifus** ppld; int m, sindex;
{
	LongDifus* pld;
	int i, n, mi, mpi, j, index, pindex;
	int* map, *omap;
	Node* nd, *pnd;
	hoc_Item* qsec;
	double rall, dxp, dxc;
	
	*ppld = pld = (LongDifus*)emalloc(sizeof(LongDifus));
	n = memb_list[m].nodecount;

	pld->mindex = (int*)ecalloc(n, sizeof(int));
	pld->pindex = (int*)ecalloc(n, sizeof(int));
	pld->state = (double**)ecalloc(n, sizeof(double*));
	pld->a = (double*)ecalloc(n, sizeof(double));
	pld->b = (double*)ecalloc(n, sizeof(double));
	pld->d = (double*)ecalloc(n, sizeof(double));
	pld->rhs = (double*)ecalloc(n, sizeof(double));
	pld->af = (double*)ecalloc(n, sizeof(double));
	pld->bf = (double*)ecalloc(n, sizeof(double));
	pld->vol = (double*)ecalloc(n, sizeof(double));
	pld->dc = (double*)ecalloc(n, sizeof(double));

	/* make a map from node_index to memb_list index. -1 means no exist*/
	map = (int*)ecalloc(v_node_count, sizeof(int));
	omap = (int*)ecalloc(n, sizeof(int));
	for (i=0; i < v_node_count; ++i) {
		map[i] = -1;
	}
	for (i=0; i < n; ++i) {
		map[memb_list[m].nodelist[i]->v_node_index] = i;
	}
#if 0
for (i=0; i < v_node_count; ++i) {
	printf("%d index=%d\n", i, map[i]);
}
#endif
	/* order the indices for efficient gaussian elimination */
	/* But watch out for 0 area nodes. Use the parent of parent */
	/* But if parent of parent does not have diffusion mechanism
	   check the parent section */
	/* And watch out for root. Use first node of root section */
	for (i=0, j=0; i < v_node_count; ++i) {
		if (map[i] > -1) {
			pld->mindex[j] = map[i];
			omap[map[i]] = j; /* from memb list index to order */
			pnd = v_parent[i];
			pindex = map[pnd->v_node_index];
			if (pindex == -1) {/* maybe this was zero area node */
				pnd = v_parent[pnd->v_node_index];
				if (pnd) {
					pindex = map[pnd->v_node_index];
					if (pindex < 0) { /* but what about the	parent section */
						Section* psec = v_node[i]->sec->parentsec;
						if (psec) {
							pnd = psec->pnode[0];
							pindex = map[pnd->v_node_index];
						}
					}
				}else{ /* maybe this section is not the root */
					Section* psec = v_node[i]->sec->parentsec;
					if (psec) {
						pnd = psec->pnode[0];
						pindex = map[pnd->v_node_index];
					}
				}
			}
			if (pindex > -1) {
				pld->pindex[j] = omap[pindex];
			}else{
				pld->pindex[j] = -1;
			}
			++j;
		}
	}
	for (i=0; i < n; ++i) {
	/* For every child with a parent having this mechanism */
	/* Also child may butte end to end with parent or attach to middle */
		mi = pld->mindex[i];
		if (sindex < 0) {
			pld->state[i] = memb_list[m].pdata[mi][-sindex - 1].pval;
		}else{
			pld->state[i] = memb_list[m].data[mi] + sindex;
		}
		nd = memb_list[m].nodelist[mi];
		pindex = pld->pindex[i];
		if (pindex > -1) {
			mpi = pld->mindex[pindex];
			pnd = memb_list[m].nodelist[mpi];
			if (nd->sec_node_index_ == 0) {
				rall = nd->sec->prop->dparam[4].val;
			}else{
				rall = 1.;
			}
			dxc = section_length(nd->sec)
					/ ((double)(nd->sec->nnode - 1));
			dxp = section_length(pnd->sec)
					/ ((double)(pnd->sec->nnode - 1));
			pld->af[i] = 2*rall/dxp/(dxc + dxp);
			pld->bf[i] = 2/dxc/(dxc + dxp);
		}
	}
#if 0
	for (i=0; i < n; ++i) {
printf("i=%d pin=%d mi=%d :%s node %d state[(%i)]=%g\n", i, pld->pindex[i],
	pld->mindex[i], secname(memb_list[m].nodelist[pld->mindex[i]]->sec),
	memb_list[m].nodelist[pld->mindex[i]]->sec_node_index_
	, sindex, pld->state[i][0]);
	}
#endif
	free(map);
	free(omap);
}

static LongDifus* check(v, m, sindex) void** v; int m, sindex; {
	LongDifus** ppld, *pld;
	if (memb_list[m].nodecount == 0) {
		return (LongDifus*)0;
	}
	ppld = (LongDifus**)v;
	pld = *ppld;
	/* watch out for diam and structure change */
	if (!pld || pld->schange != structure_change_cnt) {
		longdifusfree(ppld);
		longdifusalloc(ppld, m, sindex);
		pld = *ppld;
		pld->schange = structure_change_cnt;
	}
	return pld;
}

static void stagger(m, diffunc, v, ai, sindex, dindex)
	int m;
	double (*diffunc)();
	void** v;
	int ai, sindex, dindex;
{
	LongDifus* pld;
	int i, n, di;
	double dc, vol, dfdi, dx;
	double** data;
	Datum** pdata;

	di = dindex + ai;

	pld = check(v, m, sindex);
	if(!pld) return;

	n = memb_list[m].nodecount;
	data = memb_list[m].data;
	pdata = memb_list[m].pdata;

	/*flux and volume coefficients (if dc is constant this is too often)*/
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		int mi = pld->mindex[i];
		pld->dc[i] = (*diffunc)(ai, data[mi], pdata[mi], pld->vol+i, &dfdi);
		pld->d[i] = 0.;
#if 0
		if (dfdi) {
			pld->d[i] += fabs(dfdi)/pld->vol[i]/pld->state[i][ai];
		}
#endif
		if (pin > -1) {
			/* D * area between compartments */
			dc = (pld->dc[i] + pld->dc[pin])/2.;

			pld->a[i] = -pld->af[i] * dc / pld->vol[pin];
			pld->b[i] = -pld->bf[i] * dc / pld->vol[i];
		}
	}
	/* setup matrix */
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		int mi = pld->mindex[i];
		pld->d[i] += 1./dt;
		pld->rhs[i] = pld->state[i][ai]/dt;
		if (pin > -1) {
			pld->d[i] -= pld->b[i];
			pld->d[pin] -= pld->a[i];
		}
	}
#if 0
for (i=0; i < n; ++i) { double a,b;
	if (pld->pindex[i] > -1) {
		a = pld->a[i];
		b = pld->b[i];
	}else{ a=b=0.;}
	printf("i=%d a=%g b=%g d=%g rhs=%g state=%g\n",
	 i, a, b, pld->d[i], pld->rhs[i], pld->state[i][ai]);
}
#endif
	/* triang */
	for (i = n - 1; i > 0; --i) {
		int pin = pld->pindex[i];
		if (pin > -1) {
			double p;
			p = pld->a[i] / pld->d[i];
			pld->d[pin] -= p * pld->b[i];
			pld->rhs[pin] -= p * pld->rhs[i];
		}
	}
	/* bksub */
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		if (pin > -1) {
			pld->rhs[i] -= pld->b[i] * pld->rhs[pin];
		}
		pld->rhs[i] /= pld->d[i];
	}
	/* update answer */
	for (i=0; i < n; ++i) {
		pld->state[i][ai] = pld->rhs[i];
	}
}

static void ode(m, diffunc, v, ai, sindex, dindex)
	int m;
	double (*diffunc)();
	void** v;
	int ai, sindex, dindex;
{
	LongDifus* pld;
	int i, n, di;
	double dc, vol, dfdi;
	double** data;
	Datum** pdata;

	di = dindex + ai;

	pld = check(v, m, sindex);
	if(!pld) return;

	n = memb_list[m].nodecount;
	data = memb_list[m].data;
	pdata = memb_list[m].pdata;
	
	/*flux and volume coefficients (if dc is constant this is too often)*/
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		int mi = pld->mindex[i];
		pld->dc[i] = (*diffunc)(ai, data[mi], pdata[mi], pld->vol+i, &dfdi);
		if (pin > -1) {
			/* D * area between compartments */
			dc = (pld->dc[i] + pld->dc[pin])/2.;

			pld->a[i] = pld->af[i] * dc / pld->vol[pin];
			pld->b[i] = pld->bf[i] * dc / pld->vol[i];
		}
	}
	/* add terms to diffeq */
	for (i=0; i < n; ++i) {
		double dif;
		int pin = pld->pindex[i];
		int mi = pld->mindex[i];
#if 0
		pld->d[i] = data[mi][di];
#endif
		if (pin > -1) {
			dif = (pld->state[pin][ai]  - pld->state[i][ai]);
			data[mi][di] += dif*pld->b[i];
			data[pld->mindex[pin]][di] -= dif*pld->a[i];
		}
	}
#if 0
	for (i=0; i < n; ++i) {
		int mi = pld->mindex[i];
		printf("%d olddstate=%g new=%g\n", i, pld->d[i], data[mi][di]);
	}
#endif
}


static void matsol(m, diffunc, v, ai, sindex, dindex)
	int m;
	double (*diffunc)();
	void** v;
	int ai, sindex, dindex;
{
	LongDifus* pld;
	int i, n, di;
	double dc, vol, dfdi;
	double** data;
	Datum** pdata;

	di = dindex + ai;

	pld = check(v, m, sindex);
	if(!pld) return;

	n = memb_list[m].nodecount;
	data = memb_list[m].data;
	pdata = memb_list[m].pdata;
	
	/*flux and volume coefficients (if dc is constant this is too often)*/
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		int mi = pld->mindex[i];
		pld->dc[i] = (*diffunc)(ai, data[mi], pdata[mi], pld->vol+i, &dfdi);
		pld->d[i] = 0.;
		if (dfdi) {
			pld->d[i] += fabs(dfdi)/pld->vol[i]/pld->state[i][ai];
#if 0
printf("i=%d state=%g vol=%g dfdc=%g\n", i, pld->state[i][ai],pld->vol[i], pld->d[i]);
#endif
		}
		if (pin > -1) {
			/* D * area between compartments */
			dc = (pld->dc[i] + pld->dc[pin])/2.;

			pld->a[i] = -pld->af[i] * dc / pld->vol[pin];
			pld->b[i] = -pld->bf[i] * dc / pld->vol[i];
		}
	}
	/* setup matrix */
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		int mi = pld->mindex[i];
		pld->d[i] += 1./dt;
		pld->rhs[i] = data[mi][di]/dt;
		if (pin > -1) {
			pld->d[i] -= pld->b[i];
			pld->d[pin] -= pld->a[i];
		}
	}
#if 0
for (i=0; i < n; ++i) { double a,b;
	int mi = pld->mindex[i];
	if (pld->pindex[i] > -1) {
		a = pld->a[i];
		b = pld->b[i];
	}else{ a=b=0.;}
	printf("i=%d a=%g b=%g d=%g rhs=%g dstate=%g\n",
	 i, a, b, pld->d[i], pld->rhs[i], data[mi][di]);
}
#endif
	/* triang */
	for (i = n - 1; i > 0; --i) {
		int pin = pld->pindex[i];
		if (pin > -1) {
			double p;
			p = pld->a[i] / pld->d[i];
			pld->d[pin] -= p * pld->b[i];
			pld->rhs[pin] -= p * pld->rhs[i];
		}
	}
	/* bksub */
	for (i=0; i < n; ++i) {
		int pin = pld->pindex[i];
		if (pin > -1) {
			pld->rhs[i] -= pld->b[i] * pld->rhs[pin];
		}
		pld->rhs[i] /= pld->d[i];
	}
	/* update answer */
	for (i=0; i < n; ++i) {
		int mi = pld->mindex[i];
		data[mi][di] = pld->rhs[i];
	}
}


