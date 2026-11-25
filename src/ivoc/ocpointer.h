#pragma once

#include <InterViews/observe.h>
#include "oc2iv.h"
class StmtInfo;

class OcPointer: public Observer {
  public:
    OcPointer(const char*, neuron::container::data_handle<double>);
    virtual ~OcPointer();
    virtual void update(Observable*);
    void assign(double);
    neuron::container::data_handle<double> p_;
    char* s_;
    StmtInfo* sti_;
    bool valid_;
};

class StmtInfo {
  public:
    StmtInfo(const char*);
    virtual ~StmtInfo();
    void play_one(double);
    void parse();
    std::string stmt_{};
    Symlist* symlist_;
    Symbol* symstmt_;
};
