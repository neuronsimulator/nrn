#include <../../nrnconf.h>

#if defined(__GO32__)
#define HAVE_IV 0
#endif

//#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ivstream.h>
#include <math.h>
#include <errno.h>

#include <OS/math.h>
#include "fourier.h"

#if HAVE_IV
#include <InterViews/glyph.h>
#include <InterViews/hit.h>
#include <InterViews/event.h>
#include <InterViews/color.h>
#include <InterViews/brush.h>
#include <InterViews/window.h>
#include <InterViews/printer.h>
#include <InterViews/label.h>
#include <InterViews/font.h>
#include <InterViews/background.h>
#include <InterViews/style.h>
//#include <OS/string.h>

#include <IV-look/kit.h>
#else
#include <InterViews/resource.h>
#include <OS/list.h>
#endif

#if defined(SVR4)
extern "C" {extern void exit(int status);};
#endif

#include "classreg.h"
#if HAVE_IV
#include "apwindow.h"
#include "ivoc.h"
#include "graph.h"
#endif

#ifndef PI
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif
#define PI M_PI
#endif
#define BrainDamaged 0  //The Sun CC compiler but it doesn't hurt to leave it in
#if BrainDamaged
#define FWrite(arg1,arg2,arg3,arg4) if (fwrite((char*)(arg1),arg2,arg3,arg4) != arg3) { hoc_execerror("fwrite error", 0); }
#define FRead(arg1,arg2,arg3,arg4) if (fread((char*)(arg1),arg2,arg3,arg4) != arg3) { hoc_execerror("fread error", 0); }
#else
#define FWrite(arg1,arg2,arg3,arg4) if (fwrite(arg1,arg2,arg3,arg4) != arg3){}
#define FRead(arg1,arg2,arg3,arg4) if (fread(arg1,arg2,arg3,arg4) != arg3) {}
#endif

static double dmaxint_;

// Definitions allow machine independent write and read
// note that must include BYTEHEADER at head of routine
// The policy is that each machine vwrite binary information in
// without swapping, ie. native endian.
// On reading, the type is checked to decide whether
// byteswapping should be performed

#define BYTEHEADER int _II__;  char *_IN__; char _OUT__[16]; int BYTESWAP_FLAG=0;
#define BYTESWAP(_X__,_TYPE__) \
    if (BYTESWAP_FLAG == 1) { \
	_IN__ = (char *) &(_X__); \
	for (_II__=0;_II__<sizeof(_TYPE__);_II__++) { \
		_OUT__[_II__] = _IN__[sizeof(_TYPE__)-_II__-1]; } \
	(_X__) = *((_TYPE__ *) &_OUT__); \
    }

#include "ivocvect.h"

// definition of SampleHistogram from the gnu c++ class library
#include <SmplHist.h>

// definition of random numer generator
#include "random1.h"
#include <Uniform.h>

// definition of comparison functions
#include <d_defs.h>

#if HAVE_IV
#include "utility.h"
#endif
#include "oc2iv.h"
#include "parse.h"
#include "ocfile.h"

extern "C" {
extern Object* hoc_thisobject;
extern Symlist* hoc_top_level_symlist;
extern void nrn_exit(int);
IvocVect* (*nrnpy_vec_from_python_p_)(void*);
Object** (*nrnpy_vec_to_python_p_)(void*);
Object** (*nrnpy_vec_as_numpy_helper_)(int, double*);
};


int cmpfcn(double a, double b) { return ((a) <= (b))? (((a) == (b))? 0 : -1) : 1; }


// math functions with error checking defined in oc/SRC/math.c
extern "C" {
  extern double hoc_Log(double x), hoc_Log10(double x), hoc_Exp(double x), hoc_Sqrt(double x);
  extern double hoc_scan(FILE*);
}

static int narg() {
  int i=0;
  while (ifarg(i++));
  return i-2;
}

#define MAX_FIT_PARAMS 20

#define TWO_BYTE_HIGH  65535.
#define ONE_BYTE_HIGH  255.
#define ONE_BYTE_HALF  128.

#define EPSILON 1e-9

extern "C" {
	extern void notify_freed_val_array(double*, size_t);
	extern void install_vector_method(const char* name, Pfrd_vp);
	extern int vector_instance_px(void*, double**);
	extern int vector_arg_px(int, double**);
        extern int nrn_mlh_gsort (double* vec, int *base_ptr, int total_elems, doubleComparator cmp);
};

IvocVect::IvocVect(Object* o) : ParentVect(){obj_ = o; label_ = NULL; MUTCONSTRUCT(0)}
IvocVect::IvocVect(int l, Object* o) : ParentVect(l){obj_ = o; label_ = NULL; MUTCONSTRUCT(0)}
IvocVect::IvocVect(int l, double fill_value, Object* o) : ParentVect(l, fill_value){obj_ = o; label_ = NULL; MUTCONSTRUCT(0)}
IvocVect::IvocVect(IvocVect& v, Object* o) : ParentVect(v) {obj_ = o; label_ = NULL; MUTCONSTRUCT(0)}

IvocVect::~IvocVect(){
	MUTDESTRUCT
	if (label_) {
		delete [] label_;
	}
	notify_freed_val_array(vec(), capacity());
}

void IvocVect::label(const char* label) {
	if (label_) {
		delete [] label_;
		label_ = NULL;
	}
	if (label) {
		label_ = new char[strlen(label) + 1];
		strcpy(label_, label);
	}
}

static const char* nullstr = "";

static const char** v_label(void* v) {
	Vect* x  = (Vect*)v;
	if (ifarg(1)) {
		x->label(gargstr(1));
	}
	if (x->label_) {
		return (const char**)&x->label_;
	}
	return &nullstr;
}

static void same_err(const char* s, Vect* x, Vect* y) {
	if (x == y) {
		hoc_execerror(s, " argument needs to be copied first");
	}
}

/* the Vect->at(start, end) function was used in about a dozen places
for the purpose of dealing with a subrange of elements. However that
function clones the subrange and returns a new Vect. This caused a
memory leak and was needlessly inefficient for those functions.
To fix both problems for these specific functions
we use a special vector which does not have
it's own space but merely a pointer and capacity to the right space of
the original vector. The usage is restricted to only once at a time, i.e.
can't use two subvecs at once and never do anything which affects the
memory space.
*/

static IvocVect* subvec_; // allocated when registered.
IvocVect* IvocVect::subvec(int start, int end) {
	subvec_->len = end - start + 1;
	subvec_->s = s + start;
	return subvec_;
}

void IvocVect::resize(int newlen) { // all that for this
	long oldcap = capacity();
	if (newlen > space) {
		notify_freed_val_array(vec(), capacity());
	}
	ParentVect::resize(newlen);
	for (;oldcap < newlen; ++oldcap) {
		elem(oldcap) = 0.;
	}
}

void IvocVect::resize_chunk(int newlen, int extra) {
	if (newlen > space) {
		long x = newlen + extra;
		if (extra == 0) {
			x = ListImpl_best_new_count(newlen, sizeof(double));
		}
//		printf("resize_chunk %d\n", x);
		resize(x);
	}
	resize(newlen);		
}

#if HAVE_IV
/*static*/ class GraphMarkItem : public GraphItem {
public:
	GraphMarkItem(Glyph* g) : GraphItem(g){}
	virtual ~GraphMarkItem(){};
	virtual void erase(Scene* s, GlyphIndex i, int type) {
		if (type & GraphItem::ERASE_LINE) {
			s->remove(i);
		}
	}
};
#endif

static void* v_cons(Object* o) {
	double fill_value = 0.;
	int n = 0;
	Vect* vec;
	if (ifarg(1)) {
		if (hoc_is_double_arg(1)) {
			n = int(chkarg(1,0,1e10));
			if (ifarg(2)) fill_value = *getarg(2);
			vec =  new Vect(n,fill_value, o);
		}else{
			if (!nrnpy_vec_from_python_p_) {
				hoc_execerror("Python not available", 0);
			}
			vec = (*nrnpy_vec_from_python_p_)(new Vect(0, 0, o));
		}
	}else{
		vec = new Vect(0, 0, o);
	}
	return vec;
}


static void v_destruct(void* v) {
	delete (Vect*)v;
}

static Symbol* svec_;

// extern "C" vector functions used by ocmatrix.dll
// can also be used in mod files
Vect* vector_new(int n, Object* o){return new Vect(n, o);}
Vect* vector_new0(){return new Vect();}
Vect* vector_new1(int n){return new Vect(n);}
Vect* vector_new2(Vect* v){return new Vect(*v);}
void vector_delete(Vect* v){delete v;}
int vector_buffer_size(Vect* v){return v->buffer_size();}
int vector_capacity(Vect* v){return v->capacity();}
void vector_resize(Vect* v, int n){v->resize(n);}
Object** vector_temp_objvar(Vect* v){return v->temp_objvar();}
double* vector_vec(Vect* v){return v->vec();}
Object** vector_pobj(Vect* v){return &v->obj_;}
char* vector_get_label(Vect* v) { return v->label_; }
void vector_set_label(Vect* v, char* s) { v->label(s); }
void vector_append(Vect* v, double x){
  long n = v->capacity();
  v->resize_chunk(n+1);
  v->elem(n) = x;
}

#ifdef WIN32
#if !defined(USEMATRIX) || USEMATRIX == 0
#include "../windll/dll.h"
extern "C" {extern char* neuron_home;}

void load_ocmatrix() {
	struct DLL* dll = NULL;
	char buf[256];
	sprintf(buf, "%s\\lib\\ocmatrix.dll", neuron_home);
	dll = dll_load(buf);
	if (dll) {
		Pfri mreg = (Pfri)dll_lookup(dll, "_Matrix_reg");
		if (mreg) {
			(*mreg)();
		}
	}else{
		printf("No Matrix class.\n");
	}
}
#endif
#endif

Object** IvocVect::temp_objvar() {
	IvocVect* v = (IvocVect*)this;
	Object** po;
	if (v->obj_) {
		po = hoc_temp_objptr(v->obj_);
	}else{
		po = hoc_temp_objvar(svec_, (void*)v);
		obj_ = *po;
	}
	return po;
}

extern "C" { // bug in cray compiler requires this
void install_vector_method(const char* name, double (*f)(void*)) {
	Symbol* s_meth;
	if (hoc_table_lookup(name, svec_->u.ctemplate->symtable)) {
		hoc_execerror(name, " already a method in the Vector class");
	}
	s_meth = hoc_install(name, FUNCTION, 0.0, &svec_->u.ctemplate->symtable);
	s_meth->u.u_proc->defn.pfd = (Pfrd)f;
#define PUBLIC_TYPE 1
	s_meth->cpublic = PUBLIC_TYPE;
}
}

int vector_instance_px(void* v, double** px) {
	Vect* x = (Vect*)v;
	*px = x->vec();
	return x->capacity();
}

Vect* vector_arg(int i) {
	Object* ob = *hoc_objgetarg(i);
	if (!ob || ob->ctemplate != svec_->u.ctemplate) {
		check_obj_type(ob, "Vector");
	}
	return (Vect*)(ob->u.this_pointer);
}

int is_vector_arg(int i) {
	Object* ob = *hoc_objgetarg(i);
	if (!ob || ob->ctemplate != svec_->u.ctemplate) {
		return 0;
	}
	return 1;
}

int vector_arg_px(int i, double** px) {
	Vect* x = vector_arg(i);
	*px = x->vec();
	return x->capacity();
}

extern void nrn_vecsim_add(void*, bool);
extern void nrn_vecsim_remove(void*);

static int possible_destvec(int arg, Vect*& dest) {
	if (ifarg(arg) && hoc_is_object_arg(arg)) {
	 	dest = vector_arg(arg);
	 	return arg+1;
	}else{
		dest = new Vect();
		return arg;
	}
}

static double v_record(void* v) {
	nrn_vecsim_add(v, true);
	return 1.;
}

static double v_play_remove(void* v) {
	nrn_vecsim_remove(v);
	return 1.;
}

static double v_play(void* v) {
	nrn_vecsim_add(v, false);
	return 1.;
}

static double v_fwrite(void* v) {
	Vect* vp = (Vect*)v;
	void* s;
	int x_max = vp->capacity()-1;
	int start = 0;
	int end = x_max;
	if (ifarg(2)) {
	  start = int(chkarg(2,0,x_max));
	  end = int(chkarg(3,0,x_max));
	}
	s = (void*)(&vp->elem(start));
	const char* x = (const char*)s;

    Object* ob = *hoc_objgetarg(1);
    check_obj_type(ob, "File");
    OcFile* f = (OcFile*)(ob->u.this_pointer);
	FILE* fp = f->file();
	if (!fp) {
		return 0.;
	}
	int n = end-start+1;
	BinaryMode(f);
	return (double)fwrite(x,sizeof(double),n,fp);
}

