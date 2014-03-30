#include <../../nrnconf.h>
#undef check
#include "nonlinz.h"
#include <InterViews/resource.h>
#if defined(__GO32__)
#define NoInlineComplex
#include <_complex.h>
#if defined(NoInlineComplex)
inline double  Complex::real() const { return re; }
inline double  Complex::imag() const { return im; }

inline Complex::Complex() {}
inline Complex::Complex(const Complex& y) :re(y.real()), im(y.imag()) {}
inline Complex::Complex(double r, double i) :re(r), im(i) {}

inline Complex::~Complex() {}

inline Complex&  Complex::operator =  (const Complex& y) 
{ 
  re = y.real(); im = y.imag(); return *this; 
} 

inline Complex&  Complex::operator += (const Complex& y)
{ 
  re += y.real();  im += y.imag(); return *this; 
}

inline Complex&  Complex::operator += (double y)
{ 
  re += y; return *this; 
}

inline Complex&  Complex::operator -= (const Complex& y)
{ 
  re -= y.real();  im -= y.imag(); return *this; 
}

inline Complex&  Complex::operator -= (double y)
{ 
  re -= y; return *this; 
}

inline Complex&  Complex::operator *= (const Complex& y)
{  
  double r = re * y.real() - im * y.imag();
  im = re * y.imag() + im * y.real(); 
  re = r; 
  return *this; 
}

inline Complex&  Complex::operator *= (double y)
{  
  re *=  y; im *=  y; return *this; 
}

//  functions

inline int  operator == (const Complex& x, const Complex& y)
{
  return x.real() == y.real() && x.imag() == y.imag();
}

inline int  operator == (const Complex& x, double y)
{
  return x.imag() == 0.0 && x.real() == y;
}

inline int  operator != (const Complex& x, const Complex& y)
{
  return x.real() != y.real() || x.imag() != y.imag();
}

inline int  operator != (const Complex& x, double y)
{
  return x.imag() != 0.0 || x.real() != y;
}

inline Complex  operator - (const Complex& x)
{
  return Complex(-x.real(), -x.imag());
}

inline Complex  conj(const Complex& x)
{
  return Complex(x.real(), -x.imag());
}

inline Complex  operator + (const Complex& x, const Complex& y)
{
  return Complex(x.real() + y.real(), x.imag() + y.imag());
}

inline Complex  operator + (const Complex& x, double y)
{
  return Complex(x.real() + y, x.imag());
}

inline Complex  operator + (double x, const Complex& y)
{
  return Complex(x + y.real(), y.imag());
}

inline Complex  operator - (const Complex& x, const Complex& y)
{
  return Complex(x.real() - y.real(), x.imag() - y.imag());
}

inline Complex  operator - (const Complex& x, double y)
{
  return Complex(x.real() - y, x.imag());
}

inline Complex  operator - (double x, const Complex& y)
{
  return Complex(x - y.real(), -y.imag());
}

inline Complex  operator * (const Complex& x, const Complex& y)
{
  return Complex(x.real() * y.real() - x.imag() * y.imag(), 
                 x.real() * y.imag() + x.imag() * y.real());
}

inline Complex  operator * (const Complex& x, double y)
{
  return Complex(x.real() * y, x.imag() * y);
}

inline Complex  operator * (double x, const Complex& y)
{
  return Complex(x * y.real(), x * y.imag());
}

inline double  real(const Complex& x)
{
  return x.real();
}

inline double  imag(const Complex& x)
{
  return x.imag();
}

inline double  abs(const Complex& x)
{
  return hypot(x.real(), x.imag());
}

inline double  norm(const Complex& x)
{
  return (x.real() * x.real() + x.imag() * x.imag());
}

inline double  arg(const Complex& x)
{
  return atan2(x.imag(), x.real());
}

inline Complex  polar(double r, double t)
{
  return Complex(r * cos(t), r * sin(t));
}
#endif

#else
#include <Complex.h>
#endif
#include "nrnoc2iv.h"
#include "classreg.h"
#include <ivstream.h>
#include <stdio.h>
extern "C" {
#include "membfunc.h"
extern void nrn_rhs(NrnThread*);
extern void nrn_lhs(NrnThread*);
extern int tree_changed;
extern int v_structure_change;
extern void setup_topology();
extern void recalc_diam();
}

typedef void (*Pfrv4)(int, Node**, double**, Datum**);

