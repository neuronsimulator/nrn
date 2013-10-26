#ifndef vrecitem_h
#define vrecitem_h

#include <ivlist.h>
#include <netcon.h>
#include <ivocvect.h>

class PlayRecord;
class PlayRecordSave;
class VecRecordDiscreteSave;
class VecPlayStepSave;
class VecPlayContinuousSave;
class StmtInfo;
struct NrnThread;
struct Section;

// SaveState subtypes for PlayRecordType
#define VecRecordDiscreteType 1
#define VecRecordDtType 2
#define VecPlayStepType 3
#define VecPlayContinuousType 4

// used by PlayRecord subclasses that utilize discrete events
class PlayRecordEvent : public DiscreteEvent {
public:
	PlayRecordEvent();
	virtual ~PlayRecordEvent();
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual void frecord_init(TQItem* q);
	virtual NrnThread* thread();
	PlayRecord* plr_;
	static unsigned long playrecord_send_;
	static unsigned long playrecord_deliver_;
};

// common interface for Play and Record for all integration methods.
class PlayRecord {
public:
	PlayRecord(double* pd);
	virtual ~PlayRecord();
	virtual void play_init(){}	// called near beginning of finitialize
	virtual void record_init(){}	// called near end of finitialize and frecord_init()
	virtual void continuous(double t){} // play - every f(y, t) or res(y', y, t); record - advance_tn and initialize flag
	virtual void deliver(double t, NetCvode*){} // at associated DiscreteEvent
	virtual PlayRecordEvent* event() { return nil;}
	virtual void pr(); // print identifying info
	virtual int type() { return 0; }

	// install normally calls one of these. Cvode may be nil.
	void play_add();
	void record_add();

	// administration
	virtual void update_ptr(double*);
	virtual void frecord_init(TQItem*) {}
	// for example, subclasses use things that may not go out of existence but wish us
	// to remove ourselves from the PlayRecord system. e.g Vector.play_remove().


	double* pd_;
	Point_process* pnt_;
	int ith_; // The thread index
};

declarePtrList(PlayRecList, PlayRecord)

class TvecRecord: public PlayRecord {
public:
	TvecRecord(NrnThread*, IvocVect* tvec);
	virtual ~TvecRecord();
	virtual void record_init();
	virtual void continuous(double t);

	IvocVect* t_;
	NrnThread* nt_;
};

class YvecRecord : public PlayRecord {
public:
	YvecRecord(double*, IvocVect* y);
	virtual ~YvecRecord();
	virtual void install();
	virtual void record_init();
	virtual void continuous(double t);

	IvocVect* y_;
};

class VecRecordDiscrete : public PlayRecord {
public:
	VecRecordDiscrete(double*, IvocVect* y, IvocVect* t);
	virtual ~VecRecordDiscrete();
	virtual void install();
	virtual void record_init();
	virtual PlayRecordEvent* event() { return e_;}
	virtual void deliver(double t, NetCvode*);

	virtual void frecord_init(TQItem*);
	
	virtual int type() { return VecRecordDiscreteType; }

	IvocVect* y_;
	IvocVect* t_;
	PlayRecordEvent* e_;
};

class VecRecordDt : public PlayRecord {
public:
	VecRecordDt(double*, IvocVect* y, double dt);
	virtual ~VecRecordDt();
	virtual void install();
	virtual void record_init();
	virtual void deliver(double t, NetCvode*);
	virtual PlayRecordEvent* event() { return e_;}

	virtual void frecord_init(TQItem*);
	virtual int type() { return VecRecordDtType; }
	
	IvocVect* y_;
	double dt_;
	PlayRecordEvent* e_;
};

class VecPlayStep : public PlayRecord {
public:
	VecPlayStep(double*, IvocVect* y, IvocVect* t, double dt);
	void init(IvocVect* y, IvocVect* t, double dt);
	virtual ~VecPlayStep();
	virtual void install();
	virtual void play_init();
	virtual void deliver(double t, NetCvode*);
	virtual PlayRecordEvent* event() { return e_;}
	virtual void pr();
	virtual int type() { return VecPlayStepType; }

	IvocVect* y_;
	IvocVect* t_;
	double dt_;
	int current_index_;

	PlayRecordEvent* e_;
};
class VecPlayContinuous : public PlayRecord {
public:
	VecPlayContinuous(double*, IvocVect* y, IvocVect* t, IvocVect* discon);
	virtual ~VecPlayContinuous();
	void init(IvocVect* y, IvocVect* t, IvocVect* tdiscon);
	virtual void install();
	virtual void play_init();
	virtual void deliver(double t, NetCvode*);
	virtual PlayRecordEvent* event() { return e_;}
	virtual void pr();

	void continuous(double tt);
	double interpolate(double tt);
	double interp(double th, double x0, double x1){ return x0 + (x1 - x0)*th; }
	void search(double tt);
	virtual int type() { return VecPlayContinuousType; }

	IvocVect* y_;
	IvocVect* t_;
	IvocVect* discon_indices_;
	int last_index_;
	int discon_index_;
	int ubound_index_;

	PlayRecordEvent* e_;
};

#endif
