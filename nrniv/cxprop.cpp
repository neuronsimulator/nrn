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
#include <nrnmenu.h>
#include <arraypool.h>

extern "C" {
extern void nrn_mk_prop_pools(int);
extern void nrn_cache_prop_realloc();
extern int nrn_is_ion(int);
void nrn_update_ion_pointer(Symbol* sion, Datum* dp, int id, int ip);

void* nrn_pool_create(long count, int itemsize);
void nrn_pool_delete(void* pool);
void* nrn_pool_alloc(void* pool);
void nrn_pool_free(void* pool, void* item);
void nrn_pool_freeall(void* pool);

}

declareArrayPool(CharArrayPool, char)
implementArrayPool(CharArrayPool, char)

#define APSIZE 1000
declareArrayPool(DoubleArrayPool, double)
implementArrayPool(DoubleArrayPool, double)
declareArrayPool(DatumArrayPool, Datum)
implementArrayPool(DatumArrayPool, Datum)

static int force;
static int npools_;
static DoubleArrayPool** dblpools_;
static DatumArrayPool** datumpools_;
static void mk_prop_pools(int n);

#define NRN_MECH_REORDER 1

/*
Based on the nrn_threads tml->ml->nodelist order from the last
call to nrn_cache_prop_realloc() (which writes a data file) from
a previous launch, on this launch, read that file and create
pools matched to the space needed by each thread and which, from
the sequence of data allocation requests returns space that ends
up being laid out in memory in just the way we want.
The data file format is
maxtype // so we can be sure we have large enough npools_
nthread // number of threads
nmech // number of mechanisms used in this thread
type sz1 sz2 ntget cnt // mechanism type, double/Datum array size total number of times alloc was called, how many needed for this thread
// the above specifies the pool allocation
// note that the pool chain order is the same as the thread order
// there are nthread of the following lists
cnt // number of mechanisms in thread
type i seq // i is the tml->ml->data[i], seq is the allocation order
// ie we want

Note that the overall memory allocation sequence has to be identical
to the original sequence in terms of get/put for the final
layout to be exactly right for cache efficiency and for threads not
to share cache lines. However, if this is not the case, the memory allocation
is still correct, just not as efficient.
*/

// Alex: don't yet know if the reorder stuff is a useful analogy
// probably not.
#if NRN_MECH_REORDER

