#ifndef oc2iv_h
#define oc2iv_h

#include <string.h>
#include <stdio.h>
// common things in oc that can be used by ivoc
extern "C" {

#include "hocdec.h"
extern int hoc_obj_run(const char*, Object*);
extern int hoc_argtype(int);
extern boolean hoc_is_double_arg(int);
extern boolean hoc_is_pdouble_arg(int);
extern boolean hoc_is_str_arg(int);
extern boolean hoc_is_object_arg(int);
extern char* gargstr(int);
extern char** hoc_pgargstr(int);
extern double* getarg(int);
extern double* hoc_pgetarg(int);
extern Object** hoc_objgetarg(int);
extern Object* hoc_name2obj(const char* name, int index);
extern int ifarg(int);
extern char** hoc_temp_charptr();
extern void hoc_assign_str(char** pstr, const char* buf);
extern double chkarg(int, double low, double high);
extern double hoc_call_func(Symbol*, int narg); // push first arg first. Warning: if the function is inside an object make sure you know what you are doing.
extern double hoc_call_objfunc(Symbol*, int narg, Object*); // call a fuction within the context of an object.
extern double hoc_ac_;
extern double hoc_epsilon;
extern void hoc_ret();
extern void ret(double);
extern void hoc_pushx(double);
extern void hoc_pushstr(char**);
extern void hoc_pushobj(Object**);
extern void hoc_push_object(Object*);
extern void hoc_pushpx(double*);
extern double hoc_xpop();
extern double* hoc_pxpop();
extern void hoc_execerror(const char*, const char*);
extern void hoc_warning(const char*, const char*);
extern double* hoc_val_pointer(const char*);
extern Symbol* hoc_lookup(const char*);
extern Symbol* hoc_table_lookup(const char*, Symlist*);
extern Symbol* hoc_install(const char*, int, double, Symlist**);
extern Objectdata* hoc_objectdata;
extern int hoc_total_array_data(Symbol*, Objectdata*);
extern char* hoc_araystr(Symbol*, int, Objectdata*);
extern char* hoc_object_name(Object*);
extern char* hoc_object_pathname(Object*);
extern const char* expand_env_var(const char*);
extern void check_obj_type(Object*, const char*);
extern boolean is_obj_type(Object*, const char*);
extern void hoc_obj_ref(Object*); // nil allowed
extern void hoc_obj_unref(Object*); // nil allowed
extern void hoc_dec_refcount(Object**);
extern Object** hoc_temp_objvar(Symbol* template_symbol, void* cpp_object);
extern Object** hoc_temp_objptr(Object*);
extern void hoc_new_object_asgn(Object** obp, Symbol* template_symbol, void* cpp_object);
extern void hoc_audit_command(const char*);
extern HocSymExtension* hoc_var_extra(const char*);
extern double check_domain_limits(float*, double);
extern Object* hoc_obj_get(int i);
extern void hoc_obj_set(int i, Object*);
extern void nrn_hoc_lock();
extern void nrn_hoc_unlock();
}

//xmenu
#define CChar const char
extern void hoc_ivpanel(CChar*, boolean h = false);
extern void hoc_ivpanelmap(int scroll = -1);
extern void hoc_ivbutton(CChar* name, CChar* action, Object* pyact = 0);
extern void hoc_ivradiobutton(CChar* name, CChar* action, boolean activate = false, Object* pyact = 0);
extern void hoc_ivmenu(CChar*, boolean add2menubar = false);
extern void hoc_ivvarmenu(CChar*, CChar*, boolean add2menubar = false, Object* pyvar = nil);
extern void hoc_ivvalue(CChar* name, CChar* variable, boolean deflt=false, Object* pyvar = 0);
extern void hoc_ivfixedvalue(CChar* name, CChar* variable,
		boolean deflt=false, boolean usepointer=false);
extern void hoc_ivvalue_keep_updated(CChar* name, CChar* variable, Object* pyvar = 0);
extern void hoc_ivpvalue(CChar* name, double*, boolean deflt=false, HocSymExtension* extra=nil);
extern void hoc_ivvaluerun(CChar* name, CChar* variable, CChar* action,
	boolean deflt=false, boolean canrun=false, boolean usepointer=false, Object* pyvar = 0, Object* pyact = 0);
extern void hoc_ivpvaluerun(CChar* name, double*, CChar* action,
	boolean deflt=false, boolean canrun=false, HocSymExtension* extra=nil);

extern void hoc_ivlabel(CChar*);
extern void hoc_ivvarlabel(char**, Object* pyvar = 0);
extern void hoc_ivstatebutton(double*, CChar* name, CChar* action, int style, Object* pyvar = 0, Object* pyact = 0);
extern void hoc_ivslider(double*, float low=0, float high=100,
	float resolution=1, int steps=10,
	const char* send_cmd=nil, boolean vert=false, boolean slow = false, Object* pyvar = 0, Object* pyact = 0);

inline double* object_pval(Symbol* sym, Objectdata* od) {
	return od[sym->u.oboff].pval;
}
inline char* object_str(Symbol* sym, Objectdata* od) {
	return *od[sym->u.oboff].ppstr;
}
inline char** object_pstr(Symbol* sym, Objectdata* od) {
	return od[sym->u.oboff].ppstr;
}
inline Object** object_pobj(Symbol* sym, Objectdata* od) {
	return od[sym->u.oboff].pobj;
}
inline hoc_Item** object_psecitm(Symbol* sym, Objectdata* od) {
	return od[sym->u.oboff].psecitm;
}

class Oc2IV {
public:
	static char** object_pstr(const char* symname, Object* = nil);
	static char* object_str(const char* symname, Object* = nil);
};

class Symlist;

#ifndef OCMATRIX

class ParseTopLevel {
public:
	ParseTopLevel();
	virtual ~ParseTopLevel();
	void save();
	void restore();
private:
	Objectdata* obdsav_;
	Object* obsav_;
	Symlist* symsav_;
	boolean restored_;
};
#endif

#endif
