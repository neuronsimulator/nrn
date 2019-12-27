#ifndef ocptrvector_h
#define ocptrvector_h

#include "oc2iv.h"
class HocCommand;

class OcPtrVector {
public:
	OcPtrVector(int sz);
	virtual ~OcPtrVector();
	int size() { return size_; }
	void resize(int);
	void pset(int i, double*);
	double getval(int);
	void setval(int, double);
	void scatter(double*, int sz);	
	void gather(double*, int sz);
	void ptr_update_cmd(HocCommand*);
	void ptr_update();	
public:
	int size_;
	double** pd_;
	HocCommand* update_cmd_;
};

#endif
