#include "nrnconf.h"
#include "membdef.h"
#include <math.h>
#include <string.h>

extern void hoc_register_prop_size(int, int, int);

#define	nparm 5
static char *mechanism[] = { /*just a template*/
	"0",
	"na_ion",
	"ena", "nao", "nai", 0,
	"ina", "dina_dv_", 0,
	0
};
static double ci0_na_ion;
static double co0_na_ion;

static void ion_alloc(double*, Datum*, int);
static void ion_cur(NrnThread*, Memb_list*, int);
static void ion_init(NrnThread*, Memb_list*, int);

double nrn_nernst(), nrn_ghk();
static int na_ion, k_ion, ca_ion; /* will get type for these special ions */

int nrn_is_ion(int type) {
	return (memb_func[type].alloc == ion_alloc);
}

static int ion_global_map_size;
static double** ion_global_map;
#define global_conci(type) ion_global_map[type][0]
#define global_conco(type) ion_global_map[type][1]
#define global_charge(type) ion_global_map[type][2]

double nrn_ion_charge(int type) {
	return global_charge(type);
}

void ion_reg(char* name, double valence) {
	int i, mechtype;
	Symbol *s;
	char buf[7][50];
	double val;
#define VAL_SENTINAL -10000.

	Sprintf(buf[0], "%s_ion", name);
	Sprintf(buf[1], "e%s", name);
	Sprintf(buf[2], "%si", name);
	Sprintf(buf[3], "%so", name);
	Sprintf(buf[5], "i%s", name);
	Sprintf(buf[6], "di%s_dv_", name);
	for (i=0; i<7; i++) {
		mechanism[i+1] = buf[i];
	}
	mechanism[5] = (char *)0; /* buf[4] not used above */
	mechtype = nrn_get_mechtype(buf[0]);
	if (mechtype == 0) {
		register_mech(mechanism, ion_alloc, ion_cur, (mod_f_t)0, (mod_f_t)0, (mod_f_t)ion_init, -1, 1);
		mechtype = nrn_get_mechtype(mechanism[1]);
		hoc_register_prop_size(mechtype, nparm, 1 );
		nrn_writes_conc(mechtype, 1);
		if (ion_global_map_size <= mechtype) {
			ion_global_map_size = mechtype + 1;
			ion_global_map = (double**)erealloc(ion_global_map,
				sizeof(double*)*ion_global_map_size);
		}
		ion_global_map[mechtype] = (double*)emalloc(3*sizeof(double));
		Sprintf(buf[0], "%si0_%s", name, buf[0]);
#if 0
		scdoub[0].name = buf[0];
		scdoub[0].pdoub = ion_global_map[mechtype];		
#endif
		Sprintf(buf[1], "%so0_%s", name, buf[0]);
#if 0
		scdoub[1].name = buf[1];
		scdoub[1].pdoub = ion_global_map[mechtypepe] + 1;
		hoc_register_var(scdoub, (DoubVec*)0, (IntFunc*)0);
#endif
		if (strcmp("na", name) == 0) {
			na_ion = mechtype;
			global_conci(mechtype) = DEF_nai;
			global_conco(mechtype) = DEF_nao;
			global_charge(mechtype) = 1.;
		}else if (strcmp("k", name) == 0) {
			k_ion = mechtype;
			global_conci(mechtype) = DEF_ki;
			global_conco(mechtype) = DEF_ko;
			global_charge(mechtype) = 1.;
		}else if (strcmp("ca", name) == 0) {
			ca_ion = mechtype;
			global_conci(mechtype) = DEF_cai;
			global_conco(mechtype) = DEF_cao;
			global_charge(mechtype) = 2.;
		}else{
			global_conci(mechtype) = DEF_ioni;
			global_conco(mechtype) = DEF_iono;
			global_charge(mechtype) = VAL_SENTINAL;
		}			
#if 0 /* only for hoc? */
		for (i=0; i < 3; ++i) { /* used to be nrnocCONST */
			s->u.ppsym[i]->subtype = _AMBIGUOUS;
		}
#endif
	}
	val = global_charge(mechtype);
	if (valence != VAL_SENTINAL && val != VAL_SENTINAL && valence != val) {
		fprintf(stderr, "%s ion valence defined differently in\n\
two USEION statements (%g and %g)\n",
			buf[0], valence, global_charge(mechtype));
		nrn_exit(1);
	}else if (valence == VAL_SENTINAL && val == VAL_SENTINAL) {
		fprintf(stderr, "%s ion valence must be defined in\n\
the USEION statement of any model using this ion\n", buf[0]);
		nrn_exit(1);
	}else if (valence != VAL_SENTINAL) {
		global_charge(mechtype) = valence;
	}
}

