#include <../../nrnconf.h>

#include "nrnfilewrap.h"
#include "nrnoc2iv.h"
#include "ocfunc.h"
#include "ocjump.h"
#if HAVE_IV
#include "ivoc.h"
#endif

#include <InterViews/resource.h>

#include <utility>

extern Objectdata* hoc_top_level_data;
extern Symlist* hoc_top_level_symlist;
extern Symlist* hoc_symlist;
extern Object* hoc_thisobject;
extern int hoc_execerror_messages;

bool hoc_valid_stmt(const char* stmt, Object* ob) {
    std::string s{stmt};
    s.append(1, '\n');
    return OcJump::execute(s.c_str(), ob);
}

void hoc_execute1() {
    Object* ob{};
    int hem{1};
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            ob = *hoc_objgetarg(2);
            if (ifarg(3)) {
                hem = chkarg(3, 0., 1.);
            }
        } else {
            hem = chkarg(2, 0., 1.);
        }
    }

    auto const hemold = std::exchange(hoc_execerror_messages, hem);
    auto const old_mpiabort_flag = std::exchange(nrn_mpiabort_on_error_, 0);
    bool const b = hoc_valid_stmt(hoc_gargstr(1), ob);
    nrn_mpiabort_on_error_ = old_mpiabort_flag;
    hoc_execerror_messages = hemold;
    hoc_retpushx(b);
}

#if HAVE_IV
bool Oc::valid_expr(Symbol* s) {
    return OcJump::execute(s->u.u_proc->defn.in);
}

bool Oc::valid_stmt(const char* stmt, Object* ob) {
    return hoc_valid_stmt(stmt, ob);
}
#endif
//------------------------------------------------------------------
void hoc_execute(Inst*);

namespace {
struct saved_state {
    saved_state() {
        // not complete but it is good for expressions and it can be improved
        oc_save_hoc_oop(&o1, &o2, &o4, &o5);
        oc_save_code(&c1, &c2, c3, &c4, &c5, &c6, &c7, &c8, c9, &c10, &c11, &c12);
        oc_save_input_info(&i1, &i2, &i3, &i4);
        oc_save_cabcode(&cc1, &cc2);
    }
    void restore() {
        oc_restore_hoc_oop(&o1, &o2, &o4, &o5);
        oc_restore_code(&c1, &c2, c3, &c4, &c5, &c6, &c7, &c8, c9, &c10, &c11, &c12);
        oc_restore_input_info(i1, i2, i3, i4);
        oc_restore_cabcode(&cc1, &cc2);
    }

  private:
    // hoc_oop
    Object* o1{};
    Objectdata* o2{};
    int o4{};
    Symlist* o5{};

    // code
    Inst* c1{};
    Inst* c2{};
    std::size_t c3{};
    nrn::oc::frame* c4{};
    int c5{};
    int c6{};
    Inst* c7{};
    nrn::oc::frame* c8{};
    std::size_t c9{};
    Symlist* c10{};
    Inst* c11{};
    int c12{};

    // input_info
    const char* i1{};
    int i2{};
    int i3{};
    NrnFILEWrap* i4{};

    // cabcode
    int cc1{};
    int cc2{};
};
}  // namespace

bool OcJump::execute(Inst* p) {
    saved_state before{};
    try_catch_depth_increment tell_children_we_will_catch{};
    try {
        hoc_execute(p);
        return true;
    } catch (...) {
        before.restore();
        return false;
    }
}

bool OcJump::execute(const char* stmt, Object* ob) {
    saved_state before{};
    try_catch_depth_increment tell_children_we_will_catch{};
    try {
        hoc_obj_run(stmt, ob);
        return true;
    } catch (...) {
        before.restore();
        return false;
    }
}

void* OcJump::fpycall(void* (*f)(void*, void*), void* a, void* b) {
    saved_state before{};
    try_catch_depth_increment tell_children_we_will_catch{};
    try {
        return (*f)(a, b);
    } catch (...) {
        before.restore();
        throw;
    }
}

ObjectContext::ObjectContext(Object* obj) {
    oc_save_hoc_oop(&a1, &a2, &a4, &a5);
    hoc_thisobject = obj;
    if (obj) {
        hoc_objectdata = obj->u.dataspace;
        hoc_symlist = obj->ctemplate->symtable;
    } else {
        hoc_objectdata = hoc_top_level_data;
        hoc_symlist = hoc_top_level_symlist;
    }
}

ObjectContext::~ObjectContext() {
    oc_restore_hoc_oop(&a1, &a2, &a4, &a5);
}
