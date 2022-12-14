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
// the advantage of this w.r.t `static std::vector<...> datumpools_;` is that the vector destructor
// is not called at application shutdown, see e.g. discussion in
// https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables around global
// variables that are not trivially destructible
static auto& datumpools() {
    static auto& x = *new std::vector<std::unique_ptr<DatumArrayPool>>{};
    return x;
}

void nrn_mk_prop_pools(int n) {
    if (n > datumpools().size()) {
        datumpools().resize(n);
    }
}

Datum* nrn_prop_datum_alloc(int type, int count, Prop* p) {
    if (!datumpools()[type]) {
        datumpools()[type] = std::make_unique<DatumArrayPool>(1000, count);
    }
    assert(datumpools()[type]->d2() == count);
    p->_alloc_seq = datumpools()[type]->ntget();
    auto* const ppd = datumpools()[type]->alloc();  // allocates storage for the datums
    for (int i = 0; i < count; ++i) {
        // Call the Datum constructor
        new (ppd + i) Datum();
    }
    return ppd;
}

void nrn_prop_datum_free(int type, Datum* ppd) {
    // if (type > 1) printf("nrn_prop_datum_free %d %s %p\n", type,
    // memb_func[type].sym->name, ppd);
    if (ppd) {
        auto* const datumpool = datumpools()[type].get();
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
    auto* const sec = secpool_->alloc();
    // Call the Section constructor
    new (sec) Section();
    return sec;
}

void nrn_section_free(Section* s) {
    // Call the Section destructor
    s->~Section();
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
        for (auto& pdatum: datumpools()) {
            if (pdatum && pdatum->nget() == 0) {
                pdatum.reset();
            }
        }
    } else {
        Printf("poolshrink --- type name (dbluse, size) (datumuse, size)\n");
        for (auto i = 0; i < datumpools().size(); ++i) {
            auto& pdatum = datumpools()[i];
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
