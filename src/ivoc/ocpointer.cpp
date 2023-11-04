#include <../../nrnconf.h>
/*
 provide a pointer to the interpreter
    p = new Pointer(string) or p = new Pointer(&var)
    val = p.val
    p.val = val
    &p.val can be an argument
    Optional second arg can be a statement containing $1 for generalized
      assignment. It will be executed (and p.val assigned) when
    p.assign(val)
*/
#include <InterViews/observe.h>
#include <string.h>
#include "classreg.h"
#include "oc_ansi.h"
#include "oc2iv.h"
#include "ocpointer.h"
#include "parse.hpp"
#include "ocnotify.h"

#if HAVE_IV
#include "ivoc.h"
#endif

OcPointer::OcPointer(const char* st, double* d)
    : Observer() {
    sti_ = NULL;
    s_ = new char[strlen(st) + 1];
    strcpy(s_, st);
    p_ = d;
    valid_ = true;
    nrn_notify_when_double_freed(p_, this);
}

OcPointer::~OcPointer() {
    if (sti_) {
        delete sti_;
    }
    delete[] s_;
    nrn_notify_pointer_disconnect(this);
}

void OcPointer::update(Observable*) {
    valid_ = false;
}

void OcPointer::assign(double x) {
    assert(valid_);
    *p_ = x;
    if (sti_) {
        sti_->play_one(x);
    }
}

static double assign(void* v) {
    OcPointer* ocp = (OcPointer*) v;
    if (!ocp->valid_) {
        hoc_execerror("Pointer points to freed address:", ocp->s_);
    }
    ocp->assign(*getarg(1));
    return *ocp->p_;
}

static const char** pname(void* v) {
    OcPointer* ocp = (OcPointer*) v;
    return (const char**) &ocp->s_;
}

static Member_func members[] = {{"val", 0},          // will be changed below
                                {"assign", assign},  // will call assign_stmt if it exists
                                {0, 0}};

static Member_ret_str_func s_memb[] = {{"s", pname}, {nullptr, nullptr}};


static void* cons(Object*) {
    double* p;
    const char* s;
    if (hoc_is_pdouble_arg(1)) {
        p = hoc_pgetarg(1);
        s = "unknown";
    } else {
        s = gargstr(1);
        ParseTopLevel ptl;
        p = hoc_val_pointer(s);
    }
    if (!p) {
        hoc_execerror("Pointer constructor failed", 0);
    }
    OcPointer* ocp = new OcPointer(s, p);
    if (ifarg(2)) {
        ocp->sti_ = new StmtInfo(gargstr(2));
    }
    return (void*) ocp;
}

static void destruct(void* v) {
    delete (OcPointer*) v;
}

static void steer_val(void* v) {
    OcPointer* ocp = (OcPointer*) v;
    hoc_spop();
    if (!ocp->valid_) {
        hoc_execerror("Pointer points to freed address:", ocp->s_);
    }
    hoc_pushpx(ocp->p_);
}

void OcPointer_reg() {
    class2oc("Pointer", cons, destruct, members, NULL, NULL, s_memb);
    // now make the val variable an actual double
    Symbol* sv = hoc_lookup("Pointer");
    Symbol* sx = hoc_table_lookup("val", sv->u.ctemplate->symtable);
    sx->type = VAR;
    sx->arayinfo = NULL;
    sv->u.ctemplate->steer = steer_val;
}

StmtInfo::StmtInfo(const char* s)
    : stmt_(s) {
    parse();
}

StmtInfo::~StmtInfo() {
    hoc_free_list(&symlist_);
}


void StmtInfo::parse() {
    char buf[256], *d;
    const char* s;
    symlist_ = NULL;
    ParseTopLevel ptl;
    bool see_arg = false;
    for (s = stmt_.c_str(), d = buf; *s; ++s, ++d) {
        if (*s == '$' && s[1] == '1') {
            strcpy(d, "hoc_ac_");
            s++;
            d += 6;
            see_arg = true;
        } else {
            *d = *s;
        }
    }
    if (!see_arg) {
        strcpy(d, "=hoc_ac_");
        d += 8;
    }
    *d = '\0';
    symstmt_ = hoc_parse_stmt(buf, &symlist_);
}

void StmtInfo::play_one(double val) {
    ParseTopLevel ptl;
    hoc_ac_ = val;
    hoc_run_stmt(symstmt_);
}
