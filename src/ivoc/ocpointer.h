#ifndef ocpointer_h
#define ocpointer_h

#include "neuron/container/data_handle.hpp"
#include <InterViews/observe.h>
#include <OS/string.h>
#include "oc2iv.h"
class StmtInfo;

class OcPointer: public Observer {
  public:
    OcPointer(const char*, neuron::container::data_handle<double>);
    virtual ~OcPointer();
    virtual void update(Observable*);
    void assign(double);
    neuron::container::data_handle<double> pd_;
    char* s_;
    StmtInfo* sti_;
};

class StmtInfo {
  public:
    StmtInfo(const char*);
    virtual ~StmtInfo();
    void play_one(double);
    void parse();
    CopyString* stmt_;
    Symlist* symlist_;
    Symbol* symstmt_;
};

#endif
