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
#include <OS/math.h>
#if HAVE_IV
#include "graph.h"
#endif
#include "gui-redirect.h"
extern "C" {
	extern Object** (*nrnpy_gui_helper_)(const char* name, Object* obj);
	extern double (*nrnpy_object_to_double_)(Object*);
}

extern "C" int hoc_return_type_code;
static double dummy;

static Symbol* pv_class_sym_;

OcPtrVector::OcPtrVector(int sz) {
	label_ = NULL;
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
	if (label_) { free(label_); } // allocated by strdup
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

static const char* nullstr = "";

static const char** ptr_label(void* v) {
	char*& s = ((OcPtrVector*)v)->label_;
        if (ifarg(1)) {
	  if (s) { free(s); }
	  s = strdup(gargstr(1));
	}
	if (s) {
	  return (const char**)(&((OcPtrVector*)v)->label_);
	}
	return &nullstr;
}

static double resize(void* v) {
	hoc_return_type_code = 1; // integer
	((OcPtrVector*)v)->resize((int(chkarg(1, 1., 2e9))));
	return double(((OcPtrVector*)v)->size());
}

static double get_size(void* v){
	hoc_return_type_code = 1; // integer
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

//  a copy of ivocvect::v_plot with y+i replaced by y[i]
static int narg() {
  int i=0;
  while (ifarg(i++));
  return i-2;
}

static double ptr_plot(void* v) {
	TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("PtrVector.plot", pv_class_sym_, v);
	OcPtrVector* opv = (OcPtrVector*)v;
#if HAVE_IV
IFGUI
        int i;
	double** y = opv->pd_;
	int n = opv->size_;
	char* label = opv->label_;

        Object* ob1 = *hoc_objgetarg(1);
	check_obj_type(ob1, "Graph");
	Graph* g = (Graph*)(ob1->u.this_pointer);

	GraphVector* gv = new GraphVector("");

	if (ifarg(5)) {
		hoc_execerror("PtrVector.plot:", "too many arguments");
	}
	if (narg() == 3) {
	  gv->color((colors->color(int(*getarg(2)))));
	  gv->brush((brushes->brush(int(*getarg(3)))));
	} else if (narg() == 4) {
	  gv->color((colors->color(int(*getarg(3)))));
	  gv->brush((brushes->brush(int(*getarg(4)))));
	}

	if (narg() == 2 || narg() == 4) {
	         // passed a vector or xinterval and possibly line attributes
	   if (hoc_is_object_arg(2)) {
                 // passed a vector
             Vect* vp2 = vector_arg(2);
	     n = Math::min(n, vp2->capacity());
	     for (i=0; i < n; ++i) gv->add(vp2->elem(i), y[i]);
           } else {
                 // passed xinterval
	     double interval = *getarg(2);
             for (i=0; i < n; ++i) gv->add(i * interval, y[i]);
           }
	} else {
	         // passed line attributes or nothing
	   for (i=0; i < n; ++i) gv->add(i, y[i]);
	}

	if (label) {
		GLabel* glab = g->label(label);
		gv->label(glab);
		((GraphItem*)g->component(g->glyph_index(glab)))->save(false);
	}
	g->append(new GPolyLineItem(gv));
	
        g->flush();
ENDGUI
#endif
	return 0.0;
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
	"plot", ptr_plot,
	0, 0
};

static Member_ret_str_func retstr_members[] = {
	"label", ptr_label,
        0,0
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
	class2oc("PtrVector", cons, destruct, members, 0, 0, retstr_members);
	pv_class_sym_ = hoc_lookup("PtrVector");
}