static double v_fread(void* v) {
	Vect* vp = (Vect*)v;
	void* s = (void*)(vp->vec());

        Object* ob = *hoc_objgetarg(1);
	check_obj_type(ob, "File");
	OcFile* f = (OcFile*)(ob->u.this_pointer);

	if (ifarg(2)) vp->resize(int(chkarg(2,0,1e10)));
	int n = vp->capacity();

        int type = 4;
	if (ifarg(3)) {
           type = int(chkarg(3,1,5));
        }

	FILE* fp = f->file();
	if (!fp) {
		return 0.;
	}

	int i;
	BinaryMode(f)
	if (n > 0) switch (type) {

        case 5:         // short ints
        {          
           short *xs = (short *)malloc(n * (unsigned)sizeof(short));
           FRead(xs,sizeof(short),n,fp);
           for (i=0;i<n;++i) {
	     vp->elem(i) = double(xs[i]);
	   }
           free((char *)xs);
           break;
        }

        case 4:        // doubles
           FRead(&(vp->elem(0)),sizeof(double),n,fp);
           break;
            
        case 3:         // floats
	{
           float *xf = (float *)malloc(n * (unsigned)sizeof(float));
           FRead(xf,sizeof(float),n,fp);
           for (i=0;i<n;++i) {
	     vp->elem(i) = double(xf[i]);
	   }
           free((char *)xf);
           break;
        }
          
        case 2:         // unsigned short ints
        {          

           unsigned short *xi = (unsigned short *)malloc(n * (unsigned)sizeof(unsigned short));
           FRead(xi,sizeof(unsigned short),n,fp);
           for (i=0;i<n;++i) {
	     vp->elem(i) = double(xi[i]);
	   }
           free((char *)xi);
           break;
        }
          
        case 1:         // char
        {         
           char *xc = (char *)malloc(n * (unsigned)sizeof(char));
           FRead(xc,sizeof(char),n,fp);
           for (i=0;i<n;++i) {
	     vp->elem(i) = double(xc[i]);
	   }
           free(xc);
           break;
        }
	}
	return 1;
}

static double v_vwrite(void* v) {
	Vect* vp = (Vect*)v;

        Object* ob = *hoc_objgetarg(1);
        check_obj_type(ob, "File");
        OcFile* f = (OcFile*)(ob->u.this_pointer);

	FILE* fp = f->file();
	if (!fp) {
		return 0.;
	}

	 BinaryMode(f)
	// first, write the size of the vector
	int n = vp->capacity();
        FWrite(&n,sizeof(int),1,fp);

// next, write the type of elements
        int type = 4;
	if (ifarg(2)) {
           type = int(chkarg(2,1,5));
        }
        FWrite(&type,sizeof(int),1,fp);

// convert the data if necessary
        int i;
        void *s;
        const char* x;

        double min,r,sf,sub,intermed;
        switch (type) {

        case 5:     // integers as ints (no scaling)
	{
           int* xi = (int *)malloc(n * sizeof(int));
           for (i=0;i<n;++i) {
	     xi[i] = (int)(vp->elem(i));
           }
	   FWrite(xi,sizeof(int),n,fp);
           free((char *)xi);
           break;
        }

        case 4:     // doubles (no conversion unless BYTESWAP used and needed)
	{
           s = (void*)(&(vp->elem(0)));	
	   x = (const char*)s;
	   FWrite(x,sizeof(double),n,fp);
           break;
        }

        case 3:     // float (simple automatic type conversion)
	{
           float* xf = (float *)malloc(n * (unsigned)sizeof(float));
           for (i=0;i<n;++i) {
	     xf[i] = float(vp->elem(i));
	   }
	   FWrite(xf,sizeof(float),n,fp);
           free((char *)xf);
           break;
        }

            
        case 2:     // short ints (scale to 16 bits with compression)
	{
           min = vp->min();
           r = vp->max()- min;
           if (r > 0) {
                sf = TWO_BYTE_HIGH/r;
           } else {
                sf = 1.;
           } 
           unsigned short* xi = (unsigned short *)malloc(n * (unsigned)sizeof(unsigned short));
           for (i=0;i<n;++i) {
              intermed = (vp->elem(i)-min)*sf;
              xi[i] = (unsigned short)intermed;
           }
           s = (void*)xi;
	   x = (const char*)s;
                      // store the info needed to reconvert
           FWrite(&sf,sizeof(double),1,fp);
           FWrite(&min,sizeof(double),1,fp);
                      // store the actual data
	   FWrite(x,sizeof(unsigned short),n,fp);
           free((char *)xi);
           break;
        }

        case 1:      // char (scale to 8 bits with compression)
 	{
           sub = ONE_BYTE_HALF;
           min = vp->min();
           r = vp->max()- min;
           if (r > 0) {
                sf = ONE_BYTE_HIGH/r;
           } else {
                sf = 1.;
           } 

           char* xc = (char *)malloc(n * (unsigned)sizeof(char));
           for (i=0;i<n;++i) {
	     xc[i] = char(((vp->elem(i)-min)*sf)-sub);
	   }
           s = (void*)xc;
	   x = (const char*)s;
                      // store the info needed to reconvert
           FWrite(&sf,sizeof(double),1,fp);
           FWrite(&min,sizeof(double),1,fp);
	   FWrite(x,sizeof(char),n,fp);
           free(xc);
           break;
        }

	}
    return 1;
}


static double v_vread(void* v) {
	Vect* vp = (Vect*)v;
	void* s = (void*)(vp->vec());

        Object* ob = *hoc_objgetarg(1);
	check_obj_type(ob, "File");
	OcFile* f = (OcFile*)(ob->u.this_pointer);
        BYTEHEADER

	FILE* fp = f->file();
	if (!fp) {
		return 0.;
	}

	BinaryMode(f)
        int n;
        FRead(&n,sizeof(int),1,fp);

        int type = 0;
        FRead(&type,sizeof(int),1,fp);

	// since the type ranges from 1 to 5 (very important that it not be 0)
	// we can check the type and decide if it needs to be byteswapped
	if (type < 1 || type > 5) {
		BYTESWAP_FLAG = 1;
	}else{
		BYTESWAP_FLAG = 0;
	}

        BYTESWAP(n,int)
        BYTESWAP(type,int)
	if (type < 1 || type > 5) { return 0.;}
        if (vp->capacity() != n) vp->resize(n);

// read as appropriate type and convert to doubles

        int i;
        double sf = 1.;
        double min = 0.;
        double add;

        switch (type) {

        case 5:         // ints; no conversion
        {          
           int *xi = (int *)malloc(n * sizeof(int));
           FRead(xi,sizeof(int),n,fp);
           for (i=0;i<n;++i) {
	     BYTESWAP(xi[i],int)
	     vp->elem(i) = double(xi[i]);
	   }
           free((char *)xi);
           break;
        }

        case 4:        // doubles
           FRead(&(vp->elem(0)),sizeof(double),n,fp);
  	   if (BYTESWAP_FLAG == 1) {
	     for (i=0;i<n;++i) { BYTESWAP(vp->elem(i),double) }  
	   }
           break;
            
        case 3:         // floats
	{
           float *xf = (float *)malloc(n * (unsigned)sizeof(float));
           FRead(xf,sizeof(float),n,fp);
           for (i=0;i<n;++i) {
	     BYTESWAP(xf[i],float)
	     vp->elem(i) = double(xf[i]);
	   }
           free((char *)xf);
           break;
        }
          
        case 2:         // unsigned short ints
        {          // convert back to double
           FRead(&sf,sizeof(double),1,fp);
           FRead(&min,sizeof(double),1,fp);
           BYTESWAP(sf,double)
           BYTESWAP(min,double)

           unsigned short *xi = (unsigned short *)malloc(n * (unsigned)sizeof(unsigned short));
           FRead(xi,sizeof(unsigned short),n,fp);
           for (i=0;i<n;++i) {
	     BYTESWAP(xi[i],short)
	     vp->elem(i) = double(xi[i]/sf +min);
	   }
           free((char *)xi);
           break;
        }
          
        case 1:         // char
        {         // convert back to double
           FRead(&sf,sizeof(double),1,fp);
           FRead(&min,sizeof(double),1,fp);
           BYTESWAP(sf,double)
           BYTESWAP(min,double)
           char *xc = (char *)malloc(n * (unsigned)sizeof(char));
           FRead(xc,sizeof(char),n,fp);
           add = ONE_BYTE_HALF;
           for (i=0;i<n;++i) {
	     vp->elem(i) = double((xc[i]+add)/sf +min);
	   }
           free(xc);
           break;
        }
		  }
	return 1;
}


static double v_printf(void *v) {
        Vect* x = (Vect*)v;
	
	int top = x->capacity()-1;
	int start = 0;
	int end = top;
	int next_arg = 1;
	const char *format = "%g\t";
	int print_file = 0;
	int extra_newline = 1; // when no File
	OcFile *f;

       	if (ifarg(next_arg) && (hoc_is_object_arg(next_arg))) {
           Object* ob = *hoc_objgetarg(1);
    	   check_obj_type(ob, "File");
           f = (OcFile*)(ob->u.this_pointer);
	   format = "%g\n";
	   next_arg++;
	   print_file = 1;
	 }
	 if (ifarg(next_arg) && (hoc_argtype(next_arg) == STRING)) {
	     format = gargstr(next_arg);
	     next_arg++;
	     extra_newline = 0;
	 }
         if (ifarg(next_arg)) {         
             start = int(chkarg(next_arg,0,top));
	     end = int(chkarg(next_arg+1,start,top));
         }

	 if (print_file) {
	   for (int i=start;i<=end;i++) {
             fprintf(f->file(),format,x->elem(i));
	   }
           fprintf(f->file(),"\n");
	 } else {
	   for (int i=start;i<=end;i++) {
	     printf(format,x->elem(i));
             if (extra_newline && !((i-start+1)%5)){ printf("\n");}
	   }
           if(extra_newline) {printf("\n");}
	 }
	return double(end-start+1);
}


static double v_scanf(void *v) {
        Vect* x = (Vect*)v;
	
	int n = -1;
	int c = 1;
	int nc = 1;
        Object* ob = *hoc_objgetarg(1);
    	check_obj_type(ob, "File");
        OcFile* f = (OcFile*)(ob->u.this_pointer);

	if (ifarg(4)) {
	  n = int(*getarg(2));
	  c = int(*getarg(3));
	  nc = int(*getarg(4));
	} else if (ifarg(3)) {
	  c = int(*getarg(2));
	  nc = int(*getarg(3));
	} else if (ifarg(2)) {
	  n = int(*getarg(2));
	} 

        if (n >= 0) { 
        	x->resize(n);
        }

	// start at the right column

	int i=0;
	
	while((n < 0 || i<n) && ! f->eof()) {

		// skip unwanted columns before
		int j;
	        for (j=1;j<c;j++) {
		  if (!f->eof()) hoc_scan(f->file());
		}

	        // read data
         	if (!f->eof()) {
			x->resize_chunk(i+1);
         		x->elem(i) = hoc_scan(f->file());
         	}

		// skip unwanted columns after
		for (j=c;j<nc;j++) {
		  if (!f->eof()) hoc_scan(f->file());
		}
		
		i++;
        }
	
	if (x->capacity() != i) x->resize(i);

	return double(i);
}

static double v_scantil(void *v) {
        Vect* x = (Vect*)v;
	double val, til;
	
	int c = 1;
	int nc = 1;
        Object* ob = *hoc_objgetarg(1);
    	check_obj_type(ob, "File");
        OcFile* f = (OcFile*)(ob->u.this_pointer);

	til = *getarg(2);
	if (ifarg(4)) {
	  c = int(*getarg(3));
	  nc = int(*getarg(4));
	}

	// start at the right column

	int i=0;
	
	for(;;) {

		// skip unwanted columns before
		int j;
	        for (j=1;j<c;j++) {
			if (hoc_scan(f->file()) == til) {
				return double(i);
			}
		}

	        // read data
		val = hoc_scan(f->file());
		if (val == til) {
			break;
		}
		x->resize_chunk(i+1);
       		x->elem(i) = val;

		// skip unwanted columns after
		for (j=c;j<nc;j++) {
			hoc_scan(f->file());
		}
		
		i++;
        }
	return double(i);
}



/*ARGSUSED*/
static Object** v_plot(void* v) {
        Vect* vp = (Vect*)v;
#if HAVE_IV
IFGUI
        int i;
	double* y = vp->vec();
	int n = vp->capacity();

        Object* ob1 = *hoc_objgetarg(1);
	check_obj_type(ob1, "Graph");
	Graph* g = (Graph*)(ob1->u.this_pointer);

	GraphVector* gv = new GraphVector("");

	if (ifarg(5)) {
		hoc_execerror("Vector.line:", "too many arguments");
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
	     for (i=0; i < n; ++i) gv->add(vp2->elem(i), y + i);
           } else {
                 // passed xinterval
	     double interval = *getarg(2);
             for (i=0; i < n; ++i) gv->add(i * interval, y + i);
           }
	} else {
	         // passed line attributes or nothing
	   for (i=0; i < n; ++i) gv->add(i, y + i);
	}

	if (vp->label_) {
		GLabel* glab = g->label(vp->label_);
		gv->label(glab);
		((GraphItem*)g->component(g->glyph_index(glab)))->save(false);
	}
	g->append(new GPolyLineItem(gv));
	
        g->flush();
ENDGUI
#endif
	return vp->temp_objvar();
}

