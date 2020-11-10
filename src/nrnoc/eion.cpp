#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/eion.cpp,v 1.10 1998/02/26 16:42:50 hines Exp */

#include 	<stdlib.h>
#include	"section.h"
#include	"neuron.h"
#include	"membfunc.h"
#include	"parse.h"
#include	"membdef.h"
#include	"nrniv_mf.h"


#undef hoc_retpushx

extern double chkarg(int, double low, double high);

extern Section *nrn_noerr_access();

extern void hoc_register_prop_size(int, int, int);

#define    nparm 5
static const char *mechanism[] = { /*just a template*/
        "0",
        "na_ion",
        "ena", "nao", "nai", 0,
        "ina", "dina_dv_", 0,
        0
};
static DoubScal scdoub[] = { /* just a template*/
        "ci0_na_ion", 0,
        "co0_na_ion", 0,
        0, 0
};

static void ion_alloc(Prop *);

static void ion_cur(NrnThread *, Memb_list *, int);

static void ion_init(NrnThread *, Memb_list *, int);

extern "C" double nrn_nernst(double, double, double), nrn_ghk(double, double, double, double);

static int na_ion, k_ion, ca_ion; /* will get type for these special ions */

int nrn_is_ion(int type) {
    return (memb_func[type].alloc == ion_alloc);
}

static int ion_global_map_size;
static double **ion_global_map;
#define global_conci(type) ion_global_map[type][0]
#define global_conco(type) ion_global_map[type][1]
#define global_charge(type) ion_global_map[type][2]

double nrn_ion_charge(Symbol *sym) {
    return global_charge(sym->subtype);
}

void ion_register(void) {
    /* hoc level registration of ion name. Return -1 if name already
    in use and not an ion;	and the mechanism subtype otherwise.
    */
    char *name;
    char *buf;
    Symbol *s;
    Symlist *sav;
    int fail;
    fail = 0;
    sav = hoc_symlist;
    hoc_symlist = hoc_top_level_symlist;
    name = gargstr(1);
    buf = static_cast<char *>(emalloc(strlen(name) + 10));
    sprintf(buf, "%s_ion", name);
    s = hoc_lookup(buf);
    if (s && s->type == MECHANISM && memb_func[s->subtype].alloc == ion_alloc) {
        hoc_symlist = sav;
        free(buf);
        hoc_retpushx((double) s->subtype);
        return;
    }
    if (s) { fail = 1; }
    sprintf(buf, "e%s", name);
    if (hoc_lookup(buf)) { fail = 1; }
    sprintf(buf, "%si", name);
    if (hoc_lookup(buf)) { fail = 1; }
    sprintf(buf, "%so", name);
    if (hoc_lookup(buf)) { fail = 1; }
    sprintf(buf, "i%s", name);
    if (hoc_lookup(buf)) { fail = 1; }
    sprintf(buf, "di%s_dv_", name);
    if (hoc_lookup(buf)) { fail = 1; }
    if (fail) {
        hoc_symlist = sav;
        free(buf);
        hoc_retpushx(-1.);
        return;
    }
    hoc_symlist = hoc_built_in_symlist;
    ion_reg(name, *getarg(2));
    hoc_symlist = sav;
    sprintf(buf, "%s_ion", name);
    s = hoc_lookup(buf);
    hoc_retpushx((double) s->subtype);
    free(buf);
}

void ion_charge(void) {
    Symbol *s;
    s = hoc_lookup(gargstr(1));
    if (!s || s->type != MECHANISM || memb_func[s->subtype].alloc != ion_alloc) {
        hoc_execerror(gargstr(1), "is not an ion mechanism");
    }
    hoc_retpushx(global_charge(s->subtype));
}

