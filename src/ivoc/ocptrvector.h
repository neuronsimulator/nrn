#ifndef ocptrvector_h
#define ocptrvector_h

#include "oc2iv.h"

class OcPtrVector {
public:
	OcPtrVector(int sz);
	virtual ~OcPtrVector();
	int size() { return size_; }
	void pset(int i, double*);
	double getval(int);
	void setval(int, double);
	void scatter(double*, int sz);	
	void gather(double*, int sz);
public:
	int size_;
	double** pd_;
};

#endif
