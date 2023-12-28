#include <../../nrnconf.h>
#include <nrnmpiuse.h>
#include "nrn_ansi.h"
#include "nrncore_write/io/nrncore_io.h"
#include "oc_ansi.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "section.h"
#include "parse.hpp"
#include "nrniv_mf.h"
#include "cabvars.h"
#include "neuron.h"
#include "neuron/container/data_handle.hpp"
#include "membdef.h"
#include "multicore.h"
#include "nrnmpi.h"

#include <vector>
#include <unordered_map>

/* change this to correspond to the ../nmodl/nocpout nmodl_version_ string*/
static char nmodl_version_[] = "7.7.0";

static char banner[] =
    "Duke, Yale, and the BlueBrain Project -- Copyright 1984-2022\n\
See http://neuron.yale.edu/neuron/credits\n";

#if defined(WIN32) || defined(NRNMECH_DLL_STYLE)
extern const char* nrn_mech_dll;      /* declared in hoc_init.cpp so ivocmain.cpp can see it */
extern int nrn_noauto_dlopen_nrnmech; /* default 0 declared in hoc_init.cpp */
#endif                                // WIN32 or NRNMEHC_DLL_STYLE

#if defined(WIN32)
#undef DLL_DEFAULT_FNAME
#define DLL_DEFAULT_FNAME "nrnmech.dll"
#endif  // WIN32

#if defined(NRNMECH_DLL_STYLE)
#if defined(DARWIN)

#ifndef DLL_DEFAULT_FNAME
#define DLL_DEFAULT_FNAME "libnrnmech.dylib"
#endif  // DLL_DEFAULT_FNAME

// error message hint with regard to mismatched arch
void nrn_possible_mismatched_arch(const char* libname) {
    if (neuron::config::system_processor == "arm64") {
        // what arch are we running on
#if __arm64__
        const char* we_are{"arm64"};
#elif __x86_64__
        const char* we_are{"x86_64"};
#endif  // !__arm64__

        // what arch did we try to dlopen
        auto const cmd_size = strlen(libname) + 100;
        auto* cmd = new char[cmd_size];
        std::snprintf(cmd, cmd_size, "lipo -archs %s 2> /dev/null", libname);
        char libname_arch[20]{0};
        FILE* p = popen(cmd, "r");
        delete[] cmd;
        if (!p) {
            return;
        }
        fgets(libname_arch, 18, p);
        if (strlen(libname_arch) == 0) {
            return;
        }
        pclose(p);

        if (strstr(libname_arch, we_are) == NULL) {
            fprintf(stderr, "libnrniv.dylib running as %s\n", we_are);
            fprintf(stderr, "but %s can only run as %s\n", libname, libname_arch);
        }
    }
}

#else  // ! DARWIN

#ifndef DLL_DEFAULT_FNAME
#define DLL_DEFAULT_FNAME "./libnrnmech.so"
#endif  // DLL_DEFAULT_FNAME

#endif  // ! DARWIN
#endif  // NRNMECH_DLL_STYLE

#include "nrnwrap_dlfcn.h"

#define CHECK(name)                            \
    if (hoc_lookup(name) != (Symbol*) 0) {     \
        IGNORE(fprintf(stderr, CHKmes, name)); \
        nrn_exit(1);                           \
    }

static char CHKmes[] = "The user defined name, %s, already exists\n";

void (*nrnpy_reg_mech_p_)(int);

int secondorder = 0;
int state_discon_allowed_;
extern int nrn_nobanner_;
double t, dt, clamp_resist, celsius, htablemin, htablemax;
int nrn_netrec_state_adjust = 0;
int nrn_sparse_partrans = 0;
hoc_List* section_list;
int nrn_global_ncell = 0; /* used to be rootnodecount */
extern double hoc_default_dll_loaded_;
extern int nrn_istty_;
extern int nrn_nobanner_;

static HocParmLimits _hoc_parm_limits[] = {{"Ra", {1e-6, 1e9}},
                                           {"L", {1e-4, 1e20}},
                                           {"diam", {1e-9, 1e9}},
                                           {"cm", {0., 1e9}},
                                           {"rallbranch", {1., 1e9}},
                                           {"nseg", {1., 1e9}},
                                           {"celsius", {-273., 1e6}},
                                           {"dt", {1e-9, 1e15}},
                                           {nullptr, {0., 0.}}};

static HocParmUnits _hoc_parm_units[] = {{"Ra", "ohm-cm"},
                                         {"L", "um"},
                                         {"diam", "um"},
                                         {"cm", "uF/cm2"},
                                         {"celsius", "degC"},
                                         {"dt", "ms"},
                                         {"t", "ms"},
                                         {"v", "mV"},
                                         {"i_cap", "mA/cm2"},
                                         {nullptr, nullptr}};

extern Symlist* nrn_load_dll_called_;
extern int nrn_load_dll_recover_error();
extern void nrn_load_name_check(const char* name);
static int memb_func_size_;
std::vector<Memb_func> memb_func;
std::vector<Memb_list> memb_list;
short* memb_order_;
Symbol** pointsym;
Point_process** point_process;
char* pnt_map; /* so prop_free can know its a point mech*/
BAMech** bamech_;

cTemplate** nrn_pnt_template_; /* for finding artificial cells */
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
NrnWatchAllocateFunc_t* nrn_watch_allocate_;
std::unordered_map<int, void (*)(Prop*)> nrn_mech_inst_destruct;

void hoc_reg_watch_allocate(int type, NrnWatchAllocateFunc_t waf) {
    nrn_watch_allocate_[type] = waf;
}

bbcore_write_t* nrn_bbcore_write_;
bbcore_write_t* nrn_bbcore_read_;

void hoc_reg_bbcore_write(int mechtype, bbcore_write_t f) {
    nrn_bbcore_write_[mechtype] = f;
}

void hoc_reg_bbcore_read(int mechtype, bbcore_write_t f) {
    nrn_bbcore_read_[mechtype] = f;
}

