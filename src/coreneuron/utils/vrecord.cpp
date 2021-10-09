/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <cstdio>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/ivocvect.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/utils/vrecitem.h"
namespace coreneuron {
extern NetCvode* net_cvode_instance;

void PlayRecordEvent::deliver(double tt, NetCvode* ns, NrnThread*) {
    plr_->deliver(tt, ns);
}

NrnThread* PlayRecordEvent::thread() {
    return nrn_threads + plr_->ith_;
}

void PlayRecordEvent::pr(const char* s, double tt, NetCvode*) {
    printf("%s PlayRecordEvent %.15g ", s, tt);
    plr_->pr();
}

PlayRecord::PlayRecord(double* pd, int ith)
    : pd_(pd)
    , ith_(ith) {}

void PlayRecord::pr() {
    printf("PlayRecord\n");
}

VecPlayContinuous::VecPlayContinuous(double* pd,
                                     IvocVect&& yvec,
                                     IvocVect&& tvec,
                                     IvocVect* discon,
                                     int ith)
    : PlayRecord(pd, ith)
    , y_(std::move(yvec))
    , t_(std::move(tvec))
    , discon_indices_(discon)
    , e_(new PlayRecordEvent{}) {
    e_->plr_ = this;
}

VecPlayContinuous::~VecPlayContinuous() {
    delete e_;
}

void VecPlayContinuous::play_init() {
    NrnThread* nt = nrn_threads + ith_;
    last_index_ = 0;
    discon_index_ = 0;
    if (discon_indices_) {
        if (discon_indices_->size() > 0) {
            ubound_index_ = (int) (*discon_indices_)[discon_index_++];
            // printf("play_init %d %g\n", ubound_index_, t_->elem(ubound_index_));
            e_->send(t_[ubound_index_], net_cvode_instance, nt);
        } else {
            ubound_index_ = t_.size() - 1;
        }
    } else {
        ubound_index_ = 0;
        e_->send(t_[ubound_index_], net_cvode_instance, nt);
    }
}

void VecPlayContinuous::deliver(double tt, NetCvode* ns) {
    NrnThread* nt = nrn_threads + ith_;
    // printf("deliver %g\n", tt);
    last_index_ = ubound_index_;
    // clang-format off

    #pragma acc update device(last_index_) if (nt->compute_gpu)
    // clang-format on
    if (discon_indices_) {
        if (discon_index_ < discon_indices_->size()) {
            ubound_index_ = (int) (*discon_indices_)[discon_index_++];
            // printf("after deliver:send %d %g\n", ubound_index_, t_->elem(ubound_index_));
            e_->send(t_[ubound_index_], ns, nt);
        } else {
            ubound_index_ = t_.size() - 1;
        }
    } else {
        if (ubound_index_ < t_.size() - 1) {
            ubound_index_++;
            e_->send(t_[ubound_index_], ns, nt);
        }
    }
    // clang-format off

    #pragma acc update device(ubound_index_) if (nt->compute_gpu)
    // clang-format on
    continuous(tt);
}

void VecPlayContinuous::continuous(double tt) {
    NrnThread* nt = nrn_threads + ith_;
    // clang-format off

    #pragma acc kernels present(this) if(nt->compute_gpu)
    {
        *pd_ = interpolate(tt);
    }
    // clang-format on
}

double VecPlayContinuous::interpolate(double tt) {
    if (tt >= t_[ubound_index_]) {
        last_index_ = ubound_index_;
        if (last_index_ == 0) {
            // printf("return last tt=%g ubound=%g y=%g\n", tt, t_->elem(ubound_index_),
            // y_->elem(last_index_));
            return y_[last_index_];
        }
    } else if (tt <= t_[0]) {
        last_index_ = 0;
        // printf("return elem(0) tt=%g t0=%g y=%g\n", tt, t_->elem(0), y_->elem(0));
        return y_[0];
    } else {
        search(tt);
    }
    double x0 = y_[last_index_ - 1];
    double x1 = y_[last_index_];
    double t0 = t_[last_index_ - 1];
    double t1 = t_[last_index_];
    // printf("IvocVectRecorder::continuous tt=%g t0=%g t1=%g theta=%g x0=%g x1=%g\n", tt, t0, t1,
    // (tt - t0)/(t1 - t0), x0, x1);
    if (t0 == t1) {
        return (x0 + x1) / 2.;
    }
    return interp((tt - t0) / (t1 - t0), x0, x1);
}

void VecPlayContinuous::search(double tt) {
    //	assert (tt > t_->elem(0) && tt < t_->elem(t_->size() - 1))
    while (tt < t_[last_index_]) {
        --last_index_;
    }
    while (tt >= t_[last_index_]) {
        ++last_index_;
    }
}

void VecPlayContinuous::pr() {
    printf("VecPlayContinuous ");
    // printf("%s.x[%d]\n", hoc_object_name(y_->obj_), last_index_);
}
}  // namespace coreneuron
