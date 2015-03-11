#include <../../nrnconf.h>
#include <nrnmpiuse.h>

extern char* nrn_version();

/* change this to correspond to the ../nmodl/nocpout nmodl_version_ string*/
static char nmodl_version_[] =
"6.2.0";

static char banner[] =
"Duke, Yale, and the BlueBrain Project -- Copyright 1984-2015\n\
See http://www.neuron.yale.edu/neuron/credits\n";

# include	<stdio.h>
#include <stdlib.h>
#include "section.h"
#include "parse.h"
#include "nrniv_mf.h"
#include "cabvars.h"
#include "neuron.h"
#include "membdef.h"
#include "nrnmpi.h"

#ifdef WIN32
#if defined(HAVE_DLFCN_H) && !defined(__MINGW32__)
#include <dlfcn.h>
#else
#define RTLD_NOW 0
extern void* dlopen(const char* name, int mode);
extern void* dlsym(void* handle, char* name);
extern int dlclose(void* handle);
extern char* dlerror();
#endif
/*#include "../mswin/windll/dll.h"*/
/*static struct DLL* dll;*/
#endif
#if defined(WIN32) || defined(NRNMECH_DLL_STYLE)
extern char* nrn_mech_dll; /* declared in hoc_init.c so ivocmain.cpp can see it */
#endif
#if defined(WIN32)
#undef DLL_DEFAULT_FNAME
#define DLL_DEFAULT_FNAME "nrnmech.dll"
#endif
#if defined(NRNMECH_DLL_STYLE)
#if defined(DARWIN)
#ifndef DLL_DEFAULT_FNAME
#define DLL_DEFAULT_FNAME "libnrnmech.dylib"
#endif
#if __GNUC__ < 4
#include "osxdlfcn.h"
#include "osxdlfcn.c"
#else
#include <dlfcn.h>
#endif
#else
#include <dlfcn.h>
#ifndef DLL_DEFAULT_FNAME
#define DLL_DEFAULT_FNAME "./libnrnmech.so"
#endif
#endif
#endif

# define	CHECK(name)	if (hoc_lookup(name) != (Symbol *)0){\
		IGNORE(fprintf(stderr, CHKmes, name));\
		nrn_exit(1);}

static char	CHKmes[] = "The user defined name, %s, already exists\n";

void (*nrnpy_reg_mech_p_)(int);

int secondorder=0;
int state_discon_allowed_;
extern int nrn_nobanner_;
double t, dt, clamp_resist, celsius, htablemin, htablemax;
hoc_List* section_list;
int nrn_global_ncell = 0; /* used to be rootnodecount */
extern double hoc_default_dll_loaded_;
extern int nrn_istty_;
extern int nrn_nobanner_;

#if FISHER
#include <stdlib.h>
#include "fisher.h"
double id_number;              /* for rcs control, set in setup_id_info() */
char login_name[20];           /* store user's login for sys.c & rcs.c    */
char *pipe_filter = "more";    /* allow for running NEURON in emacs       */
#endif

static HocParmLimits _hoc_parm_limits[] = {
	"Ra", 1e-6, 1e9,
	"L", 1e-4, 1e20,
	"diam", 1e-9, 1e9,
	"cm", 0., 1e9,
	"rallbranch", 1., 1e9,
	"nseg", 1., 1e9,
	"celsius", -273., 1e6,
	"dt", 1e-9, 1e15,
	0, 0., 0.
};

static HocParmUnits _hoc_parm_units[] = {
	"Ra", "ohm-cm",
	"L", "um",
	"diam", "um",
	"cm", "uF/cm2",
	"celsius", "degC",
	"dt", "ms",
	"t", "ms",
	"v", "mV",
	"i_cap", "mA/cm2",
	0, 0
};

static int memb_func_size_;
Memb_func* memb_func;
Memb_list* memb_list;
short* memb_order_;
Symbol** pointsym;
Point_process** point_process;
char* pnt_map;		/* so prop_free can know its a point mech*/
BAMech** bamech_;