const char** nrn_nmodl_text_;
void hoc_reg_nmodl_text(int mechtype, const char* txt) {
    nrn_nmodl_text_[mechtype] = txt;
}

const char** nrn_nmodl_filename_;
void hoc_reg_nmodl_filename(int mechtype, const char* filename) {
    nrn_nmodl_filename_[mechtype] = filename;
}

void add_nrn_has_net_event(int mechtype) {
    ++nrn_has_net_event_cnt_;
    nrn_has_net_event_ = (int*) erealloc(nrn_has_net_event_, nrn_has_net_event_cnt_ * sizeof(int));
    nrn_has_net_event_[nrn_has_net_event_cnt_ - 1] = mechtype;
}

/* values are type numbers of mechanisms which have FOR_NETCONS statement */
int nrn_fornetcon_cnt_;    /* how many models have a FOR_NETCONS statement */
int* nrn_fornetcon_type_;  /* what are the type numbers */
int* nrn_fornetcon_index_; /* what is the index into the ppvar array */

void add_nrn_fornetcons(int type, int indx) {
    int i = nrn_fornetcon_cnt_++;
    nrn_fornetcon_type_ = (int*) erealloc(nrn_fornetcon_type_, (i + 1) * sizeof(int));
    nrn_fornetcon_index_ = (int*) erealloc(nrn_fornetcon_index_, (i + 1) * sizeof(int));
    nrn_fornetcon_type_[i] = type;
    nrn_fornetcon_index_[i] = indx;
}

/* array is parallel to memb_func. All are 0 except 1 for ARTIFICIAL_CELL */
short* nrn_is_artificial_;
short* nrn_artcell_qindex_;

void add_nrn_artcell(int mechtype, int qi) {
    nrn_is_artificial_[mechtype] = 1;
    nrn_artcell_qindex_[mechtype] = qi;
}

int nrn_is_artificial(int pnttype) {
    return (int) nrn_is_artificial_[pointsym[pnttype]->subtype];
}

int nrn_is_cable(void) {
    return 1;
}

void* nrn_realpath_dlopen(const char* relpath, int flags) {
    char* abspath = NULL;
    void* handle = NULL;

    /* use realpath or _fullpath even if is already a full path */

#if defined(HAVE_REALPATH)
    abspath = realpath(relpath, NULL);
#else /* not HAVE_REALPATH */
#if defined(__MINGW32__)
    abspath = _fullpath(NULL, relpath, 0);
#else  /* not __MINGW32__ */
    abspath = strdup(relpath);
#endif /* not __MINGW32__ */
#endif /* not HAVE_REALPATH */
    if (abspath) {
        handle = dlopen(abspath, flags);
#if DARWIN
        if (!handle) {
            nrn_possible_mismatched_arch(abspath);
        }
#endif  // DARWIN
        free(abspath);
    } else {
        int patherr = errno;
        handle = dlopen(relpath, flags);
        if (!handle) {
            Fprintf(stderr,
                    "realpath failed errno=%d (%s) and dlopen failed with %s\n",
                    patherr,
                    strerror(patherr),
                    relpath);
#if DARWIN
            nrn_possible_mismatched_arch(abspath);
#endif  // DARWIN
        }
    }
    return handle;
}

int mswin_load_dll(const char* cp1) {
    void* handle;
    if (nrnmpi_myid < 1)
        if (!nrn_nobanner_ && nrn_istty_) {
            fprintf(stderr, "loading membrane mechanisms from %s\n", cp1);
        }
#if DARWIN
    handle = nrn_realpath_dlopen(cp1, RTLD_NOW);
#else   // not DARWIN
    handle = dlopen(cp1, RTLD_NOW);
#endif  // not DARWIN
    if (handle) {
        Pfrv mreg = (Pfrv) dlsym(handle, "modl_reg");
        if (mreg) {
            (*mreg)();
        } else {
            fprintf(stderr, "dlsym modl_reg failed\n%s\n", dlerror());
            dlclose(handle);
            return 0;
        }
        return 1;
    } else {
        fprintf(stderr, "dlopen failed - \n%s\n", dlerror());
    }
    return 0;
}

void hoc_nrn_load_dll(void) {
    int i;
    FILE* f;
    const char* fn;
    fn = expand_env_var(gargstr(1));
    f = fopen(fn, "rb");
    if (f) {
        fclose(f);
        nrn_load_dll_called_ = hoc_symlist;
        hoc_symlist = hoc_built_in_symlist;
        hoc_built_in_symlist = (Symlist*) 0;
        /* If hoc_execerror, recover before that call */
        i = mswin_load_dll(fn);
        hoc_built_in_symlist = hoc_symlist;
        hoc_symlist = nrn_load_dll_called_;
        nrn_load_dll_called_ = (Symlist*) 0;
        hoc_retpushx((double) i);
    } else {
        hoc_retpushx(0.);
    }
}

static DoubScal scdoub[] = {{"t", &t}, {"dt", &dt}, {nullptr, nullptr}};

