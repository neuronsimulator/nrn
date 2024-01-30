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
#include <gui-redirect.h>

struct NMODLRandom {
    NMODLRandom(Object*) {}
    ~NMODLRandom() {}
    nrnran123_State* r() {
        return (nrnran123_State*) hr_.get<void*>();
    }
    void chk() {
        if (!prop_id_) {
            hoc_execerr_ext("NMODLRandom wrapped handle is not valid");
        }
    }
    neuron::container::generic_data_handle hr_{};
    neuron::container::non_owning_identifier_without_container prop_id_{};
};

static Symbol* nmodlrandom_sym{};
#undef dmaxuint
#define dmaxuint 4294967295.

static Object** set_ids(void* v) {  // return this NMODLRandom instance
    NMODLRandom* r = (NMODLRandom*) v;
    r->chk();
    uint32_t id[3];
    for (int i = 0; i < 3; ++i) {
        id[i] = (uint32_t) (chkarg(i + 1, 0., dmaxuint));
    }
    nrnran123_setids(r->r(), id[0], id[1], id[2]);
    return hoc_temp_objptr(nrn_get_gui_redirect_obj());
}

static Object** get_ids(void* v) {  // return a Vector of size 3.
    NMODLRandom* r = (NMODLRandom*) v;
    r->chk();
    uint32_t id[3]{};
    nrnran123_getids3(r->r(), id, id + 1, id + 2);
    IvocVect* vec = vector_new1(3);
    double* data = vector_vec(vec);
    for (int i = 0; i < 3; ++i) {
        data[i] = double(id[i]);
    }
    return vector_temp_objvar(vec);
}

static Object** set_seq(void* v) {  // return this NModlRandom instance
    NMODLRandom* r = (NMODLRandom*) v;
    r->chk();
    double seq = *getarg(1);
    nrnran123_setseq(r->r(), seq);
    return hoc_temp_objptr(nrn_get_gui_redirect_obj());
}

static double get_seq(void* v) {  // return the 34 bits (seq*4 + which) as double
    NMODLRandom* r = (NMODLRandom*) v;
    r->chk();
    uint32_t seq;
    char which;
    nrnran123_getseq(r->r(), &seq, &which);
    return double(seq) * 4.0 + which;
}

static double uniform(void* v) {
    NMODLRandom* r = (NMODLRandom*) v;
    r->chk();
    return nrnran123_uniform(r->r());
}

static Member_func members[] = {{"get_seq", get_seq}, {"uniform", uniform}, {nullptr, nullptr}};

static Member_ret_obj_func retobj_members[] = {{"set_ids", set_ids},
                                               {"get_ids", get_ids},
                                               {"set_seq", set_seq},
                                               {nullptr, nullptr}};

static void* nmodlrandom_cons(Object*) {
    NMODLRandom* r = new NMODLRandom(nullptr);
    return r;
}

static void nmodlrandom_destruct(void* v) {
    NMODLRandom* r = (NMODLRandom*) v;
    delete r;
}

void NMODLRandom_reg() {
    class2oc("NMODLRandom",
             nmodlrandom_cons,
             nmodlrandom_destruct,
             members,
             nullptr,
             retobj_members,
             nullptr);
    if (!nmodlrandom_sym) {
        nmodlrandom_sym = hoc_lookup("NMODLRandom");
        assert(nmodlrandom_sym);
    }
}

Object* nrn_pntproc_nmodlrandom_wrap(void* v, Symbol* sym) {
    auto* const pnt = static_cast<Point_process*>(v);
    if (!pnt->prop) {
        if (nrn_inpython_ == 1) { /* python will handle the error */
            hoc_warning("point process not located in a section", nullptr);
            nrn_inpython_ = 2;
            return {};
        } else {
            hoc_execerror("point process not located in a section", nullptr);
        }
    }

    return nrn_nmodlrandom_wrap(pnt->prop, sym);
}

Object* nrn_nmodlrandom_wrap(Prop* prop, Symbol* sym) {
    assert(sym->type == RANGEOBJ && sym->subtype == NMODLRANDOM);
    auto& datum = prop->dparam[sym->u.rng.index];
    assert(datum.holds<void*>());

    NMODLRandom* r = new NMODLRandom(nullptr);
    r->hr_ = datum;
    r->prop_id_ = prop->id();
    Object* wrap = hoc_new_object(nmodlrandom_sym, r);
    return wrap;
}
