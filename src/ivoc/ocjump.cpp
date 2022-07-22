#include <../../nrnconf.h>
#include <InterViews/resource.h>
#include <setjmp.h>
#include <string.h>
#if CABLE
#include "nrnoc2iv.h"
#else
#include "oc2iv.h"
#endif
#include "ocfunc.h"
#include "ocjump.h"
#include "nrnfilewrap.h"
#if HAVE_IV
#include "ivoc.h"
#endif

extern Objectdata* hoc_top_level_data;
extern Symlist* hoc_top_level_symlist;
extern Symlist* hoc_symlist;
extern Object* hoc_thisobject;
extern void hoc_execute1();
extern bool hoc_valid_stmt(const char* stmt, Object* ob);
extern int hoc_execerror_messages;
void* nrn_get_oji();

static bool valid_stmt1(const char* stmt, Object* ob) {
    char* s = new char[strlen(stmt) + 2];
    strcpy(s, stmt);
    strcat(s, "\n");
    OcJump oj;
    bool val = oj.execute(s, ob);
    delete[] s;
    return val;
}

bool hoc_valid_stmt(const char* stmt, Object* ob) {
    return valid_stmt1(stmt, ob);
}

void hoc_execute1() {
    Object* ob = NULL;
    int hem = 1, hemold;
    if (ifarg(2)) {
        if (hoc_is_object_arg(2)) {
            ob = *hoc_objgetarg(2);
            if (ifarg(3)) {
                hem = (int) chkarg(3, 0., 1.);
            }
        } else {
            hem = (int) chkarg(2, 0., 1.);
        }
    }

    hemold = hoc_execerror_messages;
    hoc_execerror_messages = hem;
    int old_mpiabort_flag = nrn_mpiabort_on_error_;
    nrn_mpiabort_on_error_ = 0;
    bool b = valid_stmt1(gargstr(1), ob);
    nrn_mpiabort_on_error_ = old_mpiabort_flag;
    hoc_execerror_messages = hemold;
    hoc_ret();
    hoc_pushx(double(b));
}

// safely? return from an execution even in the presence of an execerror

class OcJumpImpl {
  public:
    OcJumpImpl();
    virtual ~OcJumpImpl();
    bool execute(Inst* p);
    bool execute(const char*, Object* ob = NULL);
    void* fpycall(void* (*f)(void*, void*), void* a, void* b);

    /* jmpbuf is not portable and I can't figure out how get a pointer to one.
    therefore hoc_execerror looks at a function pointer and if it's non-NULL
    (ljmptarget) calls it instead of doing a longjump. That means we are back
    here and can do an explicit longjump using the begin_ */
    static void ljmptarget();
    void ljmp();
    static OcJumpImpl* oji_;

  private:
    void begin();
    void restore();
    void finish();

  private:
    OcJumpImpl* prev_;
    jmp_buf begin_;

    // hoc_oop
    Object* o1;
    Objectdata* o2;
    int o4;
    Symlist* o5;

    // code
    Inst* c1;
    Inst* c2;
    Datum* c3;
    nrn::oc::frame* c4;
    int c5;
    int c6;
    Inst* c7;
    nrn::oc::frame* c8;
    Datum* c9;
    Symlist* c10;
    Inst* c11;
    int c12;

    // input_info
    const char* i1;
    int i2;
    int i3;
    NrnFILEWrap* i4;

#if CABLE
    // cabcode
    int cc1;
    int cc2;
#endif
};

/** Return handle for the current longjump buffer info.
 *  Valid until finish is called on the oji_ instance.
 **/
void* nrn_get_oji() {
    return (void*) OcJumpImpl::oji_;
}

//------------------------------------------------------------------

#if HAVE_IV
bool Oc::valid_expr(Symbol* s) {
    OcJump oj;
    return oj.execute(s->u.u_proc->defn.in);
}

bool Oc::valid_stmt(const char* stmt, Object* ob) {
    return valid_stmt1(stmt, ob);
}
#endif
//------------------------------------------------------------------

OcJump::OcJump() {
    impl_ = new OcJumpImpl();
}
OcJump::~OcJump() {
    delete impl_;
}
bool OcJump::execute(Inst* p) {
    return impl_->execute(p);
}

bool OcJump::execute(const char* stmt, Object* ob) {
    return impl_->execute(stmt, ob);
}

void* OcJump::fpycall(void* (*f)(void*, void*), void* a, void* b) {
    return impl_->fpycall(f, a, b);
}

//-------------------------------------------------------------------

OcJumpImpl::OcJumpImpl() {}
OcJumpImpl::~OcJumpImpl() {}

void OcJumpImpl::ljmptarget() {
    if (oji_) {
        oji_->ljmp();
    }
}

void OcJumpImpl::ljmp() {
    longjmp(begin_, 1);
}

OcJumpImpl* OcJumpImpl::oji_;

void hoc_execute(Inst*);

bool OcJumpImpl::execute(Inst* p) {
    begin();
#if 1
    if (setjmp(begin_)) {
        restore();
        finish();
        return false;
    } else
#endif
    {
        hoc_execute(p);
    }
    finish();
    return true;
}

bool OcJumpImpl::execute(const char* stmt, Object* ob) {
    begin();
#if 1
    if (setjmp(begin_)) {
        restore();
        finish();
        return false;
    } else
#endif
    {
        hoc_obj_run(stmt, ob);
    }
    finish();
    return true;
}

void* OcJumpImpl::fpycall(void* (*f)(void*, void*), void* a, void* b) {
    void* c = 0;
    begin();
#if 1
    if (setjmp(begin_)) {
        restore();
        finish();
        return c;
    } else
#endif
    {
        c = (*f)(a, b);
    }
    finish();
    return c;
}


extern void (*oc_jump_target_)(void);
extern int hoc_intset;
//	extern int hoc_pipeflag;
void OcJumpImpl::begin() {
    // not complete but it is good for expressions and it can be improved
    oc_save_hoc_oop(&o1, &o2, &o4, &o5);
    oc_save_code(&c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9, &c10, &c11, &c12);
    oc_save_input_info(&i1, &i2, &i3, &i4);
#if CABLE
    oc_save_cabcode(&cc1, &cc2);
#endif
    // this may not be portable since it depends on the jmp_buf being
    // an array of integers.
    oc_jump_target_ = ljmptarget;
    prev_ = oji_;
    oji_ = this;
}
void OcJumpImpl::restore() {
    oc_restore_hoc_oop(&o1, &o2, &o4, &o5);
    oc_restore_code(&c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9, &c10, &c11, &c12);
    oc_restore_input_info(i1, i2, i3, i4);
#if CABLE
    oc_restore_cabcode(&cc1, &cc2);
#endif
}
void OcJumpImpl::finish() {
    if (!prev_) {
        oc_jump_target_ = NULL;
    }
    oji_ = prev_;
#if 0
	if (hoc_intset) {
		hoc_execerror("interrupted in OcJump", 0);
	}
#endif
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
    restored_ = false;
}

ObjectContext::~ObjectContext() {
    if (!restored_) {
        restore();
    }
}

void ObjectContext::restore() {
    oc_restore_hoc_oop(&a1, &a2, &a4, &a5);
    restored_ = true;
}