void hoc_last_init(void) {
    int i;
    Pfrv* m;
    Symbol* s;

    hoc_register_var(scdoub, (DoubVec*) 0, (VoidFunc*) 0);
    nrn_threads_create(1, false);  // single thread

    if (nrnmpi_myid < 1)
        if (nrn_nobanner_ == 0) {
            Fprintf(stderr, "%s\n", nrn_version(1));
            Fprintf(stderr, "%s\n", banner);
            IGNORE(fflush(stderr));
        }
    memb_func_size_ = 30;  // initial allocation size
    memb_list.reserve(memb_func_size_);
    memb_func.resize(memb_func_size_);  // we directly resize because it is used below
    pointsym = (Symbol**) ecalloc(memb_func_size_, sizeof(Symbol*));
    point_process = (Point_process**) ecalloc(memb_func_size_, sizeof(Point_process*));
    pnt_map = static_cast<char*>(ecalloc(memb_func_size_, sizeof(char)));
    memb_func[1].alloc = cab_alloc;
    nrn_pnt_template_ = (cTemplate**) ecalloc(memb_func_size_, sizeof(cTemplate*));
    pnt_receive = (pnt_receive_t*) ecalloc(memb_func_size_, sizeof(pnt_receive_t));
    pnt_receive_init = (pnt_receive_init_t*) ecalloc(memb_func_size_, sizeof(pnt_receive_init_t));
    pnt_receive_size = (short*) ecalloc(memb_func_size_, sizeof(short));
    nrn_is_artificial_ = (short*) ecalloc(memb_func_size_, sizeof(short));
    nrn_artcell_qindex_ = (short*) ecalloc(memb_func_size_, sizeof(short));
    nrn_prop_param_size_ = (int*) ecalloc(memb_func_size_, sizeof(int));
    nrn_prop_dparam_size_ = (int*) ecalloc(memb_func_size_, sizeof(int));
    nrn_dparam_ptr_start_ = (int*) ecalloc(memb_func_size_, sizeof(int));
    nrn_dparam_ptr_end_ = (int*) ecalloc(memb_func_size_, sizeof(int));
    memb_order_ = (short*) ecalloc(memb_func_size_, sizeof(short));
    bamech_ = (BAMech**) ecalloc(BEFORE_AFTER_SIZE, sizeof(BAMech*));
    nrn_mk_prop_pools(memb_func_size_);
    nrn_bbcore_write_ = (bbcore_write_t*) ecalloc(memb_func_size_, sizeof(bbcore_write_t));
    nrn_bbcore_read_ = (bbcore_write_t*) ecalloc(memb_func_size_, sizeof(bbcore_write_t));
    nrn_nmodl_text_ = (const char**) ecalloc(memb_func_size_, sizeof(const char*));
    nrn_nmodl_filename_ = (const char**) ecalloc(memb_func_size_, sizeof(const char*));
    nrn_watch_allocate_ = (NrnWatchAllocateFunc_t*) ecalloc(memb_func_size_,
                                                            sizeof(NrnWatchAllocateFunc_t));

#if KEEP_NSEG_PARM
    {
        extern int keep_nseg_parm_;
        keep_nseg_parm_ = 1;
    }
#endif  // KEEP_NSEG_PARM

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
    register_mech(morph_mech, morph_alloc, nullptr, nullptr, nullptr, nullptr, -1, 0);
    neuron::mechanism::register_data_fields(MORPHOLOGY, neuron::mechanism::field<double>{"diam"});
    hoc_register_prop_size(MORPHOLOGY, 1, 0);
    for (m = mechanism; *m; m++) {
        (*m)();
    }
#if !defined(WIN32)
    modl_reg();
#endif  // not WIN32
    hoc_register_limits(0, _hoc_parm_limits);
    hoc_register_units(0, _hoc_parm_units);
#if defined(WIN32) || defined(NRNMECH_DLL_STYLE)
    /* use the default if it exists (and not a binary special) */
    if (!nrn_mech_dll && !nrn_noauto_dlopen_nrnmech) {
        FILE* ff = fopen(DLL_DEFAULT_FNAME, "r");
        if (ff) {
            fclose(ff);
            nrn_mech_dll = DLL_DEFAULT_FNAME;
        }
    }
    if (nrn_mech_dll) {
        hoc_default_dll_loaded_ = 1.;
#if defined(WIN32)
        /* Sometimes (windows 10 and launch recent enthought canopy) it seems that
        mswin_load_dll fails if the filename is not a full path to nrnmech.dll
        */
        if (strcmp(nrn_mech_dll, "nrnmech.dll") == 0) {
            char buf[5100];
            char* retval = getcwd(buf, 4096);
            if (retval) {
                strncat(buf, "\\", 100);
                strncat(buf, nrn_mech_dll, 100);
                mswin_load_dll(buf);
            }
        } else {
#endif /*WIN32*/
            char *cp1{}, *cp2{};
            std::string tmp{nrn_mech_dll};
            for (cp1 = tmp.data(); *cp1; cp1 = cp2) {
                for (cp2 = cp1; *cp2; ++cp2) {
                    if (*cp2 == ';') {
                        *cp2 = '\0';
                        ++cp2;
                        break;
                    }
                }
                mswin_load_dll(cp1);
            }
#if defined(WIN32)
        }
#endif /*WIN32*/
    }
#endif /* WIN32 || NRNMECH_DLL_STYLE */
    s = hoc_lookup("section_owner");
    s->type = OBJECTFUNC;

    /* verify that all ions have a defined CHARGE */
    nrn_verify_ion_charge_defined();
}

void initnrn(void) {
    secondorder = DEF_secondorder;   /* >0 means crank-nicolson. 2 means currents
                    adjusted to t+dt/2 */
    t = 0;                           /* msec */
    dt = DEF_dt;                     /* msec */
    clamp_resist = DEF_clamp_resist; /*megohm*/
    celsius = DEF_celsius;           /* degrees celsius */
    hoc_retpushx(1.);
}

static int pointtype = 1; /* starts at 1 since 0 means not point in pnt_map*/
int n_memb_func;


void reallocate_mech_data(int mechtype);
void initialize_memb_func(int mechtype,
                          nrn_cur_t cur,
                          nrn_jacob_t jacob,
                          Pvmp alloc,
                          nrn_state_t stat,
                          nrn_init_t initialize,
                          int vectorized);
void check_mech_version(const char** m);
int count_variables_in_mechanism(const char** m2, int modltypemax);
void register_mech_vars(const char** var_buffers,
                        int modltypemax,
                        Symbol* mech_symbol,
                        int mechtype,
                        int nrnpointerindex);

