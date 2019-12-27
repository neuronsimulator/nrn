#ifndef ocmatrix_h
#define ocmatrix_h

#ifndef MATRIXH
#define MAT void
#define SPMAT void
#define PERM void
#endif

#include <vector>
using std::vector;

struct Object;
class IvocVect;
class OcFullMatrix;
#define Vect IvocVect
#define Matrix OcMatrix

class OcMatrix {
public:
	enum {MFULL=1, MSPARSE, MBAND};
	static OcMatrix* instance(int nrow, int ncol, int type = MFULL);
	virtual ~OcMatrix();

	virtual double* mep(int i, int j) {unimp(); return NULL;} //matrix element pointer
	inline double& operator() (int i, int j) {return *mep(i, j);};	
	
	virtual double getval(int i, int j) { unimp(); return 0.; }
	virtual int nrow() {unimp(); return 0;}
	virtual int ncol() {unimp(); return 0;}
	virtual void resize(int, int) {unimp();}
    
    virtual void nonzeros(vector<int>& m, vector<int>& n);

	OcFullMatrix* full();
	
	inline void mulv(Vect& in, Vect& out) {mulv(&in, &out);};
	virtual void mulv(Vect* in, Vect* out){unimp();}
	virtual void mulm(Matrix* in, Matrix* out){unimp();}
	virtual void muls(double, Matrix* out){unimp();}
	virtual void add(Matrix*, Matrix* out){unimp();}
	virtual void getrow(int, Vect* out){unimp();}
	virtual void getcol(int, Vect* out){unimp();}
	virtual void getdiag(int, Vect* out){unimp();}
	virtual void setrow(int, Vect* in){unimp();}
	virtual void setcol(int, Vect* in){unimp();}
	virtual void setdiag(int, Vect* in){unimp();}
	virtual void setrow(int, double in){unimp();}
	virtual void setcol(int, double in){unimp();}
	virtual void setdiag(int, double in){unimp();}
	virtual void zero(){unimp();}
	virtual void ident(){unimp();}
	virtual void exp(Matrix* out){unimp();}
	virtual void pow(int, Matrix* out){unimp();}
	virtual void inverse(Matrix* out){unimp();}
	virtual void solv(Vect* vin, Vect* vout, bool use_lu) {unimp();}
	virtual void copy(Matrix* out){unimp();}
	virtual void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1){unimp();}
	virtual void transpose(Matrix* out){unimp();}
	virtual void symmeigen(Matrix* mout, Vect* vout){unimp();}
	virtual void svd1(Matrix* u, Matrix* v, Vect* d){unimp();}
	virtual double det(int* e){unimp(); return 0.0;}
	virtual int sprowlen(int){unimp(); return 0;}
	virtual double spgetrowval(int i, int jindx, int* j){unimp(); return 0.;}

	void unimp();
	Object** temp_objvar();
protected:
	OcMatrix(int type);
public:
	Object* obj_;
private:
	int type_;
};

extern "C" {
	extern Matrix* matrix_arg(int);
}

class OcFullMatrix : public OcMatrix {	// type 1
public:
	OcFullMatrix(int, int);
	virtual ~OcFullMatrix();

	virtual double* mep(int, int);
	virtual double getval(int i, int j);
	virtual int nrow();
	virtual int ncol();
	virtual void resize(int, int);

	virtual void mulv(Vect* in, Vect* out);
	virtual void mulm(Matrix* in, Matrix* out);
	virtual void muls(double, Matrix* out);
	virtual void add(Matrix*, Matrix* out);
	virtual void getrow(int, Vect* out);
	virtual void getcol(int, Vect* out);
	virtual void getdiag(int, Vect* out);
	virtual void setrow(int, Vect* in);
	virtual void setcol(int, Vect* in);
	virtual void setdiag(int, Vect* in);
	virtual void setrow(int, double in);
	virtual void setcol(int, double in);
	virtual void setdiag(int, double in);
	virtual void zero();
	virtual void ident();
	virtual void exp(Matrix* out);
	virtual void pow(int, Matrix* out);
	virtual void inverse(Matrix* out);
	virtual void solv(Vect* vin, Vect* vout, bool use_lu);
	virtual void copy(Matrix* out);
	virtual void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1);
	virtual void transpose(Matrix* out);
	virtual void symmeigen(Matrix* mout, Vect* vout);
	virtual void svd1(Matrix* u, Matrix* v, Vect* d);
	virtual double det(int* exponent);
private:
	MAT* m_;
	MAT* lu_factor_;
	PERM* lu_pivot_;
};

class OcSparseMatrix : public OcMatrix {	// type 2
public:
	OcSparseMatrix(int, int);
	virtual ~OcSparseMatrix();

	virtual double* mep(int, int);
	virtual double* pelm(int, int); // NULL if element does not exist
	virtual int nrow();
	virtual int ncol();
	virtual double getval(int, int);
	virtual void ident(void);
	virtual void mulv(Vect* in, Vect* out);
	virtual void solv(Vect* vin, Vect* vout, bool use_lu);

	virtual void setrow(int, Vect* in);
	virtual void setcol(int, Vect* in);
	virtual void setdiag(int, Vect* in);
	virtual void setrow(int, double in);
	virtual void setcol(int, double in);
	virtual void setdiag(int, double in);

    virtual void nonzeros(vector<int>& m, vector<int>& n);

	virtual int sprowlen(int); // how many elements in row
	virtual double spgetrowval(int i, int jindx, int* j); 
    
    virtual void zero();

private:
	SPMAT* m_;
	SPMAT* lu_factor_;
	PERM* lu_pivot_;
};

#ifndef MATRIXH
#undef MAT
#undef SPMAT
#endif

#endif
