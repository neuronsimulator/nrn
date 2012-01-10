// for equations of the form c * dy/dt = f(y)

#ifndef nrndae_h
#define nrndae_h
// this defines things needed by ocmatrix
#include <OS/list.h>

#include "ivocvect.h"
#include "matrixmap.h"
#include <list>

class NrnDAE {
public:
	int extra_eqn_count();
	void alloc(int);
	void lhs();
	void rhs();
	void dkres(double* y, double* yprime, double* delta);
	void init();
	void update();
	void dkmap(double** pv, double** pvdot);
	virtual ~NrnDAE();
	
protected:
	NrnDAE(Matrix* cmat, Vect* const yvec, Vect* const y0, int nnode, Node** const nodes, Vect* const elayer, void (*f_init)(void* data) = NULL, void* const data = NULL);
	
private:
	virtual void f_(Vect& y, Vect& yprime, int size) = 0;
	virtual MatrixMap* jacobian_(Vect& y) = 0;
    
    void (*f_init_)(void* data);
    void* data_;

	// value of jacobian_ must be multiplied by this value before use
	virtual double jacobian_multiplier_() {return 1;}

	virtual void alloc_(int size, int start, int nnode, Node** nodes, int* elayer);
	MatrixMap* c_;
    Matrix* assumed_identity_;
	Vect* y0_;
	Vect& y_;
	int size_;
	int* bmap_;
	// connection to the voltage tree part of the matrix
	int nnode_;
	Node** nodes_;

	int start_;
	Vect cyp_, yptmp_; // temporary vectors for residual.
	int* elayer_; //normally elements are 0 and refer to internal potential
	//otherwise range from 1 to nlayer and refer to vext[elayer-1]
	// vm+vext and vext must be placed in y for calculation of rhs
	void v2y();
};

void nrndae_register(NrnDAE*);
void nrndae_deregister(NrnDAE*);

typedef std::list<NrnDAE*> NrnDAEPtrList;
typedef NrnDAEPtrList::const_iterator NrnDAEPtrListIterator;

#endif

