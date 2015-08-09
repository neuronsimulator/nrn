#include <../../nrnconf.h>
#include <stdio.h>
#include <InterViews/resource.h>
#include <string.h>
#include <nrnoc2iv.h>
#include <nrniv_mf.h>
#include <classreg.h>
#include <objcmd.h>
#include <nrnste.h>
#include <netcon.h>

static double ste_transition(void* v) {
	StateTransitionEvent* ste = (StateTransitionEvent*)v;
	int src = (int)chkarg(1, 0, ste->nstate()-1);
	int dest = (int)chkarg(2, 0, ste->nstate()-1);
	double* var1 = hoc_pgetarg(3);
	double* var2 = hoc_pgetarg(4);
	HocCommand* hc = NULL;
	if (ifarg(5)) {
		Object* obj = NULL;
		if (hoc_is_str_arg(5)) {
			char* stmt = NULL;
			stmt = gargstr(5);
			if (ifarg(6)) {
				obj = *hoc_objgetarg(6);
			}
			hc = new HocCommand(stmt, obj);
		}else{
			obj = *hoc_objgetarg(5);
			hc = new HocCommand(obj);
		}
	}
	ste->transition(src, dest, var1, var2, hc);
	return 1.;
}

static double ste_state(void* v) {
	StateTransitionEvent* ste = (StateTransitionEvent*)v;
	int state = ste->state();
	if (ifarg(1)) {
		ste->state((int)chkarg(1, 0, ste->nstate()-1));
	}
	return (double)state;
}

static Member_func members[] = {
	"transition", ste_transition,
	"state", ste_state,
	0, 0
};

static void* ste_cons(Object*) {
	int nstate = (int)chkarg(1, 1, 1e6);
	Point_process* pnt = NULL;
	if (ifarg(2)) {
		Object* obj = *hoc_objgetarg(2);
		pnt = ob2pntproc(obj);
	}
	StateTransitionEvent* ste = new StateTransitionEvent(nstate, pnt);
	return ste;
}

static void ste_destruct(void* v) {
	StateTransitionEvent* ste = (StateTransitionEvent*)v;
	delete ste;
}

void StateTransitionEvent_reg() {
	class2oc("StateTransitionEvent", ste_cons, ste_destruct,
		members, NULL, NULL, NULL);
}

StateTransitionEvent::StateTransitionEvent(int nstate, Point_process* pnt){
	nstate_ = nstate;
	states_ = new STEState[nstate_];
	istate_ = 0;
	pnt_ = pnt;
	activated_ = -1;
}

StateTransitionEvent::~StateTransitionEvent(){
	deactivate();
	delete [] states_;
}

void StateTransitionEvent::deactivate() {
	if (activated_ < 0) { return; }
	STEState& s = states_[activated_];
	for (int i=0; i < s.ntrans_; ++i) {
		s.transitions_[i].deactivate();
	}
	activated_ = -1;
}

void StateTransitionEvent::activate() {
	if (activated_ >= 0) { deactivate(); }
	STEState& s = states_[istate_];
	for (int i=0; i < s.ntrans_; ++i) {
		s.transitions_[i].activate();
	}
	activated_ = istate_;
}

void StateTransitionEvent::state(int ist) {
	assert(ist >= 0 && ist < nstate_);
	deactivate();
	istate_ = ist;
	activate();
}

STEState::STEState(){
	ntrans_ = 0;
	transitions_ = NULL;
}

STEState::~STEState(){
	if (transitions_) {
		delete [] transitions_;
	}
}

STETransition* STEState::add_transition() {
	++ntrans_;
	STETransition* st = transitions_;
	transitions_ = new STETransition[ntrans_];
	if (st) {
		int i;
		for (i=0; i < ntrans_-1; ++i) {
			transitions_[i].hc_ = st[i].hc_;  st[i].hc_ = NULL;
			transitions_[i].ste_ = st[i].ste_;  st[i].ste_ = NULL;
			transitions_[i].stec_ = st[i].stec_;  st[i].stec_ = NULL;
			transitions_[i].var1_ = st[i].var1_;
			transitions_[i].var2_ = st[i].var2_;
			transitions_[i].dest_ = st[i].dest_;
			transitions_[i].var1_is_time_ = st[i].var1_is_time_;
		}
		delete [] st;
	}
	return transitions_ + ntrans_ - 1;
}

STETransition::STETransition(){
	hc_ = NULL;
	ste_ = NULL;
	stec_ = NULL;
	var1_is_time_ = false;
}

STETransition::~STETransition(){
	if (hc_) {
		delete hc_;
	}
	if (stec_) {
		delete stec_;
	}
}

void STETransition::event() {
	ste_->deactivate();
	ste_->istate_ = dest_;
	if (hc_) {
		nrn_hoc_lock();
		hc_->execute();
		nrn_hoc_unlock();
	}
	ste_->activate();
}