extern "C" {
void register_mech(const char **, Pvmp, Pvmi, Pvmi, Pvmi, Pvmi, int, int);


void ion_reg(const char *name, double valence) {
    int i, mechtype;
    Symbol *s;
    char *buf[7];
    double val;
#define VAL_SENTINAL -10000.

    {
        int n = 2 * strlen(name) + 10; /*name used twice in initialization name */
        for (i = 0; i < 7; ++i) {
            buf[i] = static_cast<char *>(emalloc(n));
        }
    }
    Sprintf(buf[0], "%s_ion", name);
    Sprintf(buf[1], "e%s", name);
    Sprintf(buf[2], "%si", name);
    Sprintf(buf[3], "%so", name);
    Sprintf(buf[5], "i%s", name);
    Sprintf(buf[6], "di%s_dv_", name);
    for (i = 0; i < 7; i++) {
        mechanism[i + 1] = buf[i];
    }
    mechanism[5] = (char *) 0; /* buf[4] not used above */
    s = hoc_lookup(buf[0]);
    if (!s || s->type != MECHANISM
        || memb_func[s->subtype].alloc != ion_alloc) {
        register_mech(mechanism, ion_alloc, ion_cur, nullptr, nullptr, ion_init, -1, 1);
        hoc_symbol_limits(hoc_lookup(buf[2]), 1e-12, 1e9);
        hoc_symbol_limits(hoc_lookup(buf[3]), 1e-12, 1e9);
        hoc_symbol_units(hoc_lookup(buf[1]), "mV");
        hoc_symbol_units(hoc_lookup(buf[2]), "mM");
        hoc_symbol_units(hoc_lookup(buf[3]), "mM");
        hoc_symbol_units(hoc_lookup(buf[5]), "mA/cm2");
        hoc_symbol_units(hoc_lookup(buf[6]), "S/cm2");
        s = hoc_lookup(buf[0]);
        mechtype = nrn_get_mechtype(mechanism[1]);
        hoc_register_prop_size(mechtype, nparm, 1);
        hoc_register_dparam_semantics(mechtype, 0, "iontype");
        nrn_writes_conc(mechtype, 1);
        if (ion_global_map_size <= s->subtype) {
            ion_global_map_size = s->subtype + 1;
            ion_global_map = (double **) erealloc(ion_global_map,
                                                  sizeof(double *) * ion_global_map_size);
        }
        ion_global_map[s->subtype] = (double *) emalloc(3 * sizeof(double));
        Sprintf(buf[0], "%si0_%s", name, s->name);
        scdoub[0].name = buf[0];
        scdoub[0].pdoub = ion_global_map[s->subtype];
        Sprintf(buf[1], "%so0_%s", name, s->name);
        scdoub[1].name = buf[1];
        scdoub[1].pdoub = ion_global_map[s->subtype] + 1;
        hoc_register_var(scdoub, (DoubVec *) 0, (VoidFunc *) 0);
        hoc_symbol_units(hoc_lookup(scdoub[0].name), "mM");
        hoc_symbol_units(hoc_lookup(scdoub[1].name), "mM");
        if (strcmp("na", name) == 0) {
            na_ion = s->subtype;
            global_conci(s->subtype) = DEF_nai;
            global_conco(s->subtype) = DEF_nao;
            global_charge(s->subtype) = 1.;
        } else if (strcmp("k", name) == 0) {
            k_ion = s->subtype;
            global_conci(s->subtype) = DEF_ki;
            global_conco(s->subtype) = DEF_ko;
            global_charge(s->subtype) = 1.;
        } else if (strcmp("ca", name) == 0) {
            ca_ion = s->subtype;
            global_conci(s->subtype) = DEF_cai;
            global_conco(s->subtype) = DEF_cao;
            global_charge(s->subtype) = 2.;
        } else {
            global_conci(s->subtype) = DEF_ioni;
            global_conco(s->subtype) = DEF_iono;
            global_charge(s->subtype) = VAL_SENTINAL;
        }
        for (i = 0; i < 3; ++i) { /* used to be nrnocCONST */
            s->u.ppsym[i]->subtype = _AMBIGUOUS;
        }
    }
    val = global_charge(s->subtype);
    if (valence != VAL_SENTINAL && val != VAL_SENTINAL && valence != val) {
        fprintf(stderr, "%s ion charge defined differently in\n\
two USEION statements (%g and %g)\n",
                s->name, valence, global_charge(s->subtype));
        nrn_exit(1);
    } else if (valence == VAL_SENTINAL && val == VAL_SENTINAL) {
        /* Not defined now but could be defined by
           a subsequent ion_reg from another model.
                   The list of ion mechanisms will be checked
                   after all mod files have been dealt with to verify
                   that they all have a defined valence.
        */
#if 0
#endif
    } else if (valence != VAL_SENTINAL) {
        global_charge(s->subtype) = valence;
    }
    for (i = 0; i < 7; ++i) {
        free(buf[i]);
    }
}
} // extern "C"

