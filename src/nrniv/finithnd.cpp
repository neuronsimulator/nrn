#include <../../nrnconf.h>

/*
statements that are called during finitialize()
Type 1 is the default.
Type 0 are called after v is set but before INITIAL blocks are called.
  This is also after clearing the event queue and playing the time 0 values.
Type 1 are after INITIAL blocks (and linmod and  init_net_events) but
  before variable step re_init or fcurrent();
Type 2 are just before return. ie after vector initial record and delivery
  of events at t=0;
Type 3 are at the very beginning of finitialize. ie structure changes
  are allowed.
*/

#include <vector>
#include <cstdio>
#include <nrnoc2iv.h>
#include <classreg.h>
#include <objcmd.h>
#include "utils/enumerate.h"

class FInitialHandler {
  public:
    FInitialHandler(int, const char*, Object*, Object* pyact = NULL);
    virtual ~FInitialHandler();
    HocCommand* stmt_;
    int type_;
    static std::vector<FInitialHandler*> fihlist_[4];
};

void nrn_fihexec(int type);
void nrn_fihexec(int type) {
    for (auto& f: FInitialHandler::fihlist_[type]) {
        f->stmt_->execute();
    }
}

static double allprint(void* v) {
    for (int type = 0; type < 4; ++type) {
        std::vector<FInitialHandler*> fl = FInitialHandler::fihlist_[type];
        if (!fl.empty()) {
            Printf("Type %d FInitializeHandler statements\n", type);
            for (auto& f: fl) {
                if (f->stmt_->pyobject()) {
                    Printf("\t%s\n", hoc_object_name(f->stmt_->pyobject()));
                } else if (f->stmt_->object()) {
                    Printf("\t%s.%s\n", hoc_object_name(f->stmt_->object()), f->stmt_->name());
                } else {
                    Printf("\t%s\n", f->stmt_->name());
                }
            }
        }
    }
    return 0.;
}

static Member_func members[] = {{"allprint", allprint}, {nullptr, nullptr}};

static void* finithnd_cons(Object*) {
    int type = 1;  // default is after INITIAL blocks are called
    int ia = 1;
    if (hoc_is_double_arg(ia)) {
        type = (int) chkarg(ia, 0, 3);
        ++ia;
    }
    char* s = NULL;
    Object* pyact = NULL;
    if (hoc_is_object_arg(ia)) {
        pyact = *hoc_objgetarg(ia);
        if (!pyact) {
            hoc_execerror("arg is None", 0);
        }
    } else {
        s = gargstr(ia);
    }
    ++ia;
    Object* obj = NULL;
    if (ifarg(ia)) {
        obj = *hoc_objgetarg(ia);
    }
    FInitialHandler* f = new FInitialHandler(type, s, obj, pyact);
    return f;
}

static void finithnd_destruct(void* v) {
    FInitialHandler* f = (FInitialHandler*) v;
    delete f;
}

void FInitializeHandler_reg() {
    class2oc("FInitializeHandler", finithnd_cons, finithnd_destruct, members, NULL, NULL, NULL);
}

std::vector<FInitialHandler*> FInitialHandler::fihlist_[4];

FInitialHandler::FInitialHandler(int i, const char* s, Object* obj, Object* pyact) {
    type_ = i;
    if (pyact) {
        stmt_ = new HocCommand(pyact);
    } else {
        stmt_ = new HocCommand(s, obj);
    }
    fihlist_[i].push_back(this);
}

FInitialHandler::~FInitialHandler() {
    delete stmt_;
    erase_first(fihlist_[type_], this);
}