static Object** v_ploterr(void* v) {
        Vect* vp = (Vect*)v;
#if HAVE_IV
IFGUI
	int n = vp->capacity();

        Object* ob1 = *hoc_objgetarg(1);
	check_obj_type(ob1, "Graph");
	Graph* g = (Graph*)(ob1->u.this_pointer);
	
	char style = '-';
	double size = 4;
	if (ifarg(4)) size = chkarg(4,0.1,100.);
	const ivColor * color = g->color();
	const ivBrush * brush = g->brush();
	if (ifarg(5)) {
	  color = colors->color(int(*getarg(5)));
	  brush = brushes->brush(int(*getarg(6)));
	}

        Vect* vp2 = vector_arg(2);
	if (vp2->capacity() < n) n = vp2->capacity();

        Vect* vp3 = vector_arg(3);
	if (vp3->capacity() < n) n = vp3->capacity();

        for (int i=0; i < n; ++i) {
	  g->begin_line();
	       
	  g->line(vp2->elem(i),vp->elem(i) - vp3->elem(i));
	  g->line(vp2->elem(i),vp->elem(i) + vp3->elem(i));
	  g->mark(vp2->elem(i), vp->elem(i) - vp3->elem(i), style, size, color, brush);      	     
	  g->mark(vp2->elem(i), vp->elem(i) + vp3->elem(i), style, size, color, brush);      	     
	}

        g->flush();
ENDGUI
#endif
	return vp->temp_objvar();
}

static Object** v_line(void* v) {
        Vect* vp = (Vect*)v;
#if HAVE_IV
IFGUI
        int i;
	int n = vp->capacity();

        Object* ob1 = *hoc_objgetarg(1);
	check_obj_type(ob1, "Graph");
	Graph* g = (Graph*)(ob1->u.this_pointer);
	char* s = vp->label_;

	if (ifarg(5)) {
		hoc_execerror("Vector.line:", "too many arguments");
	}
	if (narg() == 3) {
	  g->begin_line(colors->color(int(*getarg(2))),
			brushes->brush(int(*getarg(3))), s);
	} else if (narg() == 4) {
	  g->begin_line(colors->color(int(*getarg(3))),
			brushes->brush(int(*getarg(4))), s);
	} else {
	  g->begin_line(s);
	}

	if (narg() == 2 || narg() == 4) {
	         // passed a vector or xinterval and possibly line attributes
	   if (hoc_is_object_arg(2)) {
                 // passed a vector
             Vect* vp2 = vector_arg(2);
	     n = Math::min(n, vp2->capacity());
	     for (i=0; i < n; ++i) g->line(vp2->elem(i), vp->elem(i));
           } else {
                 // passed xinterval
	     double interval = *getarg(2);
             for (i=0; i < n; ++i) g->line(i * interval, vp->elem(i));
           }
	} else {
	         // passed line attributes or nothing
	   for (i=0; i < n; ++i) g->line(i, vp->elem(i));
	}
	
        g->flush();
ENDGUI
#endif
	return vp->temp_objvar();
}


static Object** v_mark(void* v) {
        Vect* vp = (Vect*)v;
#if HAVE_IV
IFGUI
        int i;
	int n = vp->capacity();

        Object* ob1 = *hoc_objgetarg(1);
	check_obj_type(ob1, "Graph");
	Graph* g = (Graph*)(ob1->u.this_pointer);

	char style = '+';
	if (ifarg(3)) {
		if (hoc_is_str_arg(3)) {
			style = *gargstr(3);
		} else {
			style = char(chkarg(3,0,10));
		}
	}
	double size = 12;
	if (ifarg(4)) size = chkarg(4,0.1,100.);
	const ivColor * color = g->color();
	if (ifarg(5)) color = colors->color(int(*getarg(5)));
	const ivBrush * brush = g->brush();
	if (ifarg(6)) brush = brushes->brush(int(*getarg(6)));

	if (hoc_is_object_arg(2)) {
                 // passed a vector
             Vect* vp2 = vector_arg(2);
	
	     for (i=0; i < n; ++i) {
    	       g->mark(vp2->elem(i), vp->elem(i), style, size, color, brush);       
	     }

        } else {
                 // passed xinterval
	     double interval = *getarg(2);
             for (i=0; i < n; ++i) {
	       g->mark(i*interval, vp->elem(i), style, size, color, brush);
	     }

        }
ENDGUI
#endif
	return vp->temp_objvar();
}

static Object** v_histogram(void* v) {
        Vect* x = (Vect*)v;

	double low = *getarg(1);
	double high = chkarg(2,low,1e99);
	double width = chkarg(3,0,high-low);

//	SampleHistogram h(low,high,width);
	int i;
//	for (i=0; i< x->capacity(); i++) h += x->elem(i);

//	int n = h.buckets();
	// analogous to howManyBuckets in gnu/SamplHist.cpp
	int n = int(floor((high - low)/width)) + 2;
	Vect* y = new Vect(n);
	y->fill(0.);
//	for (i=0; i< n; i++) y->elem(i) = h.inBucket(i);

	for (i=0; i < x->capacity(); ++i) {
		int ind = int(floor((x->elem(i) - low)/width)) + 1;
		if (ind >= 0 && ind < y->capacity()) {
			y->elem(ind) += 1.0;
		}
	}
	return y->temp_objvar();
}

static Object** v_hist(void* v) {

        Vect* hv = (Vect*)v;

	Vect* data = vector_arg(1);
	same_err("hist", hv, data);
	double start = *getarg(2);
	int size = int(*getarg(3));
	double step = chkarg(4,1.e-99,1.e99);
	double high = start+step*size;

//	SampleHistogram h(start,high,step);
//	for (int i=0; i< data->capacity(); i++) h += data->elem(i);

	if (hv->capacity() != size) hv->resize(size);
	hv->fill(0.);
	for (int i=0; i< data->capacity(); i++) {
	  int ind = int(floor((data->elem(i)-start)/step));
	  if (ind >=0 && ind < hv->capacity()) hv->elem(ind) += 1;
	}

//	for (i=0; i< size; i++) hv->elem(i) = h.inBucket(i);

	return hv->temp_objvar();
}



static Object** v_sumgauss(void* v) {
        Vect* x = (Vect*)v;

	double low = *getarg(1);
	double high = chkarg(2,low,1e99);
	double step = chkarg(3,1.e-99,1.e99);
	double var = chkarg(4,0,1.e99);
        Vect* w;
	bool d = false;
        if (ifarg(5)) {
	   w = vector_arg(5);
        } else {
           w = new Vect(x->capacity());
           w->fill(1);
	   d = true;
        }

	int points = int((high-low)/step + .5);
	Vect* sum = new Vect(points,0.);

	double svar = var/(step*step);

// 4/28/93 ZFM: corrected bug: replaced svar w/ var in line below
	double scale = 1./hoc_Sqrt(2.*PI*var);
	
	for (int i=0;i<x->capacity();i++) {
	  double xv = int((x->elem(i)-low)/step);
	  for (int j=0; j<points;j++) {
	    double arg = -(j-xv)*(j-xv)/(2.*svar);
	    if (arg > -20.) sum->elem(j) += hoc_Exp(arg) * scale * w->elem(i);
	  }
	}
	if (d) {
		delete w;
	}
	return sum->temp_objvar();
}

static Object** v_smhist(void* v) {
        Vect* v1 = (Vect*)v;

	Vect* data = vector_arg(1);

	double start = *getarg(2);
	int size = int(*getarg(3));
	double step = chkarg(4,1.e-99,1.e99);
	double var = chkarg(5,0,1.e99);

	int weight,i;
	Vect *w;

        if (ifarg(6)) {
	   w = vector_arg(6);
	   weight = 1;
	 } else {
	   weight = 0;
	 }

    
	// ready a gaussian to convolve
	double svar = 2*var/(step*step);
	double scale = 1/hoc_Sqrt(2.*PI*var);

	int g2 = int(sqrt(10*svar));
	int g = g2*2+1;

	int n = 1;
	while(n<size+g) n*=2;

	double *gauss = (double *)calloc(n,(unsigned)sizeof(double));

	for (i=0;i<= g2;i++) gauss[i] = scale * hoc_Exp(-i*i/svar);
	for (i=1;i<= g2;i++) gauss[g-i] = scale * hoc_Exp(-i*i/svar);

	// put the data into a time series
	double *series = (double *)calloc(n,(unsigned)sizeof(double));
	
	double high = start + n*step;
	if (weight) {
	  for (i=0;i<data->capacity();i++) {
	    if (data->elem(i) >= start && data->elem(i) < high) {
	      series[int((data->elem(i)-start)/step)] += w->elem(i);
	    }
	  }
	} else {
	  for (i=0;i<data->capacity();i++) {
	    if (data->elem(i) >= start && data->elem(i) < high) {
	      series[int((data->elem(i)-start)/step)] += 1.;
	    }
	  }
	}
	
	// ready the answer vector
	double *ans = (double *)calloc(2*n,(unsigned)sizeof(double));

	// convolve
	nrn_convlv(series,n,gauss,g,1,ans);

	// put the answer in the vector
	if (v1->capacity() != size) v1->resize(size);
	v1->fill(0.,0,size);
	for (i=0;i<size;i++) if (ans[i] > EPSILON) v1->elem(i) = ans[i];

	free((char*) series);
	free((char*) gauss);
	free((char*) ans);
	
	return v1->temp_objvar();
}


static Object** v_ind(void* v) {
        Vect* x = (Vect*)v;
	Vect* y = vector_arg(1);
	Vect* z = new Vect();

	int yv;
	int ztop = 0;
	int top = x->capacity();
	z->resize(y->capacity()); z->resize(0);
	for (int i=0;i<y->capacity();i++) {
	  yv = int(y->elem(i));
	  if ((yv < top) && (yv >= 0)) {
	    z->resize(++ztop);
	    z->elem(ztop-1) = x->elem(yv);
	  }
	}
	return z->temp_objvar();
}	


static double v_size(void* v) {
	Vect* x = (Vect*)v;
	return double(x->capacity());
}

static double v_buffer_size(void* v) {
	Vect* x = (Vect*)v;
	if (ifarg(1)) {
		int n = (int)chkarg(1, (double)x->capacity(), dmaxint_);
		x->buffer_size(n);
	}
	return x->buffer_size();
}

int IvocVect::buffer_size() {
	return space;
}

void IvocVect::buffer_size(int n) {
	double* y = new double[n];
	if (len > n) {
		len = n;
	}
	for (int i=0; i < len; ++i) {
		y[i] = s[i];
	}
	space = n;
	delete [] s;
	s = y;
}

static Object** v_resize(void* v) {
	Vect* x = (Vect*)v;
	x->resize(int(chkarg(1,0,dmaxint_)));
	return x->temp_objvar();
}

static Object** v_clear(void* v) {
	Vect* x = (Vect*)v;
	x->resize(0);
	return x->temp_objvar();
}

static double v_get(void* v) {
	Vect* x = (Vect*)v;
	return x->elem(int(chkarg(1,0,x->capacity()-1)));
}

static Object** v_set(void* v) {
	Vect* x = (Vect*)v;
	x->elem(int(chkarg(1,0,x->capacity()-1))) = *getarg(2);
	return x->temp_objvar();
}


static Object** v_append(void* v) {
        Vect* x = (Vect*)v;
        int n,j,i=1;
        while (ifarg(i)) {
	  if (hoc_argtype(i) == NUMBER) {
	    x->resize_chunk(x->capacity()+1);
	    x->elem(x->capacity()-1) = *getarg(i);
	  } else if (hoc_is_object_arg(i)) {
	    Vect* y = vector_arg(i);
	    same_err("append", x, y);
	    n = x->capacity();
	    x->resize_chunk(n+y->capacity());
	    for (j=0;j<y->capacity();j++) {
	      x->elem(n+j) = y->elem(j);
	    }
	  }
	  i++;
        }
        return x->temp_objvar();
}

static Object** v_insert(void* v) {
	// insert all before indx (first arg)
        Vect* x = (Vect*)v;
        int n,j,m,i=2;
	int indx = (int)chkarg(1, 0, x->capacity());
	m = x->capacity() - indx;
	double* z;
	if (m) {
		z = new double[m];
		for (j=0; j < m; ++j) {
			z[j] = x->elem(indx + j);
		}
	}
	x->resize(indx);
        while (ifarg(i)) {
	  if (hoc_argtype(i) == NUMBER) {
	    x->resize_chunk(x->capacity()+1);
	    x->elem(x->capacity()-1) = *getarg(i);
	  } else if (hoc_is_object_arg(i)) {
	    Vect* y = vector_arg(i);
	    same_err("insrt", x, y);
	    n = x->capacity();
	    x->resize_chunk(n+y->capacity());
	    for (j=0;j<y->capacity();j++) {
	      x->elem(n+j) = y->elem(j);
	    }
	  }
	  i++;
        }
	if (m) {
		n = x->capacity();
		x->resize(n + m);
		for (j=0; j < m; ++j) {
			x->elem(n + j) = z[j];
		}
		delete [] z;
	}
        return x->temp_objvar();
}

static Object** v_remove(void* v) {
        Vect* x = (Vect*)v;
	int i, j, n, start, end;
	start = (int)chkarg(1, 0, x->capacity()-1);
	if (ifarg(2)) {
		end = (int)chkarg(2, start, x->capacity()-1);
	}else{
		end = start;
	}
	n = x->capacity();
	for (i=start, j=end+1; j < n; ++i, ++j) {
		x->elem(i) = x->elem(j);
	}
	x->resize(i);
        return x->temp_objvar();
}