void nrn_verify_ion_charge_defined() {
    int i;
    for (i = 3; i < n_memb_func; ++i)
        if (nrn_is_ion(i)) {
            if (global_charge(i) == VAL_SENTINAL) {
                Symbol *s = memb_func[i].sym;
                Fprintf(stderr, "%s USEION CHARGE (or VALENCE) must be defined in\n\
at least one model using this ion\n", s->name);
                nrn_exit(1);
            }
        }
}

#if defined(LegacyFR) && LegacyFR == 1
#define FARADAY 96485.309
#define gasconstant 8.3134
#else
#define FARADAY 96485.33289
#define gasconstant 8.3144598
#endif

#define ktf (1000.*gasconstant*(celsius + 273.15)/FARADAY)

double nrn_nernst(double ci, double co, double z) {
/*printf("nrn_nernst %g %g %g\n", ci, co, z);*/
    if (z == 0) {
        return 0.;
    }
    if (ci <= 0.) {
        return 1e6;
    } else if (co <= 0.) {
        return -1e6;
    } else {
        return ktf / z * log(co / ci);
    }
}

extern "C" void nrn_wrote_conc(Symbol *sym, double *pe, int it) {
    if (it & 040) {
        pe[0] = nrn_nernst(pe[1], pe[2], nrn_ion_charge(sym));
    }
}

void nernst(void) {
    double val = 0.0;

    if (hoc_is_str_arg(1)) {
        Symbol *s = hoc_lookup(gargstr(1));
        if (s && ion_global_map[s->u.rng.type]) {
            Section *sec = chk_access();
//			double* nrn_rangepointer();
            Symbol *ion = memb_func[s->u.rng.type].sym;
            double z = global_charge(s->u.rng.type);
            double *ci, *co, *e, x;
            if (ifarg(2)) {
                x = chkarg(2, 0., 1.);
            } else {
                x = .5;
            }
            ci = nrn_rangepointer(sec, ion->u.ppsym[1], x);
            co = nrn_rangepointer(sec, ion->u.ppsym[2], x);
            e = nrn_rangepointer(sec, ion->u.ppsym[0], x);
            switch (s->u.rng.index) {
                case 0:
                    val = nrn_nernst(*ci, *co, z);
                    hoc_retpushx(val);
                    return;
                case 1:
                    val = *co * exp(-z / ktf * *e);
                    hoc_retpushx(val);
                    return;
                case 2:
                    val = *ci * exp(z / ktf * *e);
                    hoc_retpushx(val);
                    return;
            }
        }
        hoc_execerror(gargstr(1), " not a reversal potential or concentration");
    } else {
        val = nrn_nernst(*getarg(1), *getarg(2), *getarg(3));
/*printf("nernst=%g\n", val);*/
    }
    hoc_retpushx(val);
    return;
}

static double efun(double x) {
    if (fabs(x) < 1e-4) {
        return 1. - x / 2.;
    } else {
        return x / (exp(x) - 1);
    }
}

extern "C" double nrn_ghk(double v, double ci, double co, double z) {
    double eco, eci, temp;
    temp = z * v / ktf;
    eco = co * efun(temp);
    eci = ci * efun(-temp);
    return (.001) * z * FARADAY * (eci - eco);
}

void ghk(void) {
    double val = nrn_ghk(*getarg(1), *getarg(2), *getarg(3), *getarg(4));
    hoc_retpushx(val);
}

#if VECTORIZE
#define erev    pd[i][0]    /* From Eion */
#define conci    pd[i][1]
#define conco    pd[i][2]
#define cur    pd[i][3]
#define dcurdv    pd[i][4]

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