#define FARADAY 96485.309
#define ktf (1000.*8.3134*(celsius + 273.15)/FARADAY)
double nrn_nernst(ci, co, z) double z, ci, co; {
/*printf("nrn_nernst %g %g %g\n", ci, co, z);*/
	if (z == 0) {
		return 0.;
	}
	if (ci <= 0.) {
		return 1e6;
	}else if (co <= 0.) {
		return -1e6;
	}else{
		return ktf/z*log(co/ci);
	}
}

void nrn_wrote_conc(int type, double* pe, int it) {
	if (it & 04) {
		pe[0] = nrn_nernst(pe[1], pe[2], nrn_ion_charge(type));
	}
}

static double efun(double x) {
	if (fabs(x) < 1e-4) {
		return 1. - x/2.;
	}else{
		return x/(exp(x) - 1);
	}
}

double nrn_ghk(double v, double ci, double co, double z) {
	double eco, eci, temp;
	temp = z*v/ktf;
	eco = co*efun(temp);
	eci = ci*efun(-temp);
	return (.001)*z*FARADAY*(eci - eco);
}

#if VECTORIZE
#define erev	pd[i][0]	/* From Eion */
#define conci	pd[i][1]
#define conco	pd[i][2]
#define cur	pd[i][3]
#define dcurdv	pd[i][4]

/*
 handle erev, conci, conc0 "in the right way" according to ion_style
 default. See nrn/lib/help/nrnoc.help.
ion_style("name_ion", [c_style, e_style, einit, eadvance, cinit])

 ica is assigned
 eca is parameter but if conc exists then eca is assigned
 if conc is nrnocCONST then eca calculated on finitialize
 if conc is STATE then eca calculated on fadvance and conc finitialize
 	with global nai0, nao0

 nernst(ci, co, charge) and ghk(v, ci, co, charge) available to hoc
 and models.
*/

#define iontype ppd[i][0]	/* how _AMBIGUOUS is to be handled */
/*the bitmap is
03	concentration unused, nrnocCONST, DEP, STATE
04	initialize concentrations
030	reversal potential unused, nrnocCONST, DEP, STATE
040	initialize reversal potential
0100	calc reversal during fadvance
0200	ci being written by a model
0400	co being written by a model
*/

#define charge global_charge(type)
#define conci0 global_conci(type)
#define conco0 global_conco(type)

double nrn_nernst_coef(type) int type; {
	/* for computing jacobian element dconc'/dconc */
	return ktf/charge;
}

