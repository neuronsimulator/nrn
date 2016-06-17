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

#ifndef netcon_h
#define netcon_h

#include "coreneuron/nrnmpi/nrnmpi.h"

#undef check
#if MAC
#define NetCon nrniv_Dinfo
#endif

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
	virtual int type() { return DiscreteEventType; }

    virtual void pr(const char*, double t, NetCvode*);
};

class NetCon : public DiscreteEvent {
public:
    Point_process* target_;
    double delay_;
    double* weight_;
    bool active_;

	NetCon();
	virtual ~NetCon();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
    virtual void deliver(double,  NetCvode* ns, NrnThread*);
	virtual int type() { return NetConType; }
};

class SelfEvent : public DiscreteEvent {
public:
    double flag_;
    Point_process* target_;
    double* weight_;
    void** movable_; // actually a TQItem**

	SelfEvent();
	virtual ~SelfEvent();
	virtual void deliver(double, NetCvode*, NrnThread*);
    virtual int type() { return SelfEventType; }
    virtual void pr(const char*, double t, NetCvode*);

private:
	void call_net_receive(NetCvode*);
};


class ConditionEvent : public DiscreteEvent {
public:
    // condition detection factored out of PreSyn for re-use
    ConditionEvent();
    virtual ~ConditionEvent();
    virtual void check(NrnThread*, double sendtime, double teps = 0.0);
    virtual double value() { return -1.; }

    bool flag_; // true when below, false when above.
};


class PreSyn : public ConditionEvent {
public:
    Point_process* pntsrc_; /// Needed for prcellstate currently
    NrnThread* nt_;
    double* thvar_;
    double threshold_;
    int nc_index_; //replaces dil_, index into global NetCon** netcon_in_presyn_order_ for the current PreSyn
    int nc_cnt_; // how many netcon PreSyn has starting at nc_index_
    int output_index_;
    int gid_;
#if NRNMPI
    unsigned char localgid_; // compressed gid for spike transfer
#endif

	PreSyn();
	virtual ~PreSyn();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual int type() { return PreSynType; }

    virtual double value() { return *thvar_ - threshold_; }
	void record(double t);
};

class InputPreSyn : public DiscreteEvent {
public:
    int nc_index_; //replaces dil_, index into global NetCon** netcon_in_presyn_order_
    int nc_cnt_; // how many netcon starting at nc_index_

	InputPreSyn();
	virtual ~InputPreSyn();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual int type() { return InputPreSynType; }


};

class NetParEvent : public DiscreteEvent {
public:
    int ithread_; // for pr()
    double wx_, ws_; // exchange time and "spikes to Presyn" time

	NetParEvent();
	virtual ~NetParEvent();
	virtual void send(double, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
    virtual int type() { return NetParEventType; }

	virtual void pr(const char*, double t, NetCvode*);
};

#endif