Template** nrn_pnt_template_; /* for finding artificial cells */
/* for synaptic events. */
pnt_receive_t* pnt_receive;
pnt_receive_init_t* pnt_receive_init;
short* pnt_receive_size;

 /* values are type numbers of mechanisms which do net_send call */
int nrn_has_net_event_cnt_;
int* nrn_has_net_event_;
int* nrn_prop_param_size_;
int* nrn_prop_dparam_size_;
int* nrn_dparam_ptr_start_;
int* nrn_dparam_ptr_end_;

void  add_nrn_has_net_event(type) int type; {
	++nrn_has_net_event_cnt_;
	nrn_has_net_event_ = (int*)erealloc(nrn_has_net_event_, nrn_has_net_event_cnt_*sizeof(int));
	nrn_has_net_event_[nrn_has_net_event_cnt_ - 1] = type;
}

/* values are type numbers of mechanisms which have FOR_NETCONS statement */
int nrn_fornetcon_cnt_; /* how many models have a FOR_NETCONS statement */
int* nrn_fornetcon_type_; /* what are the type numbers */
int* nrn_fornetcon_index_; /* what is the index into the ppvar array */

void add_nrn_fornetcons(int type, int indx) {
	int i = nrn_fornetcon_cnt_++;
	nrn_fornetcon_type_ = (int*)erealloc(nrn_fornetcon_type_, (i+1)*sizeof(int));
	nrn_fornetcon_index_ = (int*)erealloc(nrn_fornetcon_index_, (i+1)*sizeof(int));
	nrn_fornetcon_type_[i] = type;
	nrn_fornetcon_index_[i]= indx;
}

/* array is parallel to memb_func. All are 0 except 1 for ARTIFICIAL_CELL */
short* nrn_is_artificial_;
short* nrn_artcell_qindex_;

void  add_nrn_artcell(int type, int qi) {
	nrn_is_artificial_[type] = 1;
	nrn_artcell_qindex_[type] = qi;
}

int nrn_is_artificial(int pnttype) {
	return (int)nrn_is_artificial_[pointsym[pnttype]->subtype];
}

int nrn_is_cable(void) {return 1;}

#if 0 && defined(WIN32)
int mswin_load_dll(char* cp1) {
	if (nrnmpi_myid < 1) if (!nrn_nobanner_ && nrn_istty_) {
		printf("loading membrane mechanisms from %s\n", cp1);
	}
	dll = dll_load(cp1);
	if (dll) {
		Pfri mreg = dll_lookup(dll, "_modl_reg");
		if (mreg) {
			(*mreg)();
		}
		return 1;
	}
	return 0;
}
#endif

#if defined(WIN32) || defined(NRNMECH_DLL_STYLE)
int mswin_load_dll(const char* cp1) { /* actually linux dlopen */
	void* handle;
	if (nrnmpi_myid < 1) if (!nrn_nobanner_ && nrn_istty_) {
		printf("loading membrane mechanisms from %s\n", cp1);
	}
	handle = dlopen(cp1, RTLD_NOW);
	if (handle) {
		Pfrv mreg = (Pfrv)dlsym(handle, "modl_reg");
		if (mreg) {
			(*mreg)();
		}else{
			printf("dlsym _modl_reg failed\n%s\n", dlerror());
			dlclose(handle);
			return 0;
		}
		return 1;
	}else{
		printf("dlopen failed - \n%s\n", dlerror());
	}
	return 0;
}
#endif

#if !MAC
void hoc_nrn_load_dll(void) {
#if defined(WIN32) || defined(NRNMECH_DLL_STYLE)
	Symlist* sav;
	int i;
	FILE* f;
	const char* fn;
	fn = expand_env_var(gargstr(1));
	f = fopen(fn, "rb");
	if (f) {
		fclose(f);
		sav = hoc_symlist;
		hoc_symlist = hoc_built_in_symlist;
		hoc_built_in_symlist = (Symlist*)0;
		i = mswin_load_dll(fn);
		hoc_built_in_symlist = hoc_symlist;
		hoc_symlist = sav;
		hoc_retpushx((double)i);
	}else{
		hoc_retpushx(0.);
	}	
#else
	hoc_warning("nrn_load_dll not available on this machine", (char*)0);
	hoc_retpushx(0.);
#endif
}
#endif