/*
It is generally an error for two models to WRITE the same concentration
*/
#if 0 /* likely determined from nrncore file format */
void nrn_check_conc_write(p_ok, pion, i) Prop* p_ok, *pion; int i; {
	static long *chk_conc_, *ion_bit_, size_;
	Prop* p;
	int flag, j, k;
	if (i == 1) {
		flag = 0200;
	}else{
		flag = 0400;
	}
	/* an embarassing hack */
	/* up to 32 possible ions */
	/* continuously compute a bitmap that allows determination
		of which models WRITE which ion concentrations */
	if (n_memb_func > size_){
	    if (!chk_conc_) {
		chk_conc_ = (long*)ecalloc(2*n_memb_func, sizeof(long));
		ion_bit_ = (long*)ecalloc(n_memb_func, sizeof(long));
	    }else{
		chk_conc_ = (long*)erealloc(chk_conc_, 2*n_memb_func*sizeof(long));
		ion_bit_ = (long*)erealloc(ion_bit_, n_memb_func*sizeof(long));
		for (j = size_; j < n_memb_func; ++j) {
			chk_conc_[2*j] = 0;
			chk_conc_[2*j+1] = 0;
			ion_bit_[j] = 0;
		}
	    }
	    size_ = n_memb_func;
	}
	for (k=0, j=0; j < n_memb_func; ++j) {
		if (nrn_is_ion(j)) {
			ion_bit_[j] = (1<<k);
			++k;
			assert(k < sizeof(long)*8);
		}
	}

	chk_conc_[2*p_ok->type + i] |= ion_bit_[pion->type];
	if (pion->dparam[0].i & flag) {
		/* now comes the hard part. Is the possibility in fact actual.*/
		for (p = pion->next; p; p = p->next) {
			if (p == p_ok) {
				continue;
			}
			if (chk_conc_[2*p->type + i] & ion_bit_[pion->type]) {
				char buf[300];
sprintf(buf, "%.*s%c is being written at the same location by %s and %s",
				    (int)strlen(memb_func[pion->type].sym->name)-4,
					memb_func[pion->type].sym->name,
					((i == 1)? 'i' : 'o'),
					memb_func[p_ok->type].sym->name,
					memb_func[p->type].sym->name);
				hoc_warning(buf, (char*)0);
			}
		}
	}
	pion->dparam[0].i |= flag;
}
#endif

#if 0 /* hoc interface to specify the style. */
ion_style() {
	Symbol* s;
	int istyle, i, oldstyle;
	Section* sec;
	Prop* p, *nrn_mechanism();

	s = hoc_lookup(gargstr(1));
	if (!s || s->type != MECHANISM || !nrn_is_ion(mechtype)) {
		hoc_execerror(gargstr(1), " is not an ion");
	}

	sec = chk_access();
	p = nrn_mechanism(mechtype, sec->pnode[0]);
	oldstyle = -1;
	if (p) {
		oldstyle = p->dparam[0].i;
	}

   if (ifarg(2)) {
	istyle = (int)chkarg(2, 0., 3.); /* c_style */
	istyle += 010*(int)chkarg(3, 0., 3.); /* e_style */
	istyle += 040*(int)chkarg(4, 0., 1.); /* einit */
	istyle += 0100*(int)chkarg(5, 0., 1.); /* eadvance */
	istyle += 04*(int)chkarg(6, 0., 1.); /* cinit*/

#if 0	/* global effect */
	{
		int count;
		Datum** ppd;
		v_setup_vectors();
		count = memb_list[mechtype].nodecount;
		ppd = memb_list[mechtype].pdata;
		for (i=0; i < count; ++i) {
			iontype = (iontype&(0200+0400)) + istyle;
		}
	}
#else	/* currently accessed section */
	{
		for (i=0; i < sec->nnode; ++i) {
			p = nrn_mechanism(mechtype, sec->pnode[i]);
			if (p) {
				p->dparam[0].i &= (0200+0400);
				p->dparam[0].i += istyle;
			}
		}
	}
#endif
   }
	ret((double)oldstyle);
}
#endif

#if 0 /* unlikely to be used by nrnbbcore */
int nrn_vartype(sym) Symbol* sym; {
	int i;
	i = sym->subtype;
	if (i == _AMBIGUOUS) {
		Section* sec;
		Prop* p, *nrn_mechanism();
		sec = nrn_noerr_access();
		if (!sec) {
			return nrnocCONST;
		}
		p = nrn_mechanism(sym->u.rng.type, sec->pnode[0]);
		if (p) {
			int it = p->dparam[0].i;
			if (sym->u.rng.index == 0) { /* erev */
				i = (it & 030)>>3; /* unused, nrnocCONST, DEP, or STATE */
			}else{	/* concentration */
				i = (it & 03);
			}
		}
	}
	return i;
}
#endif

