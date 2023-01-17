/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/network/netcon.hpp"
#include "coreneuron/utils/ivocvect.hpp"
namespace coreneuron {
class PlayRecord;

#define PlayRecordType        0
#define VecPlayContinuousType 4
#define PlayRecordEventType   21

// used by PlayRecord subclasses that utilize discrete events
class PlayRecordEvent: public DiscreteEvent {
  public:
    PlayRecordEvent() = default;
    virtual ~PlayRecordEvent() = default;
    virtual void deliver(double, NetCvode*, NrnThread*) override;
    virtual void pr(const char*, double t, NetCvode*) override;
    virtual NrnThread* thread();
    PlayRecord* plr_;
    static unsigned long playrecord_send_;
    static unsigned long playrecord_deliver_;
    virtual int type() const override {
        return PlayRecordEventType;
    }
};

// common interface for Play and Record for all integration methods.
class PlayRecord {
  public:
    PlayRecord(double* pd, int ith);
    virtual ~PlayRecord() = default;
    virtual void play_init() {}  // called near beginning of finitialize
    virtual void continuous(double) {
    }  // play - every f(y, t) or res(y', y, t); record - advance_tn and initialize flag
    virtual void deliver(double, NetCvode*) {}  // at associated DiscreteEvent
    virtual PlayRecordEvent* event() {
        return nullptr;
    }
    virtual void pr();  // print identifying info
    virtual int type() const {
        return PlayRecordType;
    }

    double* pd_;
    int ith_;  // The thread index
};

class VecPlayContinuous: public PlayRecord {
  public:
    VecPlayContinuous(double*, IvocVect&& yvec, IvocVect&& tvec, IvocVect* discon, int ith);
    virtual ~VecPlayContinuous();
    virtual void play_init() override;
    virtual void deliver(double tt, NetCvode*) override;
    virtual PlayRecordEvent* event() override {
        return e_;
    }
    virtual void pr() override;

    void continuous(double tt) override;
    double interpolate(double tt);
    double interp(double th, double x0, double x1) {
        return x0 + (x1 - x0) * th;
    }
    void search(double tt);

    virtual int type() const override {
        return VecPlayContinuousType;
    }

    IvocVect y_;
    IvocVect t_;
    IvocVect* discon_indices_;
    std::size_t last_index_{};
    std::size_t discon_index_{};
    std::size_t ubound_index_{};

    PlayRecordEvent* e_ = nullptr;  // Need to be a raw pointer for acc
};
}  // namespace coreneuron
