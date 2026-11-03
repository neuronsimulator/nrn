#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/eion.cpp,v 1.10 1998/02/26 16:42:50 hines Exp */

#include <cstdlib>
#include "cabcode.h"
#include "section.h"
#include "neuron.h"
#include "neuron/cache/mechanism_range.hpp"
#include "membfunc.h"
#include "parse.hpp"
#include "membdef.h"
#include "nrniv_mf.h"
#include "nrnunits.h"

#include <array>
#include <string>

#include <map>
#include <set>

#undef hoc_retpushx


static constexpr auto nparm = 5;
static constexpr auto ndparam = 1;
static const char* mechanism[] = {/*just a template*/
                                  "0",
                                  "na_ion",
                                  "ena",
                                  "nao",
                                  "nai",
                                  0,
                                  "ina",
                                  "dina_dv_",
                                  0,
                                  0};
static DoubScal scdoub[] = {/* just a template*/
                            {"ci0_na_ion", 0},
                            {"co0_na_ion", 0},
                            {0, 0}};

static void ion_alloc(Prop*);

static void ion_cur(neuron::model_sorted_token const&, NrnThread*, Memb_list*, int);

static void ion_init(neuron::model_sorted_token const&, NrnThread*, Memb_list*, int);

static int na_ion, k_ion, ca_ion; /* will get type for these special ions */

int nrn_is_ion(int type) {
    return (memb_func[type].alloc == ion_alloc);
}

static int ion_global_map_size;
static double** ion_global_map;
#define global_conci(type)  ion_global_map[type][0]
#define global_conco(type)  ion_global_map[type][1]
#define global_charge(type) ion_global_map[type][2]

double nrn_ion_charge(Symbol* sym) {
    return global_charge(sym->subtype);
}

void ion_register(void) {
    /* hoc level registration of ion name. Return -1 if name already
    in use and not an ion;  and the mechanism subtype otherwise.
    */
    char* name;
    Symbol* s;
    Symlist* sav;
    int fail;
    fail = 0;
    sav = hoc_symlist;
    hoc_symlist = hoc_top_level_symlist;
    name = gargstr(1);
    auto const buf_size = strlen(name) + 10;
    char* const buf = static_cast<char*>(emalloc(buf_size));
    std::snprintf(buf, buf_size, "%s_ion", name);
    s = hoc_lookup(buf);
    if (s && s->type == MECHANISM && memb_func[s->subtype].alloc == ion_alloc) {
        hoc_symlist = sav;
        free(buf);
        if (*getarg(2) != global_charge(s->subtype)) {
            hoc_execerr_ext("%s already defined with charge %g, cannot redefine with charge %g",
                            s->name,
                            global_charge(s->subtype),
                            *getarg(2));
        }
        hoc_retpushx((double) s->subtype);
        return;
    }
    if (s) {
        fail = 1;
    }
    std::snprintf(buf, buf_size, "e%s", name);
    if (hoc_lookup(buf)) {
        fail = 1;
    }
    std::snprintf(buf, buf_size, "%si", name);
    if (hoc_lookup(buf)) {
        fail = 1;
    }
    std::snprintf(buf, buf_size, "%so", name);
    if (hoc_lookup(buf)) {
        fail = 1;
    }
    std::snprintf(buf, buf_size, "i%s", name);
    if (hoc_lookup(buf)) {
        fail = 1;
    }
    std::snprintf(buf, buf_size, "di%s_dv_", name);
    if (hoc_lookup(buf)) {
        fail = 1;
    }
    if (fail) {
        hoc_symlist = sav;
        free(buf);
        hoc_retpushx(-1.);
        return;
    }
    double charge = *getarg(2);
    hoc_symlist = hoc_built_in_symlist;
    if (strcmp(name, "ca") == 0 && charge != 2.0) {
        // In the very rare edge case that ca is not yet defined
        // define with charge 2.0
        ion_reg(name, 2.0);
        // and emit a recoverable error as above to avoid an exit in ion_reg
        free(buf);
        hoc_execerr_ext("ca_ion already defined with charge 2, cannot redefine with charge %g\n",
                        charge);
    }
    ion_reg(name, charge);
    hoc_symlist = sav;
    std::snprintf(buf, buf_size, "%s_ion", name);
    s = hoc_lookup(buf);
    hoc_retpushx((double) s->subtype);
    free(buf);
}