/* if vectorized then thread_data_size added to it */
void nrn_register_mech_common(const char** m,
                              Pvmp alloc,
                              nrn_cur_t cur,
                              nrn_jacob_t jacob,
                              nrn_state_t stat,
                              nrn_init_t initialize,
                              int nrnpointerindex, /* if -1 then there are none */
                              int vectorized) {
    // initialize at first entry, it will be incremented at exit of the function
    static int mechtype = 2; /* 0 unused, 1 for cable section */
    int modltype;
    int modltypemax;
    Symbol* mech_symbol;
    const char** m2;

    nrn_load_name_check(m[1]);

    reallocate_mech_data(mechtype);

    initialize_memb_func(mechtype, cur, jacob, alloc, stat, initialize, vectorized);

    check_mech_version(m);

    mech_symbol = hoc_install(m[1], MECHANISM, 0.0, &hoc_symlist);
    mech_symbol->subtype = mechtype;
    memb_func[mechtype].sym = mech_symbol;
    m2 = m + 2;
    if (nrnpointerindex == -1) {
        modltypemax = STATE;
    } else {
        modltypemax = NRNPOINTER;
    }
    int nvars = count_variables_in_mechanism(m2, modltypemax);
    mech_symbol->s_varn = nvars;
    mech_symbol->u.ppsym = (Symbol**) emalloc((unsigned) (nvars * sizeof(Symbol*)));

    register_mech_vars(m2, modltypemax, mech_symbol, mechtype, nrnpointerindex);
    ++mechtype;
    n_memb_func = mechtype;
    // n_memb_func has changed, so any existing NrnThread do not know about the
    // new mechanism
    v_structure_change = 1;
}

void register_mech_vars(const char** var_buffers,
                        int modltypemax,
                        Symbol* mech_symbol,
                        int mechtype,
                        int nrnpointerindex) {
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
    int pindx = 0;
    int modltype;
    int j, k;
    for (j = 0, k = 0, modltype = nrnocCONST; modltype <= modltypemax; modltype++, j++) {
        for (; var_buffers[j]; j++, k++) {
            Symbol* var_symbol;
            std::string varname(var_buffers[j]);  // copy out the varname to allow modifying it
            int index = 1;
            unsigned nsub = 0;
            auto subscript = varname.find('[');
            if (subscript != varname.npos) {
#if EXTRACELLULAR
                if (varname[subscript + 1] == 'N') {
                    index = nlayer;
                } else
#endif  // EXTRACELLULAR
                {
                    index = std::stoi(varname.substr(subscript + 1));
                }
                nsub = 1;
                varname.erase(subscript);
            }
            if ((var_symbol = hoc_lookup(varname.c_str()))) {
                IGNORE(fprintf(stderr, CHKmes, varname.c_str()));
            } else {
                var_symbol = hoc_install(varname.c_str(), RANGEVAR, 0.0, &hoc_symlist);
                var_symbol->subtype = modltype;
                var_symbol->u.rng.type = mechtype;
                var_symbol->cpublic = 1;
                if (modltype == NRNPOINTER) { /* not in p array */
                    var_symbol->u.rng.index = nrnpointerindex;
                } else {
                    var_symbol->u.rng.index = pindx;
                }
                if (nsub) {
                    var_symbol->arayinfo = (Arrayinfo*) emalloc(sizeof(Arrayinfo) +
                                                                nsub * sizeof(int));
                    var_symbol->arayinfo->a_varn = nullptr;
                    var_symbol->arayinfo->refcount = 1;
                    var_symbol->arayinfo->nsub = nsub;
                    var_symbol->arayinfo->sub[0] = index;
                }
                if (modltype == NRNPOINTER) {
                    if (nrn_dparam_ptr_end_[mechtype] == 0) {
                        nrn_dparam_ptr_start_[mechtype] = nrnpointerindex;
                    }
                    nrnpointerindex += index;
                    nrn_dparam_ptr_end_[mechtype] = nrnpointerindex;
                } else {
                    pindx += index;
                }
            }
            mech_symbol->u.ppsym[k] = var_symbol;
        }
    }
}

int count_variables_in_mechanism(const char** m2, int modltypemax) {
    int j;
    int modltype;
    int nvars;
    // count the number of variables registered in this mechanism
    for (j = 0, nvars = 0, modltype = nrnocCONST; modltype <= modltypemax; modltype++) {
        // while we have not encountered a 0 (sentinel for variable type)
        while (m2[j++]) {
            nvars++;
        }
    }
    return nvars;
}