extern void nrn_threads_create(int);

static DoubScal scdoub[] = {
	"t", &t,
	"dt", &dt,
	0,0
};

void hoc_last_init(void)
{
	int i;
	Pfrv *m;
	Symbol *s;

	hoc_register_var(scdoub, (DoubVec*)0, (VoidFunc*)0);
	nrn_threads_create(1);

 	if (nrnmpi_myid < 1) if (nrn_nobanner_ == 0) { 
	    Fprintf(stderr, "%s\n", nrn_version(1));
	    Fprintf(stderr, "%s\n", banner);
	    IGNORE(fflush(stderr));
 	} 
	memb_func_size_ = 30;
	memb_func = (Memb_func*)ecalloc(memb_func_size_, sizeof(Memb_func));
	memb_list = (Memb_list*)ecalloc(memb_func_size_, sizeof(Memb_list));
	pointsym = (Symbol**)ecalloc(memb_func_size_, sizeof(Symbol*));
	point_process = (Point_process**)ecalloc(memb_func_size_, sizeof(Point_process*));
	pnt_map = (char*)ecalloc(memb_func_size_, sizeof(char));
	memb_func[1].alloc = cab_alloc;
	nrn_pnt_template_ = (Template**)ecalloc(memb_func_size_, sizeof(Template*));
	pnt_receive = (pnt_receive_t*)ecalloc(memb_func_size_, sizeof(pnt_receive_t));
	pnt_receive_init = (pnt_receive_init_t*)ecalloc(memb_func_size_, sizeof(pnt_receive_init_t));
	pnt_receive_size = (short*)ecalloc(memb_func_size_, sizeof(short));
	nrn_is_artificial_ = (short*)ecalloc(memb_func_size_, sizeof(short));
	nrn_artcell_qindex_ = (short*)ecalloc(memb_func_size_, sizeof(short));
	nrn_prop_param_size_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_prop_dparam_size_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_dparam_ptr_start_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	nrn_dparam_ptr_end_ = (int*)ecalloc(memb_func_size_, sizeof(int));
	memb_order_ = (short*)ecalloc(memb_func_size_, sizeof(short));
	bamech_ = (BAMech**)ecalloc(BEFORE_AFTER_SIZE, sizeof(BAMech*));
	nrn_mk_prop_pools(memb_func_size_);
	
#if KEEP_NSEG_PARM
	{extern int keep_nseg_parm_; keep_nseg_parm_ = 1; }
#endif
#if FISHER
	/* get login_name from 'LOGNAME' */
	strcpy(login_name, getenv("LOGNAME"));
	
	if (getenv("CAT_PIPE")) {
	    pipe_filter = "cat";   /* preferred for emacs environment */
	} else {
	    pipe_filter = "more";  /* preferred for xterm environment */
	}
#endif

	section_list = hoc_l_newlist();
	
	CHECK("v");
	s = hoc_install("v", RANGEVAR, 0.0, &hoc_symlist);
	s->u.rng.type = VINDEX;

	CHECK("i_membrane_");
	s = hoc_install("i_membrane_", RANGEVAR, 0.0, &hoc_symlist);
	s->u.rng.type = IMEMFAST;
	
	for (i = 0; usrprop[i].name; i++) {
		CHECK(usrprop[i].name);
		s = hoc_install(usrprop[i].name, UNDEF, 0.0, &hoc_symlist);
		s->type = VAR;
		s->subtype = USERPROPERTY;
		s->u.rng.type = usrprop[i].type;
		s->u.rng.index = usrprop[i].index;
	}
	SectionList_reg();
	SectionRef_reg();
	register_mech(morph_mech, morph_alloc, (Pvmi)0, (Pvmi)0, (Pvmi)0, (Pvmi)0, -1, 0);
	for (m = mechanism; *m; m++) {
		(*m)();
	}
#if !MAC
	modl_reg();
#endif
	hoc_register_limits(0, _hoc_parm_limits);
	hoc_register_units(0, _hoc_parm_units);
#if defined(WIN32) || defined(NRNMECH_DLL_STYLE)
	if (!nrn_mech_dll) { /* use the default if it exists */
		FILE* ff = fopen(DLL_DEFAULT_FNAME, "r");
		if (ff) {
			fclose(ff);
			nrn_mech_dll = DLL_DEFAULT_FNAME;
		}
	}
	if (nrn_mech_dll) {
		char *cp1, *cp2;
		hoc_default_dll_loaded_ = 1.;
		for (cp1 = nrn_mech_dll; *cp1; cp1 = cp2) {
			for (cp2 = cp1; *cp2; ++cp2) {
				if (*cp2 == ';') {
					*cp2 = '\0';
					++cp2;
					break;
				}
			}
			mswin_load_dll(cp1);
		}
	}
#endif
	s = hoc_lookup("section_owner");
	s->type = OBJECTFUNC;
}