/* the ion mechanism it flag  defines how _AMBIGUOUS is to be interpreted */
void nrn_promote(Datum* dparam, int conc, int rev){
	int oldconc, oldrev;
	int* it = &dparam[0];
	oldconc = (*it & 03);
	oldrev = (*it & 030)>>3;
	/* precedence */
	if (oldconc < conc) {
		oldconc = conc;
	}
	if (oldrev < rev) {
		oldrev = rev;
	}
	/* promote type */
	if (oldconc > 0 && oldrev < 2) {
		oldrev = 2;
	}
	*it &= ~0177;	/* clear the bitmap */
	*it += oldconc + 010*oldrev;
	if (oldconc == 3) { /* if state then cinit */
		*it += 4;
		if (oldrev == 2) { /* if not state (WRITE) then eadvance */
			*it += 0100;
		}
	}
	if (oldconc > 0 && oldrev == 2) { /*einit*/
		*it += 040;
	}
}

/* Must be called prior to any channels which update the currents */
static void ion_cur(NrnThread* nt, Memb_list* ml, int type) {
	int count = ml->nodecount;
	double **pd = ml->data;
	Datum **ppd = ml->pdata;
	int i;
/*printf("ion_cur %s\n", memb_func[type].sym->name);*/
#if _CRAY
#pragma _CRI ivdep
#endif
	for (i=0; i < count; ++i) {
		dcurdv = 0.;
		cur = 0.;
		if (iontype & 0100) {
			erev = nrn_nernst(conci, conco, charge);
		}
	};
}

/* Must be called prior to other models which possibly also initialize
	concentrations based on their own states
*/
static void ion_init(NrnThread* nt, Memb_list* ml, int type) {
	int count = ml->nodecount;
	double **pd = ml->data;
	Datum **ppd = ml->pdata;
	int i;
/*printf("ion_init %s\n", memb_func[type].sym->name);*/
#if _CRAY
#pragma _CRI ivdep
#endif
	for (i=0; i < count; ++i) {
		if (iontype & 04) {
			conci = conci0;
			conco = conco0;
		}
	}
#if _CRAY
#pragma _CRI ivdep
#endif
	for (i=0; i < count; ++i) {
		if (iontype & 040) {
			erev = nrn_nernst(conci, conco, charge);
		}
	}
}

static void ion_alloc(double* data, Datum* pdata, int type) {
	double *pd[1];
	int i=0;
	pd[0] = data;	

	cur = 0.;
	dcurdv = 0.;
	if (type == na_ion) {
		erev = DEF_ena;
		conci = DEF_nai;
		conco = DEF_nao;
	}else if (type == k_ion) {
		erev = DEF_ek;
		conci = DEF_ki;
		conco = DEF_ko;
	}else if (type == ca_ion) {
		erev = DEF_eca;
		conci = DEF_cai;
		conco = DEF_cao;
	}else{
		erev = DEF_eion;
		conci = DEF_ioni;
		conco = DEF_iono;
	}
	
	pdata[0] = 0;
}

void second_order_cur(NrnThread* _nt) {
	extern int secondorder;
	NrnThreadMembList* tml;
	Memb_list* ml;
	int j, i, i2;
#define c 3
#define dc 4
  if (secondorder == 2) {
	for (tml = _nt->tml; tml; tml = tml->next) if (memb_func[tml->index].alloc == ion_alloc) {
		ml = tml->ml;
		i2 = ml->nodecount;
		for (i = 0; i < i2; ++i) {
			ml->data[i][c] += ml->data[i][dc]
			   * ( VEC_RHS(i) )
			;
		}
	}
   }
}

#endif
