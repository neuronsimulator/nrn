#include <../../nrnconf.h>

#if HAVE_IV
#include "ivoc.h"
#endif
#include "nrniv_mf.h"
#include "nrnoc2iv.h"
#include "ocobserv.h"
#include "ivocvect.h"
#include <stdio.h>

#include "ocpointer.h"
#include "vrecitem.h"
#include "netcvode.h"
#include "cvodeobj.h"

extern double t;
extern NetCvode* net_cvode_instance;

// Vector.play_remove()
void nrn_vecsim_remove(void* v) {
    PlayRecord* pr;
    while ((pr = net_cvode_instance->playrec_uses(v)) != 0) {
        delete pr;
    }
}

void nrn_vecsim_add(void* v, bool record) {
    IvocVect *yvec, *tvec, *dvec;
    extern short* nrn_is_artificial_;
    char* s = NULL;
    double ddt;
    Object* ppobj = NULL;
    int iarg = 0;

    yvec = (IvocVect*) v;

    if (hoc_is_object_arg(1)) {
        iarg = 1;
        ppobj = *hoc_objgetarg(1);
        if (!ppobj || ppobj->ctemplate->is_point_ <= 0 ||
            nrn_is_artificial_[ob2pntproc(ppobj)->prop->_type]) {
            hoc_execerror("Optional first arg is not a POINT_PROCESS", 0);
        }
    }
    neuron::container::data_handle<double> dh{};
    if (record == false && hoc_is_str_arg(iarg + 1)) {  // statement involving $1
        // Vector.play("proced($1)", ...)
        s = gargstr(iarg + 1);
    } else if (record == false && hoc_is_double_arg(iarg + 1)) {  // play that element
                                                                  // Vector.play(index)
        // must be a VecPlayStep and nothing else
        VecPlayStep* vps = (VecPlayStep*) net_cvode_instance->playrec_uses(v);
        if (vps) {
            int j = (int) chkarg(iarg + 1, 0., yvec->size() - 1);
            if (vps->si_) {
                vps->si_->play_one(yvec->elem(j));
            }
        }
        return;
    } else {
        // Vector.play(&SEClamp[0].amp1, ...)
        // Vector.record(&SEClamp[0].i, ...)
        dh = hoc_hgetarg<double>(iarg + 1);
    }
    tvec = NULL;
    dvec = NULL;
    ddt = -1.;
    int con = 0;
    if (ifarg(iarg + 2)) {
        if (hoc_is_object_arg(iarg + 2)) {
            // Vector...(..., tvec)
            tvec = vector_arg(iarg + 2);
        } else {
            // Vector...(..., Dt)
            ddt = chkarg(iarg + 2, 1e-9, 1e10);
        }
        if (ifarg(iarg + 3)) {
            if (hoc_is_double_arg(iarg + 3)) {
                con = (int) chkarg(iarg + 3, 0., 1.);
            } else {
                dvec = vector_arg(iarg + 3);
                con = 1;
            }
        }
    }

    // tvec can be used for many record/play items
    //	if (tvec) { nrn_vecsim_remove(tvec); }
    if (record) {
        // yvec can be used only for one record (but many play)
        if (yvec) {
            nrn_vecsim_remove(yvec);
        }
        if (tvec) {
            new VecRecordDiscrete(std::move(dh), yvec, tvec, ppobj);
        } else if (ddt > 0.) {
            new VecRecordDt(std::move(dh), yvec, ddt, ppobj);
        } else if (static_cast<double const*>(dh) == &t) {
            new TvecRecord(chk_access(), yvec, ppobj);
        } else {
            new YvecRecord(std::move(dh), yvec, ppobj);
        }
    } else {
        if (con) {
            if (!tvec) {
                hoc_execerror(
                    "Second argument of Vector.play in continuous mode must be a time vector", 0);
            }
            if (s) {
                new VecPlayContinuous(s, yvec, tvec, dvec, ppobj);
            } else {
                new VecPlayContinuous(std::move(dh), yvec, tvec, dvec, ppobj);
            }
        } else {
            if (!tvec && ddt == -1.) {
                chkarg(iarg + 2, 1e-9, 1e10);
            }
            if (s) {
                new VecPlayStep(s, yvec, tvec, ddt, ppobj);
            } else {
                new VecPlayStep(std::move(dh), yvec, tvec, ddt, ppobj);
            }
        }
    }
}

