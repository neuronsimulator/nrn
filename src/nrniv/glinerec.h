#ifndef glinerec_h
#define glinerec_h

#include "nrnoc2iv.h"
#include "vrecitem.h"
#include "graph.h"

#include <vector>
#include <utility>

typedef std::vector<std::pair<double*, IvocVect*>> GLineRecordEData;

class GLineRecord: public PlayRecord {
  public:
    GLineRecord(GraphLine*);
    virtual ~GLineRecord();
    virtual void install(Cvode* cv) {
        record_add(cv);
    }
    virtual void record_init();
    virtual void continuous(double t);
    virtual bool uses(void* v) {
        return (void*) gl_ == v;
    }
    virtual int type() {
        return GLineRecordType;
    }

    void plot(int, double);
    GraphLine* gl_;
    IvocVect* v_;  // to allow CoreNEURON to save trajectory.

    void fill_pd();
    void fill_pd1();
    GLineRecordEData pd_and_vec_;
    bool saw_t_;
};

class GVectorRecord: public PlayRecord {
  public:
    GVectorRecord(GraphVector*);
    virtual ~GVectorRecord();
    virtual void install(Cvode* cv) {
        record_add(cv);
    }
    virtual void record_init();
    virtual void continuous(double t);
    virtual bool uses(void* v) {
        return (void*) gv_ == v;
    }
    virtual int type() {
        return GVectorRecordType;
    }

    int count();
    neuron::container::data_handle<double> pdata(int);
    GraphVector* gv_;
};

#endif
