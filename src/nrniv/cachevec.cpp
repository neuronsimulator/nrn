#include "hoclist.h"      // ITERATE, hoc_List, hoc_Item
#include "netcvode.h"     // NetCvode
#include "nrn_ansi.h"     // nrniv_recalc_ptrs
#include "ocptrvector.h"  // OcPtrVector
#include <parse.hpp>      // TEMPLATE

#include <cassert>  // assert

extern NetCvode* net_cvode_instance;
static Symbol* ptrvecsym_;

void nrniv_recalc_ptrs() {
    // PlayRecord and PreSyn pointers
    net_cvode_instance->recalc_ptrs();
}

void nrn_recalc_ptrvector() {
    // update pointers used by PtrVector
    if (!ptrvecsym_) {
        ptrvecsym_ = hoc_lookup("PtrVector");
        assert(ptrvecsym_->type == TEMPLATE);
    }
    hoc_Item* q{};
    hoc_List* const hl{ptrvecsym_->u.ctemplate->olist};
    ITERATE(q, hl) {
        auto* const op = static_cast<OcPtrVector*>(OBJ(q)->u.this_pointer);
        op->ptr_update();
    }
}