void ion_charge(void) {
    Symbol* s;
    s = hoc_lookup(gargstr(1));
    if (!s || s->type != MECHANISM || memb_func[s->subtype].alloc != ion_alloc) {
        hoc_execerror(gargstr(1), "is not an ion mechanism");
    }
    hoc_retpushx(global_charge(s->subtype));
}

static std::vector<double> naparmdflt{DEF_ena, DEF_nai, DEF_nao};
static std::vector<double> kparmdflt{DEF_ek, DEF_ki, DEF_ko};
static std::vector<double> caparmdflt{DEF_eca, DEF_cai, DEF_cao};
static std::vector<double> ionparmdflt{DEF_eion, DEF_ioni, DEF_iono};

void reg_parm_default(int mechtype, const std::string& name) {
    if (name == "na") {
        hoc_register_parm_default(mechtype, &naparmdflt);
    } else {
        hoc_register_parm_default(mechtype, &ionparmdflt);
    }
}

void ion_reg(const char* name, double valence) {
    int i, mechtype;
    Symbol* s;
    double val;
    std::array<std::string, 7> buf{};
    std::string name_s{name};
#define VAL_SENTINAL -10000.
    buf[0] = name_s + "_ion";
    buf[1] = "e" + name_s;
    buf[2] = name_s + "i";
    buf[3] = name_s + "o";
    buf[5] = "i" + name_s;
    buf[6] = "di" + name_s + "_dv_";
    for (i = 0; i < 7; i++) {
        mechanism[i + 1] = buf[i].c_str();
    }
    mechanism[5] = nullptr; /* buf[4] not used above */
    s = hoc_lookup(buf[0].c_str());
    if (!s || s->type != MECHANISM || memb_func[s->subtype].alloc != ion_alloc) {
        register_mech(mechanism, ion_alloc, ion_cur, nullptr, nullptr, ion_init, -1, 1);
        hoc_symbol_limits(hoc_lookup(buf[2].c_str()), 1e-12, 1e9);
        hoc_symbol_limits(hoc_lookup(buf[3].c_str()), 1e-12, 1e9);
        hoc_symbol_units(hoc_lookup(buf[1].c_str()), "mV");
        hoc_symbol_units(hoc_lookup(buf[2].c_str()), "mM");
        hoc_symbol_units(hoc_lookup(buf[3].c_str()), "mM");
        hoc_symbol_units(hoc_lookup(buf[5].c_str()), "mA/cm2");
        hoc_symbol_units(hoc_lookup(buf[6].c_str()), "S/cm2");
        s = hoc_lookup(buf[0].c_str());
        mechtype = nrn_get_mechtype(mechanism[1]);
        reg_parm_default(mechtype, name_s);
        using neuron::mechanism::field;
        neuron::mechanism::register_data_fields(mechtype,
                                                field<double>{buf[1]},  // erev
                                                field<double>{buf[2]},  // conci
                                                field<double>{buf[3]},  // conco
                                                field<double>{buf[5]},  // cur
                                                field<double>{buf[6]},  // dcurdv
                                                field<int>{"iontype", "iontype"});
        hoc_register_prop_size(mechtype, nparm, ndparam);
        hoc_register_dparam_semantics(mechtype, 0, "iontype");
        nrn_writes_conc(mechtype, 1);
        if (ion_global_map_size <= s->subtype) {
            ion_global_map_size = s->subtype + 1;
            ion_global_map = (double**) erealloc(ion_global_map,
                                                 sizeof(double*) * ion_global_map_size);
        }
        ion_global_map[s->subtype] = (double*) emalloc(3 * sizeof(double));
        buf[0] = name_s + "i0_" + s->name;
        scdoub[0].name = buf[0].c_str();
        scdoub[0].pdoub = ion_global_map[s->subtype];
        buf[1] = name_s + "o0_" + s->name;
        scdoub[1].name = buf[1].c_str();
        scdoub[1].pdoub = ion_global_map[s->subtype] + 1;
        hoc_register_var(scdoub, (DoubVec*) 0, (VoidFunc*) 0);
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
        fprintf(stderr,
                "%s ion charge defined differently in\n\
two USEION statements (%g and %g)\n",
                s->name,
                valence,
                global_charge(s->subtype));
        nrn_exit(1);
    } else if (valence == VAL_SENTINAL && val == VAL_SENTINAL) {
        /* Not defined now but could be defined by
           a subsequent ion_reg from another model.
                   The list of ion mechanisms will be checked
                   after all mod files have been dealt with to verify
                   that they all have a defined valence.
        */
    } else if (valence != VAL_SENTINAL) {
        global_charge(s->subtype) = valence;
    }
}

