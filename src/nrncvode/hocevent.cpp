#include <objcmd.h>
#include <pool.h>
#include <netcon.h>
#include <nrnoc2iv.h>
#include <mymath.h>

using HocEventPool = MutexPool<HocEvent>;
HocEventPool* HocEvent::hepool_;

HocEvent::HocEvent() {
    stmt_ = nullptr;
    ppobj_ = nullptr;
    reinit_ = 0;
}

HocEvent::~HocEvent() {
    if (stmt_) {
        delete stmt_;
    }
}

void HocEvent::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s HocEvent %s %.15g\n", s, stmt_ ? stmt_->name() : "", tt);
}

HocEvent* HocEvent::alloc(const char* stmt, Object* ppobj, int reinit, Object* pyact) {
    if (!hepool_) {
        nrn_hoc_lock();
        if (!hepool_) {
            hepool_ = new HocEventPool(100, 1);
        }
        nrn_hoc_unlock();
    }
    HocEvent* he = hepool_->alloc();
    he->stmt_ = nullptr;
    he->ppobj_ = ppobj;
    he->reinit_ = reinit;
    if (pyact) {
        he->stmt_ = new HocCommand(pyact);
    } else if (stmt) {
        he->stmt_ = new HocCommand(stmt);
    }
    return he;
}

void HocEvent::hefree() {
    if (stmt_) {
        delete stmt_;
        stmt_ = nullptr;
    }
    hepool_->hpfree(this);
}

void HocEvent::clear() {
    if (stmt_) {
        delete stmt_;
        stmt_ = nullptr;
    }
}

void HocEvent::deliver(double tt, NetCvode* nc, NrnThread* nt) {
    extern double t;
    if (!ppobj_) {
        nc->allthread_handle(tt, this, nt);
        return;
    }
    if (stmt_) {
        if (nrn_nthread > 1 || nc->is_local()) {
            if (!ppobj_) {
                hoc_execerror(
                    "multiple threads and/or local variable time step method require an "
                    "appropriate POINT_PROCESS arg to CVode.event to safely execute:",
                    stmt_->name());
            }
            Cvode* cv = (Cvode*) ob2pntproc(ppobj_)->nvi_;
            if (cv && cvode_active_) {
                nc->local_retreat(tt, cv);
                if (reinit_) {
                    cv->set_init_flag();
                }
                nt->_t = cv->t_;
            }
            nrn_hoc_lock();
            t = tt;
        } else if (cvode_active_ && reinit_) {
            nc->retreat(tt, nc->gcv_);
            assert(MyMath::eq(tt, nc->gcv_->t_, NetCvode::eps(tt)));
            assert(tt == nt->_t);
            nc->gcv_->set_init_flag();
            t = tt;
        } else {
            t = nt_t = tt;
        }
        stmt_->execute(false);
        if (nrn_nthread > 1 || nc->is_local()) {
            nrn_hoc_unlock();
        }
    }
    hefree();
}

void HocEvent::allthread_handle() {
    if (stmt_) {
        stmt_->execute(false);
    } else {
        tstopset;
    }
    hefree();
}

void HocEvent::pgvts_deliver(double tt, NetCvode* nc) {
    deliver(tt, nc, nrn_threads);
}

void HocEvent::reclaim() {
    if (hepool_) {
        hepool_->free_all();
    }
}

DiscreteEvent* HocEvent::savestate_save() {
    //	pr("HocEvent::savestate_save", 0, net_cvode_instance);
    HocEvent* he = new HocEvent();
    if (stmt_) {
        if (stmt_->pyobject()) {
            he->stmt_ = new HocCommand(stmt_->pyobject());
        } else {
            he->stmt_ = new HocCommand(stmt_->name(), stmt_->object());
        }
        he->reinit_ = reinit_;
        he->ppobj_ = ppobj_;
    }
    return he;
}

void HocEvent::savestate_restore(double tt, NetCvode* nc) {
    //	pr("HocEvent::savestate_restore", tt, nc);
    HocEvent* he = alloc(nullptr, nullptr, 0);
    NrnThread* nt = nrn_threads;
    if (stmt_) {
        if (stmt_->pyobject()) {
            he->stmt_ = new HocCommand(stmt_->pyobject());
        } else {
            he->stmt_ = new HocCommand(stmt_->name(), stmt_->object());
        }
        he->reinit_ = reinit_;
        he->ppobj_ = ppobj_;
        if (ppobj_) {
            nt = (NrnThread*) ob2pntproc(ppobj_)->_vnt;
        }
    }
    nc->event(tt, he, nt);
}

DiscreteEvent* HocEvent::savestate_read(FILE* f) {
    HocEvent* he = new HocEvent();
    int have_stmt, have_obj, index;
    char stmt[256], objname[100], buf[200];
    Object* obj = nullptr;
    //	nrn_assert(fscanf(f, "%d %d\n", &have_stmt, &have_obj) == 2);
    nrn_assert(fgets(buf, 200, f));
    nrn_assert(sscanf(buf, "%d %d\n", &have_stmt, &have_obj) == 2);
    if (have_stmt) {
        nrn_assert(fgets(stmt, 256, f));
        stmt[strlen(stmt) - 1] = '\0';
        if (have_obj) {
            //			nrn_assert(fscanf(f, "%s %d\n", objname, &index) == 1);
            nrn_assert(fgets(buf, 200, f));
            nrn_assert(sscanf(buf, "%s %d\n", objname, &index) == 1);
            obj = hoc_name2obj(objname, index);
        }
        he->stmt_ = new HocCommand(stmt, obj);
    }
    return he;
}

void HocEvent::savestate_write(FILE* f) {
    fprintf(f, "%d\n", HocEventType);
    fprintf(f, "%d %d\n", stmt_ ? 1 : 0, (stmt_ && stmt_->object()) ? 1 : 0);
    if (stmt_) {
        fprintf(f, "%s\n", stmt_->name());
        if (stmt_->object()) {
            fprintf(f, "%s %d\n", stmt_->object()->ctemplate->sym->name, stmt_->object()->index);
        }
    }
}
