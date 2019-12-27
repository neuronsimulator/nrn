#ifndef nrnste_h
#define nrnste_h
//StateTransitionEvent is a finite state machine in which a transtion occurs
// when the transition condition is true. For speed the transition condition
// is of the form *var1 > *var2 and allows second order interpolation.
// Transition conditions are checked only if the source state is the
// current state.

#include <OS/list.h>

class HocCommand;
class StateTransitionEvent;
class STECondition;

declarePtrList(STEList, StateTransitionEvent)

class STETransition {
public:
	STETransition();
	virtual ~STETransition();
	void event(); // from STECondition::deliver
	virtual double value() { return *var1_ - *var2_;}
	void activate(); // add ste_ to watch list
	void deactivate(); // remove ste_ from watch list

	double* var1_;
	double* var2_;
	HocCommand* hc_;
	StateTransitionEvent* ste_;
	STECondition* stec_;
	int dest_;
	bool var1_is_time_;
};

class STEState {
public:
	STEState();
	virtual ~STEState();
	STETransition* add_transition();
	int ntrans_;
	STETransition* transitions_;
};

class StateTransitionEvent {
public:
	StateTransitionEvent(int nstate, Point_process*);
	virtual ~StateTransitionEvent();
	void transition(int src, int dest, double* var1, double* var2, HocCommand*);
	void state(int i); // set current state  -- update watch list.
	int state(){return istate_;}
	int nstate() { return nstate_;}
	void activate();
	void deactivate();

	int nstate_;
	int istate_;
	STEState* states_;
	Point_process* pnt_;
	int activated_;
};


#endif