void initnrn(void) {
	secondorder = DEF_secondorder;	/* >0 means crank-nicolson. 2 means currents
				   adjusted to t+dt/2 */
	t = 0;		/* msec */
	dt = DEF_dt;	/* msec */
	clamp_resist = DEF_clamp_resist;	/*megohm*/
	celsius = DEF_celsius;	/* degrees celsius */
	hoc_retpushx(1.);
}

static int pointtype = 1; /* starts at 1 since 0 means not point in pnt_map*/
int n_memb_func;

/* if vectorized then thread_data_size added to it */
void nrn_register_mech_common(
	const char **m,
	Pvmp alloc,
	Pvmi cur,
	Pvmi jacob,
	Pvmi stat,
	Pvmi initialize,
	int nrnpointerindex, /* if -1 then there are none */
	int vectorized
){
	static int type = 2;	/* 0 unused, 1 for cable section */
	int j, k, modltype, pindx, modltypemax;
	Symbol *s;
	const char **m2;

	if (type >= memb_func_size_) {
		memb_func_size_ += 20;
		memb_func = (Memb_func*)erealloc(memb_func, memb_func_size_*sizeof(Memb_func));
		memb_list = (Memb_list*)erealloc(memb_list, memb_func_size_*sizeof(Memb_list));
		pointsym = (Symbol**)erealloc(pointsym, memb_func_size_*sizeof(Symbol*));
		point_process = (Point_process**)erealloc(point_process, memb_func_size_*sizeof(Point_process*));
		pnt_map = (char*)erealloc(pnt_map, memb_func_size_*sizeof(char));
		nrn_pnt_template_ = (Template**)erealloc(nrn_pnt_template_, memb_func_size_*sizeof(Template*));
		pnt_receive = (pnt_receive_t*)erealloc(pnt_receive, memb_func_size_*sizeof(pnt_receive_t));
		pnt_receive_init = (pnt_receive_init_t*)erealloc(pnt_receive_init, memb_func_size_*sizeof(pnt_receive_init_t));
		pnt_receive_size = (short*)erealloc(pnt_receive_size, memb_func_size_*sizeof(short));
		nrn_is_artificial_ = (short*)erealloc(nrn_is_artificial_, memb_func_size_*sizeof(short));
		nrn_artcell_qindex_ = (short*)erealloc(nrn_artcell_qindex_, memb_func_size_*sizeof(short));
		nrn_prop_param_size_ = (int*)erealloc(nrn_prop_param_size_, memb_func_size_*sizeof(int));
		nrn_prop_dparam_size_ = (int*)erealloc(nrn_prop_dparam_size_, memb_func_size_*sizeof(int));
		nrn_dparam_ptr_start_ = (int*)erealloc(nrn_dparam_ptr_start_, memb_func_size_*sizeof(int));
		nrn_dparam_ptr_end_ = (int*)erealloc(nrn_dparam_ptr_end_, memb_func_size_*sizeof(int));
		memb_order_ = (short*)erealloc(memb_order_, memb_func_size_*sizeof(short));
		for (j=memb_func_size_ - 20; j < memb_func_size_; ++j) {
			pnt_map[j] = 0;
			point_process[j] = (Point_process*)0;
			pointsym[j] = (Symbol*)0;
			nrn_pnt_template_[j] = (Template*)0;
			pnt_receive[j] = (pnt_receive_t)0;
			pnt_receive_init[j] = (pnt_receive_init_t)0;
			pnt_receive_size[j] = 0;
			nrn_is_artificial_[j] = 0;
			nrn_artcell_qindex_[j] = 0;
			memb_order_[j] = 0;
		}
		nrn_mk_prop_pools(memb_func_size_);
	}

	nrn_prop_param_size_[type] = 0; /* fill in later */
	nrn_prop_dparam_size_[type] = 0; /* fill in later */
	nrn_dparam_ptr_start_[type] = 0; /* fill in later */
	nrn_dparam_ptr_end_[type] = 0; /* fill in later */
	memb_func[type].current = cur;
	memb_func[type].jacob = jacob;
	memb_func[type].alloc = alloc;
	memb_func[type].state = stat;
	memb_func[type].initialize = initialize;
	memb_func[type].destructor = (void*)0;
#if VECTORIZE
	memb_func[type].vectorized = vectorized ? 1:0;
	memb_func[type].thread_size_ = vectorized ? (vectorized - 1) : 0;
	memb_func[type].thread_mem_init_ = (void*)0;
	memb_func[type].thread_cleanup_ = (void*)0;
	memb_func[type].thread_table_check_ = (void*)0;
	memb_func[type]._update_ion_pointers = (void*)0;
	memb_func[type].is_point = 0;
	memb_func[type].hoc_mech = (void*)0;
	memb_list[type].nodecount = 0;
	memb_list[type]._thread = (Datum*)0;
	memb_order_[type] = type;
#endif
#if CVODE
	memb_func[type].ode_count = (void*)0;
	memb_func[type].ode_map = (void*)0;
	memb_func[type].ode_spec = (void*)0;
	memb_func[type].ode_matsol = (void*)0;
	memb_func[type].ode_synonym = (void*)0;
	memb_func[type].singchan_ = (void*)0;
#endif
	/* as of 5.2 nmodl translates so that the version string
	   is the first string in m. This allows the neuron application
	   to determine if nmodl c files are compatible with this version
	   Note that internal mechanisms have a version of "0" and are
	   by nature consistent.
	*/
	
/*printf("%s %s\n", m[0], m[1]);*/
	if (strcmp(m[0], "0") == 0) { /* valid by nature */
	}else if (m[0][0] != '6') { /* must be 5.1 or before */
fprintf(stderr, "Mechanism %s needs to be re-translated.\n\
It's pre version 6.0 \"c\" code is incompatible with this neuron version.\n", m[0]);
		nrn_exit(1);
	}else if (strcmp(m[0], nmodl_version_) != 0){
fprintf(stderr, "Mechanism %s needs to be re-translated.\n\
It's version %s \"c\" code is incompatible with this neuron version.\n",
			m[1], m[0]);
		nrn_exit(1);
	}
	CHECK(m[1]);
	s = hoc_install(m[1], MECHANISM, 0.0, &hoc_symlist);
	s->subtype = type;
	memb_func[type].sym = s;
/*	printf("%s type=%d\n", s->name, type);*/
	m2 = m + 2;
	if (nrnpointerindex == -1) {
		modltypemax = STATE;
	} else {
		modltypemax = NRNPOINTER;
	}
	for (k=0, j=0, modltype=nrnocCONST; modltype<=modltypemax; modltype++, j++){
		/*EMPTY*/
		for (; m2[j]; j++, k++) {
			;
		}
	}
	s->s_varn = k;
	s->u.ppsym = (Symbol **) emalloc((unsigned)(j*sizeof(Symbol *)));
/* this is set up for the possiblility of overloading range variables.
We are currently not allowing this. Hence the #if.
If never overloaded then no reason for list of symbols for each mechanism.
*/
/* the indexing is confusing because k refers to index in the range indx list
and j refers to index in mechanism list which has 0 elements to separate
nrnocCONST, DEPENDENT, and STATE */
/* variable pointers added on at end, if they exist */
/* allowing range variable arrays. Must extract dimension info from name[%d]*/
/* pindx refers to index into the p-array */
	pindx = 0;
	for (j=0, k=0, modltype=nrnocCONST; modltype <= modltypemax; modltype++, j++) {
		for (; m2[j]; j++, k++) {
			Symbol *s2;
			char buf[200], *cp; int indx; unsigned nsub=0;
			strcpy(buf, m2[j]); /* not allowed to change constant string */
			indx = 1;
			cp = strchr(buf, '[');
			if (cp) {
#if EXTRACELLULAR
				if (cp[1] == 'N') {
					indx = nlayer;
				}else
#endif
				{
					sscanf(cp+1, "%d", &indx);
				}
				nsub = 1;
				*cp = '\0';
			}
			/*SUPPRESS 624*/
			if ((s2 = hoc_lookup(buf))) {
#if 0
				if (s2->subtype != RANGEVAR) {
					IGNORE(fprintf(stderr, CHKmes,
					buf));
				}
#else
IGNORE(fprintf(stderr, CHKmes, buf));
#endif
			}else{
			  s2 = hoc_install(buf, RANGEVAR, 0.0, &hoc_symlist);
				s2->subtype = modltype;
				s2->u.rng.type = type;
				s2->public = 1;
				if (modltype == NRNPOINTER) { /* not in p array */
					s2->u.rng.index = nrnpointerindex;
				} else {
					s2->u.rng.index = pindx;
				}
			  if (nsub) {
				s2->arayinfo = (Arrayinfo *) emalloc(
				  sizeof(Arrayinfo) + nsub * sizeof(int));
				s2->arayinfo->a_varn = (unsigned *)0;
				s2->arayinfo->refcount = 1;
				s2->arayinfo->nsub = nsub;
				s2->arayinfo->sub[0] = indx;
			  }
			  if (modltype == NRNPOINTER) {
				if (nrn_dparam_ptr_end_[type] == 0) {
					nrn_dparam_ptr_start_[type] = nrnpointerindex;
				}
				nrnpointerindex += indx;
				nrn_dparam_ptr_end_[type] = nrnpointerindex;
			  }else {
				pindx += indx;
			  }
			}
			s->u.ppsym[k] = s2;
		}
	}
	++type;
	n_memb_func = type;
}