void reallocate_mech_data(int mechtype) {
    if (mechtype >= memb_func_size_) {
        memb_func_size_ += 20;
        pointsym = (Symbol**) erealloc(pointsym, memb_func_size_ * sizeof(Symbol*));
        point_process = (Point_process**) erealloc(point_process,
                                                   memb_func_size_ * sizeof(Point_process*));
        pnt_map = static_cast<char*>(erealloc(pnt_map, memb_func_size_ * sizeof(char)));
        nrn_pnt_template_ = (cTemplate**) erealloc(nrn_pnt_template_,
                                                   memb_func_size_ * sizeof(cTemplate*));
        pnt_receive = (pnt_receive_t*) erealloc(pnt_receive,
                                                memb_func_size_ * sizeof(pnt_receive_t));
        pnt_receive_init = (pnt_receive_init_t*) erealloc(pnt_receive_init,
                                                          memb_func_size_ *
                                                              sizeof(pnt_receive_init_t));
        pnt_receive_size = (short*) erealloc(pnt_receive_size, memb_func_size_ * sizeof(short));
        nrn_is_artificial_ = (short*) erealloc(nrn_is_artificial_, memb_func_size_ * sizeof(short));
        nrn_artcell_qindex_ = (short*) erealloc(nrn_artcell_qindex_,
                                                memb_func_size_ * sizeof(short));
        nrn_prop_param_size_ = (int*) erealloc(nrn_prop_param_size_, memb_func_size_ * sizeof(int));
        nrn_prop_dparam_size_ = (int*) erealloc(nrn_prop_dparam_size_,
                                                memb_func_size_ * sizeof(int));
        nrn_dparam_ptr_start_ = (int*) erealloc(nrn_dparam_ptr_start_,
                                                memb_func_size_ * sizeof(int));
        nrn_dparam_ptr_end_ = (int*) erealloc(nrn_dparam_ptr_end_, memb_func_size_ * sizeof(int));
        memb_order_ = (short*) erealloc(memb_order_, memb_func_size_ * sizeof(short));
        nrn_bbcore_write_ = (bbcore_write_t*) erealloc(nrn_bbcore_write_,
                                                       memb_func_size_ * sizeof(bbcore_write_t));
        nrn_bbcore_read_ = (bbcore_write_t*) erealloc(nrn_bbcore_read_,
                                                      memb_func_size_ * sizeof(bbcore_write_t));
        nrn_nmodl_text_ = (const char**) erealloc(nrn_nmodl_text_,
                                                  memb_func_size_ * sizeof(const char*));
        nrn_nmodl_filename_ = (const char**) erealloc(nrn_nmodl_filename_,
                                                      memb_func_size_ * sizeof(const char*));
        nrn_watch_allocate_ =
            (NrnWatchAllocateFunc_t*) erealloc(nrn_watch_allocate_,
                                               memb_func_size_ * sizeof(NrnWatchAllocateFunc_t));
        for (int j = memb_func_size_ - 20; j < memb_func_size_; ++j) {
            pnt_map[j] = 0;
            point_process[j] = (Point_process*) 0;
            pointsym[j] = (Symbol*) 0;
            nrn_pnt_template_[j] = (cTemplate*) 0;
            pnt_receive[j] = (pnt_receive_t) 0;
            pnt_receive_init[j] = (pnt_receive_init_t) 0;
            pnt_receive_size[j] = 0;
            nrn_is_artificial_[j] = 0;
            nrn_artcell_qindex_[j] = 0;
            memb_order_[j] = 0;
            nrn_bbcore_write_[j] = (bbcore_write_t) 0;
            nrn_bbcore_read_[j] = (bbcore_write_t) 0;
            nrn_nmodl_text_[j] = (const char*) 0;
            nrn_nmodl_filename_[j] = (const char*) 0;
            nrn_watch_allocate_[j] = (NrnWatchAllocateFunc_t) 0;
        }
        nrn_mk_prop_pools(memb_func_size_);
    }
}

void initialize_memb_func(int mechtype,
                          nrn_cur_t cur,
                          nrn_jacob_t jacob,
                          Pvmp alloc,
                          nrn_state_t stat,
                          nrn_init_t initialize,
                          int vectorized) {
    assert(mechtype >= memb_list.size());
    memb_list.resize(mechtype + 1);
    memb_func.resize(mechtype + 1);
    nrn_prop_param_size_[mechtype] = 0;  /* fill in later */
    nrn_prop_dparam_size_[mechtype] = 0; /* fill in later */
    nrn_dparam_ptr_start_[mechtype] = 0; /* fill in later */
    nrn_dparam_ptr_end_[mechtype] = 0;   /* fill in later */
    memb_func[mechtype].current = cur;
    memb_func[mechtype].jacob = jacob;
    memb_func[mechtype].alloc = alloc;
    memb_func[mechtype].state = stat;
    memb_func[mechtype].set_initialize(initialize);
    memb_func[mechtype].destructor = nullptr;
    memb_func[mechtype].vectorized = vectorized ? 1 : 0;
    memb_func[mechtype].thread_size_ = vectorized ? (vectorized - 1) : 0;
    memb_func[mechtype].thread_mem_init_ = nullptr;
    memb_func[mechtype].thread_cleanup_ = nullptr;
    memb_func[mechtype].thread_table_check_ = nullptr;
    memb_func[mechtype].is_point = 0;
    memb_func[mechtype].hoc_mech = nullptr;
    memb_func[mechtype].setdata_ = nullptr;
    memb_func[mechtype].dparam_semantics = nullptr;
    memb_order_[mechtype] = mechtype;
    memb_func[mechtype].ode_count = nullptr;
    memb_func[mechtype].ode_map = nullptr;
    memb_func[mechtype].ode_spec = nullptr;
    memb_func[mechtype].ode_matsol = nullptr;
    memb_func[mechtype].ode_synonym = nullptr;
    memb_func[mechtype].singchan_ = nullptr;
}

void check_mech_version(const char** m) {
    /* as of 5.2 nmodl translates so that the version string
        is the first string in m. This allows the neuron application
        to determine if nmodl c files are compatible with this version
        Note that internal mechanisms have a version of "0" and are
        by nature consistent.
    */

    /*printf("%s %s\n", m[0], m[1]);*/
    if (strcmp(m[0], "0") == 0) { /* valid by nature */
    } else if (m[0][0] > '9') {   /* must be 5.1 or before */
        Fprintf(stderr,
                "Mechanism %s needs to be re-translated.\n\
It's pre version 6.0 \"c\" code is incompatible with this neuron version.\n",
                m[0]);
        if (nrn_load_dll_recover_error()) {
            hoc_execerror("Mechanism needs to be retranslated:", m[0]);
        } else {
            nrn_exit(1);
        }
    } else if (strcmp(m[0], nmodl_version_) != 0) {
        Fprintf(stderr,
                "Mechanism %s needs to be re-translated.\n\
It's version %s \"c\" code is incompatible with this neuron version.\n",
                m[1],
                m[0]);
        if (nrn_load_dll_recover_error()) {
            hoc_execerror("Mechanism needs to be retranslated:", m[1]);
        } else {
            nrn_exit(1);
        }
    }
}

