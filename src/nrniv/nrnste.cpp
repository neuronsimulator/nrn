#include <../../nrnconf.h>
#include <stdio.h>
#include <InterViews/resource.h>
#include <string.h>
#include <nrnoc2iv.h>
#include <classreg.h>
#include <objcmd.h>
#include <nrnste.h>

static double ste_transition(void* v) {
	StateTransitionEvent* ste = (StateTransitionEvent*)v;
	int src = (int)chkarg(1, 0, ste->nstate()-1);
	int dest = (int)chkarg(2, 0, ste->nstate()-1);
	if (src == dest) {
		hoc_execerror("source and destination are the same", 0);
	}
	double* var1 = hoc_pgetarg(3);
	double* var2 = hoc_pgetarg(4);
	char* stmt = gargstr(5);
	Object* obj = nil;
	if (ifarg(6)) {
		obj = hoc_obj_get(6);
	}
	ste->transition(src, dest, var1, var2, 1, stmt, obj);
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
	int nstate = (int)chkarg(1, 2, 1e6);
	StateTransitionEvent* ste = new StateTransitionEvent(nstate);
	return ste;
}

static void ste_destruct(void* v) {
	StateTransitionEvent* ste = (StateTransitionEvent*)v;
	delete ste;
}

void StateTransitionEvent_reg() {
	class2oc("StateTransitionEvent", ste_cons, ste_destruct,
		members, nil, nil);
}

implementPtrList(STEList, StateTransitionEvent)

STEList* StateTransitionEvent::stelist_;

StateTransitionEvent::StateTransitionEvent(int nstate){
	if (!stelist_) {
		stelist_ = new STEList(10);
	}
	stelist_->append(this);
	nstate_ = nstate;
	states_ = new STEState[nstate_];
	istate_ = 0;
	stelist_change();
}

StateTransitionEvent::~StateTransitionEvent(){
	long i, cnt = stelist_->count();
	for (i=0; i < cnt; ++i) {
		if (stelist_->item(i) == this) {
			stelist_->remove(i);
		}
	}
	// since global cvode ste_list_ shares this list the global step
	// remains valid. Only the local step method needs to be re-calculated.

	assert (i < cnt);
	delete [] states_;
	stelist_change();
}

void StateTransitionEvent::transition(int src, int dest, double* var1, double* var2,
	int order, const char* stmt, Object* obj
){
	STETransition* st = states_[src].add_transition();
	st->dest_ = dest;
	st->var1_ = var1;
	st->var2_ = var2;
	st->order_ = order;
	if (stmt && strlen(stmt) > 0) {
		st->stmt_ = new HocCommand(stmt, obj);
	}else{
		st->stmt_ = nil;
	}
}

void StateTransitionEvent::execute(int itrans){
	STETransition* st = states_[istate_].transitions_+itrans;
	istate_ = st->dest_;
	if (st->stmt_) {
		st->stmt_->execute();
		st->oldval1_ = -1e9;
		st->oldval2_ = -1e8;
	}
}

STEState::STEState(){
	ntrans_ = 0;
	transitions_ = nil;
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
			transitions_[i].stmt_ = st[i].stmt_;
			st[i].stmt_ = nil;
			transitions_[i].var1_ = st[i].var1_;
			transitions_[i].var2_ = st[i].var2_;
			transitions_[i].oldval1_ = st[i].oldval1_;
			transitions_[i].oldval2_ = st[i].oldval2_;
			transitions_[i].dest_ = st[i].dest_;
			transitions_[i].order_ = st[i].order_;
		}
		delete [] st;
	}
	return transitions_ + ntrans_ - 1;
}

int STEState::condition(double& tr){
	int i;
	for (i=0; i < ntrans_; ++i) {
		if (transitions_[i].condition(tr)) {
			return i;
		}
	}
	return -1;
}

STETransition::STETransition(){
	stmt_ = nil;
}

STETransition::~STETransition(){
	if (stmt_) {
		delete stmt_;
	}
}

boolean STETransition::condition(double& tr){
	if (*var1_ > *var2_) {
		return true;
	}
	return false;
}