static double v_contains(void* v) {
        Vect* x = (Vect*)v;
        double g = *getarg(1);
        for (int i=0;i<x->capacity();i++) {
           if (Math::equal(x->elem(i), g, hoc_epsilon)) return 1.;
        }
        return 0.;
}


static Object** v_copy(void* v) {
        Vect* y = (Vect*)v;

        Vect* x = vector_arg(1);

	int top = x->capacity()-1;
	int srcstart = 0;
	int srcend = top;
	int srcinc = 1;

        int deststart = 0;
	int destinc = 1;

	if (ifarg(2) && hoc_is_object_arg(2)) {
		Vect* srcind = vector_arg(2);
		int ns = srcind->capacity();
		int nx = x->capacity();
		if (ifarg(3)) {
			Vect* destind = vector_arg(3);
			if (destind->capacity() < ns) {
				ns = destind->capacity();
			}
			int ny = y->capacity();
			for (int i=0; i < ns; ++i) {
				int ix = (int) (srcind->elem(i) + hoc_epsilon);
				int iy = (int) (destind->elem(i) + hoc_epsilon);
				if (ix >= 0 && iy >= 0 && ix < nx && iy < ny) {
					y->elem(iy) = x->elem(ix);
				}
			}
		}else{
			if (y->capacity() < nx) {
				nx = y->capacity();
			}
			for (int i=0; i < ns; ++i) {
				int ii = (int)(srcind->elem(i) + hoc_epsilon);
				if (ii >= 0 && ii < nx) {
					y->elem(ii) = x->elem(ii);
				}
			}
		}
		return y->temp_objvar();
	}

        if (ifarg(2) && !(ifarg(3))) {
            deststart = int(*getarg(2));
        } else {
            if (ifarg(4)) {
               deststart = int(*getarg(2));
    	       srcstart = int(chkarg(3,0,top));
   	       srcend = int(chkarg(4,-1,top));
		if (ifarg(5)) {
			destinc = int(chkarg(5, 1, dmaxint_));
			srcinc = int(chkarg(6, 1, dmaxint_));
		}			
            } else if (ifarg(3)) {
    	       srcstart = int(chkarg(2,0,top));
   	       srcend = int(chkarg(3,-1,top));
            }
        }
	if (srcend == -1) {
		srcend = top;
	}else if (srcend < srcstart) {
		hoc_execerror("Vector.copy: src_end arg smaller than src_start", 0);
	}
        int size = (srcend-srcstart)/srcinc;
        size *= destinc;
        size += deststart+1;
	if (y->capacity() < size) {
            y->resize(size);
        }else if (y->capacity() > size && !ifarg(2)) {
        	y->resize(size);
        }
	int i, j;
       	for (i=srcstart, j=deststart; i<=srcend; i+=srcinc, j+=destinc) {
       		y->elem(j) = x->elem(i);
       	}

        return y->temp_objvar();
}


static Object** v_at(void* v) {
        Vect* x = (Vect*)v;

	int top = x->capacity()-1;
	int start = 0;
	int end = top;
	if (ifarg(1)) {
	  start = int(chkarg(1,0,top));
	}
	if (ifarg(2)) {
	  end = int(chkarg(2,start,top));
	}
	int size = end-start+1;
	Vect* y = new Vect(size);
// ZFM: fixed bug -- i<size, not i<=size
	for (int i=0; i<size; i++) y->elem(i) = x->elem(i+start);

	return y->temp_objvar();
}

#define USE_MLH_GSORT 0
#if !USE_MLH_GSORT
typedef struct {double x; int i;} SortIndex;

static int sort_index_cmp(const void* a, const void* b) {
	double x = ((SortIndex*)a)->x;
	double y = ((SortIndex*)b)->x;
	if (x > y) return (1);
	if (x < y) return (-1);
	return (0);
}

static Object** v_sortindex(void* v) {
	// v.index(vsrc, vsrc.sortindex) sorts vsrc into v
	int i, n;
        Vect* x = (Vect*)v;
	n = x->capacity();
	Vect* y;
	possible_destvec(1, y);
	y->resize(n);

	SortIndex* si = new SortIndex[n];
	for (i=0; i < n; ++i) {
		si[i].i = i;
		si[i].x = x->elem(i);
	}
	
// On BlueGene
// qsort apparently can generate errno 38 "Function not implemented"
	qsort((void*)si, n, sizeof(SortIndex), sort_index_cmp);
	errno = 0;
	for (i=0; i<n; i++) y->elem(i) = (double)si[i].i;

	delete [] si;
	return y->temp_objvar();
}
#else
static Object** v_sortindex(void* v) {
	// v.index(vsrc, vsrc.sortindex) sorts vsrc into v
	int i, n;
        Vect* x = (Vect*)v;
	n = x->capacity();
	Vect* y;
	possible_destvec(1, y);
	y->resize(n);

	int* si = new int[n];
	for (i=0; i < n; ++i) {
		si[i] = i;
	}
	nrn_mlh_gsort(vector_vec(x), si, n, cmpfcn);
	for (i=0; i<n; i++) y->elem(i) = (double)si[i];

	delete [] si;
	return y->temp_objvar();
}
#endif // USE_MLH_GSORT

static Object** v_c(void* v) {
	return v_at(v);
}

static Object** v_cl(void* v) {
	Object** r = v_at(v);
	((Vect*)((*r)->u.this_pointer))->label(((Vect*)v)->label_);
	return r;
}

static Object** v_interpolate(void* v) {
	Vect* yd = (Vect*)v;
	Vect* xd = vector_arg(1);
	Vect* xs = vector_arg(2);
	Vect* ys;
	int flag, is, id, nd, ns;
	double thet;
	ns = xs->capacity();
	nd = xd->capacity();
	if (ifarg(3)) {
		ys = vector_arg(3);
		flag = 0;
	}else{
		ys = new Vect(*yd);
		flag = 1;
	}
	yd->resize(nd);
	// before domain
	for (id = 0; id < nd && xd->elem(id) <= xs->elem(0); ++id) {
		yd->elem(id) = ys->elem(0);
	}
	// in domain
	for (is = 1; is < ns && id < nd; ++is) {
		if (xs->elem(is) <= xs->elem(is-1)) {
			continue;
		}
		while (xd->elem(id) <= xs->elem(is)) {
thet = (xd->elem(id) - xs->elem(is-1))/(xs->elem(is) - xs->elem(is-1));
			yd->elem(id) = (1.-thet)*(ys->elem(is-1)) + thet*(ys->elem(is));
			++id;
			if (id >= nd) {
				break;
			}
		}
	}
	// after domain
	for (; id < nd; ++id) {
		yd->elem(id) = ys->elem(ns-1);
	}
	
	if (flag) {
		delete ys;
	}
	
	return yd->temp_objvar();
}

static int possible_srcvec(ParentVect*& src, Vect* dest, int& flag) {
	if (ifarg(1) && hoc_is_object_arg(1)) {
	 	src = vector_arg(1);
	 	flag = 0;
	 	return 2;
	}else{
		src = new ParentVect(*dest);
		flag = 1;
		return 1;
	}
}

static Object** v_where(void* v) {
        Vect* y = (Vect*)v;
	ParentVect* x;
	int iarg, flag;
	
	iarg = possible_srcvec(x, y, flag);

	int n = x->capacity();
	int m = 0;
	int i;

	char* op = gargstr(iarg++);
	double value = *getarg(iarg++);
	double value2;
	
	y->resize(0);

	if (!strcmp(op,"==")) {
	  for (i=0; i<n; i++) {
	    if (Math::equal(x->elem(i), value, hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"!=")) {
	  for (i=0; i<n; i++) {
	    if (!Math::equal(x->elem(i), value, hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,">")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) > value + hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"<")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) < value - hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,">=")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) >= value - hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"<=")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) <= value + hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"()")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) > value + hoc_epsilon) && (x->elem(i) < value2 - hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"[]")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) >= value - hoc_epsilon) && (x->elem(i) <= value2 + hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"[)")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) >= value - hoc_epsilon) && (x->elem(i) < value2 - hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else if (!strcmp(op,"(]")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) > value + hoc_epsilon) && (x->elem(i) <= value2 + hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = x->elem(i);
	    }
	  }
	} else {
	  hoc_execerror("Vector","Invalid comparator in .where()\n");
	}
	if (flag) {
		delete x;
	}
	return y->temp_objvar();
}

static double v_indwhere(void* v) {

  Vect* x = (Vect*)v;
  int i, iarg, m=0;
  char* op;
  double value,value2;

    op = gargstr(1);
    value = *getarg(2);
    iarg = 3;

  int n = x->capacity();


	if (!strcmp(op,"==")) {
	  for (i=0; i<n; i++) {
	    if (Math::equal(x->elem(i), value, hoc_epsilon)) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"!=")) {
	  for (i=0; i<n; i++) {
	    if (!Math::equal(x->elem(i),  value, hoc_epsilon)) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,">")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) > value + hoc_epsilon) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"<")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) < value - hoc_epsilon) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,">=")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) >= value - hoc_epsilon) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"<=")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) <= value + hoc_epsilon) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"()")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) > value + hoc_epsilon) && (x->elem(i) < value2 - hoc_epsilon)) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"[]")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) >= value - hoc_epsilon) && (x->elem(i) <= value2 + hoc_epsilon)) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"[)")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) >= value - hoc_epsilon) && (x->elem(i) < value2 - hoc_epsilon)) {
	      return i;
	    }
	  }
	} else if (!strcmp(op,"(]")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) > value + hoc_epsilon) && (x->elem(i) <= value2 + hoc_epsilon)) {
	      return i;
	    }
	  }
	} else {
	  hoc_execerror("Vector","Invalid comparator in .indwhere()\n");
	}
   
	return -1.;
}

static Object** v_indvwhere(void* v) {

  Vect* y = (Vect*)v;
  ParentVect* x;
  int i, iarg, m=0, flag;
  char* op;
  double value,value2;

	iarg = possible_srcvec(x, y, flag);
    op = gargstr(iarg++);
    value = *getarg(iarg++);
    y->resize(0);

  int n = x->capacity();


	if (!strcmp(op,"==")) {
	  for (i=0; i<n; i++) {
	    if (Math::equal(x->elem(i), value, hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"!=")) {
	  for (i=0; i<n; i++) {
	    if (!Math::equal(x->elem(i), value, hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,">")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) > value + hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"<")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) < value - hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,">=")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) >= value - hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"<=")) {
	  for (i=0; i<n; i++) {
	    if (x->elem(i) <= value + hoc_epsilon) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"()")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) > value + hoc_epsilon) && (x->elem(i) < value2 - hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"[]")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) >= value - hoc_epsilon) && (x->elem(i) <= value2 + hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"[)")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) >= value - hoc_epsilon) && (x->elem(i) < value2 - hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else if (!strcmp(op,"(]")) {     
	  value2 = *getarg(iarg);
	  for (i=0; i<n; i++) {
	    if ((x->elem(i) > value + hoc_epsilon) && (x->elem(i) <= value2 + hoc_epsilon)) {
	      y->resize_chunk(++m);
	      y->elem(m-1) = i;
	    }
	  }
	} else {
	  hoc_execerror("Vector","Invalid comparator in .indvwhere()\n");
	}
   
	if (flag) {
		delete x;
	}
	return y->temp_objvar();
}

static Object** v_fill(void* v) 
{
  Vect* x = (Vect*)v;
  int top = x->capacity()-1;
  int start = 0;
  int end = top;
  if (ifarg(2)) {
    start = int(chkarg(2,0,top));
    end = int(chkarg(3,start,top));
  }
  x->fill(*getarg(1),start,end-start+1);
  return x->temp_objvar();
}

static Object** v_indgen(void* v) 
{
  Vect* x = (Vect*)v;

  int n = x->capacity();

  double start = 0.;
  double step = 1.;
  double end = double(n-1);

  if (ifarg(1)) {
    if (ifarg(3)) {
      start = *getarg(1);
      end = *getarg(2);
      step = chkarg(3,Math::min(start-end,end-start),Math::max(start-end,end-start));
      double xn = floor((end-start)/step + EPSILON)+1.;
      if (xn > dmaxint_) {
	hoc_execerror("size too large", 0);
      } else if (xn < 0) {
	hoc_execerror("empty set", 0);
      }
      n = int(xn);
      if (n != x->capacity()) x->resize(n);
    } else if (ifarg(2)) {
    	start = *getarg(1);
    	step = chkarg(2,-dmaxint_,dmaxint_);
    } else {
      step = chkarg(1,-dmaxint_,dmaxint_);
    }
  }
  for (int i=0;i<n;i++) {
    x->elem(i) = double(i)*step+start;
  }
  return x->temp_objvar();
}

static Object** v_addrand(void* v)
{
        Vect* x = (Vect*)v;
        Object* ob = *hoc_objgetarg(1);
	check_obj_type(ob, "Random");
	Rand* r = (Rand*)(ob->u.this_pointer);
	int top = x->capacity()-1;
	int start = 0;
	int end = top;
	if (ifarg(2)) {
	  start = int(chkarg(2,0,top));
	  end = int(chkarg(3,start,top));
	}
	for (int i=start;i<=end;i++) x->elem(i) += (*(r->rand))();
	return x->temp_objvar();
}

