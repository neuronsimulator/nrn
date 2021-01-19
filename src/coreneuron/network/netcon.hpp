/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef netcon_h
#define netcon_h

#include "coreneuron/mpi/nrnmpi.h"

#undef check
#if MAC
#define NetCon nrniv_Dinfo
#endif
namespace coreneuron {
class PreSyn;
class InputPreSyn;
class TQItem;
struct NrnThread;
struct Point_process;
class NetCvode;

#define DiscreteEventType 0
#define TstopEventType 1
#define NetConType 2
#define SelfEventType 3
#define PreSynType 4
#define NetParEventType 7
#define InputPreSynType 20

class DiscreteEvent {
  public:
    DiscreteEvent();
    virtual ~DiscreteEvent();
    virtual void send(double deliverytime, NetCvode*, NrnThread*);
    virtual void deliver(double t, NetCvode*, NrnThread*);
    virtual int type() const {
        return DiscreteEventType;
    }
    virtual bool require_checkpoint() {
        return true;
    }
    virtual void pr(const char*, double t, NetCvode*);
};

class NetCon : public DiscreteEvent {
  public:
    bool active_;
    double delay_;
    Point_process* target_;
    union {
        int weight_index_;
        int srcgid_;  // only to help InputPreSyn during setup
        // before weights are read and stored. Saves on transient
        // memory requirements by avoiding storage of all group file
        // netcon_srcgid lists. ie. that info is copied into here.
    } u;

    NetCon();
    virtual ~NetCon();
    virtual void send(double sendtime, NetCvode*, NrnThread*) override;
    virtual void deliver(double, NetCvode* ns, NrnThread*) override;
    virtual int type() const override {
        return NetConType;
    }
    virtual void pr(const char*, double t, NetCvode*) override;
};

class SelfEvent : public DiscreteEvent {
  public:
    double flag_;
    Point_process* target_;
    void** movable_;  // actually a TQItem**
    int weight_index_;

    SelfEvent();
    virtual ~SelfEvent();
    virtual void deliver(double, NetCvode*, NrnThread*) override;
    virtual int type() const override {
        return SelfEventType;
    }

    virtual void pr(const char*, double t, NetCvode*) override;

  private:
    void call_net_receive(NetCvode*);
};

class ConditionEvent : public DiscreteEvent {
  public:
    // condition detection factored out of PreSyn for re-use
    ConditionEvent();
    virtual ~ConditionEvent();
    virtual bool check(NrnThread*);
    virtual double value(NrnThread*) {
        return -1.;
    }

    int flag_;  // true when below, false when above. (changed from bool to int to avoid cray acc
                // bug(?))
};

class PreSyn : public ConditionEvent {
  public:
#if NRNMPI
    unsigned char localgid_;  // compressed gid for spike transfer
#endif
    int nc_index_;  // replaces dil_, index into global NetCon** netcon_in_presyn_order_
    int nc_cnt_;    // how many netcon starting at nc_index_
    int output_index_;
    int gid_;
    double threshold_;
    int thvar_index_;  // >=0 points into NrnThread._actual_v
    Point_process* pntsrc_;

    PreSyn();
    virtual ~PreSyn();
    virtual void send(double sendtime, NetCvode*, NrnThread*) override;
    virtual void deliver(double, NetCvode*, NrnThread*) override;
    virtual int type() const override {
        return PreSynType;
    }

    virtual double value(NrnThread*) override;
    void record(double t);
#if NRN_MULTISEND
    int multisend_index_;
#endif
};

class InputPreSyn : public DiscreteEvent {
  public:
    int nc_index_;  // replaces dil_, index into global NetCon** netcon_in_presyn_order_
    int nc_cnt_;    // how many netcon starting at nc_index_

    InputPreSyn();
    virtual ~InputPreSyn();
    virtual void send(double sendtime, NetCvode*, NrnThread*) override;
    virtual void deliver(double, NetCvode*, NrnThread*) override;
    virtual int type() const override {
        return InputPreSynType;
    }
#if NRN_MULTISEND
    int multisend_phase2_index_;
#endif
};

class NetParEvent : public DiscreteEvent {
  public:
    int ithread_;     // for pr()
    double wx_, ws_;  // exchange time and "spikes to Presyn" time

    NetParEvent();
    virtual ~NetParEvent() = default;
    virtual void send(double, NetCvode*, NrnThread*) override;
    virtual void deliver(double, NetCvode*, NrnThread*) override;
    virtual int type() const override {
        return NetParEventType;
    }

    virtual void pr(const char*, double t, NetCvode*) override;
};
}  // namespace coreneuron
#endif