class Imp {
public:
	Imp();
	virtual ~Imp();
	// v(x)/i(x) and  v(loc)/i(x) == v(x)/i(loc)
	void compute(double freq, bool nonlin = false);
	void location(Section*, double);
	double transfer_amp(Section*, double);
	double input_amp(Section*, double);
	double transfer_phase(Section*, double);
	double input_phase(Section*, double);
	double ratio_amp(Section*, double);
private:
	int loc(Section*, double);
	void alloc();
	void impfree();
	void check();
	void setmat(double);
	void setmat1();

	void LUDecomp();
	void solve();
public:
	double deltafac_;
private:
	int n;
	Complex* transfer;
	Complex* input;
	Complex* d;	/* diagonal */
	Complex* pivot;
	int istim;	/* where current injected */
	Section* sloc_;
	double xloc_;
	NonLinImp* nli_;
};
	
static void* cons(Object*) {
	Imp* imp = new Imp();
	return (void*)imp;
}

static void destruct(void* v) {
	Imp* imp = (Imp*)v;
	delete imp;
}

static double compute(void* v) {
	Imp* imp = (Imp*)v;
	bool nonlin = false;
	if (ifarg(2)) {
		nonlin = *getarg(2) ? true : false;
	}
	imp->compute(*getarg(1), nonlin);
	return 1.;
}

static double location(void* v) {
	Imp* imp = (Imp*)v;
	imp->location(chk_access(), chkarg(1, 0., 1.));
	return 0.;
}

static double transfer_amp(void* v) {
	Imp* imp = (Imp*)v;
	return imp->transfer_amp(chk_access(), chkarg(1, 0., 1.));
}
static double input_amp(void* v) {
	Imp* imp = (Imp*)v;
	return imp->input_amp(chk_access(), chkarg(1, 0., 1.));
}
static double transfer_phase(void* v) {
	Imp* imp = (Imp*)v;
	return imp->transfer_phase(chk_access(), chkarg(1, 0., 1.));
}
static double input_phase(void* v) {
	Imp* imp = (Imp*)v;
	return imp->input_phase(chk_access(), chkarg(1, 0., 1.));
}
static double ratio_amp(void* v) {
	Imp* imp = (Imp*)v;
	return imp->ratio_amp(chk_access(), chkarg(1, 0., 1.));
}

static double deltafac(void* v) {
	Imp* imp = (Imp*)v;
	if (ifarg(1)) {
		imp->deltafac_ = chkarg(1, 1e-10, 1);
	}
	return imp->deltafac_;
}

static Member_func members[] = {
	"compute", compute,
	"loc", location,
	"input", input_amp,
	"transfer", transfer_amp,
	"ratio", ratio_amp,
	"input_phase", input_phase,
	"transfer_phase", transfer_phase,
	"deltafac", deltafac,
	0, 0
};

void Impedance_reg() {
	class2oc("Impedance", cons, destruct, members, NULL, NULL, NULL);
}

Imp::Imp(){
	n = 0;
	d = NULL;
	pivot = NULL;
	transfer = NULL;
	input = NULL;
	nli_ = NULL;
	
	sloc_ = NULL;
	xloc_ = 0.;
	istim = 0;
	deltafac_ = .001;
}

Imp::~Imp(){
	if (sloc_) {
		section_unref(sloc_);
	}
	impfree();
}

void Imp::impfree(){
	if (d) {
		delete [] d;
		delete [] transfer;
		delete [] input;
		delete [] pivot;
		d = NULL;
	}
	if (nli_) {
		delete nli_;
		nli_ = NULL;
	}
}

void Imp::check() {
	NrnThread* _nt = nrn_threads;
	nrn_thread_error("Impedance works with only one thread");
	if (sloc_ && !sloc_->prop) {
		section_unref(sloc_);
		sloc_ = NULL;
	}
	if (tree_changed) {
		setup_topology();
	}
	if (v_structure_change) {
		recalc_diam();
	}
	if (n != _nt->end) {
		alloc();
	}
}

void Imp::alloc(){
	NrnThread* _nt = nrn_threads;
	impfree();
	n = _nt->end;
	d = new Complex[n];
	transfer = new Complex[n];
	input = new Complex[n];
	pivot = new Complex[n];
}
int Imp::loc(Section* sec, double x){
	Node* nd;
	int i;
	nd = node_exact(sec, x);
	i = nd->v_node_index;
	return i;
}

double Imp::transfer_amp(Section* sec, double x){
	check();
	int vloc = loc(sec, x);
	return nli_ ? nli_->transfer_amp(istim, vloc) : abs(transfer[vloc]);
}

