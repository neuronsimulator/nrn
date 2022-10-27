#ifndef nrn_dataclass_h
#define nrn_dataclass_h

#include <InterViews/resource.h>

class NrnPropertyImpl;
class SectionListImpl;
class NrnNodeImpl;

class NrnProperty {
  public:
    NrnProperty(Prop*);
    NrnProperty(const char*);
    virtual ~NrnProperty();
    const char* name() const;
    int type() const;
    bool is_point() const;
    bool deleteable();

    Symbol* first_var();
    bool more_var();
    Symbol* next_var();
    Symbol* find(const char* rangevar);
    Symbol* var(int);
    int prop_index(const Symbol*) const;
    neuron::container::data_handle<double> prop_pval(const Symbol*, int arrayindex = 0) const;

    Prop* prop() const;
    int var_type(Symbol*) const;
    // vartype=0, 1, 2, 3 means all, PARAMETER, ASSIGNED, STATE
    static bool assign(Prop* src, Prop* dest, int vartype = 0);

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

#if 0
class NrnSection {
public:
	NrnSection(Section* sec);
	virtual ~NrnSection();
	void section(Section*);
	Section* section();
	Node* node(int index);
	bool is_mechanism(int type);
	double* var_pointer(Symbol*, int index=0, int inode=0);
	double* var_pointer(const char*); // eg. ca_cadifus[2](.7)
private:
	NrnNodeImpl* nni_;
};
#endif

#endif