void register_mech(const char** m,
                   Pvmp alloc,
                   nrn_cur_t cur,
                   nrn_jacob_t jacob,
                   nrn_state_t stat,
                   nrn_init_t initialize,
                   int nrnpointerindex, /* if -1 then there are none */
                   int vectorized) {
    int mechtype = n_memb_func;
    nrn_register_mech_common(m, alloc, cur, jacob, stat, initialize, nrnpointerindex, vectorized);
    if (nrnpy_reg_mech_p_) {
        (*nrnpy_reg_mech_p_)(mechtype);
    }
}

void nrn_writes_conc(int mechtype, int unused) {
    static int lastion = EXTRACELL + 1;
    int i;
    for (i = n_memb_func - 2; i >= lastion; --i) {
        memb_order_[i + 1] = memb_order_[i];
    }
    memb_order_[lastion] = mechtype;
#if 0
	printf("%s reordered from %d to %d\n", memb_func[mechtype].sym->name, mechtype, lastion);
#endif  // 0
    if (nrn_is_ion(mechtype)) {
        ++lastion;
    }
}

namespace {
/**
 * @brief Translate a dparam semantic string to integer form.
 *
 * This logic used to live inside hoc_register_dparam_semantics.
 */

// name to int map for the negative types
// xx_ion and #xx_ion will get values of type and type+1000 respectively
static std::unordered_map<std::string, int> name_to_negint = {{"area", -1},
                                                              {"iontype", -2},
                                                              {"cvodeieq", -3},
                                                              {"netsend", -4},
                                                              {"pointer", -5},
                                                              {"pntproc", -6},
                                                              {"bbcorepointer", -7},
                                                              {"watch", -8},
                                                              {"diam", -9},
                                                              {"fornetcon", -10},
                                                              {"random", -11}};

int dparam_semantics_to_int(std::string_view name) {
    if (auto got = name_to_negint.find(std::string{name}); got != name_to_negint.end()) {
        return got->second;
    } else {
        bool const i{name[0] == '#'};
        Symbol* s = hoc_lookup(std::string{name.substr(i)}.c_str());
        if (s && s->type == MECHANISM) {
            return s->subtype + i * 1000;
        }
        throw std::runtime_error("unknown dparam semantics: " + std::string{name});
    }
}

std::vector<int> indices_of_type(
    const char* semantic_type,
    std::vector<std::pair<const char*, const char*>> const& dparam_info) {
    std::vector<int> indices{};
    int inttype = dparam_semantics_to_int(std::string{semantic_type});
    for (auto i = 0; i < dparam_info.size(); ++i) {
        if (dparam_semantics_to_int(dparam_info[i].second) == inttype) {
            indices.push_back(i);
        }
    }
    return indices;
}

static std::unordered_map<int, std::vector<int>> mech_random_indices{};

void update_mech_ppsym_for_modlrandom(
    int mechtype,
    std::vector<std::pair<const char*, const char*>> const& dparam_info) {
    std::vector<int> indices = indices_of_type("random", dparam_info);
    mech_random_indices[mechtype] = indices;
    if (indices.empty()) {
        return;
    }
    Symbol* mechsym = memb_func[mechtype].sym;
    int is_point = memb_func[mechtype].is_point;

    int k = mechsym->s_varn;
    mechsym->s_varn += int(indices.size());
    mechsym->u.ppsym = (Symbol**) erealloc(mechsym->u.ppsym, mechsym->s_varn * sizeof(Symbol*));


    for (auto i: indices) {
        auto& p = dparam_info[i];
        Symbol* ransym{};
        if (is_point) {
            ransym = hoc_install(p.first, RANGEOBJ, 0.0, &(nrn_pnt_template_[mechtype]->symtable));
        } else {
            std::string s{p.first};
            s += "_";
            s += mechsym->name;
            ransym = hoc_install(s.c_str(), RANGEOBJ, 0.0, &hoc_symlist);
        }
        ransym->subtype = NMODLRANDOM;
        ransym->u.rng.type = mechtype;
        ransym->cpublic = 1;
        ransym->u.rng.index = i;
        mechsym->u.ppsym[k++] = ransym;
    }
}

}  // namespace

namespace neuron::mechanism::detail {
void register_data_fields(int mechtype,
                          std::vector<std::pair<const char*, int>> const& param_info,
                          std::vector<std::pair<const char*, const char*>> const& dparam_info) {
    nrn_prop_param_size_[mechtype] = param_info.size();
    nrn_prop_dparam_size_[mechtype] = dparam_info.size();
    delete[] std::exchange(memb_func[mechtype].dparam_semantics, nullptr);
    if (!dparam_info.empty()) {
        memb_func[mechtype].dparam_semantics = new int[dparam_info.size()];
        for (auto i = 0; i < dparam_info.size(); ++i) {
            // dparam_info[i].first is the name of the variable, currently unused...
            memb_func[mechtype].dparam_semantics[i] = dparam_semantics_to_int(
                dparam_info[i].second);
        }
    }
    // Translate param_info into the type we want to use internally now we're fully inside NEURON
    // library code (wheels...)
    std::vector<container::Mechanism::Variable> param_info_new{};
    std::transform(param_info.begin(),
                   param_info.end(),
                   std::back_inserter(param_info_new),
                   [](auto const& old) -> container::Mechanism::Variable {
                       return {old.first, old.second};
                   });
    // Create a per-mechanism data structure as part of the top-level
    // neuron::model() structure.
    auto& model = neuron::model();
    model.delete_mechanism(mechtype);  // e.g. extracellular can call hoc_register_prop_size
                                       // multiple times
    auto& mech_data = model.add_mechanism(mechtype,
                                          memb_func[mechtype].sym->name,  // the mechanism name
                                          std::move(param_info_new));  // names and array dimensions
                                                                       // of double-valued
                                                                       // per-instance variables
    memb_list[mechtype].set_storage_pointer(&mech_data);
    update_mech_ppsym_for_modlrandom(mechtype, dparam_info);
}
}  // namespace neuron::mechanism::detail
namespace neuron::mechanism {
template <>
int const* get_array_dims<double>(int mech_type) {
    if (mech_type < 0) {
        return nullptr;
    }
    return neuron::model()
        .mechanism_data(mech_type)
        .get_array_dims<container::Mechanism::field::FloatingPoint>();
}
template <>
double* const* get_data_ptrs<double>(int mech_type) {
    if (mech_type < 0) {
        return nullptr;
    }
    return neuron::model()
        .mechanism_data(mech_type)
        .get_data_ptrs<container::Mechanism::field::FloatingPoint>();
}
template <>
int get_field_count<double>(int mech_type) {
    if (mech_type < 0) {
        return -1;
    }
    return neuron::model()
        .mechanism_data(mech_type)
        .get_tag<container::Mechanism::field::FloatingPoint>()
        .num_variables();
}
}  // namespace neuron::mechanism