static void read_temp1() {
	// return;
	FILE* f;
	int nscan, maxtype, imech, nmech, type, sz1, sz2, ntget, ith, nth, i, j, cnt, seq;
	char line[200];
	sprintf(line, "temp_%d_%d", nrnmpi_myid, nrnmpi_numprocs);
	f = fopen(line, "r");
	if (!f) { return; }
	force = 1;
	assert(fgets(line, 200, f)); assert(sscanf(line, "%d", &maxtype) == 1);
	mk_prop_pools(maxtype);
	long* ntget1 = new long[maxtype];
	for (i=0; i < maxtype; ++i) { ntget1[i] = 0; }

	// allocate the pool space
	assert(fgets(line, 200, f)); assert(sscanf(line, "%d", &nth) == 1);
	for (ith=0; ith < nth; ++ith) {
		assert(fgets(line, 200, f)); assert(sscanf(line, "%d", &nmech) == 1);
		for (imech=0; imech < nmech; ++imech) {
			assert(fgets(line, 200, f));
assert(sscanf(line, "%d %d %d %d %d", &type, &sz1, &sz2, &ntget, &cnt) == 5);
//		printf("(%d %d %d %d %d) %s", type, sz1, sz2, ntget, cnt, line);
			ntget1[type] = ntget;
			if (sz1) {
				if (!dblpools_[type]) {
					dblpools_[type] = new DoubleArrayPool(cnt, sz1);
				}else{
					dblpools_[type]->grow(cnt);
				}
			}
			if (sz2) {
				if (!datumpools_[type]) {
					datumpools_[type] = new DatumArrayPool(cnt, sz2);
				}else{
					datumpools_[type]->grow(cnt);
				}
			}
		}	
	}
	for (i = 0; i < maxtype; ++i) {
		if (dblpools_[i] && dblpools_[i]->size() < ntget1[i]) {
			dblpools_[i]->grow(ntget1[i] - dblpools_[i]->size());
		}
		if (datumpools_[i] && datumpools_[i]->size() < ntget1[i]) {
			datumpools_[i]->grow(ntget1[i] - datumpools_[i]->size());
		}
	}
	delete [] ntget1;

	// now the tricky part, put items in an unnatural order
	// first set all pointers to 0
	for (i = 0; i < maxtype; ++i) {
		if (dblpools_[i]) {
			double** items = dblpools_[i]->items();
			int sz = dblpools_[i]->size();
			for (int j=0; j < sz; ++j) {
				items[j] = 0;
			}
		}
		if (datumpools_[i]) {
			Datum** items = datumpools_[i]->items();
			int sz = datumpools_[i]->size();
			for (int j=0; j < sz; ++j) {
				items[j] = 0;
			}
		}
	}

	// then set the proper seq pointers
	DoubleArrayPool** p1 = new DoubleArrayPool*[npools_];
	DatumArrayPool** p2 = new DatumArrayPool*[npools_];
	int* chain = new int[npools_];
	for (i=0; i < npools_; ++i) {
		p1[i] = dblpools_[i];
		p2[i] = datumpools_[i];
		chain[i] = 0;
	}
	for (ith = 0; ith < nth; ++ith) {
		assert(fgets(line, 200, f)); assert(sscanf(line, "%d", &cnt) == 1);
		for (i=0; i < cnt; ++i) {
			assert(fgets(line, 200, f));
			assert(sscanf(line, "%d %d %d", &type, &j, &seq));
			if (dblpools_[type]) {
				double** items = dblpools_[type]->items();
				assert(items[seq] == 0);
				items[seq] = p1[type]->element(j);
				++chain[type];
			}
			if (datumpools_[type]) {
				Datum** items = datumpools_[type]->items();
				assert(items[seq] == 0);
				items[seq] = p2[type]->element(j);
				++chain[type];
			}
		}
		for (i=0; i < npools_; ++i) {
			if (chain[i]) {
				if (p1[i] && p2[i]) {
					assert(chain[i] == (p1[i]->chain_size() + p2[i]->chain_size()));
				}else if (p1[i]) {
					assert(chain[i] == p1[i]->chain_size());
				}else if (p2[i]) {
					assert(chain[i] == p2[i]->chain_size());
				}
				if (p1[i]) {p1[i] = p1[i]->chain();}
				if (p2[i]) {p2[i] = p2[i]->chain();}
				chain[i] = 0;
			}
		}
	}
	// finally set the rest
	for (i=0; i < npools_; ++i) {
		if (p1[i]) {
			int j = 0;
			int k = 0;
			int n = dblpools_[i]->size();
			int sz = p1[i]->chain_size();
			double** items = dblpools_[i]->items();
			for (j = 0; j < n; ++j) {
				if (items[j] == 0) {
					assert(k < sz);
					items[j] = p1[i]->element(k);
					++k;
				}
			}
			assert(k == sz);
		}
		if (p2[i]) {
			int j = 0;
			int k = 0;
			int n = datumpools_[i]->size();
			int sz = p2[i]->chain_size();
			Datum** items = datumpools_[i]->items();
			for (j = 0; j < n; ++j) {
				if (items[j] == 0) {
					assert(k < sz);
					items[j] = p2[i]->element(k);
					++k;
				}
			}
			assert(k == sz);
		}
	}
	delete [] p1;
	delete [] p2;
	fclose(f);
}
#endif // NRN_MECH_REORDER

