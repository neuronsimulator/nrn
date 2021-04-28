#ifndef kssingle_h
#define kssingle_h

#include <math.h>
#include <OS/math.h>
#include <kschan.h>
#include <netcon.h>

#include <mcran4.h>

class KSSingleTrans;
class KSSingleState;
class KSSingle;
class KSChan;

class KSSingleNodeData : public DiscreteEvent {
public:
	KSSingleNodeData();
	virtual ~KSSingleNodeData();	

	// subclassed DiscreteEvent methods
	virtual void deliver(double t, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);

	// specific to KSSingleNodeData
	int nsingle_;
	double* statepop_; // points to prop->param state array
	int filledstate_; // used when nsingle_ == 1, index of populated state
	double vlast_; // voltage at which the transition rates were calculated
	double t0_; // last transition time. <= t on entry to step calculations.
	double t1_; // next. > t + dt on exit from step calculations.
	int next_trans_; // if t1_ takes effect, this is the trans.
	Point_process** ppnt_;
	KSSingle* kss_;
	TQItem* qi_;
};

class KSSingle {
public:
	KSSingle(KSChan*);
	virtual ~KSSingle();

	void alloc(Prop*, int sindex);
	void init(double v, double* s, KSSingleNodeData* snd, NrnThread*);

	void state(Node*, double*, Datum*, NrnThread*);
	void cv_update(Node*, double*, Datum*, NrnThread*);
	void one(double, KSSingleNodeData*, NrnThread*);
	void do1trans(KSSingleNodeData*);
	void next1trans(KSSingleNodeData*);
	void multi(double, KSSingleNodeData*, NrnThread*);
	void doNtrans(KSSingleNodeData*);
	void nextNtrans(KSSingleNodeData*);

	bool vsame(double x, double y) {return Math::equal(x, y, vres_);}
	double exprand() { return -log(mcell_ran4a(&idum_)); }
	double unifrand(double range) { return mcell_ran4a(&idum_)*range; }
	int rvalrand(int);

	int ntrans_, nstate_, sndindex_;
	KSSingleTrans* transitions_;
	KSSingleState* states_;
	double* rval_;
	bool uses_ligands_;
	static double vres_;
	static unsigned int idum_;

	static unsigned long singleevent_deliver_;
	static unsigned long singleevent_move_;
};

class KSSingleTrans {
public:
	KSSingleTrans();
	virtual ~KSSingleTrans();
	double rate(Point_process*);
	double rate(double v) { return fac_ * (f_ ? kst_->alpha(v) : kst_->beta(v)); }
	double rate(Datum* pd) { return fac_ * (f_ ? kst_->alpha(pd) : kst_->beta()); }
	int src_;
	int target_;
	KSTransition* kst_;
	bool f_;
	double fac_;
};

class KSSingleState {
public:
	KSSingleState();
	virtual ~KSSingleState();
	int ntrans_;
	int* transitions_;
};

#endif