VecPlayStep::VecPlayStep(neuron::container::data_handle<double> dh,
                         IvocVect* y,
                         IvocVect* t,
                         double dt,
                         Object* ppobj)
    : PlayRecord(std::move(dh), ppobj) {
    // printf("VecPlayStep\n");
    init(y, t, dt);
}

VecPlayStep::VecPlayStep(const char* s, IvocVect* y, IvocVect* t, double dt, Object* ppobj)
    : PlayRecord(chk_access()->pnode[0]->v_handle(), ppobj) {
    // printf("VecPlayStep\n");
    init(y, t, dt);
    si_ = new StmtInfo(s);
}

void VecPlayStep::init(IvocVect* y, IvocVect* t, double dt) {
    y_ = y;
    t_ = t;
    dt_ = dt;
    ObjObservable::Attach(y_->obj_, this);
    if (t_) {
        ObjObservable::Attach(t_->obj_, this);
    }
    e_ = new PlayRecordEvent();
    e_->plr_ = this;
    si_ = NULL;
}


VecPlayStep::~VecPlayStep() {
    // printf("~VecPlayStep\n");
    ObjObservable::Detach(y_->obj_, this);
    if (t_) {
        ObjObservable::Detach(t_->obj_, this);
    }
    delete e_;
    if (si_) {
        delete si_;
    }
}

void VecPlayStep::disconnect(Observable*) {
    //	printf("%s VecPlayStep disconnect\n", hoc_object_name(y_->obj_));
    delete this;
}

void VecPlayStep::install(Cvode* cv) {
    play_add(cv);
}

void VecPlayStep::play_init() {
    current_index_ = 0;
    NrnThread* nt = nrn_threads;
    if (cvode_ && cvode_->nth_) {
        nt = cvode_->nth_;
    }
    if (t_) {
        if (t_->size() > 0) {
            e_->send(t_->elem(0), net_cvode_instance, nt);
        }
    } else {
        e_->send(0., net_cvode_instance, nt);
    }
}

void VecPlayStep::deliver(double tt, NetCvode* ns) {
    NrnThread* nt = nrn_threads + ith_;
    if (cvode_) {
        cvode_->set_init_flag();
        if (cvode_->nth_) {
            nt = cvode_->nth_;
        }
    }
    if (si_) {
        t = tt;
        nrn_hoc_lock();
        si_->play_one(y_->elem(current_index_++));
        nrn_hoc_unlock();
    } else {
        auto const val = y_->elem(current_index_++);
        if (pd_) {
            *pd_ = val;
        } else {
            std::ostringstream oss;
            oss << "VecPlayStep::deliver: invalid " << pd_;
            throw std::runtime_error(std::move(oss).str());
        }
    }
    if (current_index_ < y_->size()) {
        if (t_) {
            if (current_index_ < t_->size()) {
                e_->send(t_->elem(current_index_), ns, nt);
            }
        } else {
            e_->send(tt + dt_, ns, nt);
        }
    }
}


void VecPlayStep::pr() {
    Printf("VecPlayStep ");
    Printf("%s.x[%d]\n", hoc_object_name(y_->obj_), current_index_);
}

VecPlayContinuous::VecPlayContinuous(neuron::container::data_handle<double> pd,
                                     IvocVect* y,
                                     IvocVect* t,
                                     IvocVect* discon,
                                     Object* ppobj)
    : PlayRecord(std::move(pd), ppobj) {
    // printf("VecPlayContinuous\n");
    init(y, t, discon);
}