double Imp::input_amp(Section* sec, double x){
	check();
	return nli_ ? nli_->input_amp(loc(sec, x)) : abs(input[loc(sec, x)]);
}

double Imp::transfer_phase(Section* sec, double x){
	check();
	return nli_ ? nli_->transfer_phase(istim, loc(sec, x)) : arg(transfer[loc(sec, x)]);
}

double Imp::input_phase(Section* sec, double x){
	check();
	return nli_ ? nli_->input_phase(loc(sec, x)) : arg(input[loc(sec, x)]);
}

double Imp::ratio_amp(Section* sec, double x){
	check();
	int i = loc(sec, x);
	return nli_ ? nli_->ratio_amp(i, istim) : (abs(transfer[i]/input[i]));
}

void Imp::location(Section* sec, double x){
	if (sloc_) {
		section_unref(sloc_);
	}
	sloc_ = sec;
	xloc_ = x;
	if (sloc_) {
		section_ref(sloc_);
	}
}

void Imp::compute(double freq, bool nonlin){
	check();
	if (sloc_) {
		istim = loc(sloc_, xloc_);
	}else{
		hoc_execerror("Impedance stimulus location is not specified.", 0);
	}
	if (n == 0) return;
	double omega = 1e-6*2*3.14159265358979323846*freq; // wC has units of mho/cm2
	if (nonlin) {
		if (!nli_) {
			nli_ = new NonLinImp();
		}
		nli_->compute(omega, deltafac_);
	}else{
		if (nli_) {
			delete nli_;
			nli_ = NULL;
		}
		setmat(omega);
		LUDecomp();
		solve();
	}
}

void Imp::setmat(double omega) {
	NrnThread* _nt = nrn_threads;
	int i;
	setmat1();
	for (i=0; i < n; ++i) {
		d[i] = Complex(NODED(_nt->_v_node[i]), NODERHS(_nt->_v_node[i]) * omega);
		transfer[i] = 0.;
	}
	transfer[istim] = 1.e2/NODEAREA(_nt->_v_node[istim]); // injecting 1nA
	// rhs returned is then in units of mV or MegOhms
}


void Imp::setmat1() {
//printf("Imp::setmat1\n");
	/*
		The calculated g is good til someone else
		changes something having to do with the matrix.
	*/
	NrnThread* _nt = nrn_threads;
	Memb_list* mlc = _nt->tml->ml;
	int i;
	assert(_nt->tml->index == CAP);
	for (i=0; i < nrn_nthread; ++i) {
		double cj = nrn_threads[i].cj;
		nrn_threads[i].cj = 0;
		nrn_rhs(nrn_threads+i); // not useful except that many model description set g while
			// computing i
		nrn_lhs(nrn_threads+i);
		nrn_threads[i].cj = cj;
	}
	for (i=0; i < n; ++i) {
		NODERHS(_nt->_v_node[i]) = 0;
	}
	for (i=0; i < mlc->nodecount; ++i) {
		NODERHS(mlc->nodelist[i]) = mlc->data[i][0];
	}
}

void Imp::LUDecomp() {
	int i, ip;
	NrnThread* _nt = nrn_threads;
	int i1, i2, i3;
	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;
	for (i=i3-1; i >= i2; --i) {
		ip = _nt->_v_parent[i]->v_node_index;
		pivot[i] = NODEA(_nt->_v_node[i]) / d[i];
		d[ip] -= pivot[i] * NODEB(_nt->_v_node[i]);
	}
}

void Imp::solve() {
	int i, ip, j;
    for (j=0; j < nrn_nthread; ++j) {
	NrnThread* _nt = nrn_threads + j;
	int i1, i2, i3;
	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;
	for (i=istim; i >= i2; --i) {
		ip = _nt->_v_parent[i]->v_node_index;
		transfer[ip] -= transfer[i] * pivot[i];
	}
	for (i=i1; i < i2; ++i) {
		transfer[i] /= d[i];
		input[i] = 1./d[i];
	}
	for (i=i2; i < i3; ++i) {
		ip = _nt->_v_parent[i]->v_node_index;
		transfer[i] -= NODEB(_nt->_v_node[i]) * transfer[ip];
		transfer[i] /= d[i];
		input[i] = (1 + input[ip]*pivot[i]*NODEB(_nt->_v_node[i]))/d[i];
	}
	// take into account area
	for (i=i2; i < i3; ++i) {
		input[i] *= 1e2/NODEAREA(_nt->_v_node[i]);
	}
    }
}
