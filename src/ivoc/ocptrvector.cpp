#include <../../nrnconf.h>
/*
 construct a vector of pointers to variables and
 provide fast scatter/gather between Vector and those variables.
	p = new PtrVector(size)
	p.pset(i, &varname)
	val = p.getval(i)
	p.setval(i, value)
	p.scatter(Vector)
	p.gather(Vector)
*/
#include "classreg.h"
#include "oc2iv.h"
#include "ocptrvector.h"
#include "objcmd.h"
#include "ivocvect.h"

static double dummy;

OcPtrVector::OcPtrVector(int sz) {
	pd_ = new double*[sz];
	size_ = sz;
	update_cmd_ = NULL;
	for (int i=0; i < sz; ++i) {
		pd_[i] = &dummy;
	}
}

OcPtrVector::~OcPtrVector() {
	delete [] pd_;
	ptr_update_cmd(NULL);
}

void OcPtrVector::resize(int sz) {
	if (size_ == sz) { return; }
	delete [] pd_;
	pd_ = new double*[sz];
	size_ = sz;
	for (int i=0; i < sz; ++i) {
		pd_[i] = &dummy;
	}
}

void OcPtrVector::ptr_update_cmd(HocCommand* hc) {
	if (update_cmd_) {
		delete update_cmd_;
		update_cmd_ = NULL;
	}
	update_cmd_ = hc;
}

void OcPtrVector::ptr_update() {
	if (update_cmd_) {
		update_cmd_->execute(false);
	}else{
		hoc_warning("PtrVector has no ptr_update callback", NULL);
	}
}

void OcPtrVector::pset(int i, double* px) {
	assert (i < size_);
	pd_[i] = px;
}

void OcPtrVector::scatter(double* src, int sz) {
	assert(size_ == sz);
	for (int i=0; i < sz; ++i) {
		*pd_[i] = src[i];
	}
}

void OcPtrVector::gather(double* dest, int sz) {
	assert(size_ == sz);
	for (int i=0; i < sz; ++i) {
		dest[i] = *pd_[i];
	}
}

void OcPtrVector::setval(int i, double x) {
	assert(i < size_);
	*pd_[i] = x;
}

double OcPtrVector::getval(int i) {
	assert(i < size_);
	return *pd_[i];
}

static double resize(void* v) {
	((OcPtrVector*)v)->resize((int(chkarg(1, 1., 2e9))));
	return double(((OcPtrVector*)v)->size());
}

static double get_size(void* v){
	return ((OcPtrVector*)v)->size();
}

static double pset(void* v){
	OcPtrVector* opv = (OcPtrVector*)v;
	int i = int(chkarg(1, 0., opv->size()));
	opv->pset(i, hoc_pgetarg(2));
	return opv->getval(i);
}

static double getval(void* v){
	OcPtrVector* opv = (OcPtrVector*)v;
	int i = int(chkarg(1, 0., opv->size()));
	return opv->getval(i);
}

static double setval(void* v){
	OcPtrVector* opv = (OcPtrVector*)v;
	int i = int(chkarg(1, 0., opv->size()));
	opv->setval(i, *hoc_getarg(2));
	return opv->getval(i);
}

static double  scatter(void* v){
	OcPtrVector* opv = (OcPtrVector*)v;
	Vect* src = vector_arg(1);
	opv->scatter(vector_vec(src), vector_capacity(src));
	return 0.;
}

static double gather(void* v){
	OcPtrVector* opv = (OcPtrVector*)v;
	Vect* dest = vector_arg(1);
	opv->gather(vector_vec(dest), vector_capacity(dest));
	return 0.;
}

static double ptr_update_callback(void* v) {
	OcPtrVector* opv = (OcPtrVector*)v;
	HocCommand* hc = NULL;
	if (ifarg(1) && hoc_is_object_arg(1)) {
		hc = new HocCommand(*hoc_objgetarg(1));
	}else if (ifarg(1)) {
		Object* o = NULL;
		if (ifarg(2)) {
			o = *hoc_objgetarg(2);
		}
		hc = new HocCommand(hoc_gargstr(1), o);
	}
	opv->ptr_update_cmd(hc);
	return 0.;
}

static Member_func members[] = {
	"size", get_size,
	"resize", resize,
	"pset", pset,
	"setval", setval,
	"getval", getval,
	"scatter", scatter,
	"gather", gather,
	"ptr_update_callback", ptr_update_callback,
	0, 0
};

static void* cons(Object*) {
	int sz;
	sz = int(chkarg(1, 1., 2e9));
	OcPtrVector* ocpv = new OcPtrVector(sz);
	return (void*)ocpv;
}

static void destruct(void* v) {
	delete (OcPtrVector*)v;
}

void OcPtrVector_reg() {
	class2oc("PtrVector", cons, destruct, members, 0, 0, 0);
}