/**
 * @brief Support mechanism FUNCTION/PROCEDURE python syntax seg.mech.f()
 *
 * Python (density) mechanism registration uses nrn_mechs2func_map to
 * create a per mechanism map of f members that can be called directly
 * without prior call to setmech.
 */
void hoc_register_npy_direct(int mechtype, NPyDirectMechFunc* f) {
    auto& fmap = nrn_mech2funcs_map[mechtype] = {};
    for (int i = 0; f[i].name; ++i) {
        fmap[f[i].name] = &f[i];
    }
}
std::unordered_map<int, NPyDirectMechFuncs> nrn_mech2funcs_map;

/**
 * @brief Legacy way of registering mechanism data/pdata size.
 *
 * Superseded by neuron::mechanism::register_data_fields.
 */
void hoc_register_prop_size(int mechtype, int psize, int dpsize) {
    assert(nrn_prop_param_size_[mechtype] == psize);
    assert(nrn_prop_dparam_size_[mechtype] == dpsize);
}

/**
 * @brief Legacy way of registering pdata semantics.
 *
 * Superseded by neuron::mechanism::register_data_fields.
 */
void hoc_register_dparam_semantics(int mechtype, int ix, const char* name) {
    assert(memb_func[mechtype].dparam_semantics[ix] == dparam_semantics_to_int(name));
}

int nrn_dparam_semantics_to_int(const char* name) {
    return dparam_semantics_to_int(name);
}

/**
 * @brief dparam indices with random semantics for mechtype
 */
std::vector<int>& nrn_mech_random_indices(int type) {
    return mech_random_indices[type];
}

void hoc_register_cvode(int i,
                        nrn_ode_count_t cnt,
                        nrn_ode_map_t map,
                        nrn_ode_spec_t spec,
                        nrn_ode_matsol_t matsol) {
    memb_func[i].ode_count = cnt;
    memb_func[i].ode_map = map;
    memb_func[i].ode_spec = spec;
    memb_func[i].ode_matsol = matsol;
}
void hoc_register_synonym(int i, nrn_ode_synonym_t syn) {
    memb_func[i].ode_synonym = syn;
}

void register_destructor(Pvmp d) {
    memb_func[n_memb_func - 1].destructor = d;
}

int point_reg_helper(Symbol* s2) {
    pointsym[pointtype] = s2;
    s2->cpublic = 0;
    pnt_map[n_memb_func - 1] = pointtype;
    memb_func[n_memb_func - 1].is_point = 1;
    if (nrnpy_reg_mech_p_) {
        (*nrnpy_reg_mech_p_)(n_memb_func - 1);
    }
    return pointtype++;
}

extern void class2oc_base(const char*,
                          void* (*cons)(Object*),
                          void (*destruct)(void*),
                          Member_func*,
                          int (*checkpoint)(void**),
                          Member_ret_obj_func*,
                          Member_ret_str_func*);