void register_mech(
	const char **m,
	Pvmp alloc,
	Pvmi cur,
	Pvmi jacob,
	Pvmi stat,
	Pvmi initialize,
	int nrnpointerindex, /* if -1 then there are none */
	int vectorized
){
	int type = n_memb_func;
	nrn_register_mech_common(m, alloc, cur, jacob, stat, initialize,
		nrnpointerindex, vectorized);
	if (nrnpy_reg_mech_p_) {
		(*nrnpy_reg_mech_p_)(type);
	}
}

void nrn_writes_conc(int type, int unused) {
	static int lastion = EXTRACELL+1;
	int i;
	for (i=n_memb_func - 2; i >= lastion; --i) {
		memb_order_[i+1] = memb_order_[i];
	}
	memb_order_[lastion] = type;
#if 0
	printf("%s reordered from %d to %d\n", memb_func[type].sym->name, type, lastion);
#endif
	if (nrn_is_ion(type)) {
		++lastion;
	}
}

void hoc_register_prop_size(int type, int psize, int dpsize) {
	nrn_prop_param_size_[type] = psize;
	nrn_prop_dparam_size_[type] = dpsize;
}

#if CVODE
void hoc_register_cvode(
	int i,
	nrn_ode_count_t cnt,
	nrn_ode_map_t map,
	Pvmi spec,
	Pvmi matsol
){
	memb_func[i].ode_count = cnt;
	memb_func[i].ode_map = map;
	memb_func[i].ode_spec = spec;
	memb_func[i].ode_matsol = matsol;
}
void hoc_register_synonym(int i, void (*syn)(int, double**, Datum**)){
	memb_func[i].ode_synonym = syn;
}
#endif

