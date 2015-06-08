/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/nrnconf.h"

#include <stdio.h>
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/ivocvect.h"
#include "coreneuron/nrniv/netcvode.h"
#include "coreneuron/nrniv/vrecitem.h"

extern NetCvode* net_cvode_instance;

PlayRecordEvent::PlayRecordEvent() {}
PlayRecordEvent::~PlayRecordEvent() {}

void PlayRecordEvent::frecord_init(TQItem* q) {
	plr_->frecord_init(q);
}

void PlayRecordEvent::deliver(double tt, NetCvode* ns, NrnThread*) {
	plr_->deliver(tt, ns);
}

NrnThread* PlayRecordEvent::thread() { return nrn_threads + plr_->ith_; }

void PlayRecordEvent::pr(const char* s, double tt, NetCvode*) {
	printf("%s PlayRecordEvent %.15g ", s, tt);
	plr_->pr();
}

PlayRecord::PlayRecord(double* pd, int ith) {
//printf("PlayRecord::PlayRecord %p\n", this);
	pd_ = pd;
	ith_ = ith;
}

PlayRecord::~PlayRecord() {
//printf("PlayRecord::~PlayRecord %p\n", this);
}

void PlayRecord::pr() {
	printf("PlayRecord\n");
}

VecPlayStep::VecPlayStep(double* pd, IvocVect* yvec, IvocVect* tvec, double dtt, int ith) : PlayRecord(pd, ith) {
//printf("VecPlayStep\n");
	init(yvec, tvec, dtt);
}

void VecPlayStep::init(IvocVect* yvec, IvocVect* tvec, double dtt) {
	y_ = yvec;
	t_ = tvec;
	dt_ = dtt;
	e_ = new PlayRecordEvent();
	e_->plr_ = this;
}


VecPlayStep::~VecPlayStep() {
//printf("~VecPlayStep\n");
	delete e_;
}

void VecPlayStep::play_init() {
	current_index_ = 0;
	NrnThread* nt = nrn_threads + ith_;
	if (t_) {
		if (t_->capacity() > 0) {
			e_->send(t_->elem(0), net_cvode_instance, nt);
		}
	}else{
			e_->send(0., net_cvode_instance, nt);
	}
}

void VecPlayStep::deliver(double tt, NetCvode* ns) {
	NrnThread* nt = nrn_threads + ith_;
	*pd_ = y_->elem(current_index_++);
	if (current_index_ < y_->capacity()) {
		if (t_) {
			if (current_index_ < t_->capacity()) {
				e_->send(t_->elem(current_index_), ns, nt);
			}
		}else{
			e_->send(tt + dt_, ns, nt);
		}
	}
}

	
void VecPlayStep::pr() {
	printf("VecPlayStep ");
//	printf("%s.x[%d]\n", hoc_object_name(y_->obj_), current_index_);
}

VecPlayContinuous::VecPlayContinuous(double* pd, IvocVect* yvec, IvocVect* tvec, IvocVect* discon, int ith) : PlayRecord(pd, ith) {
//printf("VecPlayContinuous\n");
	init(yvec, tvec, discon);
}

void VecPlayContinuous::init(IvocVect* yvec, IvocVect* tvec, IvocVect* discon) {
	y_ = yvec;
	t_ = tvec;
	discon_indices_ = discon;
	ubound_index_ = 0;
	last_index_ = 0;
	e_ = new PlayRecordEvent();
	e_->plr_ = this;
}


VecPlayContinuous::~VecPlayContinuous() {
//printf("~VecPlayContinuous\n");
	delete e_;
}

void VecPlayContinuous::play_init() {
	NrnThread* nt = nrn_threads + ith_;
	last_index_ = 0;
	discon_index_ = 0;
	if (discon_indices_) {
		if (discon_indices_->capacity() > 0) {
			ubound_index_ = (int)discon_indices_->elem(discon_index_++);
//printf("play_init %d %g\n", ubound_index_, t_->elem(ubound_index_));
			e_->send(t_->elem(ubound_index_), net_cvode_instance, nt);
		}else{
			ubound_index_ = t_->capacity()-1;
		}
	}else{
		ubound_index_ = 0;
		e_->send(t_->elem(ubound_index_), net_cvode_instance, nt);
	}
}

void VecPlayContinuous::deliver(double tt, NetCvode* ns) {
	NrnThread* nt = nrn_threads + ith_;
//printf("deliver %g\n", tt);
	last_index_ = ubound_index_;
	if (discon_indices_) {
		if (discon_index_ < discon_indices_->capacity()) {
			ubound_index_ = (int)discon_indices_->elem(discon_index_++);
//printf("after deliver:send %d %g\n", ubound_index_, t_->elem(ubound_index_));
			e_->send(t_->elem(ubound_index_), ns, nt);
		}else{
			ubound_index_ = t_->capacity() - 1;
		}
	}else{
		if (ubound_index_ < t_->capacity() - 1) {
			ubound_index_++;
			e_->send(t_->elem(ubound_index_), ns, nt);
		}
	}
	continuous(tt);
}

	
void VecPlayContinuous::continuous(double tt) {
	*pd_ = interpolate(tt);
}

double VecPlayContinuous::interpolate(double tt) {
	if (tt >= t_->elem(ubound_index_)) {
		last_index_ = ubound_index_;
		if (last_index_ == 0) {
//printf("return last tt=%g ubound=%g y=%g\n", tt, t_->elem(ubound_index_), y_->elem(last_index_));
			return y_->elem(last_index_);
		}
	}else if (tt <= t_->elem(0)) {
		last_index_ = 0;
//printf("return elem(0) tt=%g t0=%g y=%g\n", tt, t_->elem(0), y_->elem(0));
		return y_->elem(0);
	}else{
		search(tt);
	}
	double x0 = y_->elem(last_index_-1);
	double x1 = y_->elem(last_index_);
	double t0 = t_->elem(last_index_ - 1);
	double t1 = t_->elem(last_index_);
//printf("IvocVectRecorder::continuous tt=%g t0=%g t1=%g theta=%g x0=%g x1=%g\n", tt, t0, t1, (tt - t0)/(t1 - t0), x0, x1);
	if (t0 == t1) { return (x0 + x1)/2.; }
	return interp((tt - t0)/(t1 - t0), x0, x1);
}

void VecPlayContinuous::search(double tt) {
//	assert (tt > t_->elem(0) && tt < t_->elem(t_->capacity() - 1))
	while ( tt < t_->elem(last_index_)) {
		--last_index_;
	}
	while ( tt >= t_->elem(last_index_)) {
		++last_index_;
	}
}

void VecPlayContinuous::pr() {
	printf("VecPlayContinuous ");
//	printf("%s.x[%d]\n", hoc_object_name(y_->obj_), last_index_);
}