int point_register_mech(const char** m,
                        Pvmp alloc,
                        nrn_cur_t cur,
                        nrn_jacob_t jacob,
                        nrn_state_t stat,
                        nrn_init_t initialize,
                        int nrnpointerindex,
                        int vectorized,

                        void* (*constructor)(Object*),
                        void (*destructor)(void*),
                        Member_func* fmember) {
    Symlist* sl;
    Symbol *s, *s2;
    nrn_load_name_check(m[1]);
    class2oc_base(m[1], constructor, destructor, fmember, nullptr, nullptr, nullptr);
    s = hoc_lookup(m[1]);
    sl = hoc_symlist;
    hoc_symlist = s->u.ctemplate->symtable;
    s->u.ctemplate->steer = steer_point_process;
    s->u.ctemplate->is_point_ = pointtype;
    nrn_register_mech_common(m, alloc, cur, jacob, stat, initialize, nrnpointerindex, vectorized);
    nrn_pnt_template_[n_memb_func - 1] = s->u.ctemplate;
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
#endif  // 0

int _ninits;

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

int state_discon_flag_ = 0;
void state_discontinuity(int i, double* pd, double d) {
    if (state_discon_allowed_ && state_discon_flag_ == 0) {
        *pd = d;
        /*printf("state_discontinuity t=%g pd=%lx d=%g\n", t, (long)pd, d);*/
    }
}

void hoc_register_limits(int mechtype, HocParmLimits* limits) {
    int i;
    Symbol* sym;
    for (i = 0; limits[i].name; ++i) {
        sym = (Symbol*) 0;
        if (mechtype && memb_func[mechtype].is_point) {
            Symbol* t;
            t = hoc_lookup(memb_func[mechtype].sym->name);
            sym = hoc_table_lookup(limits[i].name, t->u.ctemplate->symtable);
        }
        if (!sym) {
            sym = hoc_lookup(limits[i].name);
        }
        hoc_symbol_limits(sym, limits[i].bnd[0], limits[i].bnd[1]);
    }
}

void hoc_register_units(int mechtype, HocParmUnits* units) {
    int i;
    Symbol* sym;
    for (i = 0; units[i].name; ++i) {
        sym = (Symbol*) 0;
        if (mechtype && memb_func[mechtype].is_point) {
            Symbol* t;
            t = hoc_lookup(memb_func[mechtype].sym->name);
            sym = hoc_table_lookup(units[i].name, t->u.ctemplate->symtable);
        }
        if (!sym) {
            sym = hoc_lookup(units[i].name);
        }
        hoc_symbol_units(sym, units[i].units);
    }
}

void hoc_reg_ba(int mt, nrn_bamech_t f, int type) {
    BAMech* bam;
    switch (type) { /* see bablk in src/nmodl/nocpout.cpp */
    case 11:
        type = BEFORE_BREAKPOINT;
        break;
    case 22:
        type = AFTER_SOLVE;
        break;
    case 13:
        type = BEFORE_INITIAL;
        break;
    case 23:
        type = AFTER_INITIAL;
        break;
    case 14:
        type = BEFORE_STEP;
        break;
    default:
        printf("before-after processing type %d for %s not implemented\n",
               type,
               memb_func[mt].sym->name);
        nrn_exit(1);
    }
    bam = (BAMech*) emalloc(sizeof(BAMech));
    bam->f = f;
    bam->type = mt;
    bam->next = nullptr;
    // keep in call order
    if (!bamech_[type]) {
        bamech_[type] = bam;
    } else {
        BAMech* last;
        for (last = bamech_[type]; last->next; last = last->next) {
        }
        last->next = bam;
    }
}

void _cvode_abstol(Symbol** s, double* tol, int i) {
    if (s && s[i]->extra) {
        double x;
        x = s[i]->extra->tolerance;
        if (x != 0) {
            tol[i] *= x;
        }
    }
}

void hoc_register_tolerance(int mechtype, HocStateTolerance* tol, Symbol*** stol) {
    int i;
    Symbol* sym;
    /*printf("register tolerance for %s\n", memb_func[mechtype].sym->name);*/
    for (i = 0; tol[i].name; ++i) {
        if (memb_func[mechtype].is_point) {
            Symbol* t;
            t = hoc_lookup(memb_func[mechtype].sym->name);
            sym = hoc_table_lookup(tol[i].name, t->u.ctemplate->symtable);
        } else {
            sym = hoc_lookup(tol[i].name);
        }
        hoc_symbol_tolerance(sym, tol[i].tolerance);
    }

    if (memb_func[mechtype].ode_count) {
        if (auto const n = memb_func[mechtype].ode_count(mechtype); n > 0) {
            auto* const psym = new Symbol* [n] {};
            Node node{};  // dummy node
            node.sec_node_index_ = 0;
            prop_alloc(&(node.prop), MORPHOLOGY, &node);         /* in case we need diam */
            auto* p = prop_alloc(&(node.prop), mechtype, &node); /* this and any ions */
            // Fill `pv` with pointers to `2*n` parameters inside `p`
            std::vector<neuron::container::data_handle<double>> pv(2 * n);
            memb_func[mechtype].ode_map(p, 0, pv.data(), pv.data() + n, nullptr, mechtype);
            // The first n elements of `pv` are "pv", the second n are "pvdot"
            for (int i = 0; i < n; ++i) {
                // `index` is the legacy index of `pv[i]` inside mechanism instance `p`
                auto const [p, index] = [&h = pv[i]](Prop* p) {
                    for (; p; p = p->next) {
                        int legacy_index{};
                        auto const num_params = p->param_num_vars();
                        for (auto i_param = 0; i_param < num_params; ++i_param) {
                            auto const array_dim = p->param_array_dimension(i_param);
                            for (auto j = 0; j < array_dim; ++j, ++legacy_index) {
                                if (h == p->param_handle(i_param, j)) {
                                    return std::make_pair(p, legacy_index);
                                }
                            }
                        }
                    }
                    std::ostringstream oss;
                    oss << "could not find " << h << " starting from " << *p;
                    throw std::runtime_error(oss.str());
                }(node.prop);
                /* need to find symbol for this */
                auto* msym = memb_func[p->_type].sym;
                for (int j = 0; j < msym->s_varn; ++j) {
                    auto* vsym = msym->u.ppsym[j];
                    if (vsym->type == RANGEVAR && vsym->u.rng.index == index) {
                        psym[i] = vsym;
                        /*printf("identified %s at index %d of %s\n", vsym->name, index,
                         * msym->name);*/
                        if (ISARRAY(vsym)) {
                            int const na = vsym->arayinfo->sub[0];
                            for (int k = 1; k < na; ++k) {
                                psym[++i] = vsym;
                            }
                        }
                        break;
                    }
                }
            }
            *stol = psym;
        }
    }
}

void _nrn_thread_reg(int i, int cons, void (*f)(Datum*)) {
    if (cons == 1) {
        memb_func[i].thread_mem_init_ = f;
    } else if (cons == 0) {
        memb_func[i].thread_cleanup_ = f;
    }
}

void _nrn_thread_table_reg(int i, nrn_thread_table_check_t f) {
    memb_func[i].thread_table_check_ = f;
}

void _nrn_setdata_reg(int i, void (*call)(Prop*)) {
    memb_func[i].setdata_ = call;
}
/* there is some question about the _extcall_thread variables, if any. */
double nrn_call_mech_func(Symbol* s, int narg, Prop* p, int mechtype) {
    void (*call)(Prop*) = memb_func[mechtype].setdata_;
    if (call) {
        (*call)(p);
    }
    return hoc_call_func(s, narg);
}

void nrnunit_use_legacy() {
    hoc_warning("nrnunit_use_legacy() is deprecated as only modern units are supported.",
                "If you want to still use legacy unit you can use a version of nrn < 9.");
    if (ifarg(1)) {
        int arg = (int) chkarg(1, 0, 1);
        if (arg == 1) {
            hoc_execerror(
                "'nrnunit_use_legacy(1)' have been called but legacy units are no more supported.",
                nullptr);
        }
    }
    hoc_retpushx(0.);  // This value means modern unit
}
