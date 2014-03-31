#ifndef singlech_h
#define singlech_h

class SingleChanInfo;
class SingleChanState;
class NrnProperty;
class Vect;
class OcMatrix;
class Rand;

class SingleChan {
public:
	SingleChan(const char*);
	SingleChan(OcMatrix*);
	virtual ~SingleChan();
	void set_rates(double v);
	void set_rates(OcMatrix*);
	void set_rates(int, int, double tau);
	const char* name(int);
	int index(const char*);
	int current_state();
	void current_state(int);
	int cond(int);
	void cond(int, int);
	int current_cond();
	double state_transition();
	double cond_transition();
	void state_transitions(Vect* dt, Vect* state);
	void cond_transitions(Vect* dt, Vect* cond);
	int n();
	void get_rates(OcMatrix*);
	void setrand(Rand*);
public:
	SingleChanState* state_;
private:
	double (SingleChan::*erand_)();
	double erand1();
	double erand2();
	Rand* r_;
private:
	SingleChanInfo* info_;
	NrnProperty* nprop_;
	int current_;
};
#endif
