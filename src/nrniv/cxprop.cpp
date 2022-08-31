#include <../../nrnconf.h>

/*
allocate and free property data and Datum arrays for nrniv
this allows for the possibility of
greater cache efficiency
*/

#include <stdio.h>
#include <stdlib.h>
#include <InterViews/resource.h>
#include "nrniv_mf.h"
#include <nrnmpi.h>
#include <nrnoc2iv.h>
#include <membfunc.h>
#include <nrnmenu.h>
#include <arraypool.h>
#include <structpool.h>

extern void nrn_mk_prop_pools(int);
extern void nrn_cache_prop_realloc();
extern int nrn_is_ion(int);
void nrn_delete_prop_pool(int type);
#if EXTRACELLULAR
extern void nrn_extcell_update_param();
#endif
extern void nrn_recalc_ptrs(double* (*) (double*) );
static double* recalc_ptr(double*);

using CharArrayPool = ArrayPool<char>;

#define APSIZE 1000
using DoubleArrayPool = ArrayPool<double>;
using DatumArrayPool = ArrayPool<Datum>;

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
type sz1 sz2 ntget cnt // mechanism type, double/Datum array size total number of times alloc was
called, how many needed for this thread
// the above specifies the pool allocation
// note that the pool chain order is the same as the thread order
// there are nthread of the following lists
cnt // number of mechanisms in thread
type i seq // i is the tml->ml->_data[i], seq is the allocation order
// ie we want

Note that the overall memory allocation sequence has to be identical
to the original sequence in terms of get/put for the final
layout to be exactly right for cache efficiency and for threads not
to share cache lines. However, if this is not the case, the memory allocation
is still correct, just not as efficient.
*/

#if NRN_MECH_REORDER

