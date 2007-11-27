#ifndef linmod_h
#define linmod_h

#include <OS/list.h>
#include "ocmatrix.h"
#include "ivocvect.h"
#include "nrnoc2iv.h"

class MatrixMap {
public:
	MatrixMap(Matrix*);
	~MatrixMap();
	Matrix* m_;

	void alloc(int, int, Node**, int*);
	void mmfree();
	void add(double fac);

	// the map
	int plen_;
	double** pm_;
	double** ptree_;
};

class LinearModelAddition {
public:
	LinearModelAddition(Matrix* c, Matrix* g, Vect* y, Vect* y0, Vect* b,
		int nnode, Node** nodes, Vect* elayer = nil);
	virtual ~LinearModelAddition();
	int extra_eqn_count();
	void alloc(int);
	void lmafree();
	void init();
	void lhs();
	void rhs();
	void dkmap(double**, double**);
	void dkres(double*, double*, double*);
	void dkpsol(double);
	void update();

private:
	MatrixMap* c_;
	MatrixMap* g_;
	Vect* b_;
	Vect* y_;
	Vect* y0_;
	// connection to the voltage tree part of the matrix
	int nnode_;
	Node** nodes_;

	int size_;
	int start_;
	int* bmap_;
	Vect* gy_, *cyp_;
	Vect* ytmp_, *yptmp_; // temporary vectors used along with cy_ for res.
	int* elayer_; //normally elements are 0 and refer to internal potential
	//otherwise range from 1 to nlayer and refer to vext[elayer-1]
	// vm+vext and vext must be placed in y for calculation of rhs
	void v2y();
};

declarePtrList(LinmodPtrList, LinearModelAddition)

#endif