#define iontype ppd[i][0].i    /* how _AMBIGUOUS is to be handled */
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

extern "C" double nrn_nernst_coef(int type) {
    /* for computing jacobian element dconc'/dconc */
    return ktf / charge;
}

/*
It is generally an error for two models to WRITE the same concentration
*/
extern "C" void nrn_check_conc_write(Prop *p_ok, Prop *pion, int i) {
    static long *chk_conc_, *ion_bit_, size_;
    Prop *p;
    int flag, j, k;
    if (i == 1) {
        flag = 0200;
    } else {
        flag = 0400;
    }
    /* an embarassing hack */
    /* up to 32 possible ions */
    /* continuously compute a bitmap that allows determination
        of which models WRITE which ion concentrations */
    if (n_memb_func > size_) {
        if (!chk_conc_) {
            chk_conc_ = (long *) ecalloc(2 * n_memb_func, sizeof(long));
            ion_bit_ = (long *) ecalloc(n_memb_func, sizeof(long));
        } else {
            chk_conc_ = (long *) erealloc(chk_conc_, 2 * n_memb_func * sizeof(long));
            ion_bit_ = (long *) erealloc(ion_bit_, n_memb_func * sizeof(long));
            for (j = size_; j < n_memb_func; ++j) {
                chk_conc_[2 * j] = 0;
                chk_conc_[2 * j + 1] = 0;
                ion_bit_[j] = 0;
            }
        }
        size_ = n_memb_func;
    }
    for (k = 0, j = 0; j < n_memb_func; ++j) {
        if (nrn_is_ion(j)) {
            ion_bit_[j] = (1 << k);
            ++k;
            assert(k < sizeof(long) * 8);
        }
    }

    chk_conc_[2 * p_ok->type + i] |= ion_bit_[pion->type];
    if (pion->dparam[0].i & flag) {
        /* now comes the hard part. Is the possibility in fact actual.*/
        for (p = pion->next; p; p = p->next) {
            if (p == p_ok) {
                continue;
            }
            if (chk_conc_[2 * p->type + i] & ion_bit_[pion->type]) {
                char buf[300];
                sprintf(buf, "%.*s%c is being written at the same location by %s and %s",
                        (int) strlen(memb_func[pion->type].sym->name) - 4,
                        memb_func[pion->type].sym->name,
                        ((i == 1) ? 'i' : 'o'),
                        memb_func[p_ok->type].sym->name,
                        memb_func[p->type].sym->name);
                hoc_warning(buf, (char *) 0);
            }
        }
    }
    pion->dparam[0].i |= flag;
}

void ion_style(void) {
    Symbol *s;
    int istyle, i, oldstyle;
    Section *sec;
    Prop *p; //, *nrn_mechanism();

    s = hoc_lookup(gargstr(1));
    if (!s || s->type != MECHANISM || !nrn_is_ion(s->subtype)) {
        hoc_execerror(gargstr(1), " is not an ion");
    }

    sec = chk_access();
    p = nrn_mechanism(s->subtype, sec->pnode[0]);
    oldstyle = -1;
    if (p) {
        oldstyle = p->dparam[0].i;
    }

    if (ifarg(2)) {
        istyle = (int) chkarg(2, 0., 3.); /* c_style */
        istyle += 010 * (int) chkarg(3, 0., 3.); /* e_style */
        istyle += 040 * (int) chkarg(4, 0., 1.); /* einit */
        istyle += 0100 * (int) chkarg(5, 0., 1.); /* eadvance */
        istyle += 04 * (int) chkarg(6, 0., 1.); /* cinit*/

#if 0    /* global effect */
        {
            int count;
            Datum** ppd;
            v_setup_vectors();
            count = memb_list[s->subtype].nodecount;
            ppd = memb_list[s->subtype].pdata;
            for (i=0; i < count; ++i) {
                iontype = (iontype&(0200+0400)) + istyle;
            }
        }
#else	/* currently accessed section */
        {
            for (i = 0; i < sec->nnode; ++i) {
                p = nrn_mechanism(s->subtype, sec->pnode[i]);
                if (p) {
                    p->dparam[0].i &= (0200 + 0400);
                    p->dparam[0].i += istyle;
                }
            }
        }
#endif
    }
    hoc_retpushx((double) oldstyle);
}

