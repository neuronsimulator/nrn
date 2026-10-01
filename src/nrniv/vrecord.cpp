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
#include "vecplay_tplus.h"
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

void VecPlayContinuous::forcing_tplus(double tt, double* value, double* deriv) const {
    // Full sample arrays; active upper knot is ubound_index_ (same as interpolate).
    const int n = t_ ? static_cast<int>(t_->size()) : 0;
    // IvocVect stores doubles contiguously via data() / &elem(0)
    const double* y = (y_ && n > 0) ? &y_->elem(0) : nullptr;
    const double* tv = (t_ && n > 0) ? &t_->elem(0) : nullptr;
    if (nrn_vecplay_continuous_tplus(n, y, tv, tt, ubound_index_, value, deriv) != 0) {
        if (value) {
            *value = 0.;
        }
        if (deriv) {
            *deriv = 0.;
        }
    }
}

int nrn_collect_forcing_tplus(double tt, std::vector<NrnForcingTPlus>& out) {
    out.clear();
    if (!net_cvode_instance) {
        return 0;
    }
    std::vector<PlayRecord*>* prl = net_cvode_instance->playrec_list();
    if (!prl) {
        return 0;
    }
    for (std::size_t i = 0; i < prl->size(); ++i) {
        PlayRecord* pr = (*prl)[i];
        if (!pr || pr->type() != VecPlayContinuousType) {
            continue;
        }
        auto* vpc = static_cast<VecPlayContinuous*>(pr);
        NrnForcingTPlus e;
        e.playrec_index = static_cast<int>(i);
        e.ubound_index = vpc->ubound_index_;
        vpc->forcing_tplus(tt, &e.value, &e.deriv);
        e.label[0] = '\0';
        if (vpc->y_ && vpc->y_->obj_) {
            const char* nm = hoc_object_name(vpc->y_->obj_);
            if (nm) {
                std::snprintf(e.label, sizeof e.label, "%s", nm);
            }
        }
        if (e.label[0] == '\0') {
            std::snprintf(e.label, sizeof e.label, "VecPlayContinuous[%d]", e.playrec_index);
        }
        out.push_back(e);
    }
    return static_cast<int>(out.size());
}

void nrn_dump_forcing_tplus(FILE* f, double tt, const std::vector<NrnForcingTPlus>& entries) {
    if (!f) {
        return;
    }
    fprintf(f,
            "--- forcing t+ info (t=%.15g)  n_play_continuous=%d ---\n",
            tt,
            (int) entries.size());
    fprintf(f, "  (right-limit u and classical u'; 1-jet of exogenous continuous Vector.play)\n");
    if (entries.empty()) {
        fprintf(f, "  (none)\n");
        return;
    }
    fprintf(f, "  %4s %10s %16s %16s  %s\n", "idx", "ubound", "u(t+)", "u'(t+)", "label");
    for (const auto& e: entries) {
        fprintf(f,
                "  %4d %10d %16.8g %16.8g  %s\n",
                e.playrec_index,
                e.ubound_index,
                e.value,
                e.deriv,
                e.label);
    }
}

double VecPlayContinuous::interpolate(double tt) {
    // Keep last_index_ cache in sync with historical search side effects, then
    // evaluate with the shared t⁺ geometry (value only). Value at/after t0
    // matches prior play (linear segments); only the t⁺ *derivative* treatment
    // of t==t0 differs for forcing (outgoing slope).
    if (tt >= t_->elem(ubound_index_)) {
        last_index_ = ubound_index_;
        if (last_index_ == 0) {
            return y_->elem(last_index_);
        }
    } else if (tt < t_->elem(0)) {
        last_index_ = 0;
        return y_->elem(0);
    } else if (tt == t_->elem(0)) {
        last_index_ = (ubound_index_ > 0) ? 1 : 0;
        if (last_index_ == 0) {
            return y_->elem(0);
        }
        // fall through to shared evaluator via tplus (value = y0 on first segment)
    } else {
        search(tt);
    }
    double value = 0.;
    const int n = static_cast<int>(t_->size());
    nrn_vecplay_continuous_tplus(n, &y_->elem(0), &t_->elem(0), tt, ubound_index_, &value, nullptr);
    return value;
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