static void mk_prop_pools(int n) {
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

void nrn_mk_prop_pools(int n) {
#if NRN_MECH_REORDER
	if (force == 0) { read_temp1(); }
#endif
	mk_prop_pools(n);
}

double* nrn_prop_data_alloc(int type, int count, Prop* p) {
	if (!dblpools_[type]) {
		dblpools_[type] = new DoubleArrayPool(APSIZE, count);
	}
	assert(dblpools_[type]->d2() == count);
	p->_alloc_seq = dblpools_[type]->ntget();
	double* pd = dblpools_[type]->alloc();
//if (type > 1) printf("nrn_prop_data_alloc %d %s %d %p\n", type, memb_func[type].sym->name, count, pd);
	return pd;
}

Datum* nrn_prop_datum_alloc(int type, int count, Prop* p) {
	int i;
	Datum* ppd;
	if (!datumpools_[type]) {
		datumpools_[type] = new DatumArrayPool(APSIZE, count);
	}
	assert(datumpools_[type]->d2() == count);
	p->_alloc_seq = datumpools_[type]->ntget();
	ppd = datumpools_[type]->alloc();
//if (type > 1) printf("nrn_prop_datum_alloc %d %s %d %p\n", type, memb_func[type].sym->name, count, ppd);
	for (i=0; i < count; ++i) { ppd[i]._pvoid = 0; }
	return ppd;
}

void nrn_prop_data_free(int type, double* pd) {
//if (type > 1) printf("nrn_prop_data_free %d %s %p\n", type, memb_func[type].sym->name, pd);
	if (pd) {
		dblpools_[type]->hpfree(pd);
	}
}

void nrn_prop_datum_free(int type, Datum* ppd) {
//if (type > 1) printf("nrn_prop_datum_free %d %s %p\n", type, memb_func[type].sym->name, ppd);
	if (ppd) {
		datumpools_[type]->hpfree(ppd);
	}
}


int nrn_prop_is_cache_efficient() {
	DoubleArrayPool** p = new DoubleArrayPool*[npools_];
	int r = 1;
	for (int i=0; i < npools_; ++i) {
		p[i] = dblpools_[i];
	}
	for (int it = 0; it < nrn_nthread; ++it) {
		NrnThread* nt = nrn_threads + it;
		for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
			Memb_list* ml = tml->ml;
			int i = tml->index;
		    if (ml->nodecount > 0) {
			if (!p[i]) {
//printf("thread %d mechanism %s pool chain does not exist\n", it, memb_func[i].sym->name);
				r = 0;
				continue;
			}
			if (p[i]->chain_size() != ml->nodecount) {
//printf("thread %d mechanism %s chain_size %d nodecount=%d\n", it, memb_func[i].sym->name, p[i]->chain_size(), ml->nodecount);
				r = 0;
				p[i] = p[i]->chain();
				continue;
			}
			for (int j = 0; j < ml->nodecount; ++j) {
				if (p[i]->element(j) != ml->data[j]) {
//printf("thread %d mechanism %s instance %d  element %p data %p\n",
//it, memb_func[i].sym->name, j, p[i]->element(j), (ml->data + j));
					r = 0;
				}
			}
			p[i] = p[i]->chain();
		    }
		}
	}
	delete [] p;
	return r;
}


// for avoiding interthread cache line sharing
// each thread needs its own pool instance
void* nrn_pool_create(long count, int itemsize) {
	CharArrayPool* p = new CharArrayPool(count, itemsize);
	return (void*)p;
}
void nrn_pool_delete(void* pool) {
	CharArrayPool* p = (CharArrayPool*)pool;
	delete p;
}
void* nrn_pool_alloc(void* pool) {
	CharArrayPool* p = (CharArrayPool*)pool;
	return (void*)p->alloc();
}
void nrn_pool_free(void* pool, void* item) {
	CharArrayPool* p = (CharArrayPool*)pool;
	p->hpfree((char*)item);
}
void nrn_pool_freeall(void* pool) {
	CharArrayPool* p = (CharArrayPool*)pool;
	p->free_all();
}