static Object** v_setrand(void* v)
{
        Vect* x = (Vect*)v;
        Object* ob = *hoc_objgetarg(1);
	check_obj_type(ob, "Random");
	Rand* r = (Rand*)(ob->u.this_pointer);
	int top = x->capacity()-1;
	int start = 0;
	int end = top;
	if (ifarg(2)) {
	  start = int(chkarg(2,0,top));
	  end = int(chkarg(3,start,top));
	}
	for (int i=start;i<=end;i++) x->elem(i) = (*(r->rand))();
	return x->temp_objvar();
}





static Object** v_apply(void* v)
{
  Vect* x = (Vect*)v;
  char* func = gargstr(1); 
  int top = x->capacity()-1;
  int start = 0;
  int end = top;
  Object* ob;
  if (ifarg(2)) {
    start = int(chkarg(2,0,top));
    end = int(chkarg(3,start,top));
  }
  Symbol* s = hoc_lookup(func);
  ob = hoc_thisobject;
  if (!s) {
	ob = NULL;
	s = hoc_table_lookup(func, hoc_top_level_symlist);
	if (!s) {
	  	hoc_execerror(func, " is undefined");
	}
  }
  for (int i=start; i<=end; i++) {
    hoc_pushx(x->elem(i));
    x->elem(i) = hoc_call_objfunc(s, 1, ob);
  }
  return x->temp_objvar();
}

static double v_reduce(void* v)
{
  Vect* x = (Vect*)v;
  double base = 0;
  int top = x->capacity()-1;
  int start = 0;
  int end = top;
  if (ifarg(3)) {
    start = int(chkarg(3,0,top));
    end = int(chkarg(4,start,top));
  }
  char* func = gargstr(1);
  if (ifarg(2)) base = *getarg(2);
  Symbol* s = hoc_lookup(func);
  if (!s) {
  	hoc_execerror(func, " is undefined");
  }
  for (int i=start; i<=end; i++) {
    hoc_pushx(x->elem(i));
    base += hoc_call_func(s, 1);
  }
  return base;
}


static double v_min(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    return x->subvec(start, end)->min();
  } else {
    return x->min();
  }
}

static double v_min_ind(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    return x->subvec(start, end)->min_index()+start;
  } else {
    return x->min_index();
  }
}

static double v_max(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    return x->subvec(start, end)->max();
  } else {
    return x->max();
  }
}

static double v_max_ind(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    return (x->subvec(start,end))->max_index()+start;
  } else {
    return x->max_index();
  }
}

static double v_sum(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    return (x->subvec(start,end))->sum();
  } else {
    return x->sum();
  }
}

static double v_sumsq(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    return (x->subvec(start,end))->sumsq();
  } else {
    return x->sumsq();
  }
}

static double v_mean(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    if (end - start < 1) {
	hoc_execerror("end - start", "must be > 0");
    }
    return (x->subvec(start,end))->mean();
  } else {
    if (x->capacity() < 1) {
	hoc_execerror("Vector", "must have size > 0");
    }
    return x->mean();
  }
}

static double v_var(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    if (end - start < 1) {
	hoc_execerror("end - start", "must be > 1");
    }
    return (x->subvec(start,end))->var();
  } else {
    if (x->capacity() < 2) {
	hoc_execerror("Vector", "must have size > 1");
    }
    return x->var();
  }
}

static double v_stdev(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    if (end - start < 1) {
	hoc_execerror("end - start", "must be > 1");
    }
    return (x->subvec(start,end))->stdDev();
  } else {
    if (x->capacity() < 2) {
	hoc_execerror("Vector", "must have size > 1");
    }
    return x->stdDev();
  }
}

static double v_stderr(void* v)
{
  Vect* x = (Vect*)v;
  int x_max = x->capacity()-1;
  if (ifarg(1)) {
    int start = int(chkarg(1,0,x_max));
    int end = int(chkarg(2,0,x_max));
    if (end - start < 1) {
	hoc_execerror("end - start", "must be > 1");
    }
    return (x->subvec(start,end))->stdDev()/hoc_Sqrt(double(end-start+1));
  } else {
    if (x->capacity() < 2) {
	hoc_execerror("Vector", "must have size > 1");
    }
    return x->stdDev()/hoc_Sqrt((double)x_max+1.);
  }
}

static double v_meansqerr(void* v1)
{
  Vect* x = (Vect*)v1;

    Vect* y = vector_arg(1);
    Vect* w = NULL;

	if (ifarg(2)) {
		w = vector_arg(2);
	}
    double err = 0.;
    int size = x->capacity();
    if (size > y->capacity() || !size) {
      hoc_execerror("Vector","Vector argument too small\n");
    } else {
	if (w) {
	    if (size > w->capacity()) {
	       hoc_execerror("Vector","second Vector size too small\n");
	    }
	    for (int i=0;i<size;i++) {
		double diff = x->elem(i) - y->elem(i);
		err += diff*diff*w->elem(i);
	    }
	}else{
	      for (int i=0;i<size;i++) {
		double diff = x->elem(i) - y->elem(i);
		err += diff*diff;
	      }
	}
    }
  return err/size;
}

static double v_dot(void* v1)
{
  Vect* x = (Vect*)v1;
  Vect* y = vector_arg(1);
  return (*x) * (*y);
}

static double v_mag(void* v1)
{
  Vect* x = (Vect*)v1;
  return hoc_Sqrt(x->sumsq());
}

static Object** v_from_double(void* v) {
	Vect* x = (Vect*)v;
	int i;
	int n = (int)*getarg(1);
	double* px = hoc_pgetarg(2);
	x->resize(n);
	for (i=0; i < n; ++i) {
		x->elem(i) = px[i];
	}
	return x->temp_objvar();
}

static Object** v_add(void* v1)
{
  Vect* x = (Vect*)v1;
  if (hoc_argtype(1) == NUMBER) {
    (*x) += *getarg(1);
  } 
  if (hoc_is_object_arg(1)) {
    Vect* y = vector_arg(1);
    if (x->capacity() != y->capacity()) {
      hoc_execerror("Vector","Vector argument to .add() wrong size\n");
    } else {
      (*x) += (*y);
    }
  }
  return x->temp_objvar();
}


static Object** v_sub(void* v1)
{
  Vect* x = (Vect*)v1;
  if (hoc_argtype(1) == NUMBER) {
    (*x) -= *getarg(1);
  } 
  if (hoc_is_object_arg(1)) {
    Vect* y = vector_arg(1);
    if (x->capacity() != y->capacity()) {
      hoc_execerror("Vector","Vector argument to .sub() wrong size\n");
    } else {
    (*x) -= (*y); 
    }
  }
  return x->temp_objvar();
}

static Object** v_mul(void* v1)
{
  Vect* x = (Vect*)v1;
  if (hoc_argtype(1) == NUMBER) {
    (*x) *= *getarg(1);
  } 
  if (hoc_is_object_arg(1)) {
    Vect* y = vector_arg(1);
    if (x->capacity() != y->capacity()) {
      hoc_execerror("Vector","Vector argument to .mult() wrong size\n");
    } else {
      x->product(*y);
    }
  }
  return x->temp_objvar();
}


static Object** v_div(void* v1)
{
  Vect* x = (Vect*)v1;
  if (hoc_argtype(1) == NUMBER) {
    (*x) /= *getarg(1);
  } 
  if (hoc_is_object_arg(1)) {
    Vect* y = vector_arg(1);
    if (x->capacity() != y->capacity()) {
      hoc_execerror("Vector","Vector argument to .div() wrong size\n");
    } else {
      x->quotient(*y);
    }
  }
  return x->temp_objvar();
}

static double v_scale(void* v1)
{
  Vect* x = (Vect*)v1;

    double a = *getarg(1);
    double b = *getarg(2);
    double sf;
    double r = x->max()-x->min();
    if (r > 0) {
      sf = (b-a)/r;
      (*x) -= x->min();
      (*x) *= sf;
      (*x) += a;
    } else {
      sf = 0.;
    }
    return sf;
}

static double v_eq(void* v1)
{
  Vect* x = (Vect*)v1;
  Vect* y = vector_arg(1);
  int i, n = x->capacity();
  if (n != y->capacity()) {
  	return false;
  }
  for (i=0; i < n; ++i) {
  	if (!Math::equal(x->elem(i), y->elem(i), hoc_epsilon)) {
  		return false;
  	}
  }
  return true;
}


/*=============================================================================

	FITTING A MULTIVARIABLE FUNCTION FROM DATA -- SIMPLEX METHOD

		double simplex( p,n,trial,a,b,c,d )
		double	*p;
		   vector of dimension n, containing variables
		int	n;
		   number of variables of the function to fit
		int	trial; 
		   number of trials of simplex loop
		   trial = 0: do simplex until the minimum pole has found
		   trial > 0: do simplex [trial] times.
		double a,b,c,d
		   values of contraction/expansion of the simplex;
		   control the rate of the search in multidim variable space
		   must be chosen by trial and error
			2.0, 1.4, 0.7, 0.3  -> standard values

		return most optimized eval value
			(negative if error occured)

	A function must be supplied by the user:

		external function DOUBLE EVAL(double *p);

	This function evaluates the mean square error between the 
	the data and the multivariable function with the values of 
	variables in vector p.

=============================================================================*/


#define SIMPLEX_MAXN	1e+300
#define	SIMPLEX_INORM	1.2

/* 1.2: normal, 
   1.3: extensive search,
   1.1: final adjustments 
*/


#define SIMPLEX_ALPHA    2.0
#define SIMPLEX_BETA     1.4
#define SIMPLEX_GAMMA    0.7
#define SIMPLEX_DELTA    0.3

/*
		   values of contraction/expansion of the simplex;
		   control the rate of the search in multidim variable space
		   must be chosen by trial and error
			2.0, 1.4, 0.7, 0.3  -> standard values
*/

static double	splx_evl_;
static int	renew_;



static double eval(double *p, int n, Vect *x, Vect *y, char *fcn) 
{
  
  double sq_err = 0.;
  double guess;
  int i;
  double dexp,t,amp1,tau1,amp2,tau2;

  if (!strcmp(fcn,"exp2")) {
    if (n < 4) hoc_execerror("Vector",".fit(\"exp2\") requires amp1,tau1,amp2,tau2");
    amp1 = p[0];
    tau1 = p[1];
    amp2 = p[2];
    tau2 = p[3];
  
    for (i=0;i<x->capacity();i++) {
      t = x->elem(i);
      dexp = amp1*hoc_Exp(-t/tau1) + amp2*hoc_Exp(-t/tau2);
      guess = dexp - y->elem(i);
      sq_err += guess * guess;
    }
  } else if (!strcmp(fcn,"charging")) {
    if (n < 4) hoc_execerror("Vector",".fit(\"charging\") requires amp1,tau1,amp2,tau2");
    amp1 = p[0];
    tau1 = p[1];
    amp2 = p[2];
    tau2 = p[3];
  
    for (i=0;i<x->capacity();i++) {
      t = x->elem(i);
      dexp = amp1*(1-hoc_Exp(-t/tau1)) + amp2*(1-hoc_Exp(-t/tau2));
      guess = dexp - y->elem(i);
      sq_err += guess * guess;
    }
  } else if (!strcmp(fcn,"exp1")) {
    if (n < 2) hoc_execerror("Vector",".fit(\"exp1\") requires amp,tau");
    amp1 = p[0];
    tau1 = p[1];

      
    for (i=0;i<x->capacity();i++) {
      t = x->elem(i);
      dexp = amp1*hoc_Exp(-t/tau1);
      guess = dexp - y->elem(i);
      sq_err += guess * guess;
    }
  } else if (!strcmp(fcn,"line")) {
    if (n < 2) hoc_execerror("Vector",".fit(\"line\") requires slope,intercept");
     
    for (i=0;i<x->capacity();i++) {
      guess = (p[0]*x->elem(i) + p[1]) - y->elem(i);
      sq_err += guess * guess;
    }
  } else if (!strcmp(fcn,"quad")) {
    if (n < 3) hoc_execerror("Vector",".fit(\"quad\") requires ax^2+bx+c");
     
    for (i=0;i<x->capacity();i++) {
      guess = (p[0]*x->elem(i)*x->elem(i) + p[1]*x->elem(i) + p[2]) - y->elem(i);
      sq_err += guess * guess;
    }
  } else {
    for (i=0;i<x->capacity();i++) {
    
// first the independent variable
      hoc_pushx(x->elem(i));
// then other parameters
      for (int j=0;j<n;j++) hoc_pushx(p[j]);
	   
      guess = hoc_call_func(hoc_lookup(fcn), n+1) - y->elem(i);
      sq_err += guess * guess;
    }
  }
  return sq_err/x->capacity();
}


static double eval_error(double *p, int n, Vect *x, Vect *y, char *fcn)
{
  double retval;

  if (renew_ > 3*(n+1)) {
    retval = eval(p,n,x,y,fcn);
    if (retval == splx_evl_) return(splx_evl_); else return( SIMPLEX_MAXN );
  } else{
    retval = eval(p,n,x,y,fcn);
    if (splx_evl_ > retval) splx_evl_ = retval;
    return(retval);
  }
}