void register_destructor(Pvmp d) {
	memb_func[n_memb_func - 1].destructor = d;
}

int point_reg_helper(Symbol* s2) {
	pointsym[pointtype] = s2;
	s2->public = 0;
	pnt_map[n_memb_func-1] = pointtype;
	memb_func[n_memb_func-1].is_point = 1;
	if (nrnpy_reg_mech_p_) {
		(*nrnpy_reg_mech_p_)(n_memb_func-1);
	}
	return pointtype++;
}

int point_register_mech(
	const char **m,
	Pvmp alloc,
	Pvmi cur,
	Pvmi jacob,
	Pvmi stat,
	Pvmi initialize,
	int nrnpointerindex,
	int vectorized,

	void* (*constructor)(Object*),
	void (*destructor)(void*),
	Member_func* fmember
){
	extern void steer_point_process();
	Symlist* sl;
	Symbol* s, *s2;
	void class2oc();
	CHECK(m[1]);
	class2oc(m[1], constructor, destructor, fmember, (void*)0, (void*)0, (void*)0);
	s = hoc_lookup(m[1]);
	sl = hoc_symlist;
	hoc_symlist = s->u.template->symtable;
	s->u.template->steer = steer_point_process;
	s->u.template->is_point_ = pointtype;
	nrn_register_mech_common(m, alloc, cur, jacob, stat, initialize,
		nrnpointerindex, vectorized);
	nrn_pnt_template_[n_memb_func-1] = s->u.template;
	s2 = hoc_lookup(m[1]);
	hoc_symlist = sl;
	return point_reg_helper(s2);
}

