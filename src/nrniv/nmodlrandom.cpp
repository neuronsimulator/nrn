#include <../../nrnconf.h>

/*
HOC wrapper object for NMODL NEURON block RANDOM variables to give a HOC
pointprocess.ranvar.method(...)
sec.ranvar_mech(x).method(...)
and Python
poinprocess.ranvar.method(...)
sec(x).mech.ranvar.method(...)
syntax
*/

#include <section.h>
#include <parse.hpp>
#include <nrnran123.h>
#include <classreg.h>

struct NMODLRandom {
    NMODLRandom(Object*, void* r) {
        printf("NMODLRandom %p r=%p\n", this, r);
        r_ = (nrnran123_State*) r;
    }
    ~NMODLRandom() {
        printf("~NMODLRandom %p r=%p\n", this, r_);
    }
    nrnran123_State* r_;
};

static Member_func members[] = {{nullptr, nullptr}};

static void* nmodlrandom_cons(Object*) {
    NMODLRandom* r = new NMODLRandom(nullptr, nullptr);
    return r;
}

static void nmodlrandom_destruct(void* v) {
    NMODLRandom* r = (NMODLRandom*) v;
    delete r;
}

void NMODLRandom_reg() {
    class2oc(
        "NMODLRandom", nmodlrandom_cons, nmodlrandom_destruct, members, nullptr, nullptr, nullptr);
}

Object* nrn_pntproc_nmodlrandom_wrap(void* v, Symbol* sym) {
    static Symbol* nmodlrandom_sym{};
    auto* const pnt = static_cast<Point_process*>(v);
    if (!pnt->prop) {
        if (nrn_inpython_ == 1) { /* python will handle the error */
            // is this necessary if, instead, there is a try...
            hoc_warning("point process not located in a section", nullptr);
            nrn_inpython_ = 2;
            return {};
        } else {
            hoc_execerror("point process not located in a section", nullptr);
        }
    }
    assert(sym->type == RANGEVAR && sym->subtype == NMODLRANDOM);
    auto& datum = pnt->prop->dparam[sym->u.rng.index];
    assert(datum.holds<void*>());

    if (!nmodlrandom_sym) {
        nmodlrandom_sym = hoc_lookup("NMODLRandom");
        assert(nmodlrandom_sym);
    }

    NMODLRandom* r = new NMODLRandom(nullptr, datum.get<void*>());
    Object* wrap = hoc_new_object(nmodlrandom_sym, r);

    return wrap;
}