static double 	simplex(double *p, int n, Vect *x, Vect *y, char* fcn)
{
	int	i,j;
	int	emaxp;	/* max position */
	int	eminp;
	int	ptr;

	double	*evortex;	/* eval value of votexes */
	double	*gvortex;	/* the vector of gravity */
	double	*vortex;	/* parameter matrix */
	double	*nvortex;	/* new vortex */
	double	emax;
	double	emin;
	double	fv1;

	evortex = (double *)calloc(n+1,(unsigned)sizeof(double));
	gvortex = (double *)calloc(n, (unsigned)sizeof(double));
	vortex  = (double *)calloc(n*(n+1),(unsigned)sizeof(double));
	nvortex = (double *)calloc(n*4,(unsigned)sizeof(double));

	if( 0==evortex || 0==gvortex || 0==vortex || 0==nvortex ){
		printf("allocation error in simplex()\n");
		nrn_exit(1);
	}


// NEXT:;
	/* make the initial vortex matrix */
	for(i=0;i<n+1;i++){
	   for(j=0;j<n;j++){
		vortex[i*n+j]=p[j];
		if(i==j) vortex[i*n+j] *=SIMPLEX_INORM;
	   }
	}

	/* evaluate the n+1 of vortexes  and make the maximal one
	to be pointed out by emaxp */
	for(i=0;i<n+1;i++) evortex[i]=eval_error( &vortex[i*n],n,x,y,fcn);

Label2:;
	emin = emax = evortex[0];
	emaxp = eminp =0;
	for(i=0;i<n+1;i++){
		fv1=evortex[i];
		if( fv1>emax ){
			emax=fv1;
			emaxp=i;
		}
		if( fv1<emin ){
			emin = fv1;
			eminp=i;
		}
	}
	/* calculate the center of gravity */
	for(i=0;i<n;i++) gvortex[i]=0.0;
	for(i=0;i<n+1;i++)
	   for(j=0;j<n;j++) gvortex[j] += vortex[i*n+j];
	for(i=0;i<n;i++)
	   gvortex[i] = ( gvortex[i] - vortex[emaxp*n+i] ) / n;

	/* calculate the new vortexes */

	for(i=0;i<n;i++)
	   nvortex[i]=(1.0-SIMPLEX_ALPHA)*vortex[emaxp*n+i]+SIMPLEX_ALPHA*gvortex[i];
	fv1=eval_error( &nvortex[0],n,x,y,fcn);
	if( fv1 < evortex[emaxp] ){
		ptr = 0;
		goto UPDATE;
	}

	for(i=0;i<n;i++)
	   nvortex[1*n+i]=(1.0-SIMPLEX_BETA)*vortex[emaxp*n+i]+SIMPLEX_BETA*gvortex[i];
	fv1=eval_error( &nvortex[1*n],n,x,y,fcn );
	if( fv1 < evortex[emaxp] ){
		ptr = 1;
		goto UPDATE;
	}

	for(i=0;i<n;i++)
	   nvortex[2*n+i]=(1.0-SIMPLEX_GAMMA)*vortex[emaxp*n+i]+SIMPLEX_GAMMA*gvortex[i];
	fv1=eval_error( &nvortex[2*n],n,x,y,fcn );
	if( fv1 < evortex[emaxp] ){
		ptr = 2;
		goto UPDATE;
	}

	for(i=0;i<n;i++)
	   nvortex[3*n+i]=(1.0-SIMPLEX_DELTA)*vortex[emaxp*n+i]+SIMPLEX_DELTA*gvortex[i];
	fv1=eval_error( &nvortex[3*n],n,x,y,fcn );
	if( fv1 < evortex[emaxp] ){
		ptr = 3;
		goto UPDATE;
	}

	goto Label3;



UPDATE:;
	/* update the vortex matrix */
	if( splx_evl_ == fv1 ) renew_++;
	for(i=0;i<n;i++) vortex[emaxp*n+i]=nvortex[ptr*n+i];
	evortex[emaxp]=fv1;
	goto Label2;


Label3:;
	/*
	**  search for the vortex that represents smallest eval vaule
	*/
	emin=evortex[0];
	eminp=0;
	for(i=0;i<n+1;i++){
		if( evortex[i]<emin ){
			emin=evortex[i];
			eminp=i;
		}
	}

	for(i=0;i<n;i++) p[i]=vortex[eminp*n+i];
	free((char*) gvortex );
	free((char*) evortex );
	free((char*) vortex );
	free((char*) nvortex );
	return( emin );
}


double call_simplex(double *p, int n, Vect *x, Vect *y, char *fcn, int trial)
{
  double retval;

  if (!trial) {
    while (1) {
      renew_ = 0;
      splx_evl_ = SIMPLEX_MAXN;
      retval = simplex(p,n,x,y,fcn);
      if (!renew_ || splx_evl_ <= retval) break;
    }
  } else {
    for (int i=0;i<trial;i++) {
      renew_ = 0;
      splx_evl_ = SIMPLEX_MAXN;
      retval = simplex(p,n,x,y,fcn);
      if (!renew_  || splx_evl_ <= retval) break;
    }
  }
  return (retval);
}

static double v_fit(void* v) {

       int trial = 0;
/*		   number of trials of simplex loop
		   trial = 0: do simplex until the minimum pole has found
		   trial > 0: do simplex [trial] times.
*/


// get the data vector
       Vect* y = (Vect*)v;
       
// get a vector to place the fitted function
    Vect* fitted = vector_arg(1);
    if (fitted->capacity() != y->capacity()) fitted->resize(y->capacity());

// get a function to fit
	char *fcn;
        fcn = gargstr(2);

// get the independent variable

    Vect* x = vector_arg(3);
    if (x->capacity() != y->capacity()) {
      hoc_execerror("Vector","Indep argument to .fit() wrong size\n");
    }

// get the parameters of the function

	double * p_ptr[MAX_FIT_PARAMS];
	double p[MAX_FIT_PARAMS];

	if (ifarg(MAX_FIT_PARAMS)) {
	  hoc_execerror("Vector","Too many parameters to fit()\n");
	}
        int n = 0;
	while(ifarg(4+n)) {
	         // pointer to parameter
          p_ptr[n] = hoc_pgetarg(4+n);
		 // parameter value
	  p[n] = *(p_ptr[n]);
	  n++;
	}
	
	double meansqerr = call_simplex(p,n,x,y,fcn,trial);
       
// assign data to pointers where they came from;
	int i;
       for(i=0;i<n;i++) {
	 *(p_ptr[i]) = p[i];
       }

       if (!strcmp(fcn,"exp2")) {
	 for(i=0;i<x->capacity();i++) {
	   fitted->elem(i) = p[0]*hoc_Exp(-(x->elem(i)/p[1])) + 
    	                     p[2]*hoc_Exp(-(x->elem(i)/p[3]));
	 }
       } else if (!strcmp(fcn,"charging")) {
	 for(i=0;i<x->capacity();i++) {
	   fitted->elem(i) = p[0]*(1-hoc_Exp(-(x->elem(i)/p[1]))) +
    	                     p[2]*(1-hoc_Exp(-(x->elem(i)/p[3])));
	 }
       } else if (!strcmp(fcn,"exp1")) {
	 for(i=0;i<x->capacity();i++) {
	   fitted->elem(i) = p[0]*hoc_Exp(-(x->elem(i)/p[1]));
	 }
       } else if (!strcmp(fcn,"line")) {
	 for(i=0;i<x->capacity();i++) {
	   fitted->elem(i) = p[0]*x->elem(i)+p[1];
	 }
       } else if (!strcmp(fcn,"quad")) {
	 for(i=0;i<x->capacity();i++) {
	   fitted->elem(i) = p[0]*x->elem(i)*x->elem(i) + p[1]*x->elem(i) + p[2];
	 }
       } else {
	 for(i=0;i<x->capacity();i++) {
	   hoc_pushx(x->elem(i));
	   for (int j=0;j<n;j++) hoc_pushx(p[j]);
	   fitted->elem(i) = hoc_call_func(hoc_lookup(fcn), n+1);
	 }
       }

       return meansqerr;
}


// FOURIER analysis


static Object** v_correl(void* v) {
  Vect* v3 = (Vect*)v;

  Vect* v1;
  Vect* v2;

  // first data set
  v1 = vector_arg(1);

  // second data set
  if (ifarg(2)) {
    v2 = vector_arg(2);
  } else {
    v2 = v1;
  }

  // make both data sets equal integer power of 2
  int v1n = v1->capacity();
  int v2n = v2->capacity();
  int m = (v1n > v2n) ? v1n : v2n;
  int n = 1;
  while(n < m) n*=2;

  double *d1 = (double *)calloc(n,(unsigned)sizeof(double));
  int i;
  for (i=0;i<v1n;++i) d1[i] = v1->elem(i);
  double *d2 = (double *)calloc(n,(unsigned)sizeof(double));
  for (i=0;i<v2n;++i) d2[i] = v2->elem(i);
  double *ans = (double *)calloc(n,(unsigned)sizeof(double));

  nrn_correl(d1, d2, n, ans);

  if (v3->capacity() != n) v3->resize(n);
  for (i=0;i<n;++i) v3->elem(i)=ans[i];
  free((char *)d1);
  free((char *)d2);
  free((char *)ans);

  return v3->temp_objvar();
}

static Object** v_convlv(void* v) {
  Vect* v3 = (Vect*)v;

  Vect* v1, *v2;

  // data set
  v1 = vector_arg(1);


  // filter
  v2 = vector_arg(2);

  // convolve unless isign is -1, then deconvolve!
  int isign;
  if (ifarg(3)) isign = (int)(*getarg(3)); else isign = 1;

  // make both data sets equal integer power of 2
  int v1n = v1->capacity();
  int v2n = v2->capacity();
  int m = (v1n > v2n) ? v1n : v2n;
  int n = 1;
  while(n < m) n*=2;

  double *data = (double *)calloc(n,(unsigned)sizeof(double));
  int i;
  for (i=0;i<v1n;++i) data[i] = v1->elem(i);

  // assume respns is given in "wrap-around" order, 
  // with countup t=0..t=n/2 followed by countdown t=n..t=n/2
  // v2n should be an odd <= n

  double *respns = (double *)calloc(n,(unsigned)sizeof(double));
  for (i=0;i<v2n;i++) respns[i] = v2->elem(i);
  
  double *ans = (double *)calloc(2*n,(unsigned)sizeof(double));
 
  nrn_convlv(data,n,respns,v2n,isign,ans);

  if (v3->capacity() != n) v3->resize(n);
  for (i=0;i<n;++i) v3->elem(i)=ans[i];

  free((char *)data);
  free((char *)respns);
  free((char *)ans);

  return v3->temp_objvar();
}


static Object** v_spctrm(void* v) {
  Vect* ans = (Vect*)v;

  // n data pts will be divided into k partitions of size m
  // the spectrum will have m values from 0 to m/2 cycles/dt.
  //  n = (2*k+1)*m

  // data set
  Vect* v1 = vector_arg(1);

  int dc = v1->capacity();
  int mr;
  if (ifarg(2)) mr = (int)(*getarg(2)); else mr = dc/8;

  // make sure the length of partitions is integer power of 2
  int m = 1;
  while(m<mr) m*=2;  

  int k = int(ceil((double(dc)/m-1.)/2.));
  int n = (2*k+1)*m;

  double *data = (double *)calloc(n,(unsigned)sizeof(double));
  for (int i=0;i<dc;++i) data[i] = v1->elem(i);

  if (ans->capacity() < m) ans->resize(m);
  nrn_spctrm(data, &ans->elem(0), m, k);

  free((char *)data);

  return ans->temp_objvar();
}

static Object** v_filter(void* v) {
  Vect* v3 = (Vect*)v;
  Vect* v1;
  Vect *v2;
  int iarg = 1;

  // data set
  if (hoc_is_object_arg(iarg)) {
	v1 = vector_arg(iarg++);
  }else{
  	v1 = v3;
  }

  // filter
  v2 = vector_arg(iarg);

  // make both data sets equal integer power of 2
  int v1n = v1->capacity();
  int v2n = v2->capacity();
  int m = (v1n > v2n) ? v1n : v2n;
  int n = 1;
  while(n < m) n*=2;

  double *data = (double *)calloc(n,(unsigned)sizeof(double));
  int i;
  for (i=0;i<v1n;++i) data[i] = v1->elem(i);

  double *filter = (double *)calloc(n,(unsigned)sizeof(double));
  for (i=0;i<v2n;i++) filter[i] = v2->elem(i);
  
  double *ans = (double *)calloc(2*n,(unsigned)sizeof(double));
 
  nrngsl_realft(filter,n,1);
  
  nrn_convlv(data,n,filter,v2n,1,ans);

  if (v3->capacity() != n) v3->resize(n);
  for (i=0;i<n;++i) v3->elem(i)=ans[i];

  free((char *)data);
  free((char *)filter);
  free((char *)ans);

  return v3->temp_objvar();
}


