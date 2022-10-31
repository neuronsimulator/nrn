#include "arraypool.h"   // ArrayPool
#include "hocdec.h"      // Datum
#include "section.h"     // Section
#include "structpool.h"  // Pool

#include <memory>
#include <vector>

// allocate and free Datum arrays for nrniv this allows for the possibility of
// greater cache efficiency

using CharArrayPool = ArrayPool<char>;
using DatumArrayPool = ArrayPool<Datum>;
using SectionPool = Pool<Section>;

static SectionPool* secpool_;
static std::vector<std::unique_ptr<DatumArrayPool>> datumpools_;

void nrn_mk_prop_pools(int n) {
    if (n > datumpools_.size()) {
        datumpools_.resize(n);
    }
}

Datum* nrn_prop_datum_alloc(int type, int count, Prop* p) {
    if (!datumpools_[type]) {
        datumpools_[type] = std::make_unique<DatumArrayPool>(1000, count);
    }
    assert(datumpools_[type]->d2() == count);
    p->_alloc_seq = datumpools_[type]->ntget();
    auto* ppd = datumpools_[type]->alloc();  // allocates storage for the datums
    // if (type > 1) printf("nrn_prop_datum_alloc %d %s %d %p\n", type, memb_func[type].sym->name,
    // count, ppd);
    for (int i = 0; i < count; ++i) {
        auto* const semantics = memb_func[type].dparam_semantics;
        // Call the Datum constructor, either setting it to be a
        // generic_data_handle or a std::monostate value.
        new (ppd + i) Datum();
        if (semantics &&
            (semantics[i] == -5 /* POINTER */ || semantics[i] == -7 /* BBCOREPOINTER */)) {
            new (ppd + i) Datum(neuron::container::generic_data_handle{});
        } else {
            new (ppd + i) Datum();
        }
    }
    return ppd;
}

void nrn_prop_datum_free(int type, Datum* ppd) {
    // if (type > 1) printf("nrn_prop_datum_free %d %s %p\n", type,
    // memb_func[type].sym->name, ppd);
    if (ppd) {
        auto* const datumpool = datumpools_[type].get();
        assert(datumpool);
        // Destruct the Datums
        auto const count = datumpool->d2();
        for (auto i = 0; i < count; ++i) {
            ppd[i].~Datum();
        }
        // Deallocate them
        datumpool->hpfree(ppd);
    }
}

Section* nrn_section_alloc() {
    if (!secpool_) {
        secpool_ = new SectionPool(1000);
    }
    return secpool_->alloc();
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

void nrn_poolshrink(int shrink) {
    if (shrink) {
        for (int i = 0; i < npools_; ++i) {
            auto& pdatum = datumpools_[i];
            if (pdatum && pdatum->nget() == 0) {
                delete datumpools_[i];
                datumpools_[i] = NULL;
            }
        }
    } else {
        Printf("poolshrink --- type name (dbluse, size) (datumuse, size)\n");
        for (int i = 0; i < npools_; ++i) {
            auto& pdatum = datumpools_[i];
            if (pdatum) {
                Printf("%d %s (%ld, %d)\n",
                       i,
                       (memb_func[i].sym ? memb_func[i].sym->name : "noname"),
                       (pdatum ? pdatum->nget() : 0),
                       (pdatum ? pdatum->size() : 0));
            }
        }
    }
}

// for avoiding interthread cache line sharing
// each thread needs its own pool instance
void* nrn_pool_create(long count, int itemsize) {
    return new CharArrayPool(count, itemsize);
}
void nrn_pool_delete(void* pool) {
    delete static_cast<CharArrayPool*>(pool);
}
void* nrn_pool_alloc(void* pool) {
    return static_cast<CharArrayPool*>(pool)->alloc();
}
void nrn_pool_free(void* pool, void* item) {
    static_cast<CharArrayPool*>(pool)->hpfree(static_cast<char*>(item));
}
void nrn_pool_freeall(void* pool) {
    static_cast<CharArrayPool*>(pool)->free_all();
}