VecPlayContinuous::VecPlayContinuous(const char* s,
                                     IvocVect* y,
                                     IvocVect* t,
                                     IvocVect* discon,
                                     Object* ppobj)
    : PlayRecord(chk_access()->pnode[0]->v_handle(), ppobj) {
    // printf("VecPlayContinuous\n");
    init(y, t, discon);
    si_ = new StmtInfo(s);
}

void VecPlayContinuous::init(IvocVect* y, IvocVect* t, IvocVect* discon) {
    y_ = y;
    t_ = t;
    discon_indices_ = discon;
    ubound_index_ = 0;
    last_index_ = 0;
    ObjObservable::Attach(y_->obj_, this);
    if (t_) {
        ObjObservable::Attach(t_->obj_, this);
    }
    if (discon_indices_) {
        ObjObservable::Attach(discon_indices_->obj_, this);
    }
    e_ = new PlayRecordEvent();
    e_->plr_ = this;
    si_ = NULL;
}


VecPlayContinuous::~VecPlayContinuous() {
    // printf("~VecPlayContinuous\n");
    ObjObservable::Detach(y_->obj_, this);
    ObjObservable::Detach(t_->obj_, this);
    if (discon_indices_) {
        ObjObservable::Detach(discon_indices_->obj_, this);
    }
    delete e_;
    if (si_) {
        delete si_;
    }
}

void VecPlayContinuous::disconnect(Observable*) {
    //	printf("%s VecPlayContinuous disconnect\n", hoc_object_name(y_->obj_));
    delete this;
}

void VecPlayContinuous::install(Cvode* cv) {
    play_add(cv);
}

void VecPlayContinuous::play_init() {
    NrnThread* nt = nrn_threads;
    if (cvode_ && cvode_->nth_) {
        nt = cvode_->nth_;
    }
    last_index_ = 0;
    discon_index_ = 0;
    if (discon_indices_) {
        if (discon_indices_->size() > 0) {
            ubound_index_ = (int) discon_indices_->elem(discon_index_++);
            // printf("play_init %d %g\n", ubound_index_, t_->elem(ubound_index_));
            e_->send(t_->elem(ubound_index_), net_cvode_instance, nt);
        } else {
            ubound_index_ = t_->size() - 1;
        }
    } else {
        ubound_index_ = 0;
        e_->send(t_->elem(ubound_index_), net_cvode_instance, nt);
    }
}

void VecPlayContinuous::deliver(double tt, NetCvode* ns) {
    NrnThread* nt = nrn_threads + ith_;
    if (cvode_) {
        cvode_->set_init_flag();
        if (cvode_->nth_) {
            nt = cvode_->nth_;
        }
    }
    last_index_ = ubound_index_;
    if (discon_indices_) {
        if (discon_index_ < discon_indices_->size()) {
            ubound_index_ = (int) discon_indices_->elem(discon_index_++);
            // printf("after deliver:send %d %g\n", ubound_index_, t_->elem(ubound_index_));
            e_->send(t_->elem(ubound_index_), ns, nt);
        } else {
            ubound_index_ = t_->size() - 1;
        }
    } else {
        if (ubound_index_ < t_->size() - 1) {
            ubound_index_++;
            e_->send(t_->elem(ubound_index_), ns, nt);
        }
    }
    continuous(tt);
}


void VecPlayContinuous::continuous(double tt) {
    if (si_) {
        t = tt;
        nrn_hoc_lock();
        si_->play_one(interpolate(tt));
        nrn_hoc_unlock();
    } else {
        *pd_ = interpolate(tt);
    }
}

