/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/utils/nrnmutdec.hpp"
#include "coreneuron/network/tqueue.hpp"

#define PRINT_EVENT 0

/** QTYPE options include: spltree, pq_que
 *  STL priority queue is used instead of the splay tree by default.
 *  @todo: check if stl queue works with move_event functions.
 */

#ifdef ENABLE_SPLAYTREE_QUEUING
#define QTYPE spltree
#else
#define QTYPE pq_que
#endif
namespace coreneuron {

// defined in coreneuron/network/cvodestb.cpp
extern void init_net_events(void);
extern void nrn_play_init(void);
extern void deliver_net_events(NrnThread*);
extern void nrn_deliver_events(NrnThread*);
extern void fixed_play_continuous(NrnThread*);

struct DiscreteEvent;
class NetCvode;

extern NetCvode* net_cvode_instance;
extern void interthread_enqueue(NrnThread*);

struct InterThreadEvent {
    DiscreteEvent* de_;
    double t_;
};

class NetCvodeThreadData {
  public:
    int unreffed_event_cnt_ = 0;
    TQueue<QTYPE>* tqe_;
    std::vector<InterThreadEvent> inter_thread_events_;
    OMP_Mutex mut;

    NetCvodeThreadData();
    virtual ~NetCvodeThreadData();
    void interthread_send(double, DiscreteEvent*, NrnThread*);
    void enqueue(NetCvode*, NrnThread*);
};

class NetCvode {
  public:
    int print_event_;
    int pcnt_;
    int enqueueing_;
    NetCvodeThreadData* p;
    static double eps_;

    NetCvode(void);
    virtual ~NetCvode();
    void p_construct(int);
    void check_thresh(NrnThread*);
    static double eps(double x) {
        return eps_ * fabs(x);
    }
    TQItem* event(double tdeliver, DiscreteEvent*, NrnThread*);
    void move_event(TQItem*, double, NrnThread*);
    TQItem* bin_event(double tdeliver, DiscreteEvent*, NrnThread*);
    void deliver_net_events(NrnThread*);          // for default staggered time step method
    void deliver_events(double til, NrnThread*);  // for initialization events
    bool deliver_event(double til, NrnThread*);   // uses TQueue atomically
    void clear_events();
    void init_events();
    void point_receive(int, Point_process*, double*, double);
};
}  // namespace coreneuron