static Object** v_fft(void* v) {
  Vect* v3 = (Vect*)v;
  Vect* v1;
  int iarg = 1;

  // data set
  if (hoc_is_object_arg(iarg)) {
	v1 = vector_arg(iarg++);
  }else{
  	v1 = v3;
  }

  // inverse = -1, regular = 1

  int inv = 1;
  if (ifarg(iarg)) inv = int(chkarg(iarg,-1,1));

  // make data set integer power of 2
  int v1n = v1->capacity();
  int n = 1;
  while(n < v1n) n*=2;

  double *data = (double *)calloc(n,(unsigned)sizeof(double));
  int i;
  for (i=0;i<v1n;++i) data[i] = v1->elem(i);
  if (v3->capacity() != n) v3->resize(n);

  if (inv == -1) {
    nrn_nrc2gsl(data, &v3->elem(0), n);
    nrngsl_realft(&v3->elem(0), n, -1);
  }else{
    nrngsl_realft(data, n, 1);
    nrn_gsl2nrc(data, &v3->elem(0), n);
  }
  free((char *)data);
 
  return v3->temp_objvar();
}
  
static Object** v_spikebin(void* v) {
  Vect* ans = (Vect*)v;

  // data set
  Vect* v1 = vector_arg(1);

  double thresh = *getarg(2);

  int bin = 1;
  if (ifarg(3)) bin = int(chkarg(3,0,1e6));

  int n = v1->capacity()/bin;
  if (ans->capacity() != n) ans->resize(n);
  ans->fill(0);

  int firing = 0;
  int k;

  for (int i=0; i<n;i++) {
    for (int j=0; j<bin;j++) {
      k = i*bin+j;
      if (v1->elem(k) >= thresh && !firing) {
	firing = 1;
	ans->elem(i)=1;
      } else if (firing && v1->elem(k) < thresh) {
	firing = 0;
      }
    }
  }
    
  return ans->temp_objvar();
}

static Object** v_rotate(void* v) 
{
  Vect* a = (Vect*)v;

  int wrap = 1;
  int rev = 0;

  int n = a->capacity(); 
  int r = int(*getarg(1));
  if (ifarg(2)) wrap = 0;
 
  if (r>n) r = r%n;
  if (r<0) { r = n-(Math::abs(r)%n); rev = 1; }

  if (r > 0) {
    int rc = n-r;
 
    double *hold = (double *)calloc(n,(unsigned)sizeof(double));

    int i;
    if (wrap) {
      for (i=0;i<rc;i++) hold[i+r] = a->elem(i);
      for (i=0;i<r;i++) hold[i] = a->elem(i+rc);
    } else {
      if (rev==0) {
	for (i=0;i<rc;i++) hold[i+r] = a->elem(i);
	for (i=0;i<r;i++) hold[i] = 0.;
      } else {
	for (i=0;i<r;i++) hold[i] = a->elem(i+rc);
	for (i=r;i<n;i++) hold[i] = 0.;
      }
    }
    for (i=0;i<n;i++) a->elem(i) = hold[i];


    free((char *)hold);
  }

  return a->temp_objvar();
}

static Object** v_deriv(void* v)
{
  Vect* ans = (Vect*)v;
  ParentVect* v1;
  int flag, iarg = 1;

  // data set
	iarg = possible_srcvec(v1, ans, flag);

  int n = v1->capacity();
  if (n < 2) {
hoc_execerror("Can't take derivative of Vector with less than two points", 0);
  }
  if (ans->capacity() != n) ans->resize(n);

  double dx = 1;
  if(ifarg(iarg)) dx = *getarg(iarg++);

  int sym = 2;
  if(ifarg(iarg)) sym = int(chkarg(iarg++,1,2));


 if (sym == 2) {
  // use symmetrical form -- see NumRcpC p. 187.
  // at boundaries use single-sided form

   ans->elem(0) = (v1->elem(1) - v1->elem(0))/dx;
   ans->elem(n-1) = (v1->elem(n-1) - v1->elem(n-2))/dx;
   dx = dx*2;
   for (int i=1;i<n-1;i++) {
     ans->elem(i) = (v1->elem(i+1) - v1->elem(i-1))/dx;
   }
 } else {
   // use single-sided form
   ans->resize(n-1);
   for (int i=0;i<n-1;i++) {
     ans->elem(i) = (v1->elem(i+1) - v1->elem(i))/dx;
   }
 }   
  if (flag) {
  	delete v1;
  }
  return ans->temp_objvar();
}

static Object** v_integral(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  int iarg = 1;

  // data set
  if (ifarg(iarg) && hoc_is_object_arg(iarg)) {
	v1 = vector_arg(iarg++);
  }else{
  	v1 = ans;
  }

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  double dx = 1.;
  if(ifarg(iarg)) dx = *getarg(iarg++);

  ans->elem(0) = v1->elem(0);
  for (int i=1;i<n;i++) {
    ans->elem(i) = ans->elem(i-1) + v1->elem(i) * dx;
  }
  return ans->temp_objvar();
}

static double v_trigavg(void* v)
{
  ParentVect* avg = (ParentVect*)((Vect*)v);

  // continuous data(t)
  Vect* data = vector_arg(1);

  // trigger times
  Vect* trig = vector_arg(2);

  int n = data->capacity();
  int pre = int(chkarg(3,0,n-1));
  int post = int(chkarg(4,0,n-1));
  int m = pre+post;
  if (avg->capacity() != m) avg->resize(m);
  int l = trig->capacity();
  int trcount = 0;

  (*avg) = 0.;
  
  for (int i=0;i<l;i++) {
    int tr = int(trig->elem(i));

    // throw out events within the window size of the edges
    if (tr >= pre && tr < n-post) {
      trcount++;
      for (int j=-pre;j<post;j++) {
	avg->elem(j+pre) += data->elem(tr+j);
      }
    }
  } 
  (*avg) /= trcount;
  
  return trcount;
}


static Object** v_medfltr(void* v)
{
  Vect* ans = (Vect*)v;
  ParentVect* v1;
  int w0, w1, wlen, i, flag, iarg = 1;

  // data set
	iarg = possible_srcvec(v1, ans, flag);

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  int points = 3;
  if (ifarg(iarg)) points = int(chkarg(iarg,1,n/2));

  double *res = (double *)calloc(n,(unsigned)sizeof(double));
  for (i=0; i<n; i++) {
    w0 = (i >= points) ? i-points : 0;
    w1 = (i < n-points) ? i+points : n-1;
    wlen = w1-w0;
    
    ParentVect window = v1->at(w0,wlen);
    window.sort(&cmpfcn);
    res[i] = window[wlen/2];
  }

  if (ans->capacity() != n) ans->resize(n);
  for (i=0;i <n; i++) {
    ans->elem(i) = res[i];
  }
  delete [] res;
  if (flag) {
  	delete v1;
  }
  return ans->temp_objvar();       
}

static double v_median(void* v)
{
  Vect* ans = (Vect*)v;

  int n = ans->capacity();
  if (n == 0) {
	hoc_execerror("Vector", "must have size > 0");
  }

  Vect* sorted = new Vect(*ans);
  sorted->sort(&cmpfcn);

  int n2 = n/2;
  double median;
  if (2*n2 == n) {
	median = (sorted->elem(n2-1) + sorted->elem(n2))/2.;
  }else{
	median = sorted->elem(n2);
  }
  delete sorted;

  return median;
}

static Object** v_sort(void* v)
{
  Vect* ans = (Vect*)v;
  ans->sort(&cmpfcn);
  return ans->temp_objvar();
}

static Object** v_reverse(void* v)
{
  Vect* ans = (Vect*)v;
  ans->reverse();
  return ans->temp_objvar();
}


static Object** v_sin(void* v)
{
  Vect* ans = (Vect*)v;

  int n = ans->capacity();
  double freq = *getarg(1);
  double phase = *getarg(2);
  double dx = 1;
  if(ifarg(3)) dx = *getarg(3);

  double period = 2*PI/1000*freq*dx;

  for (int i=0; i<n;i++) {
    ans->elem(i) = sin(period*i+phase);
  }
  return ans->temp_objvar();
}

static Object** v_log(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  // data set
	if (ifarg(1)) {
	  v1 = vector_arg(1);
	}else{
	  v1 = ans;
	}

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = log(v1->elem(i));
  } 
  return ans->temp_objvar();
}

static Object** v_log10(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  // data set
	if (ifarg(1)) {
	  v1 = vector_arg(1);
	}else{
	  v1 = ans;
	}

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = log10(v1->elem(i));
  } 
  return ans->temp_objvar();
}

static Object** v_rebin(void* v)
{
  Vect* ans = (Vect*)v;
  ParentVect* v1;
  int flag, iarg = 1;

  // data set
	iarg = possible_srcvec(v1, ans, flag);

  int f = int(*getarg(iarg));
  int n = v1->capacity()/f;
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = 0.;
    for (int j=0; j<f; j++) {
      ans->elem(i) += v1->elem(i*f+j);
    }
  }

  if (flag) {
  	delete v1;
  }
  return ans->temp_objvar();
}

static Object** v_resample(void* v)
{
  Vect* ans = (Vect*)v;

  // data set
  Vect* v1 = vector_arg(1);

  double f = chkarg(2,0,v1->capacity()/2);
  int n = int(v1->capacity()*f);

  Vect* temp = new Vect(n);

  for (int i=0; i<n; i++) temp->elem(i) = v1->elem(int(i/f));
  if (ans->capacity() != n) ans->resize(n);
  *((ParentVect*)ans) = *((ParentVect*)temp);

  delete temp;

  return ans->temp_objvar();
}

static Object** v_psth(void* v)
{
  Vect* ans = (Vect*)v;

  // data set
  Vect* v1 = vector_arg(1);

  double dt = chkarg(2,0,9e99);
  double trials = chkarg(3,0,9e99);
  double size = chkarg(4,0,v1->capacity()/2); int n = int(v1->capacity());

  Vect* temp = new Vect(n);

  for (int i=0; i<n; i++)  {
    int fj=0;
    int bj=0;
    double integral = v1->elem(i);
    while (integral < size) {
      if (i+fj < n-1) { fj++; integral += v1->elem(i+fj); }
      if (i-bj > 0 && integral < size) { bj++; integral += v1->elem(i-bj); }
    }
    temp->elem(i) = integral/trials*1000./((fj+bj+1)*dt);
  }

  if (ans->capacity() != n) ans->resize(n);
  *((ParentVect*)ans) = *((ParentVect*)temp);

  delete temp;

  return ans->temp_objvar();
}

static Object** v_inf(void* x)
{
  Vect* V = (Vect*)x;

  // data set
  Vect* stim = vector_arg(1);
  int n = stim->capacity();

  double dt = chkarg(2,1e-99,9e99);
  double gl = *getarg(3);
  double el = *getarg(4);
  double cm = *getarg(5)/dt;
  double th = *getarg(6);
  double res = *getarg(7);
  double refp = 0;
  if (ifarg(8)) refp = *getarg(8);

  if (V->capacity() != n) V->resize(n);

  double i=0,v,ref=0;
  
  V->elem(0) = el;

  for (int t=0;t<n-1;t++) {
    i = -gl*(V->elem(t)-el) + stim->elem(t);
    v = V->elem(t) + i/cm;
    if (v >= th && ref <= 0) {
      V->elem(t+1) = 0;
      t = t + 1;
      V->elem(t+1) = res;
      ref = refp;
    } else {
      ref = ref-dt;
      V->elem(t+1) = v;
    }
  }
  return V->temp_objvar();
}
  
static Object** v_pow(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  int iarg = 1;

  // data set
  if (hoc_is_object_arg(iarg)) {
	v1 = vector_arg(iarg++);
  }else{
  	v1 = ans;
  }

  double p = *getarg(iarg);
  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  if (p == -1) {
    for (int i=0; i<n;i++) {
      if (ans->elem(i) == 0) {
	hoc_execerror("Vector","Invalid comparator in .where()\n");
      } else {
	ans->elem(i) = 1/v1->elem(i);
      }
    }
  } else if (p == 0) {
    for (int i=0; i<n;i++) {
      ans->elem(i) = 1;
    }
  } else if (p == 0.5) {
    for (int i=0; i<n;i++) {
      ans->elem(i) = hoc_Sqrt(v1->elem(i));
    }
  } else if (p == 1) {
    for (int i=0; i<n;i++) {
      ans->elem(i) = v1->elem(i);
    }
  } else if (p == 2) {
    for (int i=0; i<n;i++) {
      ans->elem(i) = v1->elem(i)*v1->elem(i);
    }
  } else {
    for (int i=0; i<n;i++) {
      ans->elem(i) = pow(v1->elem(i),p);
    }
  }
  return ans->temp_objvar();
}

static Object** v_sqrt(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  // data set
	if (ifarg(1)) {
		v1 = vector_arg(1);
	}else{
		v1 = ans;
	}

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = hoc_Sqrt(v1->elem(i));
  } 
  return ans->temp_objvar();
}

static Object** v_abs(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  // data set
	if (ifarg(1)) {
	  v1 = vector_arg(1);
	}else{
	  v1 = ans;
	}

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = Math::abs(v1->elem(i));
  } 
  return ans->temp_objvar();
}

static Object** v_floor(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  // data set
	if (ifarg(1)) {
	  v1 = vector_arg(1);
	}else{
	  v1 = ans;
	}

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = floor(v1->elem(i));
  } 
  return ans->temp_objvar();
}

