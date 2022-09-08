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

extern int nrn_is_ion(int);
#if EXTRACELLULAR
extern void nrn_extcell_update_param();
#endif
extern void nrn_recalc_ptrs(double* (*) (double*) );

static constexpr auto APSIZE = 1000;
using CharArrayPool = ArrayPool<char>;
using DoubleArrayPool = ArrayPool<double>;
using DatumArrayPool = ArrayPool<Datum>;
using SectionPool = Pool<Section>;

static int npools_;
static DoubleArrayPool** dblpools_;
static DatumArrayPool** datumpools_;
static SectionPool* secpool_;

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
    if (n <= npools_) {
        return;
    }
    DoubleArrayPool** p1 = new DoubleArrayPool*[n];
    DatumArrayPool** p2 = new DatumArrayPool*[n];
    for (int i = 0; i < n; ++i) {
        p1[i] = 0;
        p2[i] = 0;
    }
    if (dblpools_) {
        for (int i = 0; i < npools_; ++i) {
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
    auto& generic_handle = *dp[id].generic_handle;
    long i = *static_cast<neuron::container::data_handle<double>>(generic_handle);
    assert(i >= 0 && i < np->size());
    double* pvar = np->items()[i];
    // TODO provide an assignment operator
    generic_handle = neuron::container::generic_data_handle{neuron::container::data_handle<double>{pvar + ip}};
}


void nrn_poolshrink(int shrink) {
    if (shrink) {
        for (int i = 0; i < npools_; ++i) {
            auto& pdbl = dblpools_[i];
            auto& pdatum = datumpools_[i];
            if ((pdbl && pdbl->nget() == 0)) {
                nrn_delete_prop_pool(i);
            }
            if (pdatum && pdatum->nget() == 0) {
                delete datumpools_[i];
                datumpools_[i] = NULL;
            }
        }
    } else {
        Printf("poolshrink --- type name (dbluse, size) (datumuse, size)\n");
        for (int i = 0; i < npools_; ++i) {
            auto& pdbl = dblpools_[i];
            auto& pdatum = datumpools_[i];
            if (pdbl || pdatum) {
                Printf("%d %s (%ld, %d) (%ld, %d)\n",
                       i,
                       (memb_func[i].sym ? memb_func[i].sym->name : "noname"),
                       (pdbl ? pdbl->nget() : 0),
                       (pdbl ? pdbl->size() : 0),
                       (pdatum ? pdatum->nget() : 0),
                       (pdatum ? pdatum->size() : 0));
            }
        }
    }
}

void nrn_cache_prop_realloc() {
    if (!nrn_prop_is_cache_efficient()) {
        in_place_data_realloc();
    }
}

// for avoiding interthread cache line sharing
// each thread needs its own pool instance
extern "C" void* nrn_pool_create(long count, int itemsize) {
    return new CharArrayPool(count, itemsize);
}
extern "C" void nrn_pool_delete(void* pool) {
    delete static_cast<CharArrayPool*>(pool);
}
extern "C" void* nrn_pool_alloc(void* pool) {
    return static_cast<CharArrayPool*>(pool)->alloc();
}
extern "C" void nrn_pool_free(void* pool, void* item) {
    static_cast<CharArrayPool*>(pool)->hpfree(static_cast<char*>(item));
}
extern "C" void nrn_pool_freeall(void* pool) {
    static_cast<CharArrayPool*>(pool)->free_all();
}
