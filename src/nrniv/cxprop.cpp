#include <../../nrnconf.h>

/*
allocate and free property data and Datum arrays for nrniv
this allows for the possibility of
greater cache efficiency
*/

#include <stdio.h>
#include <stdlib.h>
#include <InterViews/resource.h>
#include <nrnmpi.h>
#include <nrnoc2iv.h>
#include <membfunc.h>
#include <arraypool.h>

extern "C" {
extern void nrn_mk_prop_pools(int);
extern void nrn_cache_prop_realloc();
}

#define APSIZE 1000
declareArrayPool(DoubleArrayPool, double)
implementArrayPool(DoubleArrayPool, double)
declareArrayPool(DatumArrayPool, Datum)
implementArrayPool(DatumArrayPool, Datum)

static int npools_;
static DoubleArrayPool** dblpools_;
static DatumArrayPool** datumpools_;

#define NRN_MECH_REORDER 0

#if NRN_MECH_REORDER
static int force = 0;
static long** order;
static long* i1;
static long* i2;
static void read_temp1() {
	return;
	FILE* f;
	int is, type, ix, n, io, t, i;
	char line[200];
	sprintf(line, "temp_%d", nrnmpi_myid);
	f = fopen(line, "r");
	if (!f) { return; }
	force = 1;
	order = (long**)hoc_Ecalloc(100, sizeof(long*));
	i1 = (long*)hoc_Ecalloc(100, sizeof(long));
	i2 = (long*)hoc_Ecalloc(100, sizeof(long));

	while (fgets(line, 200, f)) {
		is = sscanf(line, "%d %d %d", &type, &ix, &n);
		if (is != 3 || ix != -1) { break; }
//		printf("(%d %d) %s", type, n, line);
		order[type] = (long*)hoc_Ecalloc(APSIZE, sizeof(long));
		for (i=0; i < 1000; ++i) { order[type][i] = APSIZE - 1000 + i; }
		for (i=0; i < n; ++i) {
			fgets(line, 200, f);
			is = sscanf(line, "%d %d %d", &t, &ix, &io);
			assert(is == 3 && type == t);
			order[type][ix] = io;
		}
	}

	fclose(f);
}
#endif // NRN_MECH_REORDER

void nrn_mk_prop_pools(int n) {
#if NRN_MECH_REORDER
	if (force == 0) { read_temp1(); }
#endif
	int  i;
	if (n > npools_) {
		DoubleArrayPool** p1 = new DoubleArrayPool*[n];
		DatumArrayPool** p2 = new DatumArrayPool*[n];
		for (i=0; i < n; ++i) {
			p1[i] = 0;
			p2[i] = 0;
		}
		if (dblpools_) {
			for (i=0; i < npools_; ++i) {
				p1[i] = dblpools_[i];
				p2[i] = datumpools_[i];
			}
			delete [] dblpools_;
			delete [] datumpools_;
		}
		dblpools_ = p1;
		datumpools_ = p2;
		npools_ = n;
	}
}

double* nrn_prop_data_alloc(int type, int count) {
	if (!dblpools_[type]) {
		dblpools_[type] = new DoubleArrayPool(APSIZE, count);
	}
#if NRN_MECH_REORDER
	if (force && order[type]) { return dblpools_[type]->element(order[type][i1[type]++]); }
#endif
	double* pd = dblpools_[type]->alloc();
//if (type > 1) printf("nrn_prop_data_alloc %d %s %d %lx\n", type, memb_func[type].sym->name, count, (long)pd);
	return pd;
}

Datum* nrn_prop_datum_alloc(int type, int count) {
	int i;
	if (!datumpools_[type]) {
		datumpools_[type] = new DatumArrayPool(APSIZE, count);
	}
#if NRN_MECH_REORDER
	if (force && order[type]) {
		Datum* ppd = datumpools_[type]->element(order[type][i2[type]++]); }
//		for (i=0; i < count; ++i) { ppd[i]._pvoid = 0; }
		return ppd;
#endif
	Datum* ppd = datumpools_[type]->alloc();
//if (type > 1) printf("nrn_prop_datum_alloc %d %s %d %lx\n", type, memb_func[type].sym->name, count, (long)ppd);
	for (i=0; i < count; ++i) { ppd[i]._pvoid = 0; }
	return ppd;
}

void nrn_prop_data_free(int type, double* pd) {
//if (type > 1) printf("nrn_prop_data_free %d %s %lx\n", type, memb_func[type].sym->name, (long)pd);
#if NRN_MECH_REORDER
	if (force && order[type]) return;
#endif
	if (pd) {
		dblpools_[type]->hpfree(pd);
	}
}

void nrn_prop_datum_free(int type, Datum* ppd) {
//if (type > 1) printf("nrn_prop_datum_free %d %s %lx\n", type, memb_func[type].sym->name, (long)ppd);
#if NRN_MECH_REORDER
	if (force && order[type]) return;
#endif
	if (ppd) {
		datumpools_[type]->hpfree(ppd);
	}
}


void nrn_cache_prop_realloc() {
#if NRN_MECH_REORDER
	FILE* f;
	char buf[100];
	int i;
	// we wish to rearrange the arrays so that they are in memb_list->data order within
	// the ArrayPools. We do not want to use up a lot of temporary space to do this
	// because there may not be much left.
	// In each pool all the pointers between put and get (nget of them) are in use.
	if (force) {
		sprintf(buf, "temp2_%d", nrnmpi_myid);
	}else{
		sprintf(buf, "temp_%d", nrnmpi_myid);
	}
	f = fopen(buf, "w");
	for (i=CAP; i < n_memb_func; ++i) if (memb_list[i].nodecount) {
		double* p0 = dblpools_[i]->pool();
		int d2 = dblpools_[i]->d2();
		Memb_list* ml = memb_list + i;
		int j, cnt = ml->nodecount;
		fprintf(f, "%d %d %d %s\n", i, -1, cnt, memb_func[i].sym->name);
		for (j=0; j < cnt; ++j) {
			fprintf(f, "%d %d %d\n", i, (ml->data[j] - p0)/d2, j);
		}
	}
	fclose(f);
#endif
}