/* some stuff from scopmath needed for built-in models */
 
#if 0
double* makevector(int nrows)
{
        double* v;
        v = (double*)emalloc((unsigned)(nrows*sizeof(double)));
        return v;
}
#endif
  
int _ninits;
void _modl_cleanup(void){}

#if 1
void _modl_set_dt(double newdt) {
	dt = newdt;
	nrn_threads->_dt = newdt;
}
void _modl_set_dt_thread(double newdt, NrnThread* nt) {
	nt->_dt = newdt;
}
double _modl_get_dt_thread(NrnThread* nt) {
	return nt->_dt;
}
#endif	

int nrn_pointing(double* pd) {
	return pd ? 1 : 0;
}

int state_discon_flag_ = 0;
void state_discontinuity(int i, double* pd, double d) {
	if (state_discon_allowed_ && state_discon_flag_ == 0) {
		*pd = d;
/*printf("state_discontinuity t=%g pd=%lx d=%g\n", t, (long)pd, d);*/
	}
}

void hoc_register_limits(int type, HocParmLimits* limits)
{
	int i;
	Symbol* sym;
	for (i=0; limits[i].name; ++i) {
		sym = (Symbol*)0;
		if (type && memb_func[type].is_point) {
			Symbol* t;
			t = hoc_lookup(memb_func[type].sym->name);
			sym = hoc_table_lookup(
				limits[i].name,
				t->u.template->symtable
			);
		}
		if (!sym) {
			sym = hoc_lookup(limits[i].name);
		}
		hoc_symbol_limits(sym, limits[i].bnd[0], limits[i].bnd[1]);
	}
}

void hoc_register_units(int type, HocParmUnits* units)
{
	int i;
	Symbol* sym;
	for (i=0; units[i].name; ++i) {
		sym = (Symbol*)0;
		if (type && memb_func[type].is_point) {
			Symbol* t;
			t = hoc_lookup(memb_func[type].sym->name);
			sym = hoc_table_lookup(
				units[i].name,
				t->u.template->symtable
			);
		}
		if (!sym) {
			sym = hoc_lookup(units[i].name);
		}
		hoc_symbol_units(sym, units[i].units);
	}
}

void hoc_reg_ba(int mt, nrn_bamech_t f, int type)
{
	BAMech* bam;
	switch (type) { /* see bablk in src/nmodl/nocpout.c */
	case 11: type = BEFORE_BREAKPOINT; break;
	case 22: type = AFTER_SOLVE; break;
	case 13: type = BEFORE_INITIAL; break;
	case 23: type = AFTER_INITIAL; break;
	case 14: type = BEFORE_STEP; break;
	default:
printf("before-after processing type %d for %s not implemented\n", type, memb_func[mt].sym->name);
		nrn_exit(1);
	}
	bam = (BAMech*)emalloc(sizeof(BAMech));
	bam->f = f;
	bam->type = mt;
	bam->next = bamech_[type];
	bamech_[type] = bam;
}