int nrn_vartype(Symbol *sym) {
    int i;
    i = sym->subtype;
    if (i == _AMBIGUOUS) {
        Section *sec;
        Prop *p; //, *nrn_mechanism();
        sec = nrn_noerr_access();
        if (!sec) {
            return nrnocCONST;
        }
        p = nrn_mechanism(sym->u.rng.type, sec->pnode[0]);
        if (p) {
            int it = p->dparam[0].i;
            if (sym->u.rng.index == 0) { /* erev */
                i = (it & 030) >> 3; /* unused, nrnocCONST, DEP, or STATE */
            } else {    /* concentration */
                i = (it & 03);
            }
        }
    }
    return i;
}

/* the ion mechanism it flag  defines how _AMBIGUOUS is to be interpreted */
extern "C" void nrn_promote(Prop *p, int conc, int rev) {
    int oldconc, oldrev;
    int *it = &p->dparam[0].i;
    oldconc = (*it & 03);
    oldrev = (*it & 030) >> 3;
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
    *it &= ~0177;    /* clear the bitmap */
    *it += oldconc + 010 * oldrev;
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
static void ion_cur(NrnThread *nt, Memb_list *ml, int type) {
    int count = ml->nodecount;
    Node **vnode = ml->nodelist;
    double **pd = ml->data;
    Datum **ppd = ml->pdata;
    int i;
/*printf("ion_cur %s\n", memb_func[type].sym->name);*/
#if _CRAY
#pragma _CRI ivdep
#endif
    for (i = 0; i < count; ++i) {
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
static void ion_init(NrnThread *nt, Memb_list *ml, int type) {
    int count = ml->nodecount;
    Node **vnode = ml->nodelist;
    double **pd = ml->data;
    Datum **ppd = ml->pdata;
    int i;
/*printf("ion_init %s\n", memb_func[type].sym->name);*/
#if _CRAY
#pragma _CRI ivdep
#endif
    for (i = 0; i < count; ++i) {
        if (iontype & 04) {
            conci = conci0;
            conco = conco0;
        }
    }
#if _CRAY
#pragma _CRI ivdep
#endif
    for (i = 0; i < count; ++i) {
        if (iontype & 040) {
            erev = nrn_nernst(conci, conco, charge);
        }
    }
}

static void ion_alloc(Prop *p) {
    double *pd[1];
    int i = 0;

    pd[0] = nrn_prop_data_alloc(p->type, nparm, p);
    p->param_size = nparm;

    cur = 0.;
    dcurdv = 0.;
    if (p->type == na_ion) {
        erev = DEF_ena;
        conci = DEF_nai;
        conco = DEF_nao;
    } else if (p->type == k_ion) {
        erev = DEF_ek;
        conci = DEF_ki;
        conco = DEF_ko;
    } else if (p->type == ca_ion) {
        erev = DEF_eca;
        conci = DEF_cai;
        conco = DEF_cao;
    } else {
        erev = DEF_eion;
        conci = DEF_ioni;
        conco = DEF_iono;
    }
    p->param = pd[0];

    p->dparam = nrn_prop_datum_alloc(p->type, 1, p);
    p->dparam->i = 0;
}

void second_order_cur(NrnThread *nt) {
    extern int secondorder;
    NrnThreadMembList *tml;
    Memb_list *ml;
    int j, i, i2;
#define c 3
#define dc 4
    if (secondorder == 2) {
        for (tml = nt->tml; tml; tml = tml->next)
            if (memb_func[tml->index].alloc == ion_alloc) {
                ml = tml->ml;
                i2 = ml->nodecount;
                for (i = 0; i < i2; ++i) {
                    ml->data[i][c] += ml->data[i][dc]
                                      * (NODERHS(ml->nodelist[i]));
                }
            }
    }
}

#endif