void nrn_verify_ion_charge_defined() {
    int i;
    for (i = 3; i < n_memb_func; ++i)
        if (nrn_is_ion(i)) {
            if (global_charge(i) == VAL_SENTINAL) {
                Symbol* s = memb_func[i].sym;
                Fprintf(stderr,
                        "%s USEION CHARGE (or VALENCE) must be defined in\n\
at least one model using this ion\n",
                        s->name);
                nrn_exit(1);
            }
        }
}

#define ktf (1000. * _gasconstant_codata2018 * (celsius + 273.15) / _faraday_codata2018)
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

void nrn_wrote_conc(Symbol* sym, double& erev, double ci, double co, int it) {
    if (it & 040) {
        erev = nrn_nernst(ci, co, nrn_ion_charge(sym));
    }
}

void nernst(void) {
    double val = 0.0;

    if (hoc_is_str_arg(1)) {
        Symbol* s = hoc_lookup(gargstr(1));
        if (s && ion_global_map[s->u.rng.type]) {
            Section* sec = chk_access();
            Symbol* ion = memb_func[s->u.rng.type].sym;
            double z = global_charge(s->u.rng.type);
            double x;
            if (ifarg(2)) {
                x = chkarg(2, 0., 1.);
            } else {
                x = .5;
            }
            auto ci = nrn_rangepointer(sec, ion->u.ppsym[1], x);
            auto co = nrn_rangepointer(sec, ion->u.ppsym[2], x);
            auto e = nrn_rangepointer(sec, ion->u.ppsym[0], x);
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

double nrn_ghk(double v, double ci, double co, double z) {
    double eco, eci, temp;
    temp = z * v / ktf;
    eco = co * efun(temp);
    eci = ci * efun(-temp);
    return (.001) * z * _faraday_codata2018 * (eci - eco);
}

void ghk(void) {
    double val = nrn_ghk(*getarg(1), *getarg(2), *getarg(3), *getarg(4));
    hoc_retpushx(val);
}

static constexpr auto iontype_index_dparam = 0;
static constexpr auto erev_index = 0; /* From Eion */
static constexpr auto conci_index = 1;
static constexpr auto conco_index = 2;
static constexpr auto cur_index = 3;
static constexpr auto dcurdv_index = 4;

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

#define charge global_charge(type)
#define conci0 global_conci(type)
#define conco0 global_conco(type)

double nrn_nernst_coef(int type) {
    /* for computing jacobian element dconc'/dconc */
    return ktf / charge;
}

/*
It is generally an error for two models to WRITE the same concentration.

nrn_check_conc_write checks that there's no write conflict; and warns if it
detects one. It also sets respective write flag in the style of the ion.

The argument `i` specifies which concentration is being written to. It's 0 for
exterior; and 1 for interior.
*/

// Each mechanism type index that writes a concentration has a set of ion
// type indices it writes to.
// ionconctype is coded as iontype*2 + i where i=1 for interior
// So number of distinct ion mechanisms is essentially unlimited, max_int/2.
static std::map<int, std::set<int>> mechtype2ionconctype;

static void add_mechtype2ionconctype(int mechtype, int iontype, int i) {
    auto& set_of_ionconctypes = mechtype2ionconctype[mechtype];
    set_of_ionconctypes.insert(2 * iontype + i);  // unique though inserting for each instance.
}

static bool mech_uses_ionconctype(int mechtype, int iontype, int i) {
    if (auto search = mechtype2ionconctype.find(mechtype); search != mechtype2ionconctype.end()) {
        auto& set_of_ionconctypes = search->second;
        return set_of_ionconctypes.count(2 * iontype + i) == 1;
    }
    return false;
}

void nrn_check_conc_write(Prop* pmech, Prop* pion, int i) {
    // Would be less redundant to generate the "database" at
    // mechanism registration time. But NMODL presently gives us this info
    // only at each mechanism instance allocation.
    add_mechtype2ionconctype(pmech->_type, pion->_type, i);

    Prop* p;
    int flag;
    if (i == 1) {
        flag = 0200;
    } else {
        flag = 0400;
    }

    auto ii = pion->dparam[iontype_index_dparam].get<int>();
    if (ii & flag) {
        // Is the possibility in fact actual. Unfortunately, uninserting the
        // mechanism that writes a concentration does not reset the flag bit.
        // So search the node property list for another mechanism that also
        // writes this ion concentration. (that is needed anyway to
        // fill out the warning message.)

        // the pion in the node property list is before mechanisms that use the ion
        for (p = pion->next; p; p = p->next) {
            if (p == pmech) {
                continue;
            }
            if (mech_uses_ionconctype(p->_type, pion->_type, i)) {
                char buf[300];
                Sprintf(buf,
                        "%.*s%c is being written at the same location by %s and %s",
                        (int) strlen(memb_func[pion->_type].sym->name) - 4,
                        memb_func[pion->_type].sym->name,
                        ((i == 1) ? 'i' : 'o'),
                        memb_func[pmech->_type].sym->name,
                        memb_func[p->_type].sym->name);
                hoc_warning(buf, (char*) 0);
            }
        }
    }
    ii |= flag;
    pion->dparam[iontype_index_dparam] = ii;
}

void ion_style(void) {
    Symbol* s;
    int istyle, i, oldstyle;
    Section* sec;
    Prop* p;

    s = hoc_lookup(gargstr(1));
    if (!s || s->type != MECHANISM || !nrn_is_ion(s->subtype)) {
        hoc_execerror(gargstr(1), " is not an ion");
    }

    sec = chk_access();
    p = nrn_mechanism(s->subtype, sec->pnode[0]);
    oldstyle = -1;
    if (p) {
        oldstyle = p->dparam[iontype_index_dparam].get<int>();
    }

    if (ifarg(2)) {
        istyle = (int) chkarg(2, 0., 3.);         /* c_style */
        istyle += 010 * (int) chkarg(3, 0., 3.);  /* e_style */
        istyle += 040 * (int) chkarg(4, 0., 1.);  /* einit */
        istyle += 0100 * (int) chkarg(5, 0., 1.); /* eadvance */
        istyle += 04 * (int) chkarg(6, 0., 1.);   /* cinit*/
        for (i = 0; i < sec->nnode; ++i) {
            p = nrn_mechanism(s->subtype, sec->pnode[i]);
            if (p) {
                auto ii = p->dparam[iontype_index_dparam].get<int>();
                ii &= (0200 + 0400);
                ii += istyle;
                p->dparam[iontype_index_dparam] = ii;
            }
        }
    }
    hoc_retpushx((double) oldstyle);
}

int nrn_vartype(const Symbol* sym) {
    int i;
    i = sym->subtype;
    if (i == _AMBIGUOUS) {
        Section* sec;
        Prop* p;
        sec = nrn_noerr_access();
        if (!sec) {
            return nrnocCONST;
        }
        p = nrn_mechanism(sym->u.rng.type, sec->pnode[0]);
        if (p) {
            auto it = p->dparam[iontype_index_dparam].get<int>();
            if (sym->u.rng.index == 0) { /* erev */
                i = (it & 030) >> 3;     /* unused, nrnocCONST, DEP, or STATE */
            } else {                     /* concentration */
                i = (it & 03);
            }
        }
    }
    return i;
}

/* the ion mechanism it flag  defines how _AMBIGUOUS is to be interpreted */
void nrn_promote(Prop* p, int conc, int rev) {
    int it = p->dparam[iontype_index_dparam].get<int>();
    int oldconc = (it & 03);
    int oldrev = (it & 030) >> 3;
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
    it &= ~0177; /* clear the bitmap */
    it += oldconc + 010 * oldrev;
    if (oldconc == 3) { /* if state then cinit */
        it += 4;
        if (oldrev == 2) { /* if not state (WRITE) then eadvance */
            it += 0100;
        }
    }
    if (oldconc > 0 && oldrev == 2) { /*einit*/
        it += 040;
    }
    p->dparam[iontype_index_dparam] = it;  // this sets iontype to 8
}

/*the bitmap is
  03  concentration unused, nrnocCONST, DEP, STATE
  04  initialize concentrations
 030  reversal potential unused, nrnocCONST, DEP, STATE
 040  initialize reversal potential
0100  calc reversal during fadvance
0200  ci being written by a model
0400  co being written by a model
*/

/* Must be called prior to any channels which update the currents */
static void ion_cur(neuron::model_sorted_token const& sorted_token,
                    NrnThread* nt,
                    Memb_list* ml,
                    int type) {
    neuron::cache::MechanismRange<nparm, ndparam> ml_cache{sorted_token, *nt, *ml, type};
    auto const count = ml->nodecount;
    /*printf("ion_cur %s\n", memb_func[type].sym->name);*/
    for (int i = 0; i < count; ++i) {
        ml_cache.fpfield<dcurdv_index>(i) = 0.0;
        ml_cache.fpfield<cur_index>(i) = 0.0;
        auto const iontype = ml->pdata[i][iontype_index_dparam].get<int>();
        if (iontype & 0100) {
            ml_cache.fpfield<erev_index>(i) = nrn_nernst(ml_cache.fpfield<conci_index>(i),
                                                         ml_cache.fpfield<conco_index>(i),
                                                         charge);
        }
    };
}

/* Must be called prior to other models which possibly also initialize
    concentrations based on their own states
*/
static void ion_init(neuron::model_sorted_token const& sorted_token,
                     NrnThread* nt,
                     Memb_list* ml,
                     int type) {
    int i;
    neuron::cache::MechanismRange<nparm, ndparam> ml_cache{sorted_token, *nt, *ml, type};
    int count = ml->nodecount;
    /*printf("ion_init %s\n", memb_func[type].sym->name);*/
    for (i = 0; i < count; ++i) {
        auto const iontype = ml->pdata[i][iontype_index_dparam].get<int>();
        if (iontype & 04) {
            ml_cache.fpfield<conci_index>(i) = conci0;
            ml_cache.fpfield<conco_index>(i) = conco0;
        }
    }
    for (i = 0; i < count; ++i) {
        auto const iontype = ml->pdata[i][iontype_index_dparam].get<int>();
        if (iontype & 040) {
            ml_cache.fpfield<erev_index>(i) = nrn_nernst(ml_cache.fpfield<conci_index>(i),
                                                         ml_cache.fpfield<conco_index>(i),
                                                         charge);
        }
    }
}

static void ion_alloc(Prop* p) {
    assert(p->param_size() == nparm);
    assert(p->param_num_vars() == nparm);
    p->param(cur_index) = 0.;
    p->param(dcurdv_index) = 0.;
    if (p->_type == na_ion) {
        p->param(erev_index) = naparmdflt[0];
        p->param(conci_index) = naparmdflt[1];
        p->param(conco_index) = naparmdflt[2];
    } else if (p->_type == k_ion) {
        p->param(erev_index) = kparmdflt[0];
        p->param(conci_index) = kparmdflt[1];
        p->param(conco_index) = kparmdflt[2];
    } else if (p->_type == ca_ion) {
        p->param(erev_index) = caparmdflt[0];
        p->param(conci_index) = caparmdflt[1];
        p->param(conco_index) = caparmdflt[2];
    } else {
        p->param(erev_index) = ionparmdflt[0];
        p->param(conci_index) = ionparmdflt[1];
        p->param(conco_index) = ionparmdflt[2];
    }
    p->dparam = nrn_prop_datum_alloc(p->_type, ndparam, p);
    p->dparam[iontype_index_dparam] = 0;
}

void second_order_cur(NrnThread* nt) {
    extern int secondorder;
    NrnThreadMembList* tml;
    Memb_list* ml;
    int i, i2;
    constexpr auto c = 3;
    constexpr auto dc = 4;
    if (secondorder == 2) {
        for (tml = nt->tml; tml; tml = tml->next)
            if (memb_func[tml->index].alloc == ion_alloc) {
                ml = tml->ml;
                i2 = ml->nodecount;
                for (i = 0; i < i2; ++i) {
                    ml->data(i, c) += ml->data(i, dc) * (NODERHS(ml->nodelist[i]));
                }
            }
    }
}