static void read_temp1() {
    // return;
    FILE* f;
    int nscan, maxtype, imech, nmech, type, sz1, sz2, ntget, ith, nth, i, j, cnt, seq;
    char line[200];
    sprintf(line, "temp_%d_%d", nrnmpi_myid, nrnmpi_numprocs);
    f = fopen(line, "r");
    if (!f) {
        return;
    }
    force = 1;
    nrn_assert(fgets(line, 200, f));
    nrn_assert(sscanf(line, "%d", &maxtype) == 1);
    mk_prop_pools(maxtype);
    long* ntget1 = new long[maxtype];
    for (i = 0; i < maxtype; ++i) {
        ntget1[i] = 0;
    }

    // allocate the pool space
    nrn_assert(fgets(line, 200, f));
    nrn_assert(sscanf(line, "%d", &nth) == 1);
    for (ith = 0; ith < nth; ++ith) {
        nrn_assert(fgets(line, 200, f));
        nrn_assert(sscanf(line, "%d", &nmech) == 1);
        for (imech = 0; imech < nmech; ++imech) {
            nrn_assert(fgets(line, 200, f));
            nrn_assert(sscanf(line, "%d %d %d %d %d", &type, &sz1, &sz2, &ntget, &cnt) == 5);
            //		printf("(%d %d %d %d %d) %s", type, sz1, sz2, ntget, cnt, line);
            ntget1[type] = ntget;
            if (sz1) {
                if (!dblpools_[type]) {
                    dblpools_[type] = new DoubleArrayPool(cnt, sz1);
                } else {
                    dblpools_[type]->grow(cnt);
                }
            }
            if (sz2) {
                if (!datumpools_[type]) {
                    datumpools_[type] = new DatumArrayPool(cnt, sz2);
                } else {
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
    delete[] ntget1;

    // now the tricky part, put items in an unnatural order
    // first set all pointers to 0
    for (i = 0; i < maxtype; ++i) {
        if (dblpools_[i]) {
            double** items = dblpools_[i]->items();
            int sz = dblpools_[i]->size();
            for (int j = 0; j < sz; ++j) {
                items[j] = 0;
            }
        }
        if (datumpools_[i]) {
            Datum** items = datumpools_[i]->items();
            int sz = datumpools_[i]->size();
            for (int j = 0; j < sz; ++j) {
                items[j] = 0;
            }
        }
    }

    // then set the proper seq pointers
    DoubleArrayPool** p1 = new DoubleArrayPool*[npools_];
    DatumArrayPool** p2 = new DatumArrayPool*[npools_];
    int* chain = new int[npools_];
    for (i = 0; i < npools_; ++i) {
        p1[i] = dblpools_[i];
        p2[i] = datumpools_[i];
        chain[i] = 0;
    }
    for (ith = 0; ith < nth; ++ith) {
        nrn_assert(fgets(line, 200, f));
        nrn_assert(sscanf(line, "%d", &cnt) == 1);
        for (i = 0; i < cnt; ++i) {
            nrn_assert(fgets(line, 200, f));
            nrn_assert(sscanf(line, "%d %d %d", &type, &j, &seq));
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
        for (i = 0; i < npools_; ++i) {
            if (chain[i]) {
                if (p1[i] && p2[i]) {
                    assert(chain[i] == (p1[i]->chain_size() + p2[i]->chain_size()));
                } else if (p1[i]) {
                    assert(chain[i] == p1[i]->chain_size());
                } else if (p2[i]) {
                    assert(chain[i] == p2[i]->chain_size());
                }
                if (p1[i]) {
                    p1[i] = p1[i]->chain();
                }
                if (p2[i]) {
                    p2[i] = p2[i]->chain();
                }
                chain[i] = 0;
            }
        }
    }
    // finally set the rest
    for (i = 0; i < npools_; ++i) {
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
    delete[] p1;
    delete[] p2;
    fclose(f);
}
#endif  // NRN_MECH_REORDER

static void mk_prop_pools(int n) {
    int i;
    if (n > npools_) {
        DoubleArrayPool** p1 = new DoubleArrayPool*[n];
        DatumArrayPool** p2 = new DatumArrayPool*[n];
        for (i = 0; i < n; ++i) {
            p1[i] = 0;
            p2[i] = 0;
        }
        if (dblpools_) {
            for (i = 0; i < npools_; ++i) {
                p1[i] = dblpools_[i];
                p2[i] = datumpools_[i];
            }
            delete[] dblpools_;
            delete[] datumpools_;
        }
        dblpools_ = p1;
        datumpools_ = p2;
        npools_ = n;
    }
}

void nrn_delete_prop_pool(int type) {
    assert(type < npools_);
    if (dblpools_[type]) {
        if (dblpools_[type]->nget() > 0) {
            hoc_execerror(memb_func[type].sym->name, "prop pool in use");
        }
        delete dblpools_[type];
        dblpools_[type] = NULL;
    }
}

void nrn_mk_prop_pools(int n) {
#if NRN_MECH_REORDER
    if (force == 0) {
        read_temp1();
    }
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
    // if (type > 1) printf("nrn_prop_data_alloc %d %s %d %p\n", type, memb_func[type].sym->name,
    // count, pd);
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
    // if (type > 1) printf("nrn_prop_datum_alloc %d %s %d %p\n", type, memb_func[type].sym->name,
    // count, ppd);
    for (i = 0; i < count; ++i) {
        ppd[i]._pvoid = 0;
    }
    return ppd;
}

void nrn_prop_data_free(int type, double* pd) {
    // if (type > 1) printf("nrn_prop_data_free %d %s %p\n", type, memb_func[type].sym->name, pd);
    if (pd) {
        dblpools_[type]->hpfree(pd);
    }
}

void nrn_prop_datum_free(int type, Datum* ppd) {
    // if (type > 1) printf("nrn_prop_datum_free %d %s %p\n", type, memb_func[type].sym->name, ppd);
    if (ppd) {
        datumpools_[type]->hpfree(ppd);
    }
}

using SectionPool = Pool<Section>;

static SectionPool* secpool_;

Section* nrn_section_alloc() {
    if (!secpool_) {
        secpool_ = new SectionPool(1000);
    }
    Section* s = secpool_->alloc();
    return s;
}
void nrn_section_free(Section* s) {
    secpool_->hpfree(s);
}

int nrn_is_valid_section_ptr(void* v) {
    if (!secpool_) {
        return 0;
    }
    return secpool_->is_valid_ptr(v);
}

int nrn_prop_is_cache_efficient() {
    DoubleArrayPool** p = new DoubleArrayPool*[npools_];
    int r = 1;
    for (int i = 0; i < npools_; ++i) {
        p[i] = dblpools_[i];
    }
    for (int it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            Memb_list* ml = tml->ml;
            int i = tml->index;
            if (ml->nodecount > 0) {
                if (!p[i]) {
                    // printf("thread %d mechanism %s pool chain does not exist\n", it,
                    // memb_func[i].sym->name);
                    r = 0;
                    continue;
                }
                if (p[i]->chain_size() != ml->nodecount) {
                    // printf("thread %d mechanism %s chain_size %d nodecount=%d\n", it,
                    // memb_func[i].sym->name, p[i]->chain_size(), ml->nodecount);
                    r = 0;
                    p[i] = p[i]->chain();
                    continue;
                }
                for (int j = 0; j < ml->nodecount; ++j) {
                    if (p[i]->element(j) != ml->_data[j]) {
                        // printf("thread %d mechanism %s instance %d  element %p data %p\n",
                        // it, memb_func[i].sym->name, j, p[i]->element(j), (ml->_data + j));
                        r = 0;
                    }
                }
                p[i] = p[i]->chain();
            }
        }
    }
    delete[] p;
    return r;
}

// in-place data reallocation only intended to work when only ion data has
// pointers to it and no one uses pointers to other prop data. If this does
// not hold, then segmentation violations are likely to occur.
// Note that the tml list is already ordered properly so ions are first.
// We can therefore call the mechanism pdata_update function (which normally
// contains pointers to ion variables) and the ion variables will exist
// in their new proper locations.

// we do one type (for all threads) at a time. Sadly, we have to keep the
// original pool in existence til the new pool is complete. And we have to
// keep old ion pools til the end.

static DoubleArrayPool** oldpools_;  // only here to allow assertion checking

// extending to gui pointers and other well-known things that hold pointers
// the idea is that there are not *that* many.
static int recalc_index_;  // the one we are working on
static double* recalc_ptr(double* old) {
    // is old in the op pool?
    for (DoubleArrayPool* op = oldpools_[recalc_index_]; op; op = op->chain()) {
        long ds = op->chain_size() * op->d2();
        if (old >= op->pool() && old < (op->pool() + ds)) {
            // if so then the value gives us the pointer in the new pool
            long offset = old - op->pool();
            offset %= op->d2();
            DoubleArrayPool* np = dblpools_[recalc_index_];
            long i = (long) (*old);
            // if (i < 0 || i >= np->size()) abort();
            assert(i >= 0 && i < np->size());
            double* n = np->items()[i] + offset;
            // printf("recalc_ptr old=%p new=%p value=%g\n", old, n, *n);
            return n;
        }
    }
    return old;
}

static hoc_List* mechstanlist_;

static int in_place_data_realloc() {
    int status = 0;
    NrnThread* nt;
    // what types are in use
    int* types = new int[n_memb_func];
    oldpools_ = new DoubleArrayPool*[n_memb_func];
    for (int i = 0; i < n_memb_func; ++i) {
        types[i] = 0;
        oldpools_[i] = dblpools_[i];
    }
    FOR_THREADS(nt) {
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            ++types[tml->index];
        }
    }
    // iterate over those types
    for (int i = 0; i < n_memb_func; ++i)
        if (types[i]) {
            int ision = nrn_is_ion(i);
            DoubleArrayPool* oldpool = dblpools_[i];
            DoubleArrayPool* newpool = 0;
            // create the pool with proper chain sizes
            int ml_total_count = 0;  // so we can know if we get them all
            FOR_THREADS(nt) {
                for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next)
                    if (i == tml->index) {
                        ml_total_count += tml->ml->nodecount;
                        if (!newpool) {
                            newpool = new DoubleArrayPool(tml->ml->nodecount, oldpool->d2());
                        } else {
                            newpool->grow(tml->ml->nodecount);
                        }
                        break;
                    }
            }
            // Any extras? Put them in a new last chain.
            // actually ml_total_count cannot be 0.
            int extra = oldpool->nget() - ml_total_count;
            assert(extra >= 0 && newpool);
            if (extra) {
                newpool->grow(extra);
            }
            newpool->free_all();  // items in pool data order
            // reset ml->_data pointers to the new pool and copy the values
            FOR_THREADS(nt) {
                for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next)
                    if (i == tml->index) {
                        Memb_list* ml = tml->ml;
                        for (int j = 0; j < ml->nodecount; ++j) {
                            double* data = ml->_data[j];
                            int ntget = newpool->ntget();
                            ml->_data[j] = newpool->alloc();
                            for (int k = 0; k < newpool->d2(); ++k) {
                                ml->_data[j][k] = data[k];
                            }
                            // store in old location enough info
                            // to construct a pointer to the new location
                            for (int k = 0; k < newpool->d2(); ++k) {
                                data[k] = double(ntget);
                            }
                        }
                        // update any Datum pointers to ion variable locations
                        void (*s)(Datum*) = memb_func[i]._update_ion_pointers;
                        if (s)
                            for (int j = 0; j < ml->nodecount; ++j) {
                                (*s)(ml->pdata[j]);
                            }
                    }
            }
            // the newpool items are in the correct order for thread cache
            // efficiency. But there may be other old items,
            // e.g MechanismStandard that are not part of the memb lists.
            // We made the chain item for them above. Now, sadly, we
            // need to iterate over the MechanismStandards to find the
            // old pool data.
            if (extra) {
                // should assert that newpool last chain is empty.
                if (!mechstanlist_) {
                    Symbol* s = hoc_lookup("MechanismStandard");
                    mechstanlist_ = s->u.ctemplate->olist;
                }
                hoc_Item* q;
                int found = 0;
                int looked_at = 0;
                ITERATE(q, mechstanlist_) {
                    ++looked_at;
                    MechanismStandard* ms = (MechanismStandard*) (OBJ(q)->u.this_pointer);
                    NrnProperty* np = ms->np();
                    if (np->type() == i && np->deleteable()) {
                        Prop* p = np->prop();
                        double* data = p->param;
                        int ntget = newpool->ntget();
                        p->param = newpool->alloc();
                        for (int k = 0; k < newpool->d2(); ++k) {
                            p->param[k] = data[k];
                        }
                        // store in old location enough info
                        // to construct a pointer to the new location
                        for (int k = 0; k < newpool->d2(); ++k) {
                            data[k] = double(ntget);
                        }
                        ++found;
                    }
                }
                // unless we missed something that holds a prop pointer.
                //			printf("%d extra %s, looked at %d, found %d\n", extra,
                // memb_func[i].sym->name, looked_at, found);
                assert(extra == found);
            }

            dblpools_[i] = newpool;
            // let the gui and other things update
            recalc_index_ = i;
            nrn_recalc_ptrs(recalc_ptr);

            if (0 && !ision) {
                delete oldpool;
                assert(oldpool == oldpools_[i]);
                oldpools_[i] = 0;
            }
        }
    // update p->param for the node properties
    Memb_list** mlmap = new Memb_list*[n_memb_func];
    FOR_THREADS(nt) {
        // make a map
        for (int i = 0; i < n_memb_func; ++i) {
            mlmap[i] = 0;
        }
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            mlmap[tml->index] = tml->ml;
            tml->ml->nodecount = 0;
        }
        // fill
        for (int i = 0; i < nt->end; ++i) {
            Node* nd = nt->_v_node[i];
            for (Prop* p = nd->prop; p; p = p->next) {
                if (memb_func[p->_type].current || memb_func[p->_type].state ||
                    memb_func[p->_type].initialize) {
                    Memb_list* ml = mlmap[p->_type];
                    assert(ml->nodelist[ml->nodecount] == nd);
                    if (!memb_func[p->_type].hoc_mech) {
                        p->param = ml->_data[ml->nodecount];
                    }
                    ++ml->nodecount;
                }
            }
        }
    }
    // one more thing to do for extracellular
#if EXTRACELLULAR
    nrn_extcell_update_param();
#endif
    // finally get rid of the old ion pools
    for (int i = 0; i < n_memb_func; ++i)
        if (types[i] && oldpools_[i]) {
            delete oldpools_[i];
        }
    delete[] oldpools_;
    delete[] types;
    delete[] mlmap;
    // if useful, we could now realloc the Datum pools and just make
    // bit copies of the Datum values.
    return status;
}

void nrn_update_ion_pointer(Symbol* sion, Datum* dp, int id, int ip) {
    int iontype = sion->subtype;
    DoubleArrayPool* np = dblpools_[iontype];
    DoubleArrayPool* op = oldpools_[iontype];
    assert(np);
    assert(op);
    assert(ip < op->d2());
    assert(1);  //  should point into pool() for one of the op pool chains
    // and the index should be a pointer to the double in np
    long i = (long) (*dp[id].pval);
    assert(i >= 0 && i < np->size());
    double* pvar = np->items()[i];
    dp[id].pval = pvar + ip;
}

void nrn_cache_prop_realloc() {
    if (!nrn_prop_is_cache_efficient()) {
        //		printf("begin nrn_prop_is_cache_efficient %d\n", nrn_prop_is_cache_efficient());
        in_place_data_realloc();
        //		printf("end nrn_prop_is_cache_efficient %d\n", nrn_prop_is_cache_efficient());
    }
    return;


#if NRN_MECH_REORDER
    nrn_prop_is_cache_efficient();
    FILE* f;
    char buf[100];
    int i, it, type;
    // we wish to rearrange the arrays so that they are in memb_list->data order within
    // the ArrayPools. We do not want to use up a lot of temporary space to do this
    // because there may not be much left.
    // In each pool all the pointers between put and get (nget of them) are in use.
    if (force) {
        sprintf(buf, "temp2_%d", nrnmpi_myid);
    } else {
        sprintf(buf, "temp_%d_%d", nrnmpi_myid, nrnmpi_numprocs);
    }
    f = fopen(buf, "w");
    fprintf(f, "%d\n", n_memb_func);
    fprintf(f, "%d\n", nrn_nthread);
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        // how many mechanisms used in this thread
        i = 0;
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            ++i;
        }
        fprintf(f, "%d\n", i);
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            Memb_list* ml = tml->ml;
            i = tml->index;
            int j, cnt = ml->nodecount;
            int sz1 = 0, sz2 = 0, ntget = 0;
            if (dblpools_[i]) {
                sz1 = dblpools_[i]->d2();
                ntget = dblpools_[i]->ntget();
            }
            if (datumpools_[i]) {
                sz2 = datumpools_[i]->d2();
            }
            fprintf(f, "%d %d %d %d %d %s\n", i, sz1, sz2, ntget, cnt, memb_func[i].sym->name);
        }
    }
    // above is enough for allocating the pools. Now the proper order info.
    Memb_list** mlmap = new Memb_list*[n_memb_func];
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        for (i = 0; i < n_memb_func; ++i) {
            mlmap[i] = 0;
        }  // unnecessary but ...
        // how many prop used in this thread
        int cnt = 0;
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            cnt += tml->ml->nodecount;
            tml->ml->nodecount = 0;  // recount them below
            mlmap[tml->index] = tml->ml;
        }
        fprintf(f, "%d\n", cnt);
        int cnt2 = 0;
        for (i = 0; i < nt->end; ++i) {
            Node* nd = nt->_v_node[i];
            for (Prop* p = nd->prop; p; p = p->next) {
                if (memb_func[p->_type].current || memb_func[p->_type].state ||
                    memb_func[p->_type].initialize) {
                    Memb_list* ml = mlmap[p->_type];
                    if (!ml || nd != ml->nodelist[ml->nodecount]) {
                        abort();
                    }
                    assert(ml && nd == ml->nodelist[ml->nodecount]);
                    nrn_assert(fprintf(f, "%d %d %ld\n", p->_type, ml->nodecount++, p->_alloc_seq) >
                               0);
                    ++cnt2;
                }
            }
        }
        assert(cnt == cnt2);
    }
    delete[] mlmap;
    fclose(f);
#endif
}

// for avoiding interthread cache line sharing
// each thread needs its own pool instance
extern "C" void* nrn_pool_create(long count, int itemsize) {
    CharArrayPool* p = new CharArrayPool(count, itemsize);
    return (void*) p;
}
extern "C" void nrn_pool_delete(void* pool) {
    CharArrayPool* p = (CharArrayPool*) pool;
    delete p;
}
extern "C" void* nrn_pool_alloc(void* pool) {
    CharArrayPool* p = (CharArrayPool*) pool;
    return (void*) p->alloc();
}
extern "C" void nrn_pool_free(void* pool, void* item) {
    CharArrayPool* p = (CharArrayPool*) pool;
    p->hpfree(static_cast<char*>(item));
}
extern "C" void nrn_pool_freeall(void* pool) {
    CharArrayPool* p = (CharArrayPool*) pool;
    p->free_all();
}
