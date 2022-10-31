#include <../../nrnconf.h>

// some functions needed by CACHEVEC. mostly to steer to the various update_ptrs
// methods.

#if HAVE_IV
#include <shapeplt.h>
#include <graph.h>
#include <ivoc.h>
#include <xmenu.h>
#endif

#include <ocnotify.h>
#include <mymath.h>
#include <nrnoc2iv.h>
#include <parse.hpp>
#include <cvodeobj.h>
#include <netcvode.h>
#include <hoclist.h>
#include <ocpointer.h>
#include <ocptrvector.h>
#include "treeset.h"

void nrniv_recalc_ptrs();
extern NetCvode* net_cvode_instance;
static Symbol* grsym_;
static Symbol* ptrsym_;
static Symbol* lmsym_;
static Symbol* pshpsym_;
static Symbol* ptrvecsym_;

void nrniv_recalc_ptrs() {
    // PlayRecord and PreSyn pointers
    net_cvode_instance->recalc_ptrs();
    hoc_List* hl;
    hoc_Item* q;
#if HAVE_IV
    // update pointers used by Graph
    if (!grsym_) {
        grsym_ = hoc_lookup("Graph");
        assert(grsym_->type == TEMPLATE);
    }
    hl = grsym_->u.ctemplate->olist;
    ITERATE(q, hl) {
        Object* obj = OBJ(q);
        Graph* g = (Graph*) obj->u.this_pointer;
        if (g) {
            g->update_ptrs();
        }
    }
    // update pointers used by PlotShape
    if (!pshpsym_) {
        pshpsym_ = hoc_lookup("PlotShape");
        assert(pshpsym_->type == TEMPLATE);
    }
    hl = pshpsym_->u.ctemplate->olist;
    ITERATE(q, hl) {
        Object* obj = OBJ(q);
        ShapePlot* ps = (ShapePlot*) obj->u.this_pointer;
        if (ps) {
            ps->update_ptrs();
        }
    }
    // update pointers used by xpanel
    HocPanel::update_ptrs();
#endif
    // update pointers used by Pointer
    if (!ptrsym_) {
        ptrsym_ = hoc_lookup("Pointer");
        assert(ptrsym_->type == TEMPLATE);
    }
    // update what LinearMechanisms are observing
    if (!lmsym_) {
        lmsym_ = hoc_lookup("LinearMechanism");
        assert(lmsym_->type == TEMPLATE);
    }
}

void nrn_recalc_ptrvector() {
    // update pointers used by PtrVector
    hoc_List* hl;
    hoc_Item* q;
    if (!ptrvecsym_) {
        ptrvecsym_ = hoc_lookup("PtrVector");
        assert(ptrvecsym_->type == TEMPLATE);
    }
    hl = ptrvecsym_->u.ctemplate->olist;
    ITERATE(q, hl) {
        Object* obj = OBJ(q);
        OcPtrVector* op = (OcPtrVector*) obj->u.this_pointer;
        op->ptr_update();
    }
}