static Object** v_tanh(void* v)
{
  Vect* ans = (Vect*)v;
  Vect* v1;
  // data set
	if (ifarg(1)) {
	  v1 = vector_arg(1);
	}else{
	  v1 = ans;
	}

  int n = v1->capacity();
  if (ans->capacity() != n) ans->resize(n);

  for (int i=0; i<n;i++) {
    ans->elem(i) = tanh(v1->elem(i));
  } 
  return ans->temp_objvar();
}

static Object** v_index(void* v)
{
  Vect* ans = (Vect*)v;
  ParentVect* data;
  Vect* index;
  bool del = false;

  if (ifarg(2)) {
	data = vector_arg(1);
	index = vector_arg(2);
  }else{
	data = ans;
  	index = vector_arg(1);
  }
  if (data == ans) {
  	data = new ParentVect(*data);
  	del = true;
  }

  int n = data->capacity();
  int m = index->capacity();
  if (ans->capacity() != m) ans->resize(m);

  for (int i=0; i<m;i++) {
    int j = int(index->elem(i));
    if (j >= 0 && j < n) {
      ans->elem(i) = data->elem(j);
    } else {
      ans->elem(i) = 0.;
    }
  }

  if (del) {
  	delete data;
  }

  return ans->temp_objvar();
}

Object** v_from_python(void* v) {
	if (!nrnpy_vec_from_python_p_) {
		hoc_execerror("Python not available", 0);
	}
	Vect* vec = (*nrnpy_vec_from_python_p_)(v);
	return vec->temp_objvar();
}

Object** v_to_python(void* v) {
	if (!nrnpy_vec_to_python_p_) {
		hoc_execerror("Python not available", 0);
	}
	return (*nrnpy_vec_to_python_p_)(v);
}

Object** v_as_numpy(void* v) {
	if (!nrnpy_vec_as_numpy_helper_) {
		hoc_execerror("Python not available", 0);
	}
	Vect* vec = (Vect*)v;
	// not a copy, shares the data! So do not change the size while
	// the python numpy array is in use.
	return (*nrnpy_vec_as_numpy_helper_)(vec->capacity(), vec->vec());
}





static Member_func v_members[] = {

	"x",		v_size,	// will be changed below
	"size",		v_size,
	"buffer_size",	v_buffer_size,
	"get",	        v_get,
	"reduce",       v_reduce,
	"min",          v_min,
	"max",          v_max,
	"min_ind",      v_min_ind,
	"max_ind",      v_max_ind,
	"sum",          v_sum,
	"sumsq",        v_sumsq,
	"mean",         v_mean,
	"var",          v_var,
	"stdev",        v_stdev,
	"stderr",       v_stderr,
	"meansqerr",    v_meansqerr,
	"mag",          v_mag,
	"contains",     v_contains,
	"median",       v_median,

	"dot",          v_dot,
	"eq",           v_eq,

	"record",	v_record,
	"play",		v_play,
	"play_remove",	v_play_remove,

        "fwrite",       v_fwrite,
	"fread",        v_fread,
	"vwrite",       v_vwrite,
	"vread",        v_vread,
	"printf",       v_printf,
	"scanf",        v_scanf,
	"scantil",        v_scantil,

	"fit",          v_fit,
	"trigavg",      v_trigavg,
	"indwhere",     v_indwhere,

	"scale",        v_scale,

 	0, 0
};

static Member_ret_obj_func v_retobj_members[] = {
	"c",		v_c,
	"cl",		v_cl,
        "at",           v_at,
	"ind",          v_ind,
	"histogram",    v_histogram,
	"sumgauss",     v_sumgauss,

	"resize",	v_resize,
	"clear",        v_clear,
	"set",	        v_set,
	"append",       v_append,
	"copy",         v_copy,
	"insrt",	v_insert,
	"remove",	v_remove,
	"interpolate",	v_interpolate,
	"from_double",	v_from_double,

	"index",        v_index,
	"apply",        v_apply,
	"add",          v_add,
	"sub",          v_sub,
	"mul",         v_mul,
	"div",          v_div,
	"fill",         v_fill,
	"indgen",       v_indgen,
	"addrand",      v_addrand,
	"setrand",      v_setrand,
	"deriv",        v_deriv,
	"integral",     v_integral,

	"sqrt",		v_sqrt,
	"abs",       	v_abs,
	"floor",       	v_floor,
	"sin",		v_sin,
	"pow",          v_pow,
	"log", 		v_log,
	"log10",	v_log10,
	"tanh",		v_tanh,
	
	"correl",       v_correl,
	"convlv",       v_convlv,
	"spctrm",       v_spctrm,
	"filter",       v_filter,
	"fft",          v_fft,
	"rotate",       v_rotate,
	"smhist",       v_smhist,
	"hist",         v_hist,
	"spikebin",     v_spikebin,
	"rebin",        v_rebin,
	"medfltr",      v_medfltr,
	"sort",         v_sort,
	"sortindex",	v_sortindex,
	"reverse",      v_reverse,
	"resample",     v_resample,
	"psth",         v_psth,
	"inf",          v_inf,

	"index",        v_index,
	"indvwhere",     v_indvwhere,

	"where",        v_where,
	
	"plot",         v_plot,
	"line",         v_line,
	"mark",         v_mark,
	"ploterr",      v_ploterr,

	"from_python",	v_from_python,
	"to_python",	v_to_python,
	"as_numpy",	v_as_numpy,

	0,0
};

static Member_ret_str_func v_retstr_members[] = {
	"label",	v_label,

	0,0
};

extern "C" {
extern int hoc_araypt(Symbol*, int);
}

int ivoc_vector_size(Object* o) {
	Vect* vp = (Vect*)o->u.this_pointer;
	return vp->capacity();
}

double* ivoc_vector_ptr(Object* o, int index) {
	check_obj_type(o, "Vector");
	Vect* vp = (Vect*)o->u.this_pointer;
	return vp->vec() + index;
}

static void steer_x(void* v) {
	Vect* vp = (Vect*)v;
	int index;
	Symbol* s = hoc_spop();
	// if you don't want to test then you could get the index off the stack
	s->arayinfo->sub[0] = vp->capacity();
	index = hoc_araypt(s, SYMBOL);
	hoc_pushpx(vp->vec() + index);
}

void Vector_reg() {
	dmaxint_ = 1073741824.;
	for(;;) {
		if (dmaxint_*2. == double(int(dmaxint_*2.))) {
			dmaxint_ *= 2.;
		}else{
			if (dmaxint_*2. - 1. == double(int(dmaxint_*2. - 1.))) {
				dmaxint_ = 2.*dmaxint_ - 1.;
			}
			break;
		}
	}		
	//printf("dmaxint=%30.20g   %d\n", dmaxint_, (long)dmaxint_);
        class2oc("Vector", v_cons, v_destruct, v_members, NULL, v_retobj_members,
        	v_retstr_members);
	svec_ = hoc_lookup("Vector");
	// now make the x variable an actual double
	Symbol* sv = hoc_lookup("Vector");
	Symbol* sx = hoc_table_lookup("x", sv->u.ctemplate->symtable);
	sx->type = VAR;
	sx->arayinfo = new Arrayinfo;
	sx->arayinfo->refcount = 1;
	sx->arayinfo->a_varn = NULL;
	sx->arayinfo->nsub = 1;
	sx->arayinfo->sub[0] = 1;
	sv->u.ctemplate->steer = steer_x;
	subvec_ = new Vect();
#if defined(WIN32) && !defined(USEMATRIX)
	load_ocmatrix();
#endif
}

// hacked version of gsort from ../gnu/d_vec.cpp
// the transformation is that everything that used to be a double* becomes
// an int* and cmp(*arg1, *arg2) becomes cmp(vec[*arg1], vec[*arg2])
// I am not sure what to do about the BYTES_PER_WORD

// An adaptation of Schmidt's new quicksort

static inline void SWAP(int* A, int* B)
{
  int tmp = *A; *A = *B; *B = tmp;
}

/* This should be replaced by a standard ANSI macro. */
#define BYTES_PER_WORD 8
#define BYTES_PER_LONG 4

/* The next 4 #defines implement a very fast in-line stack abstraction. */

#define STACK_SIZE (BYTES_PER_WORD * BYTES_PER_LONG)
#define PUSH(LOW,HIGH) do {top->lo = LOW;top++->hi = HIGH;} while (0)
#define POP(LOW,HIGH)  do {LOW = (--top)->lo;HIGH = top->hi;} while (0)
#define STACK_NOT_EMPTY (stack < top)                

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:
   
   1. Non-recursive, using an explicit stack of pointer that
      store the next array partition to sort.  To save time, this
      maximum amount of space required to store an array of
      MAX_INT is allocated on the stack.  Assuming a 32-bit integer,
      this needs only 32 * sizeof (stack_node) == 136 bits.  Pretty
      cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and 
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.  
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segements.
   
   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (n)
      stack size is needed! */
      
int nrn_mlh_gsort (double* vec, int *base_ptr, int total_elems, doubleComparator cmp)
{
/* Stack node declarations used to store unfulfilled partition obligations. */
  struct stack_node {  int *lo;  int *hi; };
  int   pivot_buffer;
  int   max_thresh   = MAX_THRESH;

  if (total_elems > MAX_THRESH)
    {
      int       *lo = base_ptr;
      int       *hi = lo + (total_elems - 1);
      int       *left_ptr;
      int       *right_ptr;
      stack_node stack[STACK_SIZE]; /* Largest size needed for 32-bit int!!! */
      stack_node *top = stack + 1;

      while (STACK_NOT_EMPTY)
        {
          {
            int *pivot = &pivot_buffer;
            {
              /* Select median value from among LO, MID, and HI. Rearrange
                 LO and HI so the three values are sorted. This lowers the 
                 probability of picking a pathological pivot value and 
                 skips a comparison for both the LEFT_PTR and RIGHT_PTR. */

              int *mid = lo + ((hi - lo) >> 1);

              if ((*cmp) (vec[*mid], vec[*lo]) < 0)
                SWAP (mid, lo);
              if ((*cmp) (vec[*hi], vec[*mid]) < 0)
              {
                SWAP (mid, hi);
                if ((*cmp) (vec[*mid], vec[*lo]) < 0)
                  SWAP (mid, lo);
              }
              *pivot = *mid;
              pivot = &pivot_buffer;
            }
            left_ptr  = lo + 1;
            right_ptr = hi - 1; 

            /* Here's the famous ``collapse the walls'' section of quicksort.  
               Gotta like those tight inner loops!  They are the main reason 
               that this algorithm runs much faster than others. */
            do 
              {
                while ((*cmp) (vec[*left_ptr], vec[*pivot]) < 0)
                  left_ptr += 1;

                while ((*cmp) (vec[*pivot], vec[*right_ptr]) < 0)
                  right_ptr -= 1;

                if (left_ptr < right_ptr) 
                  {
                    SWAP (left_ptr, right_ptr);
                    left_ptr += 1;
                    right_ptr -= 1;
                  }
                else if (left_ptr == right_ptr) 
                  {
                    left_ptr += 1;
                    right_ptr -= 1;
                    break;
                  }
              } 
            while (left_ptr <= right_ptr);

          }

          /* Set up pointers for next iteration.  First determine whether
             left and right partitions are below the threshold size. If so, 
             ignore one or both.  Otherwise, push the larger partition's
             bounds on the stack and continue sorting the smaller one. */

          if ((right_ptr - lo) <= max_thresh)
            {
              if ((hi - left_ptr) <= max_thresh) /* Ignore both small partitions. */
                POP (lo, hi); 
              else              /* Ignore small left partition. */  
                lo = left_ptr;
            }
          else if ((hi - left_ptr) <= max_thresh) /* Ignore small right partition. */
            hi = right_ptr;
          else if ((right_ptr - lo) > (hi - left_ptr)) /* Push larger left partition indices. */
            {                   
              PUSH (lo, right_ptr);
              lo = left_ptr;
            }
          else                  /* Push larger right partition indices. */
            {                   
              PUSH (left_ptr, hi);
              hi = right_ptr;
            }
        }
    }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient 
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning 
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */


  {
    int *end_ptr = base_ptr + 1 * (total_elems - 1);
    int *run_ptr;
    int *tmp_ptr = base_ptr;
    int *thresh  = (end_ptr < (base_ptr + max_thresh))? 
      end_ptr : (base_ptr + max_thresh);

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + 1; run_ptr <= thresh; run_ptr += 1)
      if ((*cmp) (vec[*run_ptr], vec[*tmp_ptr]) < 0)
        tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr)
      SWAP (tmp_ptr, base_ptr);

    /* Insertion sort, running from left-hand-side up to `right-hand-side.' 
       Pretty much straight out of the original GNU qsort routine. */

    for (run_ptr = base_ptr + 1; (tmp_ptr = run_ptr += 1) <= end_ptr; )
      {

        while ((*cmp) (vec[*run_ptr], vec[*(tmp_ptr -= 1)]) < 0)
          ;

        if ((tmp_ptr += 1) != run_ptr)
          {
            int *trav;

            for (trav = run_ptr + 1; --trav >= run_ptr;)
              {
                int c = *trav;
                int *hi, *lo;

                for (hi = lo = trav; (lo -= 1) >= tmp_ptr; hi = lo)
                  *hi = *lo;
                *hi = c;
              }
          }

      }
  }
  return 1;
}
