#ifndef glinerec_h
#define glinerec_h

#include "nrnoc2iv.h"
#include "vrecitem.h"

#include <vector>
#include <utility>
#include <string>

typedef std::vector< std::pair< double*, IvocVect* > > GLineRecordEData;

class GraphLine;

class GLineRecordExprInfo {
public:
  GLineRecordExprInfo(const char* expr);
  virtual ~GLineRecordExprInfo();
  void fill_pd();
  GLineRecordEData pd_and_vec;
  std::string expr_;
  Symbol* esym; //parsed expr
  Symlist* symlist_;
  bool saw_t;
};

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
	GLineRecordExprInfo* expr_info_;
};

#endif
