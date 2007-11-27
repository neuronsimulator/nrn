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

declarePtrList(STEList, StateTransitionEvent)

class STETransition {
public:
	STETransition();
	virtual ~STETransition();
	boolean condition(double& tr);

	int dest_;
	double* var1_;
	double* var2_;
	double oldval1_;
	double oldval2_;
	int order_;
	HocCommand* stmt_;
};

class STEState {
public:
	STEState();
	virtual ~STEState();
	int condition(double& tr);
	void execute(int);
	STETransition* add_transition();
	int ntrans_;
	STETransition* transitions_;
};

class StateTransitionEvent {
public:
	StateTransitionEvent(int nstate);
	virtual ~StateTransitionEvent();
	void transition(int src, int dest, double* var1, double* var2,
		int order, const char* stmt, Object* obj);
	void state(int i){istate_ = i;}
	int state(){return istate_;}
	int nstate() { return nstate_;}
	// return transition index for istate_
	//if tr < t then request retreat
	int condition(double& tr) { return states_[istate_].condition(tr); }
	// make the transition.
 	void execute(int);
	static STEList* stelist_;
	static void stelist_change(); // implemented in netcvode.cpp
private:
	int nstate_;
	int istate_;
	STEState* states_;
};


#endif
