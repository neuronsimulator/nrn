#pragma once

#include <InterViews/resource.h>

class NrnPropertyImpl;
class SectionListImpl;
class NrnNodeImpl;

class NrnProperty {
  public:
    NrnProperty(const char*);
    virtual ~NrnProperty();
    const char* name() const;
    int type() const;
    bool deleteable();

    Symbol* first_var();
    bool more_var();
    Symbol* next_var();
    Symbol* findsym(const char* rangevar);
    Symbol* var(int);
    neuron::container::data_handle<double> pval(const Symbol*, int index);
    // vartype=0, 1, 2, 3 means all, PARAMETER, ASSIGNED, STATE
    bool copy(bool to_prop, Prop* dest, Node* nd_dest, int vartype = 0);
    bool copy_out(NrnProperty& dest, int vartype = 0);

  private:
    NrnPropertyImpl* npi_;
};

class SectionList: public Resource {
  public:
    SectionList(Object*);
    virtual ~SectionList();
    Section* begin();
    Section* next();
    Object* nrn_object();

  private:
    SectionListImpl* sli_;
};
