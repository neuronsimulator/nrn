/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef netcvode_h
#define netcvode_h

#include "coreneuron/nrniv/tqueue.h"

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

class DiscreteEvent;
class NetCvode;

extern NetCvode* net_cvode_instance;

struct InterThreadEvent {
    DiscreteEvent* de_;
    double t_;
};

class NetCvodeThreadData {
  public:
    int unreffed_event_cnt_;
    TQueue<QTYPE>* tqe_;
    std::vector<InterThreadEvent> inter_thread_events_;
    MUTDEC

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

#endif