void _cvode_abstol(Symbol** s, double* tol, int i)
{
#if CVODE
	if (s && s[i]->extra) {
		double x;
		x = s[i]->extra->tolerance;
		if (x != 0) {
			tol[i] *= x;
		}
	}
#endif
}

void hoc_register_tolerance(int type, HocStateTolerance* tol, Symbol*** stol)
{
#if CVODE
	int i;
	Symbol* sym;
/*printf("register tolerance for %s\n", memb_func[type].sym->name);*/
	for (i = 0; tol[i].name; ++i) {
		if (memb_func[type].is_point) {
			Symbol* t;
			t = hoc_lookup(memb_func[type].sym->name);
			sym = hoc_table_lookup(
				tol[i].name,
				t->u.template->symtable
			);
		}else{
			sym = hoc_lookup(tol[i].name);
		}
		hoc_symbol_tolerance(sym, tol[i].tolerance);
	}			

	if (memb_func[type].ode_count) {
		Symbol** psym, *msym, *vsym;
		double **pv;
		Node** pnode;
		Prop* p;
		extern Node** node_construct();
		int i, j, k, n, na, index;
		
		n = (*memb_func[type].ode_count)(type);
		if (n > 0) {
			psym = (Symbol**)ecalloc(n, sizeof(Symbol*));
			pv = (double**)ecalloc(2*n, sizeof(double*));
			pnode = node_construct(1);
prop_alloc(&(pnode[0]->prop), MORPHOLOGY, pnode[0]); /* in case we need diam */
p = prop_alloc(&(pnode[0]->prop), type, pnode[0]); /* this and any ions */
(*memb_func[type].ode_map)(0, pv, pv+n, p->param, p->dparam, (double*)0, type);
			for (i=0; i < n; ++i) {
				for (p = pnode[0]->prop; p; p = p->next) {
					if (pv[i] >= p->param && pv[i] < (p->param + p->param_size)) {
						index = pv[i] - p->param;
						break;
					}
				}
				
				/* p is the prop and index is the index
					into the p->param array */
				assert(p);
				/* need to find symbol for this */
				msym = memb_func[p->type].sym;
				for (j=0; j < msym->s_varn; ++j) {
					vsym = msym->u.ppsym[j];
					if (vsym->type == RANGEVAR && vsym->u.rng.index == index) {
						psym[i] = vsym;
/*printf("identified %s at index %d of %s\n", vsym->name, index, msym->name);*/
						if (ISARRAY(vsym)) {
							na = vsym->arayinfo->sub[0];
							for (k=1; k < na; ++k) {
								psym[++i] = vsym;
							}
						}
						break;
					} 
				}
				assert (j < msym->s_varn);
			}
					
			node_destruct(pnode, 1);
			*stol = psym;
			free (pv);
		}
	}
#endif
}

void _nrn_thread_reg(int i, int cons, void(*f)(Datum*)) {
	if (cons == 1) {
		memb_func[i].thread_mem_init_ = f;
	}else if (cons == 0) {
		memb_func[i].thread_cleanup_ = f;
	}else if (cons == 2) {
		memb_func[i]._update_ion_pointers = f;
	}
}

void _nrn_thread_table_reg(int i, void(*f)(double*, Datum*, Datum*, void*, int)) {
	memb_func[i].thread_table_check_ = f;
}

void _nrn_setdata_reg(int i, void(*call)(Prop*)) {
	memb_func[i].setdata_ = call;
}
/* there is some question about the _extcall_thread variables, if any. */
double nrn_call_mech_func(Symbol* s, int narg, Prop* p, int type) {
	double x;	
	extern double hoc_call_func(Symbol*, int);
	void (*call)(Prop*) = memb_func[type].setdata_;
	if (call) {
		(*call)(p);
	}
	x = hoc_call_func(s, narg);
	return x;
}
