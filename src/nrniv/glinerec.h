#ifndef glinerec_h
#define glinerec_h

#include "nrnoc2iv.h"
#include "vrecitem.h"

class GraphLine;

class GLineRecord : public PlayRecord {
public:
	GLineRecord(GraphLine*);
	virtual ~GLineRecord();
	virtual void install(Cvode* cv) { record_add(cv); }
	virtual void record_init();
	virtual void continuous(double t);
	virtual bool uses(void* v) { return (void*)gl_ == v; }
        virtual int type() { return GLineRecordType; }

	void plot(int, double);
	GraphLine* gl_;
	IvocVect* v_; // to allow CoreNEURON to save trajectory.
};

#endif