double VecPlayContinuous::interpolate(double tt) {
    if (tt >= t_->elem(ubound_index_)) {
        last_index_ = ubound_index_;
        if (last_index_ == 0) {
            // printf("return last tt=%g ubound=%g y=%g\n", tt, t_->elem(ubound_index_),
            // y_->elem(last_index_));
            return y_->elem(last_index_);
        }
    } else if (tt <= t_->elem(0)) {
        last_index_ = 0;
        // printf("return elem(0) tt=%g t0=%g y=%g\n", tt, t_->elem(0), y_->elem(0));
        return y_->elem(0);
    } else {
        search(tt);
    }
    double x0 = y_->elem(last_index_ - 1);
    double x1 = y_->elem(last_index_);
    double t0 = t_->elem(last_index_ - 1);
    double t1 = t_->elem(last_index_);
    // printf("IvocVectRecorder::continuous tt=%g t0=%g t1=%g theta=%g x0=%g x1=%g\n", tt, t0, t1,
    // (tt - t0)/(t1 - t0), x0, x1);
    if (t0 == t1) {
        return (x0 + x1) / 2.;
    }
    return interp((tt - t0) / (t1 - t0), x0, x1);
#if 0
	// dt
	double theta = tt/dt_ - last_index_;
	interp(theta, x0, x1);
#endif
}

void VecPlayContinuous::search(double tt) {
    //	assert (tt > t_->elem(0) && tt < t_->elem(t_->size() - 1))
    while (tt < t_->elem(last_index_)) {
        --last_index_;
    }
    while (tt >= t_->elem(last_index_)) {
        ++last_index_;
    }
}

void VecPlayContinuous::pr() {
    printf("VecPlayContinuous ");
    printf("%s.x[%d]\n", hoc_object_name(y_->obj_), last_index_);
}

PlayRecordSave* VecPlayStep::savestate_save() {
    return new VecPlayStepSave(this);
}

VecPlayStepSave::VecPlayStepSave(PlayRecord* prl)
    : PlayRecordSave(prl) {
    curindex_ = ((VecPlayStep*) pr_)->current_index_;
}
VecPlayStepSave::~VecPlayStepSave() {}
void VecPlayStepSave::savestate_restore() {
    check();
    VecPlayStep* vps = (VecPlayStep*) pr_;
    vps->current_index_ = curindex_;
    if (curindex_ > 0) {
        if (vps->si_) {
            vps->si_->play_one(vps->y_->elem(curindex_ - 1));
        } else {
            *vps->pd_ = vps->y_->elem(curindex_ - 1);
        }
    }
}
void VecPlayStepSave::savestate_write(FILE* f) {
    fprintf(f, "%d\n", curindex_);
}
void VecPlayStepSave::savestate_read(FILE* f) {
    char buf[100];
    nrn_assert(fgets(buf, 100, f));
    nrn_assert(sscanf(buf, "%d\n", &curindex_) == 1);
}

PlayRecordSave* VecPlayContinuous::savestate_save() {
    return new VecPlayContinuousSave(this);
}

VecPlayContinuousSave::VecPlayContinuousSave(PlayRecord* prl)
    : PlayRecordSave(prl) {
    VecPlayContinuous* vpc = (VecPlayContinuous*) pr_;
    last_index_ = vpc->last_index_;
    discon_index_ = vpc->discon_index_;
    ubound_index_ = vpc->ubound_index_;
}
VecPlayContinuousSave::~VecPlayContinuousSave() {}
void VecPlayContinuousSave::savestate_restore() {
    check();
    VecPlayContinuous* vpc = (VecPlayContinuous*) pr_;
    vpc->last_index_ = last_index_;
    vpc->discon_index_ = discon_index_;
    vpc->ubound_index_ = ubound_index_;
    vpc->continuous(t);
}
void VecPlayContinuousSave::savestate_write(FILE* f) {
    fprintf(f, "%d %d %d\n", last_index_, discon_index_, ubound_index_);
}
void VecPlayContinuousSave::savestate_read(FILE* f) {
    char buf[100];
    nrn_assert(fgets(buf, 100, f));
    nrn_assert(sscanf(buf, "%d %d %d\n", &last_index_, &discon_index_, &ubound_index_) == 3);
}
