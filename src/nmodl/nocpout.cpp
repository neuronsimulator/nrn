#include <../../nmodlconf.h>

/* /local/src/master/nrn/src/nmodl/nocpout.c,v 4.1 1997/08/30 20:45:28 hines Exp */

/*
nrnversion is a string that is passed via the _mechanism structure
as the first arg. It will be interpreted within neuron to determine if
that version is compatible with this version.
For now try to use something of the form d.d
If this is changed then also change nrnoc/init.c
*/
const char* nmodl_version_ = "7.7.0";

/* Point processes are now interfaced to nrnoc via objectvars.
Thus, p-array variables and functions accessible to hoc do not have
suffixes, and there is a constructor, destructor.
Also hoc interface functions always have a void* _vptr arg which is
always cast to (Point_process*) and the _p and _ppvar pointers set.
This makes the old setdata and create obsolete.
*/
/* The strategy is to use as much of parout and hparout method as possible.
The bulk of the variables is in a p-array. The variables that don't belong
in this p-array are indicated by a flag with sym->subtype & NRNNOTP.
All other variables have values in that single array but not all those values
are available from HOC.

Variables accessible to NEURON are variables that appear within
GLOBAL, and RANGE statements.

Variables that do not appear in the p-array are:
 1)externally declared variables such as celsius, t.
 2)parameters and assigned not declared in the NEURON{RANGE list}
   that are global with respect to sections.
 3) variables static to this model, ie. v
 4) read only variables like "diam"
States always are in the p-array.

USEION variables in the p-array have connections to other places and
depending on the context may get their value from somewhere else, or
add their value to somewhere else, or place their value somewhere else.
The cases are:
    NONSPECIFIC and USEION WRITE  i... value added to proper ion current
        and total current.
    USEION READ  entry value assigned to local copy.
    USEION WRITE e.. ..o ..i exit value of local copy assigned to pointer..
         It is an error for an ionic current or ionic variable
        to be a STATE. Use another variable as the state, make the
        ionic variable an ASSIGNED and just assign it at the
        proper place.
      Alternatively, if they are STATE's then they should not be READ
      since their value comes from the p-array itself.

POINTER variables are like USEION variables. Unfortunately, it is up to
the hoc user to make sure they point to the proper place with a connect
statement. At this time we only check for a null pointer. The pointers
are kept in the ppvar array.

each model creates a setdata_suffix(x) (or setdata_suffix(i)) function
which sets up _p and _ppvar for use by functions in the model called
directly by hoc.
*/

#include "modl.h"
#include "parse1.hpp"

#include <algorithm>
#include <iterator>  // std::back_inserter
#include <stdlib.h>
#include <string>
#include <vector>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#define GETWD(buf) getcwd(buf, NRN_BUFSIZE)

int vectorize = 1;
/*
the idea is to put all variables into a vector of vectors so there
there is no static data. Every function has an implicit argument, underbar ix
which tells which set of data _p[ix][...] to use. There are going to have
to be limits on the kinds of scopmath functions called since many of them
need static data. We can have special versions of the most useful of these.
ie sparse.c.
Above is obsolete in detail , underbar ix is no longer used.
When vectorize = 1 then we believe the code is thread safe and most functions
pass around _p, _ppvar, _thread. When vectorize is 0 then we are definitely
not thread safe and _p and _ppvar are static.
*/


#define IONEREV 0 /* Parameter */
#define IONIN   1
#define IONOUT  2
#define IONCUR  3 /* assigned */
#define IONDCUR 4

extern int brkpnt_exists;
static const char* brkpnt_str_;
extern Symbol* indepsym;
extern List* symlist[];
extern List* ldifuslist;
extern int check_tables_threads(List*);
List* syminorder;
List* plotlist;
List* defs_list;
int electrode_current = 0;
int thread_data_index = 0;
List* thread_cleanup_list;
List* thread_mem_init_list;
List* toplocal_;
extern int protect_;
extern int protect_include_;
extern List *set_ion_variables(int), *get_ion_variables(int);
extern int netrec_need_v;

int decode_limits(Symbol* sym, double* pg1, double* pg2);
int decode_tolerance(Symbol* sym, double* pg1);


/* NEURON block information */
List* currents;
List* useion;
List* conductance_;
List* breakpoint_local_current_;
static List* rangeparm;
static List* rangedep;
static List* rangestate;
static List* nrnpointers;
static List* nmodlrandoms;
static List* nrn_mech_inst_destruct_list;
static int num_random_vars = 0;
static char suffix[256];
static const char* rsuffix; /* point process range and functions don't have suffix*/
static char* mechname;
int point_process;      /* 1 if a point process model */
int artificial_cell;    /* 1 if also explicitly declared an ARTIFICIAL_CELL */
static int diamdec = 0; /*1 if diam is declared*/
static int areadec = 0;
static int use_bbcorepointer = 0;

void defs_h(Symbol*);
int iontype(char* s1, char* s2);
void nrndeclare();
void del_range(List*);
void declare_p();
int iondef(int*);
void ion_promote(Item*);
static int ppvar_cnt;
static List* ppvar_semantics_;
static void ppvar_semantics(int, const char* semantics, const char* name, const char* type);
static int for_netcons_; /* number of FOR_NETCONS statements */
static Item* net_init_q1_;
static Item* net_init_q2_;
static int ba_index_; /* BEFORE AFTER blocks. See bablk */
static List* ba_list_;

List* state_discon_list_;
int cvode_not_allowed;
static int cvode_emit, cvode_ieq_index;
static int tqitem_index;
static int watch_index;
static List* ion_synonym;
int debugging_;
int net_receive_;
int net_send_seen_;
int net_event_seen_;
int watch_seen_; /* number of WATCH statements + 1*/
extern List* watch_alloc;
static Item* net_send_delivered_; /* location for if flag is 1 then clear the
                tqitem_ to allow  an error message for net_move */

#define SYMITER(arg)         \
    ITERATE(q, syminorder) { \
        s = SYM(q);          \
        if (s->type == arg)

#define SYMLISTITER              \
    for (i = 'A'; i <= 'z'; i++) \
    ITERATE(q, symlist[i])

/* varcount holds the index into the .var file and is saved in s->used
   parraycount holds the index into the p array and is saved in s->varnum
   pvarcount indexes pointers to variables such as ena
*/
static int varcount, parraycount;
static std::vector<std::pair<int, std::string>> ppvar_data_field_strings;
static std::vector<std::string> data_field_strings;

void nrninit() {
    currents = newlist();
    rangeparm = newlist();
    rangedep = newlist();
    rangestate = newlist();
    useion = newlist();
    nrnpointers = newlist();
    indepinstall(install("t", NAME), "0", "1", "100", "ms");
    debugging_ = 1;
    thread_cleanup_list = newlist();
    thread_mem_init_list = newlist();
    nmodlrandoms = newlist();
}

void parout() {
    int i, ioncount, pointercount, gind, emit_check_table_thread;
    Item *q, *q1;
    Symbol *s, *sion;
    double d1, d2;
    extern char* modprefix;
    char* modbase;

    defs_list = newlist(); /* relates hoc names to c-variables */
    if (brkpnt_exists) {
        brkpnt_str_ = "nrn_cur, nrn_jacob, nrn_state";
    } else {
        brkpnt_str_ = "nullptr, nullptr, nullptr";
    }

    for (modbase = modprefix + strlen(modprefix); modbase != modprefix; modbase--) {
        if (*modbase == '\\' || *modbase == '/') {
            modbase++;
            break;
        }
    }
    if (!mechname) {
        Sprintf(suffix, "_%s", modbase);
        mechname = modbase;
    } else if (strcmp(mechname, "nothing") == 0) {
        vectorize = 0;
        suffix[0] = '\0';
        mechname = modbase;
        nmodl_text = 0;
    } else {
        Sprintf(suffix, "_%s", mechname);
    }

    func_needs_setdata();  // Do FUNCTION/PROCEDURE need prior call to setdata.

    if (artificial_cell && vectorize && (thread_data_index || toplocal_)) {
        fprintf(stderr,
                "Notice: ARTIFICIAL_CELL models that would require thread specific data are not "
                "thread safe.\n");
        vectorize = 0;
    }
    if (point_process) {
        rsuffix = "";
    } else {
        rsuffix = suffix;
    }

    Lappendstr(defs_list,
               "\
\n#if !NRNGPU\
\n#undef exp\
\n#define exp hoc_Exp\
\n#endif\n\
");
    if (protect_include_) {
        Lappendstr(defs_list, "\n#include \"nmodlmutex.h\"");
    }

#if 1
    /* for easier profiling, give distinct names to otherwise reused static names */
    Sprintf(buf,
            "\n\
#define nrn_init _nrn_init_%s\n\
#define _nrn_initial _nrn_initial_%s\n\
#define nrn_cur _nrn_cur_%s\n\
#define _nrn_current _nrn_current_%s\n\
#define nrn_jacob _nrn_jacob_%s\n\
#define nrn_state _nrn_state_%s\n\
#define _net_receive _net_receive_%s\
",
            suffix,
            suffix,
            suffix,
            suffix,
            suffix,
            suffix,
            suffix);
    Lappendstr(defs_list, buf);
    SYMLISTITER {
        Symbol* s = SYM(q);
        /* note that FUNCT will be redefined anyway */
        if (s->type == NAME && s->subtype & (PROCED | DERF | KINF)) {
            Sprintf(buf, "\n#define %s %s_%s", s->name, s->name, suffix);
            Lappendstr(defs_list, buf);
        }
    }
    Lappendstr(defs_list, "\n");
#endif /* distinct names for easier profiling */

    if (vectorize) {
        Lappendstr(defs_list,
                   "\n\
#define _threadargscomma_ _ml, _iml, _ppvar, _thread, _nt,\n\
#define _threadargsprotocomma_ Memb_list* _ml, size_t _iml, Datum* _ppvar, Datum* _thread, NrnThread* _nt,\n\
#define _internalthreadargsprotocomma_ _nrn_mechanism_cache_range* _ml, size_t _iml, Datum* _ppvar, Datum* _thread, NrnThread* _nt,\n\
#define _threadargs_ _ml, _iml, _ppvar, _thread, _nt\n\
#define _threadargsproto_ Memb_list* _ml, size_t _iml, Datum* _ppvar, Datum* _thread, NrnThread* _nt\n\
#define _internalthreadargsproto_ _nrn_mechanism_cache_range* _ml, size_t _iml, Datum* _ppvar, Datum* _thread, NrnThread* _nt\n\
");
    } else {
        Lappendstr(defs_list,
                   "\n\
#define _threadargscomma_ /**/\n\
#define _threadargsprotocomma_ /**/\n\
#define _internalthreadargsprotocomma_ /**/\n\
#define _threadargs_ /**/\n\
#define _threadargsproto_ /**/\n\
#define _internalthreadargsproto_ /**/\n\
");
    }
    Lappendstr(defs_list,
               "\
	/*SUPPRESS 761*/\n\
	/*SUPPRESS 762*/\n\
	/*SUPPRESS 763*/\n\
	/*SUPPRESS 765*/\n\
	");
    Lappendstr(defs_list, "extern double *hoc_getarg(int);\n");

    nrndeclare();
    varcount = parraycount = 0;
    declare_p();
    // iondef defined _nrn_mechanism_cache_range
    ioncount = iondef(&pointercount); /* first is _nd_area if point process */
    if (vectorize) {
        Sprintf(buf, "/* Thread safe. No static _ml, _iml or _ppvar. */\n");
    } else {
        Sprintf(buf,
                "static _nrn_mechanism_cache_instance _ml_real{nullptr};\n"
                "static _nrn_mechanism_cache_range *_ml{&_ml_real};\n"
                "static size_t _iml{0};\n"
                "static Datum *_ppvar;\n");
    }
    Lappendstr(defs_list, buf);
    Lappendstr(defs_list, "static int hoc_nrnpointerindex = ");
    if (pointercount) {
        q = nrnpointers->next;
        Sprintf(buf, "%d;\n", SYM(q)->used);
    } else {
        Sprintf(buf, "-1;\n");
    }
    Lappendstr(defs_list, buf);
    /*above modified to also count and define pointers*/

    if (vectorize) {
        Lappendstr(defs_list, "static _nrn_mechanism_std_vector<Datum> _extcall_thread;\n");
    }
    if (!point_process) {
        Lappendstr(defs_list, "static Prop* _extcall_prop;\n");
        Lappendstr(defs_list,
                   "/* _prop_id kind of shadows _extcall_prop to allow validity checking. */\n");
        Lappendstr(defs_list, "static _nrn_non_owning_id_without_container _prop_id{};\n");
    }

    Lappendstr(defs_list, "/* external NEURON variables */\n");
    SYMLISTITER {
        s = SYM(q);
        if (s->nrntype & NRNEXTRN) {
            if (strcmp(s->name, "dt") == 0) {
                continue;
            }
            if (strcmp(s->name, "t") == 0) {
                continue;
            }
            if (s->subtype & ARRAY) {
                Sprintf(buf, "extern double* %s;\n", s->name);
            } else {
                Sprintf(buf, "extern double %s;\n", s->name);
            }
            Lappendstr(defs_list, buf);
        }
    }

    Lappendstr(defs_list, "/* declaration of user functions */\n");
    SYMLISTITER {
        s = SYM(q);
        if (s->subtype & (FUNCT | PROCED) && s->name[0] != '_') {
            if (point_process) {
                Sprintf(buf, "static double _hoc_%s(void*);\n", s->name);
            } else {
                Sprintf(buf, "static void _hoc_%s(void);\n", s->name);
            }
            Lappendstr(defs_list, buf);
        }
    }

    Lappendstr(defs_list,
               "static int _mechtype;\n\
extern void _nrn_cacheloop_reg(int, int);\n\
extern void hoc_register_limits(int, HocParmLimits*);\n\
extern void hoc_register_units(int, HocParmUnits*);\n\
extern void nrn_promote(Prop*, int, int);\n\
");

    if (nmodl_text) {
        Lappendstr(defs_list,
                   "\n"
                   "#define NMODL_TEXT 1\n"
                   "#if NMODL_TEXT\n"
                   "static void register_nmodl_text_and_filename(int mechtype);\n"
                   "#endif\n");
    }

    /**** create special point process functions */
    if (point_process) {
        Lappendstr(defs_list, "extern Prop* nrn_point_prop_;\n");
        Lappendstr(defs_list, "static int _pointtype;\n");
        Lappendstr(defs_list,
                   "static void* _hoc_create_pnt(Object* _ho) { void* create_point_process(int, "
                   "Object*);\n");
        Lappendstr(defs_list, "return create_point_process(_pointtype, _ho);\n}\n");
        Lappendstr(defs_list, "static void _hoc_destroy_pnt(void*);\n");
        Lappendstr(
            defs_list,
            "static double _hoc_loc_pnt(void* _vptr) {double loc_point_process(int, void*);\n");
        Lappendstr(defs_list, "return loc_point_process(_pointtype, _vptr);\n}\n");
        Lappendstr(defs_list,
                   "static double _hoc_has_loc(void* _vptr) {double has_loc_point(void*);\n");
        Lappendstr(defs_list, "return has_loc_point(_vptr);\n}\n");
        Lappendstr(defs_list, "static double _hoc_get_loc_pnt(void* _vptr) {\n");
        Lappendstr(
            defs_list,
            "double get_loc_point_process(void*); return (get_loc_point_process(_vptr));\n}\n");
    }
    /* function to set up _p and _ppvar */
    Lappendstr(defs_list, "extern void _nrn_setdata_reg(int, void(*)(Prop*));\n");
    Lappendstr(defs_list, "static void _setdata(Prop* _prop) {\n");
    if (!point_process) {
        Lappendstr(defs_list, "_extcall_prop = _prop;\n");
        Lappendstr(defs_list, "_prop_id = _nrn_get_prop_id(_prop);\n");
    }
    if (!vectorize) {
        Lappendstr(defs_list,
                   "neuron::legacy::set_globals_from_prop(_prop, _ml_real, _ml, _iml);\n"
                   "_ppvar = _nrn_mechanism_access_dparam(_prop);\n");
    }
    Lappendstr(defs_list, "}\n");

    if (point_process) {
        Lappendstr(defs_list, "static void _hoc_setdata(void* _vptr) { Prop* _prop;\n");
        Lappendstr(defs_list, "_prop = ((Point_process*)_vptr)->_prop;\n");
    } else {
        Lappendstr(defs_list,
                   "static void _hoc_setdata() {\n Prop *_prop, *hoc_getdata_range(int);\n");
        Sprintf(buf, "_prop = hoc_getdata_range(_mechtype);\n");
        Lappendstr(defs_list, buf);
    }
    Lappendstr(defs_list, "  _setdata(_prop);\n");
    if (point_process) {
        Lappendstr(defs_list, "}\n");
    } else {
        Lappendstr(defs_list, "hoc_retpushx(1.);\n}\n");
    }

    /* functions */
    Lappendstr(defs_list, "/* connect user functions to hoc names */\n");
    Lappendstr(defs_list, "static VoidFunc hoc_intfunc[] = {\n");
    if (point_process) {
        Lappendstr(defs_list, "{0, 0}\n};\n");
        Lappendstr(defs_list, "static Member_func _member_func[] = {\n");
        Sprintf(buf, "{\"loc\", _hoc_loc_pnt},\n");
        Lappendstr(defs_list, buf);
        Sprintf(buf, "{\"has_loc\", _hoc_has_loc},\n");
        Lappendstr(defs_list, buf);
        Sprintf(buf, "{\"get_loc\", _hoc_get_loc_pnt},\n");
        Lappendstr(defs_list, buf);
    } else {
        Sprintf(buf, "{\"setdata_%s\", _hoc_setdata},\n", mechname);
        Lappendstr(defs_list, buf);
    }

    SYMLISTITER {
        s = SYM(q);
        if ((s->subtype & (FUNCT | PROCED)) && s->name[0] != '_') {
            Sprintf(buf, "{\"%s%s\", _hoc_%s},\n", s->name, rsuffix, s->name);
            Lappendstr(defs_list, buf);
        }
    }
    Lappendstr(defs_list, "{0, 0}\n};\n");

    /* Direct Python call wrappers to density mechanism functions. */
    if (!point_process) {
        Lappendstr(defs_list,
                   "\n/* Direct Python call wrappers to density mechanism functions.*/\n");
        SYMLISTITER {
            s = SYM(q);
            if ((s->subtype & (FUNCT | PROCED)) && s->name[0] != '_') {
                Sprintf(buf, "static double _npy_%s(Prop*);\n", s->name, s->name);
                Lappendstr(defs_list, buf);
            }
        }
        Lappendstr(defs_list,
                   "\n"
                   "static NPyDirectMechFunc npy_direct_func_proc[] = {\n");
        SYMLISTITER {
            s = SYM(q);
            if ((s->subtype & (FUNCT | PROCED)) && s->name[0] != '_') {
                Sprintf(buf, "{\"%s\", _npy_%s},\n", s->name, s->name);
                Lappendstr(defs_list, buf);
            }
        }
        Lappendstr(defs_list, "{0, 0}\n};\n");
    }

    /* FUNCTION's are now global so callable from other models */
    /* change name to namesuffix. This propagates everywhere except
        to hoc_name*/
    /* but don't do it if suffix is empty */
    if (suffix[0])
        SYMLISTITER {
            s = SYM(q);
            if ((s->subtype & FUNCT)) {
                Sprintf(buf, "#define %s %s%s\n", s->name, s->name, suffix);
                q1 = Lappendstr(defs_list, buf);
                q1->itemtype = VERBATIM;
            }
        }
    SYMLISTITER {
        int j;
        s = SYM(q);
        if ((s->subtype & FUNCT)) {
            Sprintf(buf, "extern double %s(", s->name);
            Lappendstr(defs_list, buf);
            if (vectorize && !s->no_threadargs) {
                if (s->varnum) {
                    Lappendstr(defs_list, "_internalthreadargsprotocomma_");
                } else {
                    Lappendstr(defs_list, "_internalthreadargsproto_");
                }
            }
            for (j = 0; j < s->varnum; ++j) {
                Lappendstr(defs_list, "double");
                if (j + 1 < s->varnum) {
                    Lappendstr(defs_list, ",");
                }
            }
            Lappendstr(defs_list, ");\n");
        }
    }

    emit_check_table_thread = 0;
    if (vectorize && check_tables_threads(defs_list)) {
        emit_check_table_thread = 1;
    }

    /* per thread top LOCAL */
    /* except those that are marked assigned_to_ == 2 stay static double */
    if (vectorize && toplocal_) {
        int cnt;
        cnt = 0;
        ITERATE(q, toplocal_) {
            if (SYM(q)->assigned_to_ != 2) {
                if (SYM(q)->subtype & ARRAY) {
                    cnt += SYM(q)->araydim;
                } else {
                    ++cnt;
                }
            }
        }
        Sprintf(buf,
                "  _thread[%d] = {neuron::container::do_not_search, new double[%d]{}};\n",
                thread_data_index,
                cnt);
        lappendstr(thread_mem_init_list, buf);
        Sprintf(buf, "  delete[] _thread[%d].get<double*>();\n", thread_data_index);
        lappendstr(thread_cleanup_list, buf);
        cnt = 0;
        ITERATE(q, toplocal_) {
            if (SYM(q)->assigned_to_ != 2) {
                if (SYM(q)->subtype & ARRAY) {
                    Sprintf(buf,
                            "#define %s (_thread[%d].get<double*>() + %d)\n",
                            SYM(q)->name,
                            thread_data_index,
                            cnt);
                    cnt += SYM(q)->araydim;
                } else {
                    Sprintf(buf,
                            "#define %s _thread[%d].get<double*>()[%d]\n",
                            SYM(q)->name,
                            thread_data_index,
                            cnt);
                    ++cnt;
                }
            } else { /* stay file static */
                if (SYM(q)->subtype & ARRAY) {
                    Sprintf(buf, "static double %s[%d];\n", SYM(q)->name, SYM(q)->araydim);
                } else {
                    Sprintf(buf, "static double %s;\n", SYM(q)->name);
                }
            }
            lappendstr(defs_list, buf);
        }
        ++thread_data_index;
    }
    /* per thread global data */
    gind = 0;
    if (vectorize)
        SYMLISTITER {
            s = SYM(q);
            if (s->nrntype & (NRNGLOBAL) && s->assigned_to_ == 1) {
                if (s->subtype & ARRAY) {
                    gind += s->araydim;
                } else {
                    ++gind;
                }
            }
        }
    /* double scalars declared internally */
    Lappendstr(defs_list, "/* declare global and static user variables */\n");
    if (gind) {
        Sprintf(buf,
                "static int _thread1data_inuse = 0;\nstatic double _thread1data[%d];\n#define _gth "
                "%d\n",
                gind,
                thread_data_index);
        Lappendstr(defs_list, buf);
        Sprintf(buf,
                "if (_thread1data_inuse) {\n"
                "  _thread[_gth] = {neuron::container::do_not_search, new double[%d]{}};\n"
                "} else {\n"
                "  _thread[_gth] = {neuron::container::do_not_search, _thread1data};\n"
                "  _thread1data_inuse = 1;\n"
                "}\n",
                gind);
        lappendstr(thread_mem_init_list, buf);
        lappendstr(thread_cleanup_list,
                   " if (_thread[_gth].get<double*>() == _thread1data) {\n   "
                   "_thread1data_inuse = 0;\n  "
                   "}else{\n   delete[] _thread[_gth].get<double*>();\n  }\n");
        ++thread_data_index;
    }
    gind = 0;
    SYMLISTITER { /* globals are now global with respect to C as well as hoc */
        s = SYM(q);
        if (s->nrntype & (NRNGLOBAL)) {
            if (vectorize && s->assigned_to_ == 1) {
                if (s->subtype & ARRAY) {
                    Sprintf(buf,
                            "#define %s%s (_thread1data + %d)\n\
#define %s (_thread[_gth].get<double*>() + %d)\n",
                            s->name,
                            suffix,
                            gind,
                            s->name,
                            gind);
                } else {
                    Sprintf(buf,
                            "#define %s%s _thread1data[%d]\n\
#define %s _thread[_gth].get<double*>()[%d]\n",
                            s->name,
                            suffix,
                            gind,
                            s->name,
                            gind);
                }
                q1 = Lappendstr(defs_list, buf);
                q1->itemtype = VERBATIM;
                if (s->subtype & ARRAY) {
                    gind += s->araydim;
                } else {
                    ++gind;
                }
                continue;
            }
            if (suffix[0]) {
                Sprintf(buf, "#define %s %s%s\n", s->name, s->name, suffix);
                q1 = Lappendstr(defs_list, buf);
                q1->itemtype = VERBATIM;
            }
            decode_ustr(s, &d1, &d2, buf);
            if (s->subtype & ARRAY) {
                Sprintf(buf, "double %s[%d];\n", s->name, s->araydim);
            } else {
                Sprintf(buf, "double %s = %g;\n", s->name, d1);
            }
            Lappendstr(defs_list, buf);
        }
    }

    Lappendstr(defs_list, "/* some parameters have upper and lower limits */\n");
    Lappendstr(defs_list, "static HocParmLimits _hoc_parm_limits[] = {\n");
    SYMLISTITER {
        s = SYM(q);
        if (s->subtype & PARM) {
            double d1 = 0., d2 = 0.;
            if (decode_limits(s, &d1, &d2)) {
                if (s->nrntype & NRNGLOBAL || !point_process) {
                    Sprintf(buf, "{\"%s%s\", %g, %g},\n", s->name, suffix, d1, d2);
                } else {
                    Sprintf(buf, "{\"%s\", %g, %g},\n", s->name, d1, d2);
                }
                Lappendstr(defs_list, buf);
            }
        }
    }
    Lappendstr(defs_list, "{0, 0, 0}\n};\n");

    units_reg();

    SYMLISTITER {
        s = SYM(q);
        if (s->nrntype & (NRNSTATIC)) {
            decode_ustr(s, &d1, &d2, buf);
            if (s->subtype & ARRAY) {
                Sprintf(buf, "static double %s[%d];\n", s->name, s->araydim);
            } else {
                Sprintf(buf, "static double %s = %g;\n", s->name, d1);
            }
            Lappendstr(defs_list, buf);
        }
    }
    Lappendstr(defs_list, "/* connect global user variables to hoc */\n");
    Lappendstr(defs_list, "static DoubScal hoc_scdoub[] = {\n");
    ITERATE(q, syminorder) {
        s = SYM(q);
        if (s->nrntype & NRNGLOBAL && !(s->subtype & ARRAY)) {
            Sprintf(buf, "{\"%s%s\", &%s%s},\n", s->name, suffix, s->name, suffix);
            Lappendstr(defs_list, buf);
        }
    }
    Lappendstr(defs_list, "{0, 0}\n};\n");

    /* double vectors */
    Lappendstr(defs_list, "static DoubVec hoc_vdoub[] = {\n");
    ITERATE(q, syminorder) {
        s = SYM(q);
        if (s->nrntype & NRNGLOBAL && (s->subtype & ARRAY)) {
            Sprintf(buf, "{\"%s%s\", %s%s, %d},\n", s->name, suffix, s->name, suffix, s->araydim);
            Lappendstr(defs_list, buf);
        }
    }
    Lappendstr(defs_list, "{0, 0, 0}\n};\n");
    Lappendstr(defs_list, "static double _sav_indep;\n");
    if (ba_index_ > 0) {
        Lappendstr(defs_list,
                   "static void _ba1(Node*_nd, Datum* _ppd, Datum* _thread, NrnThread* _nt, "
                   "Memb_list* _ml, size_t _iml, _nrn_model_sorted_token const&)");
        for (i = 2; i <= ba_index_; ++i) {
            Sprintf(buf,
                    ", _ba%d(Node*_nd, Datum* _ppd, Datum* _thread, NrnThread* _nt, Memb_list* "
                    "_ml, size_t _iml, _nrn_model_sorted_token const&)",
                    i);
            Lappendstr(defs_list, buf);
        }
        Lappendstr(defs_list, ";\n");
    }

    /******** what normally goes into cabvars.h structures */

    /*declaration of the range variables names to HOC */
    Lappendstr(
        defs_list,
        "static void nrn_alloc(Prop*);\n"
        "static void nrn_init(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);\n"
        "static void nrn_state(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);\n");
    if (brkpnt_exists) {
        Lappendstr(
            defs_list,
            "static void nrn_cur(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);\n"
            "static void nrn_jacob(_nrn_model_sorted_token const&, NrnThread*, "
            "Memb_list*, int);\n");
    }
    /* count the number of pointers needed */
    num_random_vars = 0;
    ITERATE(q, nmodlrandoms) {
        num_random_vars++;
    }
    ppvar_cnt = ioncount + diamdec + pointercount + num_random_vars + areadec;
    if (net_send_seen_) {
        tqitem_index = ppvar_cnt;
        ppvar_semantics(
            ppvar_cnt,
            "netsend",
            "_tqitem",
            "void*" /* TQItem* really, but that's not defined in translated MOD file code */);
        ppvar_cnt++;
    }
    if (watch_seen_) {
        watch_index = ppvar_cnt;
        for (i = 0; i < watch_seen_; ++i) {
            // TODO: improve type safety by not using void* here
            ppvar_semantics(i + ppvar_cnt, "watch", "_watch_array", "void*");
        }
        ppvar_cnt += watch_seen_;
        Sprintf(buf, "\n#define _watch_array _ppvar + %d", watch_index);
        Lappendstr(defs_list, buf);
        Lappendstr(defs_list, "\n");
        Lappendstr(defs_list, "static void _watch_alloc(Datum*);\n");
        Lappendstr(defs_list, "extern void hoc_reg_watch_allocate(int, void(*)(Datum*));");
        Lappendstr(watch_alloc, "}\n\n");
        movelist(watch_alloc->next, watch_alloc->prev, procfunc);
    }
    if (for_netcons_) {
        Sprintf(buf, "\n#define _fnc_index %d\n", ppvar_cnt);
        Lappendstr(defs_list, buf);
        // TODO: improve type safety by not using void* here
        ppvar_semantics(ppvar_cnt, "fornetcon", "_fnc_index", "void*");
        ppvar_cnt += 1;
    }
    if (point_process) {
        Lappendstr(defs_list, "static void _hoc_destroy_pnt(void* _vptr) {\n");
        if (watch_seen_ || for_netcons_) {
            Lappendstr(defs_list, "  Prop* _prop = ((Point_process*)_vptr)->_prop;\n");
        }
        if (watch_seen_) {
            Sprintf(buf,
                    "  if (_prop) { _nrn_free_watch(_nrn_mechanism_access_dparam(_prop), "
                    "%d, %d);}\n",
                    watch_index,
                    watch_seen_);
            Lappendstr(defs_list, buf);
        }
        if (for_netcons_) {
            Sprintf(buf,
                    "  if (_prop) { "
                    "_nrn_free_fornetcon(&(_nrn_mechanism_access_dparam(_prop)[_fnc_index]"
                    ".literal_value<void*>()));}\n");
            Lappendstr(defs_list, buf);
        }
        Lappendstr(defs_list, "  destroy_point_process(_vptr);\n}\n");
    }
    if (cvode_emit) {
        cvode_ieq_index = ppvar_cnt;
        ppvar_semantics(ppvar_cnt, "cvodeieq", "_cvode_ieq", "int");
        ppvar_cnt++;
    }
    cvode_emit_interface();
    if (destructorfunc->next != destructorfunc) {
        if (!point_process) {
            diag("DESTRUCTOR only permitted for POINT_PROCESS", (char*) 0);
        }
        Lappendstr(defs_list, "static void _destructor(Prop*);\n");
    }

    if (constructorfunc->next != constructorfunc) {
        Lappendstr(defs_list, "static void _constructor(Prop*);\n");
    }

    Lappendstr(defs_list,
               "/* connect range variables in _p that hoc is supposed to know about */\n");
    Lappendstr(defs_list,
               "\
static const char *_mechanism[] = {\n\
");
    Sprintf(buf, "\"%s\",\n\"%s\",\n", nmodl_version_, mechname);
    Lappendstr(defs_list, buf);
    ITERATE(q, rangeparm) {
        s = SYM(q);
        if (s->subtype & ARRAY) {
            Sprintf(buf, "\"%s%s[%d]\",\n", s->name, rsuffix, s->araydim);
        } else {
            Sprintf(buf, "\"%s%s\",\n", s->name, rsuffix);
        }
        Lappendstr(defs_list, buf);
    }
    Lappendstr(defs_list, "0,\n");
    ITERATE(q, rangedep) {
        s = SYM(q);
        if (s->subtype & ARRAY) {
            Sprintf(buf, "\"%s%s[%d]\",\n", s->name, rsuffix, s->araydim);
        } else {
            Sprintf(buf, "\"%s%s\",\n", s->name, rsuffix);
        }
        Lappendstr(defs_list, buf);
    }
    Lappendstr(defs_list, "0,\n");
    ITERATE(q, rangestate) {
        s = SYM(q);
        if (s->subtype & ARRAY) {
            Sprintf(buf, "\"%s%s[%d]\",\n", s->name, rsuffix, s->araydim);
        } else {
            Sprintf(buf, "\"%s%s\",\n", s->name, rsuffix);
        }
        Lappendstr(defs_list, buf);
    }
    Lappendstr(defs_list, "0,\n");

    /* pointer variable names */
    ITERATE(q, nrnpointers) {
        s = SYM(q);
        if (s->subtype & ARRAY) {
            Sprintf(buf, "\"%s%s[%d]\",\n", s->name, rsuffix, s->araydim);
        } else {
            Sprintf(buf, "\"%s%s\",\n", s->name, rsuffix);
        }
        Lappendstr(defs_list, buf);
    }

    Lappendstr(defs_list, "0};\n");

    /*********Creation of the allocation function*/

    if (diamdec) {
        Lappendstr(defs_list, "static Symbol* _morphology_sym;\n");
    }
    if (areadec) {
        Lappendstr(defs_list, "extern Node* nrn_alloc_node_;\n");
    }
    ITERATE(q, useion) {
        sion = SYM(q);
        Sprintf(buf, "static Symbol* _%s_sym;\n", sion->name);
        Lappendstr(defs_list, buf);
        if (ldifuslist) {
            Sprintf(buf, "static int _type_i%s;\n", sion->name);
            lappendstr(defs_list, buf);
        }
        q = q->next->next->next;
    }

    Item* before_nrn_alloc = lappendstr(defs_list, "\n");

    Lappendstr(defs_list,
               "\n"
               "extern Prop* need_memb(Symbol*);\n"
               "static void nrn_alloc(Prop* _prop) {\n"
               "  Prop *prop_ion{};\n"
               "  Datum *_ppvar{};\n");
    if (point_process) {
        Lappendstr(defs_list,
                   " if (nrn_point_prop_) {\n"
                   "    _nrn_mechanism_access_alloc_seq(_prop) = "
                   "_nrn_mechanism_access_alloc_seq(nrn_point_prop_);\n"
                   "    _ppvar = _nrn_mechanism_access_dparam(nrn_point_prop_);\n"
                   "  } else {\n");
    }
    // need to fill _prop->dparam before calling _nrn_mechanism_cache_range(Prop*)
    if (ppvar_cnt) {
        Sprintf(buf, "  _ppvar = nrn_prop_datum_alloc(_mechtype, %d, _prop);\n", ppvar_cnt);
        Lappendstr(defs_list, buf);
        Lappendstr(defs_list, "   _nrn_mechanism_access_dparam(_prop) = _ppvar;\n");
    }
    // seems that even in the old code and with vectorize == false that the global _p, _ppvar were
    // shadowed, so don't worry about shadowing the global _ml and _iml here
    Sprintf(buf,
            "    _nrn_mechanism_cache_instance _ml_real{_prop};\n"
            "    auto* const _ml = &_ml_real;\n"
            "    size_t const _iml{};\n"
            "    assert(_nrn_mechanism_get_num_vars(_prop) == %d);\n",
            parraycount);
    Lappendstr(defs_list, buf);
    Lappendstr(defs_list, "	/*initialize range parameters*/\n");
    ITERATE(q, rangeparm) {
        s = SYM(q);
        if (s->subtype & ARRAY) {
            continue;
        }
        decode_ustr(s, &d1, &d2, buf);
        Sprintf(buf, "	%s = %g;\n", s->name, d1);
        Lappendstr(defs_list, buf);
    }
    if (point_process) {
        Lappendstr(defs_list, " }\n");
    }
    Sprintf(buf, "\t assert(_nrn_mechanism_get_num_vars(_prop) == %d);\n", parraycount);
    Lappendstr(defs_list, buf);
    if (ppvar_cnt) {
        Lappendstr(defs_list, "\t_nrn_mechanism_access_dparam(_prop) = _ppvar;\n");
        Lappendstr(defs_list, "\t/*connect ionic variables to this model*/\n");
    }
    if (diamdec) {
        Sprintf(buf, "prop_ion = need_memb(_morphology_sym);\n");
        Lappendstr(defs_list, buf);
        Sprintf(buf,
                "\t_ppvar[%d] = _nrn_mechanism_get_param_handle(prop_ion, 0); /* diam */\n",
                ioncount + pointercount),
            Lappendstr(defs_list, buf);
        ppvar_semantics(ioncount + pointercount, "diam", "diam", "double*");
    }
    if (areadec) {
        Sprintf(buf,
                "\t_ppvar[%d] = _nrn_mechanism_get_area_handle(nrn_alloc_node_);\n",
                ioncount + pointercount + diamdec),
            Lappendstr(defs_list, buf);
        ppvar_semantics(ioncount + pointercount + diamdec, "area", "area", "double*");
    }

    if (point_process) {
        ioncount = 2;
    } else {
        ioncount = 0;
    }
    ITERATE(q, useion) {
        int dcurdef = 0;
        int need_style = 0;
        sion = SYM(q);
        Sprintf(buf, "prop_ion = need_memb(_%s_sym);\n", sion->name);
        Lappendstr(defs_list, buf);
        if (ldifuslist) {
            Sprintf(buf, " _type_i%s = _nrn_mechanism_get_type(prop_ion);\n", sion->name);
            lappendstr(defs_list, buf);
        }
        ion_promote(q);
        q = q->next;
        ITERATE(q1, LST(q)) {
            SYM(q1)->nrntype |= NRNIONFLAG;
            Sprintf(buf,
                    "\t_ppvar[%d] = _nrn_mechanism_get_param_handle(prop_ion, %d); /* %s */\n",
                    ioncount++,
                    iontype(SYM(q1)->name, sion->name),
                    SYM(q1)->name);
            Lappendstr(defs_list, buf);
        }
        q = q->next;
        ITERATE(q1, LST(q)) {
            int itype = iontype(SYM(q1)->name, sion->name);

            if (SYM(q1)->nrntype & NRNIONFLAG) {
                SYM(q1)->nrntype &= ~NRNIONFLAG;
            } else {
                Sprintf(buf,
                        "\t_ppvar[%d] = _nrn_mechanism_get_param_handle(prop_ion, %d); /* %s */\n",
                        ioncount++,
                        itype,
                        SYM(q1)->name);
                Lappendstr(defs_list, buf);
            }
            if (itype == IONCUR) {
                dcurdef = 1;
                Sprintf(buf,
                        "\t_ppvar[%d] = _nrn_mechanism_get_param_handle(prop_ion, %d); /* "
                        "_ion_di%sdv */\n",
                        ioncount++,
                        IONDCUR,
                        sion->name);
                Lappendstr(defs_list, buf);
            }
            if (itype == IONIN || itype == IONOUT) {
                need_style = 1;
            }
        }
        if (need_style) {
            Sprintf(buf,
                    "\t_ppvar[%d] = _nrn_mechanism_get_param_handle(prop_ion, %d); // erev %s\n",
                    ioncount++,
                    IONEREV,
                    sion->name);
            Lappendstr(defs_list, buf);
            Sprintf(buf,
                    "\t_ppvar[%d] = {neuron::container::do_not_search, "
                    "&(_nrn_mechanism_access_dparam(prop_ion)[0].literal_value<int>())}; /* "
                    "iontype for %s */\n",
                    ioncount++,
                    sion->name);
            Lappendstr(defs_list, buf);
        }
        q = q->next;
        if (!dcurdef && ldifuslist) {
            Sprintf(
                buf,
                "\t_ppvar[%d] = _nrn_mechanism_get_param_handle(prop_ion, %d); /* _ion_di%sdv */\n",
                ioncount++,
                IONDCUR,
                sion->name);
            Lappendstr(defs_list, buf);
        }
    }


    // I've put all the nrn_mech_inst_destruct here with nmodlrandoms allocation.
    // Refactor if ever things other than nmodlrandoms need it.
    nrn_mech_inst_destruct_list = newlist();
    ITERATE(q, nmodlrandoms) {
        Sprintf(buf, "_p_%s = (void*)nrnran123_newstream();\n", SYM(q)->name);
        Lappendstr(defs_list, buf);
        Sprintf(buf, "nrnran123_deletestream(%s);\n", SYM(q)->name);
        Lappendstr(nrn_mech_inst_destruct_list, buf);
    }
    if (nrn_mech_inst_destruct_list != nrn_mech_inst_destruct_list->next) {
        auto& list = nrn_mech_inst_destruct_list;
        // registration just means adding to nrn_mech_inst_destruct
        Lappendstr(defs_list, "nrn_mech_inst_destruct[_mechtype] = _mech_inst_destruct;\n");
        // boilerplate for _mech_inst_destruct
        Linsertstr(list,
                   "\nstatic void _mech_inst_destruct(Prop* _prop) {\n"
                   " Datum* _ppvar = _nrn_mechanism_access_dparam(_prop);\n");
        Lappendstr(list, "}\n");
        movelist(list->next, list->prev, procfunc);
        // need a forward declaration before nrn_alloc.
        insertstr(before_nrn_alloc, "\nstatic void _mech_inst_destruct(Prop* _prop);\n");
    }

    if (constructorfunc->next != constructorfunc) {
        Lappendstr(defs_list, "if (!nrn_point_prop_) {_constructor(_prop);}\n");
        if (vectorize) {
            Lappendstr(procfunc,
                       "\n"
                       "static void _constructor(Prop* _prop) {\n"
                       "  _nrn_mechanism_cache_instance _ml_real{_prop};\n"
                       "  auto* const _ml = &_ml_real;\n"
                       "  size_t const _iml{};\n"
                       "  Datum *_ppvar{_nrn_mechanism_access_dparam(_prop)}, *_thread{};\n"
                       "  {\n");
        } else {
            Lappendstr(procfunc,
                       "\n"
                       "static void _constructor(Prop* _prop) {\n"
                       "  neuron::legacy::set_globals_from_prop(_prop, _ml_real, _ml, _iml);\n"
                       "  _ppvar = _nrn_mechanism_access_dparam(_prop);\n"
                       "  {\n");
        }
        movelist(constructorfunc->next, constructorfunc->prev, procfunc);
        Lappendstr(procfunc, "\n}\n}\n");
    }
    Lappendstr(defs_list, "\n}\n");

    Lappendstr(defs_list, "static void _initlists();\n");
    if (cvode_emit) {
        Lappendstr(defs_list, " /* some states have an absolute tolerance */\n");
        Lappendstr(defs_list, "static Symbol** _atollist;\n");
        Lappendstr(defs_list, "static HocStateTolerance _hoc_state_tol[] = {\n");
        ITERATE(q, rangestate) {
            double d1;
            s = SYM(q);
            if (decode_tolerance(s, &d1)) {
                if (!point_process) {
                    Sprintf(buf, "{\"%s%s\", %g},\n", s->name, suffix, d1);
                } else {
                    Sprintf(buf, "{\"%s\", %g},\n", s->name, d1);
                }
                Lappendstr(defs_list, buf);
            }
        }
        Lappendstr(defs_list, "{0, 0}\n};\n");
    }

    if (net_send_seen_) {
        if (!net_receive_) {
            diag("can't use net_send if there is no NET_RECEIVE block", (char*) 0);
        }
        Sprintf(buf, "\n#define _tqitem &(_ppvar[%d])\n", tqitem_index);
        Lappendstr(defs_list, buf);
        if (net_send_delivered_) {
            insertstr(net_send_delivered_, "  if (_lflag == 1. ) {*(_tqitem) = nullptr;}\n");
        }
    }
    if (net_receive_) {
        Lappendstr(defs_list, "static void _net_receive(Point_process*, double*, double);\n");
        if (for_netcons_) {
            Lappendstr(defs_list, "extern int _nrn_netcon_args(void*, double***);\n");
        }
        if (net_init_q1_) {
            Lappendstr(defs_list, "static void _net_init(Point_process*, double*, double);\n");
        }
    }
    if (vectorize && thread_mem_init_list->next != thread_mem_init_list) {
        Lappendstr(defs_list, "static void _thread_mem_init(Datum*);\n");
    }
    if (vectorize && thread_cleanup_list->next != thread_cleanup_list) {
        Lappendstr(defs_list, "static void _thread_cleanup(Datum*);\n");
    }
    if (use_bbcorepointer) {
        lappendstr(defs_list,
                   "static void bbcore_write(double*, int*, int*, int*, _threadargsproto_);\n");
        lappendstr(defs_list,
                   "extern void hoc_reg_bbcore_write(int, void(*)(double*, int*, int*, int*, "
                   "_threadargsproto_));\n");
        lappendstr(defs_list,
                   "static void bbcore_read(double*, int*, int*, int*, _threadargsproto_);\n");
        lappendstr(defs_list,
                   "extern void hoc_reg_bbcore_read(int, void(*)(double*, int*, int*, int*, "
                   "_threadargsproto_));\n");
    }
    Lappendstr(defs_list,
               "\
extern Symbol* hoc_lookup(const char*);\n\
extern void _nrn_thread_reg(int, int, void(*)(Datum*));\n\
void _nrn_thread_table_reg(int, nrn_thread_table_check_t);\n\
extern void hoc_register_tolerance(int, HocStateTolerance*, Symbol***);\n\
extern void _cvode_abstol( Symbol**, double*, int);\n\n\
");
    Sprintf(buf,
            "extern \"C\" void _%s_reg() {\n\
	int _vectorized = %d;\n",
            modbase,
            vectorize);
    Lappendstr(defs_list, buf);
    q = lappendstr(defs_list, "");
    Lappendstr(defs_list, "_initlists();\n");

    if (suffix[0]) { /* not "nothing" */

        ITERATE(q, useion) {
            Sprintf(buf, "\tion_reg(\"%s\", %s);\n", SYM(q)->name, STR(q->next->next->next));
            Lappendstr(defs_list, buf);
            q = q->next->next->next;
        }
        if (diamdec) {
            Lappendstr(defs_list, "\t_morphology_sym = hoc_lookup(\"morphology\");\n");
        }
        ITERATE(q, useion) {
            Sprintf(buf, "\t_%s_sym = hoc_lookup(\"%s_ion\");\n", SYM(q)->name, SYM(q)->name);
            Lappendstr(defs_list, buf);
            q = q->next->next->next;
        }
        if (point_process) {
            Sprintf(buf,
                    "\
	_pointtype = point_register_mech(_mechanism,\n\
	 nrn_alloc,%s, nrn_init,\n\
	 hoc_nrnpointerindex, %d,\n\
	 _hoc_create_pnt, _hoc_destroy_pnt, _member_func);\n",
                    brkpnt_str_,
                    vectorize ? 1 + thread_data_index : 0);
            Lappendstr(defs_list, buf);
            if (destructorfunc->next != destructorfunc) {
                Lappendstr(defs_list, "	register_destructor(_destructor);\n");
            }
        } else {
            Sprintf(buf,
                    "\
	register_mech(_mechanism, nrn_alloc,%s, nrn_init, hoc_nrnpointerindex, %d);\n",
                    brkpnt_str_,
                    vectorize ? 1 + thread_data_index : 0);
            Lappendstr(defs_list, buf);
        }
        if (vectorize && thread_data_index) {
            Sprintf(buf, " _extcall_thread.resize(%d);\n", thread_data_index);
            Lappendstr(defs_list, buf);
            if (thread_mem_init_list->next != thread_mem_init_list) {
                Lappendstr(defs_list, " _thread_mem_init(_extcall_thread.data());\n");
                if (gind) {
                    Lappendstr(defs_list, " _thread1data_inuse = 0;\n");
                }
            }
        }
        Lappendstr(defs_list, "_mechtype = nrn_get_mechtype(_mechanism[1]);\n");
        if (!point_process) {
            Lappendstr(defs_list,
                       "        hoc_register_npy_direct(_mechtype, npy_direct_func_proc);\n");
        }
        lappendstr(defs_list, "    _nrn_setdata_reg(_mechtype, _setdata);\n");
        if (vectorize && thread_mem_init_list->next != thread_mem_init_list) {
            lappendstr(defs_list, "    _nrn_thread_reg(_mechtype, 1, _thread_mem_init);\n");
        }
        if (vectorize && thread_cleanup_list->next != thread_cleanup_list) {
            lappendstr(defs_list, "    _nrn_thread_reg(_mechtype, 0, _thread_cleanup);\n");
        }
        if (emit_check_table_thread) {
            lappendstr(defs_list, "    _nrn_thread_table_reg(_mechtype, _check_table_thread);\n");
        }
        if (use_bbcorepointer) {
            lappendstr(defs_list, "  hoc_reg_bbcore_write(_mechtype, bbcore_write);\n");
            lappendstr(defs_list, "  hoc_reg_bbcore_read(_mechtype, bbcore_read);\n");
        }
        if (nmodl_text) {
            lappendstr(defs_list,
                       "#if NMODL_TEXT\n  register_nmodl_text_and_filename(_mechtype);\n#endif\n");
        }
        std::sort(ppvar_data_field_strings.begin(), ppvar_data_field_strings.end());
        std::transform(ppvar_data_field_strings.begin(),
                       ppvar_data_field_strings.end(),
                       std::back_inserter(data_field_strings),
                       [](auto const& pair) { return pair.second; });
        std::string register_data_fields{"  _nrn_mechanism_register_data_fields("};
        auto const prefix_length = register_data_fields.size() +
                                   1 /* defs_list handling adds this */;
        register_data_fields.append("_mechtype");
        for (auto const& data_field_str: data_field_strings) {
            register_data_fields.append(",\n");
            register_data_fields.append(prefix_length, ' ');
            register_data_fields.append(data_field_str);
        }
        register_data_fields.append(");\n");
        lappendstr(defs_list, register_data_fields.c_str());
        Sprintf(buf, " hoc_register_prop_size(_mechtype, %d, %d);\n", parraycount, ppvar_cnt);
        Lappendstr(defs_list, buf);
        if (watch_seen_) {
            Lappendstr(defs_list, " hoc_reg_watch_allocate(_mechtype, _watch_alloc);\n");
        }
        if (ppvar_semantics_)
            ITERATE(q, ppvar_semantics_) {
                Sprintf(buf,
                        " hoc_register_dparam_semantics(_mechtype, %d, \"%s\");\n",
                        (int) q->itemtype,
                        q->element.str);
                Lappendstr(defs_list, buf);
            }
        /* Models that write concentration need their INITIAL blocks called
           before those that read the concentration or reversal potential. */
        i = 0;
        ITERATE(q, useion) {
            ITERATE(q1, LST(q->next->next)) {
                int type;
                type = iontype(SYM(q1)->name, SYM(q)->name);
                if (type == IONIN || type == IONOUT) {
                    i += 1;
                }
            }
            q = q->next->next->next;
        }
        if (i) {
            Lappendstr(defs_list, "\tnrn_writes_conc(_mechtype, 0);\n");
        }

        if (cvode_emit) {
            Lappendstr(defs_list,
                       "\
	hoc_register_cvode(_mechtype, _ode_count, _ode_map, _ode_spec, _ode_matsol);\n");
            Lappendstr(defs_list,
                       "\
	hoc_register_tolerance(_mechtype, _hoc_state_tol, &_atollist);\n");
            if (ion_synonym) {
                Lappendstr(defs_list, "	hoc_register_synonym(_mechtype, _ode_synonym);\n");
            }
        } else if (cvode_not_allowed) {
            Lappendstr(defs_list,
                       "\
	hoc_register_cvode(_mechtype, _ode_count, 0, 0, 0);\n");
        }
        if (artificial_cell) {
            if (brkpnt_exists || !net_receive_ || nrnpointers->next != nrnpointers ||
                useion->next != useion) {
                printf(
                    "Notice: ARTIFICIAL_CELL is a synonym for POINT_PROCESS which hints that it\n\
only affects and is affected by discrete events. As such it is not\n\
located in a section and is not associated with an integrator\n");
            }
            Sprintf(buf, "add_nrn_artcell(_mechtype, %d);\n", tqitem_index);
            Lappendstr(defs_list, buf);
        }
        if (net_event_seen_) {
            Lappendstr(defs_list, "add_nrn_has_net_event(_mechtype);\n");
        }
        if (net_receive_) {
            Lappendstr(defs_list, "pnt_receive[_mechtype] = _net_receive;\n");
            if (net_init_q1_) {
                Lappendstr(defs_list, "pnt_receive_init[_mechtype] = _net_init;\n");
            }
            Sprintf(buf, "pnt_receive_size[_mechtype] = %d;\n", net_receive_);
            Lappendstr(defs_list, buf);
        }
        if (for_netcons_) {
            Sprintf(buf, "add_nrn_fornetcons(_mechtype, _fnc_index);\n");
            Lappendstr(defs_list, buf);
        }
        q = ba_list_;
        for (i = 1; i <= ba_index_; ++i) {
            List* lst;
            q = q->next;
            if (electrode_current) {
                insertstr(ITM(q),
                          " \
#if EXTRACELLULAR\n\
if (auto* const _extnode = _nrn_mechanism_access_extnode(_nd); _extnode) {\n\
   v = NODEV(_nd) + _extnode->_v[0];\n\
}else\n\
#endif\n\
{\n\
   v = NODEV(_nd);\n\
}\n");
            } else {
                insertstr(ITM(q), " v = NODEV(_nd);\n");
            }
            lst = get_ion_variables(0);
            if (lst->next != lst->prev) {
                move(lst->next, lst->prev, ITM(q));
                freelist(&lst);
            }
            q = q->next;
            lst = set_ion_variables(0);
            if (lst->next != lst->prev) {
                move(lst->next, lst->prev, ITM(q));
                freelist(&lst);
            }
            q = q->next;
            Sprintf(buf, "\thoc_reg_ba(_mechtype, _ba%d, %s);\n", i, STR(q));
            Lappendstr(defs_list, buf);
        }
        if (ldifuslist) {
            Lappendstr(defs_list, "\thoc_register_ldifus1(_difusfunc);\n");
            // don't use _nrn_model_sorted_token here because this is being inserted at the start of
            // defs_list, before _nrn_model_sorted_token is defined
            Linsertstr(defs_list,
                       "static void _difusfunc(ldifusfunc2_t, neuron::model_sorted_token const&, "
                       "NrnThread&);\n");
        }
    } /* end of not "nothing" */
    Lappendstr(defs_list,
               "\n"
               "    hoc_register_var(hoc_scdoub, hoc_vdoub, hoc_intfunc);\n");
    {
        char buf1[NRN_BUFSIZE];
        char* pf{};
#if HAVE_REALPATH && !defined(NRN_AVOID_ABSOLUTE_PATHS)
        pf = realpath(finname, NULL);
#endif
        Sprintf(buf1, "\tivoc_help(\"help ?1 %s %s\\n\");\n", mechname, pf ? pf : finname);
        if (pf) {
            free(pf);
        }
        Lappendstr(defs_list, buf1);
    }
    if (suffix[0]) {
        Lappendstr(defs_list, "hoc_register_limits(_mechtype, _hoc_parm_limits);\n");
        Lappendstr(defs_list, "hoc_register_units(_mechtype, _hoc_parm_units);\n");
    }
    Lappendstr(defs_list, "}\n"); /* end of _reg */
    if (vectorize && thread_mem_init_list->next != thread_mem_init_list) {
        Lappendstr(procfunc, "\nstatic void _thread_mem_init(Datum* _thread) {\n");
        move(thread_mem_init_list->next, thread_mem_init_list->prev, procfunc);
        Lappendstr(procfunc, "}\n");
    }
    if (vectorize && thread_cleanup_list->next != thread_cleanup_list) {
        Lappendstr(procfunc, "\nstatic void _thread_cleanup(Datum* _thread) {\n");
        move(thread_cleanup_list->next, thread_cleanup_list->prev, procfunc);
        Lappendstr(procfunc, "}\n");
    }
    if (destructorfunc->next != destructorfunc) {
        if (vectorize) {
            Lappendstr(procfunc,
                       "\n"
                       "static void _destructor(Prop* _prop) {\n"
                       "  _nrn_mechanism_cache_instance _ml_real{_prop};\n"
                       "  auto* const _ml = &_ml_real;\n"
                       "  size_t const _iml{};\n"
                       "  Datum *_ppvar{_nrn_mechanism_access_dparam(_prop)}, *_thread{};\n"
                       "  {\n");
        } else {
            Lappendstr(procfunc,
                       "\n"
                       "static void _destructor(Prop* _prop) {\n"
                       "  neuron::legacy::set_globals_from_prop(_prop, _ml_real, _ml, _iml);\n"
                       "  _ppvar = _nrn_mechanism_access_dparam(_prop);\n"
                       "  {\n");
        }
        movelist(destructorfunc->next, destructorfunc->prev, procfunc);
        Lappendstr(procfunc, "\n}\n}\n");
    }
    if (ldifuslist) {
        ldifusreg();
    }
    SYMLISTITER {
        s = SYM(q);
        if ((s->subtype & PARM)) {
            warn_ignore(s);
        }
    }
}


// Check if read/write variable from USEION is declared in a
// CONSTANT block. There are certain MOD files where this pattern
// is used and it doesn't produce a desired code. Hence, we check
// all ion variables and error if any variable is declared as CONSTANT.
void check_ion_vars_as_constant(char* ion_name, const List* ion_var_list) {
    const Item* var;
    ITERATE(var, ion_var_list) {
        const Symbol* var_sym = SYM(var);
        int type = iontype(var_sym->name, ion_name);
        if (type == IONIN || type == IONOUT || type == IONCUR || type == IONCONC ||
            type == IONEREV) {
            if (var_sym->subtype & nmodlCONST) {
                diag(var_sym->name,
                     " used in USEION statement can not be re-declared in a CONSTANT block");
            }
        }
    }
}

static void check_sufficient_ion_read_statements(std::string const& ion_name,
                                                 List* read_variables,
                                                 List* write_variables) {
    auto const have_type = [ion_name, read_variables, write_variables](int type) {
        for (auto* const ion_var_list: {read_variables, write_variables}) {
            Item* var;
            ITERATE(var, ion_var_list) {
                const Symbol* var_sym = SYM(var);
                if (iontype(var_sym->name, const_cast<char*>(ion_name.c_str())) == type) {
                    return true;
                }
            }
        }
        return false;
    };
    auto const add_readion = [read_variables](std::string name) {
        auto* const sym = install(name.c_str(), NAME);
        sym->nrntype |= IONCONC;
        sym->nrntype |= IONCONC_IMPLICIT;
        lappendsym(read_variables, sym);
    };
    bool const have_ionin{have_type(IONIN)}, have_ionout{have_type(IONOUT)};
    if (have_ionin && !have_ionout) {
        add_readion(ion_name + "o");
    } else if (have_ionout && !have_ionin) {
        add_readion(ion_name + "i");
    }
}

// check semantics of read & write variables from USEION statements
void check_useion_variables() {
    const Item* ion_var;
    ITERATE(ion_var, useion) {
        // with SoA data then if we emit any calls for nrn_wrote_conc then we need explicit READ
        // statements for all arguments
        check_sufficient_ion_read_statements(SYM(ion_var)->name,
                                             LST(ion_var->next),
                                             LST(ion_var->next->next));
        // read variables
        check_ion_vars_as_constant(SYM(ion_var)->name, LST(ion_var->next));
        // write variables
        check_ion_vars_as_constant(SYM(ion_var)->name, LST(ion_var->next->next));
        ion_var = ion_var->next->next->next;
    }
}


void warn_ignore(Symbol* s) {
    int b;
    double d1, d2;
    b = 0;
    if (s->nrntype & (NRNEXTRN | NRNPRANGEIN | NRNPRANGEOUT))
        b = 1;
    if (strcmp(s->name, "v") == 0)
        b = 1;

    decode_ustr(s, &d1, &d2, buf);
    if (d1 == 0.0)
        b = 0;
    if (b) {
        printf("Warning: Default %g of PARAMETER %s will be ignored and set by NEURON.\n",
               d1,
               s->name);
    }
}

void ldifusreg() {
    Item *q, *qdexp, *qb1, *qvexp, *qb2, *q1;
    char *cfindex, *dfdcur;
    Symbol *s, *d;
    int n;

    /* ldifuslist format: series of symbol qdexp qb1 svexp qb2
            indexforflux dflux/dconc */
    n = 0;
    ITERATE(q, ldifuslist) {
        s = SYM(q);
        q = q->next;
        qdexp = ITM(q);
        q = q->next;
        qb1 = ITM(q);
        q = q->next;
        qvexp = ITM(q);
        q = q->next;
        qb2 = ITM(q);
        q = q->next;
        cfindex = STR(q);
        q = q->next;
        dfdcur = STR(q);
        ++n;
        Sprintf(
            buf,
            "static void* _difspace%d;\n"
            "extern double nrn_nernst_coef(int);\n"
            "static double _difcoef%d(int _i, Memb_list* _ml_arg, size_t _iml, Datum* _ppvar, "
            "double* _pdvol, double* _pdfcdc, Datum* _thread, NrnThread* _nt, "
            "_nrn_model_sorted_token const& _sorted_token) {\n"
            "  _nrn_mechanism_cache_range _lmr{_sorted_token, *_nt, *_ml_arg, _ml_arg->_type()};\n"
            "  auto* const _ml = &_lmr;\n"
            "  *_pdvol = ",
            n,
            n);
        lappendstr(procfunc, buf);
        for (q1 = qvexp; q1 != qb2; q1 = q1->next) {
            lappenditem(procfunc, q1);
        }
        if (dfdcur[0]) {
            Sprintf(buf,
                    ";\n\
 if (_i == %s) {\n  *_pdfcdc = %s;\n }else{ *_pdfcdc=0.;}\n",
                    cfindex,
                    dfdcur);
        } else {
            Sprintf(buf, "; *_pdfcdc=0.;\n");
        }
        lappendstr(procfunc, buf);
        lappendstr(procfunc, "  return");
        for (q1 = qdexp; q1 != qb1; q1 = q1->next) {
            lappenditem(procfunc, q1);
        }
        lappendstr(procfunc, ";\nreturn 0;\n}\n");
    }
    lappendstr(procfunc,
               "static void _difusfunc(ldifusfunc2_t _f, _nrn_model_sorted_token const& "
               "sorted_token, NrnThread& _nt) {int _i;\n");
    n = 0;
    ITERATE(q, ldifuslist) {
        s = SYM(q);
        q = q->next;
        qdexp = ITM(q);
        q = q->next;
        qb1 = ITM(q);
        q = q->next;
        qvexp = ITM(q);
        q = q->next;
        qb2 = ITM(q);
        q = q->next;
        cfindex = STR(q);
        q = q->next;
        dfdcur = STR(q);
        ++n;

        if (s->subtype & ARRAY) {
            Sprintf(buf,
                    " for (_i=0; _i < %d; ++_i) (*_f)(_mechtype, _difcoef%d, &_difspace%d, _i, ",
                    s->araydim,
                    n,
                    n);
        } else {
            Sprintf(buf, " (*_f)(_mechtype, _difcoef%d, &_difspace%d, 0, ", n, n);
        }
        lappendstr(procfunc, buf);

        Sprintf(buf, "D%s", s->name);
        d = lookup(buf);
        assert(d);
        if (s->nrntype & IONCONC) {
            Sprintf(buf, "%d, %d", -(s->ioncount_ + 1), d->varnum);
        } else {
            Sprintf(buf, "%d, %d", s->varnum, d->varnum);
        }
        lappendstr(procfunc, buf);
        lappendstr(procfunc, ", sorted_token, _nt);\n");
    }
    lappendstr(procfunc, "}\n");
}

int decode_limits(Symbol* sym, double* pg1, double* pg2) {
    int i;
    if (sym->subtype & PARM) {
        char* cp;
        int n;
        assert(sym->u.str);
        for (n = 0, cp = sym->u.str; *cp; ++cp) {
            if (*cp == '\n') {
                ++n;
                if (n == 3) {
                    ++cp;
                    break;
                }
            }
        }
        i = sscanf(cp, "%lf %lf\n", pg1, pg2);
        if (i == 2) {
            return 1;
        }
    }
    return 0;
}

int decode_tolerance(Symbol* sym, double* pg1) {
    int i;
    if (sym->subtype & STAT) {
        char* cp;
        int n;
        for (n = 0, cp = sym->u.str; *cp; ++cp) {
            if (*cp == '\n') {
                ++n;
                if (n == 3) {
                    ++cp;
                    break;
                }
            }
        }
        i = sscanf(cp, "%lf\n", pg1);
        if (i == 1) {
            return 1;
        }
    }
    return 0;
}

void decode_ustr(Symbol* sym, double* pg1, double* pg2, char* s) /* decode sym->u.str */
{
    int i, n;
    char *cp, *cp1;

    switch (sym->subtype & (INDEP | DEP | STAT | PARM)) {
    case INDEP: /* but doesnt get all info */
    case DEP:
    case STAT:
        assert(sym && sym->u.str);
        if (sym->subtype & ARRAY) { /* see parsact.c */
            i = sscanf(sym->u.str, "[%*d]\n%lf%*c%lf", pg1, pg2);
        } else {
            i = sscanf(sym->u.str, "%lf%*c%lf", pg1, pg2);
        }
        assert(i == 2);
        for (n = 0, cp = sym->u.str; n < 2;) {
            if (*cp++ == '\n') {
                n++;
            }
        }
        for (cp1 = s; *cp != '\n';) {
            *cp1++ = *cp++;
        }
        *cp1 = '\0';
        break;

    case PARM:
        assert(sym && sym->u.str);
        if (sym->subtype & ARRAY) { /* see parsact.c */
            i = sscanf(sym->u.str, "[%*d]\n%lf\n%s", pg1, s);
        } else {
            i = sscanf(sym->u.str, "%lf\n%s", pg1, s);
        }
        if (i == 1) {
            s[0] = '\0';
            i = 2;
        }
        assert(i == 2);
        break;
    default:
        diag(sym->name, " does not have a proper declaration");
    }
    if (s[0] == '0') {
        s[0] = '\0';
    }
}

void units_reg() {
    Symbol* s;
    Item* q;
    double d1, d2;
    char u[NRN_BUFSIZE];

    Lappendstr(defs_list, "static HocParmUnits _hoc_parm_units[] = {\n");
    ITERATE(q, syminorder) {
        s = SYM(q);
        if (s->nrntype & NRNGLOBAL) {
            decode_ustr(s, &d1, &d2, u);
            if (u[0]) {
                Sprintf(buf, "{\"%s%s\", \"%s\"},\n", s->name, suffix, u);
                lappendstr(defs_list, buf);
            }
        }
    }
    ITERATE(q, rangeparm) {
        s = SYM(q);
        decode_ustr(s, &d1, &d2, u);
        if (u[0]) {
            Sprintf(buf, "{\"%s%s\", \"%s\"},\n", s->name, rsuffix, u);
            lappendstr(defs_list, buf);
        }
    }
    ITERATE(q, rangestate) {
        s = SYM(q);
        decode_ustr(s, &d1, &d2, u);
        if (u[0]) {
            Sprintf(buf, "{\"%s%s\", \"%s\"},\n", s->name, rsuffix, u);
            lappendstr(defs_list, buf);
        }
    }
    ITERATE(q, rangedep) {
        s = SYM(q);
        decode_ustr(s, &d1, &d2, u);
        if (u[0]) {
            Sprintf(buf, "{\"%s%s\", \"%s\"},\n", s->name, rsuffix, u);
            lappendstr(defs_list, buf);
        }
    }
    ITERATE(q, nrnpointers) {
        s = SYM(q);
        decode_ustr(s, &d1, &d2, u);
        if (u[0]) {
            Sprintf(buf, "{\"%s%s\", \"%s\"},\n", s->name, rsuffix, u);
            lappendstr(defs_list, buf);
        }
    }
    Lappendstr(defs_list, "{0, 0}\n};\n");
}

static void var_count(Symbol* s) {
    defs_h(s);
    s->used = varcount++;
    s->varnum = parraycount;
    std::string field{"_nrn_mechanism_field<double>{\""};
    field.append(s->name);
    field.append(1, '"');
    if (s->subtype & ARRAY) {
        field.append(", ");
        field.append(std::to_string(s->araydim));
    }
    // **ATTENTION** in AoS NEURON then parraycount was incremented by s->araydim if the variable
    // was an array. In SoA NEURON this is not done; the array dimension is communicated separately.
    ++parraycount;
    field.append("} /* ");
    field.append(std::to_string(s->varnum));
    field.append(" */");
    data_field_strings.push_back(std::move(field));
}

void defs_h(Symbol* s) {
    Item* q;

    if (s->subtype & ARRAY) {
        Sprintf(buf,
                "#define %s _ml->template data_array<%d, %d>(_iml)\n"
                "#define %s_columnindex %d\n",
                s->name,
                parraycount,
                s->araydim,
                s->name,
                parraycount);
        q = lappendstr(defs_list, buf);
    } else {
        Sprintf(buf,
                "#define %s _ml->template fpfield<%d>(_iml)\n"
                "#define %s_columnindex %d\n",
                s->name,
                parraycount,
                s->name,
                parraycount);
        q = lappendstr(defs_list, buf);
    }
    q->itemtype = VERBATIM;
}

void nrn_list(Item* q1, Item* q2) {
    List** plist = (List**) 0;
    Item* q;

    switch (SYM(q1)->type) {
    case RANGE:
        plist = (List**) 0;
        for (q = q1->next; q != q2->next; q = q->next) {
            SYM(q)->nrntype |= NRNRANGE;
        }
        break;
    case SUFFIX:
        plist = (List**) 0;
        mechname = SYM(q2)->name;
        if (strcmp(SYM(q1)->name, "POINT_PROCESS") == 0) {
            point_process = 1;
        } else if (strcmp(SYM(q1)->name, "ARTIFICIAL_CELL") == 0) {
            point_process = 1;
            artificial_cell = 1;
        }
        break;
    case ELECTRODE_CURRENT:
        electrode_current = 1;
    case NONSPECIFIC:
        plist = &currents;
        for (q = q1->next; q != q2->next; q = q->next) {
            SYM(q)->nrntype |= NRNRANGE;
        }
        break;
    case GLOBAL:
        for (q = q1->next; q != q2->next; q = q->next) {
            SYM(q)->nrntype |= NRNGLOBAL | NRNNOTP;
        }
        plist = (List**) 0;
        break;
    case EXTERNAL:
        threadsafe("Use of EXTERNAL is not thread safe.");
        for (q = q1->next; q != q2->next; q = q->next) {
            SYM(q)->nrntype |= NRNEXTRN | NRNNOTP;
        }
        plist = (List**) 0;
        break;
    case POINTER:
        threadsafe("Use of POINTER is not thread safe.");
        plist = &nrnpointers;
        for (q = q1->next; q != q2->next; q = q->next) {
            SYM(q)->nrntype |= NRNNOTP | NRNPOINTER;
        }
        break;
    case BBCOREPOINTER:
        threadsafe("Use of BBCOREPOINTER is not thread safe.");
        plist = &nrnpointers;
        for (q = q1->next; q != q2->next; q = q->next) {
            SYM(q)->nrntype |= NRNNOTP | NRNBBCOREPOINTER;
        }
        use_bbcorepointer = 1;
        break;
    case RANDOM:
        for (q = q1->next; q != q2->next; q = q->next) {
            Symbol* s = SYM(q);
            if (s->type != NAME || s->subtype || s->nrntype) {
                diag(s->name, " cannot be redeclared as RANDOM");
            }
            s->nrntype |= NRNNOTP | EXTDEF_RANDOM;
            s->type = RANDOMVAR;
        }
        plist = &nmodlrandoms;
        break;
    }
    if (plist) {
        if (!*plist) {
            *plist = newlist();
        }
        assert(q1 != q2);
        movelist(q1->next, q2, *plist);
    }
}

void bablk(int ba, int type, Item* q1, Item* q2) {
    Item *qb, *qv, *q;
    qb = insertstr(q1->prev->prev, "/*");
    insertstr(q1, "*/\n");
    if (!ba_list_) {
        ba_list_ = newlist();
    }
    Sprintf(buf,
            "static void _ba%d(Node*_nd, Datum* _ppd, Datum* _thread, NrnThread* _nt, Memb_list* "
            "_ml_arg, size_t _iml, _nrn_model_sorted_token const& _sorted_token) ",
            ++ba_index_);
    insertstr(q1, buf);
    q = q1->next;
    vectorize_substitute(insertstr(q, ""), "Datum* _ppvar;");
    qv = insertstr(q,
                   "_nrn_mechanism_cache_range _lmr{_sorted_token, *_nt, *_ml_arg, "
                   "_ml_arg->_type()}; auto* const "
                   "_ml = &_lmr;\n");
    qv = insertstr(q, "_ppvar = _ppd;\n");
    movelist(qb, q2, procfunc);

    ba = (ba == BEFORE) ? 10 : 20; /* BEFORE or AFTER */
    ba += (type == BREAKPOINT) ? 1 : 0;
    ba += (type == SOLVE) ? 2 : 0;
    ba += (type == INITIAL1) ? 3 : 0;
    ba += (type == STEP) ? 4 : 0;
    lappenditem(ba_list_, qv->next);
    lappenditem(ba_list_, q2);
    Sprintf(buf, "%d", ba);
    lappendstr(ba_list_, buf);
}

int ion_declared(Symbol* s) {
    Item* q;
    int used = 0;
    ITERATE(q, useion) {
        if (SYM(q) == s) {
            used = 1;
        }
        q = q->next->next->next;
    }
    return used;
}

void nrn_use(Item* q1, Item* q2, Item* q3, Item* q4) {
    int used, i;
    Item *q, *qr, *qw;
    List *readlist, *writelist;
    Symbol* ion;

    ion = SYM(q1);
    /* is it already used */
    used = ion_declared(SYM(q1));
    if (used) { /* READ gets promoted to WRITE */
        diag("merging of neuron models not supported", (char*) 0);
    } else { /* create all the ionic variables */
        Lappendsym(useion, ion);
        readlist = newlist();
        writelist = newlist();
        qr = lappendsym(useion, SYM0);
        qw = lappendsym(useion, SYM0);
        if (q4) {
            lappendstr(useion, STR(q4));
        } else {
            lappendstr(useion, "-10000.");
        }
        LST(qr) = readlist;
        LST(qw) = writelist;
        if (q2) {
            Item* qt = q2->next;
            move(q1->next->next, q2, readlist);
            if (q3) {
                move(qt->next, q3, writelist);
            }
        } else if (q3) {
            move(q1->next->next, q3, writelist);
        }
        ITERATE(q, readlist) {
            i = iontype(SYM(q)->name, ion->name);
            if (i == IONCUR) {
                SYM(q)->nrntype |= NRNCURIN;
            } else {
                SYM(q)->nrntype |= NRNPRANGEIN;
                if (i == IONIN || i == IONOUT) {
                    SYM(q)->nrntype |= IONCONC;
                }
            }
        }
        ITERATE(q, writelist) {
            i = iontype(SYM(q)->name, ion->name);
            if (i == IONCUR) {
                if (!currents) {
                    currents = newlist();
                }
                Lappendsym(currents, SYM(q));
                SYM(q)->nrntype |= NRNCUROUT;
            } else {
                SYM(q)->nrntype |= NRNPRANGEOUT;
                if (i == IONIN || i == IONOUT) {
                    SYM(q)->nrntype |= IONCONC;
                }
            }
        }
    }
}

int iontype(char* s1, char* s2) /* returns index of variable in ion mechanism */
{
    Sprintf(buf, "i%s", s2);
    if (strcmp(buf, s1) == 0) {
        return IONCUR;
    }
    Sprintf(buf, "e%s", s2);
    if (strcmp(buf, s1) == 0) {
        return IONEREV;
    }
    Sprintf(buf, "%si", s2);
    if (strcmp(buf, s1) == 0) {
        return IONIN;
    }
    Sprintf(buf, "%so", s2);
    if (strcmp(buf, s1) == 0) {
        return IONOUT;
    }
    Sprintf(buf, "%s is not a valid ionic variable for %s", s1, s2);
    diag(buf, (char*) 0);
    return -1;
}

static Symbol* ifnew_install(const char* name) {
    Symbol* s;

    if ((s = lookup(name)) == SYM0) {
        s = install(name, NAME);
        parminstall(s, "0", "", "");
    }
    return s;
}

void nrndeclare() {
    Symbol* s;
    Item* q;

    s = lookup("diam");
    if (s) {
        if (s->nrntype & (NRNRANGE | NRNGLOBAL)) {
            diag(s->name, "cannot be a RANGE or GLOBAL variable for this mechanism");
        }
        s->nrntype |= NRNNOTP | NRNPRANGEIN;
        diamdec = 1;
    }
    s = lookup("area");
    if (s) {
        if (s->nrntype & (NRNRANGE | NRNGLOBAL)) {
            diag(s->name, "cannot be a RANGE or GLOBAL variable for this mechanism");
        }
        s->nrntype |= NRNNOTP | NRNPRANGEIN;
        areadec = 1;
    }
    if (vectorize) {
        s = ifnew_install("v");
        s->nrntype = NRNNOTP; /* this is a lie, it goes in at end specially */
    } else {
        s = ifnew_install("v");
        s->nrntype |= NRNSTATIC | NRNNOTP;
    }
    s = ifnew_install("t");
    s->nrntype |= NRNEXTRN | NRNNOTP;
    s = ifnew_install("dt");
    s->nrntype |= NRNEXTRN | NRNNOTP;
    vectorize_substitute(lappendstr(defs_list,
                                    "\n#define t nrn_threads->_t\n#define dt nrn_threads->_dt\n"),
                         "\n#define t _nt->_t\n#define dt _nt->_dt\n");

    s = lookup("usetable");
    if (s) {
        s->nrntype |= NRNGLOBAL | NRNNOTP;
    }
    s = lookup("celsius");
    if (s) {
        s->nrntype |= NRNEXTRN | NRNNOTP;
    }
    s = lookup("celcius");
    if (s)
        diag("celcius should be spelled celsius", (char*) 0);

    ITERATE(q, syminorder) {
        s = SYM(q);
        if (s->type == NAME || s->type == PRIME) {
            if (s->subtype & PARM && s->nrntype & NRNRANGE) {
                Lappendsym(rangeparm, s);
            } else if (s->subtype & STAT) {
                s->nrntype |= NRNRANGE;
                Lappendsym(rangestate, s);
            } else if (s->subtype & DEP && s->nrntype & NRNRANGE) {
                Lappendsym(rangedep, s);
            }
            if (s != indepsym && !s->nrntype) {
                if (s->subtype & PARM) {
                    if (s->usage & EXPLICIT_DECL) {
                        s->nrntype |= NRNGLOBAL;
                        s->nrntype |= NRNNOTP;
                    } else {
                        s->nrntype |= NRNSTATIC;
                        s->nrntype |= NRNNOTP;
                    }
                }
            }
        }
    }
    /* some ionic variables don't need duplicates known to hoc */
    del_range(rangeparm);
    del_range(rangestate);
    del_range(rangedep);
}

void del_range(List* range) {
    Item *q, *q1;
    Symbol* s;

    for (q = ((Item*) range)->next; q != (Item*) range; q = q1) {
        q1 = q->next;
        s = SYM(q);
        if (s->nrntype & (NRNPRANGEIN | NRNPRANGEOUT)) {
            remove(q);
        }
    }
}


void declare_p() {
    Item* q;
    Symbol* s;

    ITERATE(q, syminorder) {
        SYM(q)->used = -1;
    }
    ITERATE(q, rangeparm) {
        var_count(SYM(q));
    }
    ITERATE(q, rangedep) {
        var_count(SYM(q));
    }
    ITERATE(q, rangestate) {
        var_count(SYM(q));
    }
    ITERATE(q, syminorder) {
        if (!(SYM(q)->nrntype & NRNNOTP) && SYM(q)->used < 0) {
            var_count(SYM(q));
        }
    }
    if (vectorize) {
        s = ifnew_install("v");
        var_count(s);
    }
    if (brkpnt_exists) {
        s = ifnew_install("_g");
        var_count(s);
    }
    if (debugging_ && net_receive_) {
        s = ifnew_install("_tsav");
        var_count(s);
    }
    linsertstr(defs_list,
               "namespace {\n"
               "template <typename T>\n"
               "using _nrn_mechanism_std_vector = std::vector<T>;\n"
               "using _nrn_model_sorted_token = neuron::model_sorted_token;\n"
               "using _nrn_mechanism_cache_range = "
               "neuron::cache::MechanismRange<number_of_floating_point_variables, "
               "number_of_datum_variables>;\n"
               "using _nrn_mechanism_cache_instance = "
               "neuron::cache::MechanismInstance<number_of_floating_point_variables, "
               "number_of_datum_variables>;\n"
               "using _nrn_non_owning_id_without_container = "
               "neuron::container::non_owning_identifier_without_container;\n"
               "template <typename T>\n"
               "using _nrn_mechanism_field = neuron::mechanism::field<T>;\n"
               "template <typename... Args>\n"
               "void _nrn_mechanism_register_data_fields(Args&&... args) {\n"
               "  neuron::mechanism::register_data_fields(std::forward<Args>(args)...);\n"
               "}\n"
               "}\n")
        ->itemtype = VERBATIM;
    Sprintf(buf, "static constexpr auto number_of_floating_point_variables = %d;\n", parraycount);
    linsertstr(defs_list, buf)->itemtype = VERBATIM;
}

List* set_ion_variables(int block)
/* 0 means equation block , 2 means initial block */
{
    /*ARGSUSED*/
    Item *q, *q1, *qconc;
    char* in;
    static List* l;

    l = newlist();
    ITERATE(q, useion) {
        in = SYM(q)->name;
        q = q->next;
        q = q->next;
        qconc = (Item*) 0;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & NRNCUROUT) {
                if (block == 0) {
                    Sprintf(buf,
                            " _ion_%s += %s",
                            SYM(q1)->name,
                            breakpoint_current(SYM(q1))->name);
                    Lappendstr(l, buf);
                    if (point_process) {
                        Sprintf(buf, "* 1.e2/ (_nd_area);\n");
                    } else {
                        Sprintf(buf, ";\n");
                    }
                } else {
                    buf[0] = '\0';
                }
            } else {
                if (iontype(SYM(q1)->name, in) != IONEREV) {
                    qconc = q1;
                }
                Sprintf(buf, " _ion_%s = %s;\n", SYM(q1)->name, SYM(q1)->name);
            }
            Lappendstr(l, buf);
        }
        q = q->next;
        /* when INITIAL block is called, if it modifies the concentrations
           then the reversal potential should be recomputed in case
           other mechanisms need the true initial value. This would be
           rare since most initial blocks do not depend on erev. Instead
           the right value will be present due to fcurrent or cvode f(y).
           However, this fastidiousness cant hurt. It just makes ion_style
           in effect always at least for initialization.
        */
        /* sure enough, someone needed to demote the ion_style so
           that erev is decoupled from concentrations. So we need
           another variable pointing to the ionstyle
        */
        if (block == 2 && qconc) {
            int const ic = iontype(SYM(qconc)->name, in);
            assert(ic == IONIN || ic == IONOUT);
            // first arg is just for the charge, last arg is the style. the old
            // code with a single double* as a 2nd parameter was problematic as
            // it implicitly assumed AoS format; now we require that explicit
            // names are defined for erev and the internal/external concentrations
            Sprintf(buf,
                    " nrn_wrote_conc(_%s_sym, _ion_%s_erev, _ion_%si, _ion_%so, _style_%s);\n",
                    in,
                    in,
                    in,
                    in,
                    in);
            Lappendstr(l, buf);
        }
    }
    return l;
}

List* get_ion_variables(int block)
/* 0 means equation block */
/* 2 means ode_spec and ode_matsol blocks */
{
    /*ARGSUSED*/
    Item *q, *q1;
    static List* l;

    l = newlist();
    ITERATE(q, useion) {
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & IONCONC_IMPLICIT) {
                continue;
            }
            if (block == 2 && (SYM(q1)->nrntype & IONCONC) && (SYM(q1)->subtype & STAT)) {
                continue;
            }
            Sprintf(buf, " %s = _ion_%s;\n", SYM(q1)->name, SYM(q1)->name);
            Lappendstr(l, buf);
            if (point_process && (SYM(q1)->nrntype & NRNCURIN)) {
                Fprintf(stderr,
                        "WARNING: Dimensions may be wrong for READ %s with POINT_PROCESS\n",
                        SYM(q1)->name);
            }
        }
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & IONCONC_IMPLICIT) {
                continue;
            }
            if (block == 2 && (SYM(q1)->nrntype & IONCONC) && (SYM(q1)->subtype & STAT)) {
                continue;
            }
            if (SYM(q1)->nrntype & IONCONC) {
                Sprintf(buf, " %s = _ion_%s;\n", SYM(q1)->name, SYM(q1)->name);
                Lappendstr(l, buf);
            }
            if (SYM(q1)->subtype & STAT) {
                if (SYM(q1)->nrntype & NRNCUROUT) {
                    Fprintf(stderr,
                            "WARNING: WRITE %s with it a STATE may not be translated correctly\n",
                            SYM(q1)->name);
                }
            }
        }
        q = q->next;
    }
    return l;
}

int iondef(int* p_pointercount) {
    int ioncount, it, need_style;
    Item *q, *q1, *q2;
    Symbol* sion;
    char ionname[256];

    ioncount = 0;
    if (point_process) {
        ioncount = 2;
        q = lappendstr(defs_list, "#define _nd_area *_ml->dptr_field<0>(_iml)\n");
        q->itemtype = VERBATIM;
        ppvar_semantics(0, "area", "_nd_area", "double*");
        ppvar_semantics(1, "pntproc", "_pntproc" /* I made this up*/, "Point_process*");
    }
    ITERATE(q, useion) {
        int dcurdef = 0;
        need_style = 0;
        sion = SYM(q);
        Sprintf(ionname, "%s_ion", sion->name);
        q = q->next;
        ITERATE(q1, LST(q)) {
            SYM(q1)->nrntype |= NRNIONFLAG;
            std::string name{"_ion_"};
            name.append(SYM(q1)->name);
            Sprintf(
                buf,
                "#define %s *(_ml->dptr_field<%d>(_iml))\n"
                "#define _p%s static_cast<neuron::container::data_handle<double>>(_ppvar[%d])\n",
                name.c_str(),
                ioncount,
                name.c_str(),
                ioncount);
            q2 = lappendstr(defs_list, buf);
            q2->itemtype = VERBATIM;
            SYM(q1)->ioncount_ = ioncount;
            ppvar_semantics(ioncount,
                            ionname,
                            ("_ion_" + std::string{SYM(q1)->name}).c_str(),
                            "double*");
            ioncount++;
        }
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & NRNIONFLAG) {
                SYM(q1)->nrntype &= ~NRNIONFLAG;
            } else {
                std::string name{"_ion_"};
                name.append(SYM(q1)->name);
                Sprintf(buf,
                        "#define %s *(_ml->dptr_field<%d>(_iml))\n"
                        "#define _p%s "
                        "static_cast<neuron::container::data_handle<double>>(_ppvar[%d])\n",
                        name.c_str(),
                        ioncount,
                        name.c_str(),
                        ioncount);
                q2 = lappendstr(defs_list, buf);
                q2->itemtype = VERBATIM;
                SYM(q1)->ioncount_ = ioncount;
                ppvar_semantics(ioncount, ionname, name.c_str(), "double*");
                ioncount++;
            }
            it = iontype(SYM(q1)->name, sion->name);
            if (it == IONCUR) {
                dcurdef = 1;
                std::string name{"_ion_di"};
                name.append(sion->name);
                name.append("dv");
                Sprintf(buf, "#define %s *(_ml->dptr_field<%d>(_iml))\n", name.c_str(), ioncount);
                q2 = lappendstr(defs_list, buf);
                q2->itemtype = VERBATIM;
                ppvar_semantics(ioncount, ionname, name.c_str(), "double*");
                ioncount++;
            }
            if (it == IONIN || it == IONOUT) { /* would have wrote_ion_conc */
                need_style = 1;
            }
        }
        if (need_style) {
            // Need to be able to explicitly reference this when calling
            // nrn_wrote_conc, the old code navigated to this value via pointer
            // arithmetic that is not valid now the mechanism data are stored in
            // SOA format
            std::string name{"_ion_"};
            name.append(sion->name);
            name.append("_erev");
            Sprintf(buf, "#define %s *_ml->dptr_field<%d>(_iml)\n", name.c_str(), ioncount);
            q2 = lappendstr(defs_list, buf);
            q2->itemtype = VERBATIM;
            ppvar_semantics(ioncount, ionname, name.c_str(), "double*");
            ioncount++;
            std::string stylename{"_style_"};
            stylename.append(sion->name);
            Sprintf(buf, "#define %s\t*_ppvar[%d].get<int*>()\n", stylename.c_str(), ioncount);
            q2 = lappendstr(defs_list, buf);
            q2->itemtype = VERBATIM;
            Sprintf(buf, "#%s", ionname);
            ppvar_semantics(ioncount, buf, stylename.c_str(), "int*");
            ioncount++;
        }
        q = q->next;
        if (!dcurdef && ldifuslist) {
            std::string name{"_ion_di"};
            name.append(sion->name);
            name.append("dv");
            Sprintf(buf, "#define %s *_ml->dptr_field<%d>(_iml)\n", name.c_str(), ioncount);
            q2 = lappendstr(defs_list, buf);
            q2->itemtype = VERBATIM;
            ppvar_semantics(ioncount, ionname, name.c_str(), "double*");
            ioncount++;
        }
    }
    *p_pointercount = 0;
    ITERATE(q, nrnpointers) {
        sion = SYM(q);
        Sprintf(buf,
                "#define %s	*_ppvar[%d].get<double*>()\n",
                sion->name,
                ioncount + *p_pointercount);
        sion->used = ioncount + *p_pointercount;
        q2 = lappendstr(defs_list, buf);
        q2->itemtype = VERBATIM;
        Sprintf(buf,
                "#define _p_%s _ppvar[%d].literal_value<void*>()\n",
                sion->name,
                ioncount + *p_pointercount);
        sion->used = ioncount + *p_pointercount;
        q2 = lappendstr(defs_list, buf);
        q2->itemtype = VERBATIM;
        if (sion->nrntype & NRNPOINTER) {
            ppvar_semantics(ioncount + *p_pointercount, "pointer", sion->name, "double*");
        } else {
            ppvar_semantics(ioncount + *p_pointercount, "bbcorepointer", sion->name, "double*");
        }
        (*p_pointercount)++;
    }

    // print all RANDOM variables
    num_random_vars = 0;
    ITERATE(q, nmodlrandoms) {
        num_random_vars++;
    }
    if (num_random_vars) {
        Sprintf(buf, "\n //RANDOM variables \n");
        lappendstr(defs_list, buf);

        int index = 0;
        ITERATE(q, nmodlrandoms) {
            Symbol* s = SYM(q);
            Sprintf(buf,
                    "#define %s	(nrnran123_State*)_ppvar[%d].get<void*>()\n",
                    s->name,
                    ioncount + *p_pointercount + index);
            lappendstr(defs_list, buf);
            Sprintf(buf,
                    "#define _p_%s _ppvar[%d].literal_value<void*>()\n",
                    s->name,
                    ioncount + *p_pointercount + index);
            lappendstr(defs_list, buf);
            ppvar_semantics(ioncount + *p_pointercount + index, "random", s->name, "void*");
            index++;
        }
        lappendstr(defs_list, "\n");
    }

    if (diamdec) { /* must be last */
        Sprintf(buf,
                "#define diam	*_ppvar[%d].get<double*>()\n",
                ioncount + *p_pointercount + num_random_vars);
        q2 = lappendstr(defs_list, buf);
        q2->itemtype = VERBATIM;
    }              /* notice that ioncount is not incremented */
    if (areadec) { /* must be last, if we add any more the administrative
            procedures must be redone */
        Sprintf(buf,
                "#define area	*_ppvar[%d].get<double*>()\n",
                ioncount + *p_pointercount + num_random_vars + diamdec);
        q2 = lappendstr(defs_list, buf);
        q2->itemtype = VERBATIM;
    } /* notice that ioncount is not incremented */
    Sprintf(buf,
            "static constexpr auto number_of_datum_variables = %d;\n",
            ioncount + *p_pointercount + num_random_vars + diamdec + areadec);
    linsertstr(defs_list, buf)->itemtype = VERBATIM;
    return ioncount;
}

void ppvar_semantics(int i, const char* semantics, const char* name, const char* type) {
    Item* q;
    if (!ppvar_semantics_) {
        ppvar_semantics_ = newlist();
    }
    q = Lappendstr(ppvar_semantics_, const_cast<char*>(semantics));  // TODO - ugly but ok for now
    q->itemtype = (short) i;
    std::string field{"_nrn_mechanism_field<"};
    field.append(type);
    field.append(">{\"");
    field.append(name);
    field.append("\", \"");
    field.append(semantics);
    field.append("\"} /* ");
    field.append(std::to_string(i));
    field.append(" */");
    // track the index because ppvar_semantics(...) is not necessarily called in order
    ppvar_data_field_strings.emplace_back(i, std::move(field));
}

List* begin_dion_stmt() {
    Item *q, *q1, *qbrak;
    static List* l;
    char* strion;

    l = newlist();
    qbrak = lappendstr(l, "\t{");
    ITERATE(q, useion) {
        strion = SYM(q)->name;
        q = q->next;
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & NRNCUROUT) {
                Sprintf(buf, " _di%s = %s;\n", strion, SYM(q1)->name);
                Lappendstr(l, buf);
                Sprintf(buf, "double _di%s;\n", strion);
                Insertstr(qbrak->next, buf);
            }
        }
        q = q->next;
    }
    return l;
}

List* end_dion_stmt(const char* strdel) {
    Item *q, *q1;
    static List* l;
    char* strion;

    l = newlist();
    ITERATE(q, useion) {
        strion = SYM(q)->name;
        q = q->next;
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & NRNCUROUT) {
                Sprintf(
                    buf, " _ion_di%sdv += (_di%s - %s)/%s", strion, strion, SYM(q1)->name, strdel);
                Lappendstr(l, buf);
                if (point_process) {
                    Lappendstr(l, "* 1.e2/ (_nd_area);\n");
                } else {
                    Lappendstr(l, ";\n");
                }
            }
        }
        q = q->next;
    }
    Lappendstr(l, "\t}\n");
    return l;
}

void ion_promote(Item* qion) {
    Item* q;
    char* in;
    int conc, rev;
    int type;
    conc = 0;
    rev = 0;
    in = SYM(qion)->name;
    ITERATE(q, LST(qion->next)) { /* check READ */
        type = iontype(SYM(q)->name, in);
        if (type == IONIN || type == IONOUT) {
            conc = 1;
        }
        if (type == IONEREV) {
            rev = 1;
        }
    }
    ITERATE(q, LST(qion->next->next)) { /* promote if WRITE */
        type = iontype(SYM(q)->name, in);
        if (type == IONIN) {
            Lappendstr(defs_list, "nrn_check_conc_write(_prop, prop_ion, 1);\n");
            conc = 3;
        }
        if (type == IONOUT) {
            Lappendstr(defs_list, "nrn_check_conc_write(_prop, prop_ion, 0);\n");
            conc = 3;
        }
        if (type == IONEREV) {
            rev = 3;
        }
    }
    if (conc || rev) {
        Sprintf(buf, "nrn_promote(prop_ion, %d, %d);\n", conc, rev);
        Lappendstr(defs_list, buf);
    }
}

#define NRNFIX(arg)          \
    if (strcmp(n, arg) == 0) \
        e = 1;

void nrn_var_assigned(Symbol* s) {
    int e;
    char* n;
    if (s->assigned_to_ == 0) {
        s->assigned_to_ = 1;
    }
    if (protect_) {
        s->assigned_to_ = 2;
    }
    e = 0;
    n = s->name;
    NRNFIX("area");
    NRNFIX("diam");
    NRNFIX("t");
    NRNFIX("dt");
    NRNFIX("celsius");
    if (e) {
        diag(s->name,
             "is a special NEURON variable that should not be\n assigned a value\
 in a model description file\n");
    }
}


static int cvode_valid_, using_cvode;
static int cvode_num_, cvode_neq_;
static Symbol* cvode_fun_;

void slist_data(Symbol* s, int indx, int findx) {
    /* format: number of pairs, followed by findx, indx pairs */
    int* pi;
    int i, n;
    if (s->slist_info_) {
        /* i'd use realloc but to avoid portability problems */
        /* this probably will never get executed anyway */
        n = s->slist_info_[0] + 1;
        pi = (int*) emalloc((1 + 2 * n) * sizeof(int));
        for (i = 2 * (n - 1); i > 0; --i) {
            pi[i] = s->slist_info_[i];
        }
        free(s->slist_info_);
        s->slist_info_ = pi;
        pi[0] = n;
        pi[2 * n - 1] = findx;
        pi[2 * n] = indx;
    } else {
        s->slist_info_ = pi = (int*) emalloc(3 * sizeof(int));
        pi[0] = 1;
        pi[1] = findx;
        pi[2] = indx;
    }
}

int slist_search(int n, Symbol* s) {
    int i, *pi;
    pi = s->slist_info_;
    if (pi == (int*) 0) {
        diag(s->name, "not really a STATE; Ie. No differential equation for it.\n");
    }
    assert(pi);
    for (i = 0; i < pi[0]; ++i) {
        if (pi[1 + 2 * i] == n) {
            return pi[2 + 2 * i];
        }
    }
    assert(0);
    return 0;
}

static void cvode_conc_map() {
    /* pv index is slist index, ppd index is to the concentration
    pointer to the ion concentration is eg. &(ion_cai). Unfortunately
    the slist index has nothing to do with the _p array index.
    To recover the slist index, an slist_index list was made for
    every slist which consists of an slist ordered list of state symbols
    */
    /*
       also must handle case where user WRITE cai but cai is not a STATE
       since inefficiency occurs due to inability to set eca when
       states are predicted
    */
    Item *q, *q1, *q2, *q3;
    int sindex;
    ITERATE(q, useion) {
        q = q->next;
        q = q->next;
        ITERATE(q1, LST(q)) {
            if (SYM(q1)->nrntype & IONCONC) {
                if ((SYM(q1)->subtype & STAT)) {
                    sindex = slist_search(cvode_num_, SYM(q1));
                    Sprintf(buf, "\t_pv[%d] = _p_ion_%s;\n", sindex, SYM(q1)->name);
                    lappendstr(procfunc, buf);
                } else { /* not a STATE but WRITE it*/
                         /*its got to have an assignment in a SOLVE block and that assignment
                         better not depend on intermediate variables that depend on states
                         because we will assign cai using only that statement prior to
                         calling the nernst equation code.
                         */
                    int b = 0;
                    if (!ion_synonym) {
                        ion_synonym = newlist();
                    }
                    ITERATE(q2, procfunc) {
                        if (q2->itemtype == SYMBOL && SYM(q2) == SYM(q1)) {
                            q3 = q2->next;
                            if (q3->itemtype == SYMBOL && strcmp(SYM(q3)->name, "=") == 0) {
                                /*printf(" found reference to %s = ...\n", SYM(q2)->name);*/
                                Sprintf(buf, "_ion_%s = ", SYM(q2)->name);
                                lappendstr(ion_synonym, buf);
                                for (q3 = q3->next; q3 != procfunc->prev; q3 = q3->next) {
                                    lappenditem(ion_synonym, q3);
                                    if (q3->itemtype == SYMBOL && SYM(q3) == semi) {
#if 0
				if (q3->itemtype == STRING && strchr(STR(q3), ';')) {
					char* e, *s = stralloc(STR(q3), (char*)0);
					e = strchr(s, ';');
					*e = '\0';
					Sprintf(buf, "%s;\n", s);
printf("|%s||%s||%s|\n",STR(q3), s, buf);
					lappendstr(ion_synonym, buf);
#endif
                                        b = 1;
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    if (b == 0) {
                        diag(SYM(q1)->name,
                             "is WRITE but is not a STATE and has no assignment statement");
                    }
                }
            }
        }
        q = q->next;
    }
}

void out_nt_ml_frag(List* p) {
    vectorize_substitute(lappendstr(p, ""), "  Datum* _ppvar;\n");
    vectorize_substitute(lappendstr(p, ""), "  size_t _iml;");
    vectorize_substitute(lappendstr(p, ""), "  _nrn_mechanism_cache_range* _ml;");
    Lappendstr(p,
               "  Node* _nd{};\n"
               "  double _v{};\n"
               "  int _cntml;\n"
               "  _nrn_mechanism_cache_range _lmr{_sorted_token, *_nt, *_ml_arg, _type};\n"
               "  _ml = &_lmr;\n"
               "  _cntml = _ml_arg->_nodecount;\n"
               "  Datum *_thread{_ml_arg->_thread};\n"
               "  for (_iml = 0; _iml < _cntml; ++_iml) {\n"
               "    _ppvar = _ml_arg->_pdata[_iml];\n"
               "    _nd = _ml_arg->_nodelist[_iml];\n"
               "    v = NODEV(_nd);\n");
}

void cvode_emit_interface() {
    List* lst;
    Item *q, *q1;
    if (cvode_not_allowed) {
        Lappendstr(defs_list,
                   "\n\
static int _ode_count(int);\n");
        Sprintf(buf,
                "\n\
static int _ode_count(int _type){ hoc_execerror(\"%s\", \"cannot be used with CVODE\"); return 0;}\n",
                mechname);
        Lappendstr(procfunc, buf);
    } else if (cvode_emit) {
        Lappendstr(defs_list,
                   "\n\
static int _ode_count(int);\n\
static void _ode_map(Prop*, int, neuron::container::data_handle<double>*, neuron::container::data_handle<double>*, double*, int);\n\
static void _ode_spec(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);\n\
static void _ode_matsol(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);\n\
");
        Sprintf(buf,
                "\n\
static int _ode_count(int _type){ return %d;}\n",
                cvode_neq_);
        Lappendstr(procfunc, buf);
        Sprintf(buf, "\n#define _cvode_ieq _ppvar[%d].literal_value<int>()\n", cvode_ieq_index);
        Lappendstr(defs_list, buf);

        if (cvode_fun_->subtype == PROCED) {
            cvode_proced_emit();
        } else {
            Lappendstr(procfunc,
                       "\nstatic void _ode_spec(_nrn_model_sorted_token const& _sorted_token, "
                       "NrnThread* _nt, Memb_list* _ml_arg, int _type) {\n");
            out_nt_ml_frag(procfunc);
            lst = get_ion_variables(1);
            if (lst->next->itemtype)
                movelist(lst->next, lst->prev, procfunc);
            Sprintf(buf, "    _ode_spec%d", cvode_num_);
            Lappendstr(procfunc, buf);
            vectorize_substitute(lappendstr(procfunc, "();\n"), "(_threadargs_);\n");
            lst = set_ion_variables(1);
            if (lst->next->itemtype)
                movelist(lst->next, lst->prev, procfunc);
            Lappendstr(procfunc, "}}\n");

            Lappendstr(procfunc,
                       "\n\
static void _ode_map(Prop* _prop, int _ieq, neuron::container::data_handle<double>* _pv, neuron::container::data_handle<double>* _pvdot, double* _atol, int _type) {");
            vectorize_substitute(lappendstr(procfunc, "\n"), "\n  Datum* _ppvar;\n");
            Sprintf(buf,
                    " _ppvar = _nrn_mechanism_access_dparam(_prop);\n"
                    "  _cvode_ieq = _ieq;\n"
                    "  for (int _i=0; _i < %d; ++_i) {\n"
                    "    _pv[_i] = _nrn_mechanism_get_param_handle(_prop, _slist%d[_i]);\n"
                    "    _pvdot[_i] = _nrn_mechanism_get_param_handle(_prop, _dlist%d[_i]);\n"
                    "    _cvode_abstol(_atollist, _atol, _i);\n"
                    "  }\n",
                    cvode_neq_,
                    cvode_num_,
                    cvode_num_);
            Lappendstr(procfunc, buf);
            /* need to take care of case where a state is an ion concentration. Replace
            the _pp pointer with a pointer to the actual ion model's concentration */
            cvode_conc_map();
            Lappendstr(procfunc, "}\n");
            if (ion_synonym) {
                Lappendstr(defs_list,
                           "static void _ode_synonym(_nrn_model_sorted_token const&, "
                           "NrnThread&, Memb_list&, int);\n");
                Lappendstr(procfunc,
                           "static void _ode_synonym(_nrn_model_sorted_token const& "
                           "_sorted_token, NrnThread& _nt, Memb_list& _ml_arg, int _type) {\n");
                Lappendstr(procfunc,
                           "_nrn_mechanism_cache_range _lmr{_sorted_token, _nt, _ml_arg, _type};\n"
                           "auto* const _ml = &_lmr;\n"
                           "auto const _cnt = _ml_arg._nodecount;\n"
                           "for (int _iml = 0; _iml < _cnt; ++_iml) {\n"
                           "  Datum* _ppvar = _ml_arg._pdata[_iml];\n");
                movelist(ion_synonym->next, ion_synonym->prev, procfunc);
                Lappendstr(procfunc, "  }\n}\n");
            }

            Sprintf(buf,
                    "static void _ode_matsol_instance%d(_internalthreadargsproto_);\n",
                    cvode_num_);
            Lappendstr(defs_list, buf);
            Sprintf(buf,
                    "\nstatic void _ode_matsol_instance%d(_internalthreadargsproto_) {\n",
                    cvode_num_);
            Lappendstr(procfunc, buf);
            if (cvode_fun_->subtype == KINF) {
                int i = cvode_num_;
                Sprintf(buf,
                        "_cvode_sparse(&_cvsparseobj%d, %d, _dlist%d, "
                        "neuron::scopmath::row_view{_ml, _iml}, "
                        "_ode_matsol%d, &_coef%d);\n",
                        i,
                        cvode_neq_,
                        i,
                        i,
                        i);
                Lappendstr(procfunc, buf);
                Sprintf(buf,
                        "_cvode_sparse_thread(&(_thread[_cvspth%d].literal_value<void*>()), %d, "
                        "_dlist%d, neuron::scopmath::row_view{_ml, _iml}, _ode_matsol%d, "
                        "_threadargs_);\n",
                        i,
                        cvode_neq_,
                        i,
                        i);
                vectorize_substitute(procfunc->prev, buf);
            } else {
                Sprintf(buf, "_ode_matsol%d", cvode_num_);
                Lappendstr(procfunc, buf);
                vectorize_substitute(lappendstr(procfunc, "();\n"), "(_threadargs_);\n");
            }
            Lappendstr(procfunc, "}\n");
            Lappendstr(procfunc,
                       "\nstatic void _ode_matsol(_nrn_model_sorted_token const& _sorted_token, "
                       "NrnThread* _nt, Memb_list* _ml_arg, int _type) {\n");
            out_nt_ml_frag(procfunc);
            lst = get_ion_variables(1);
            if (lst->next->itemtype)
                movelist(lst->next, lst->prev, procfunc);
            Sprintf(buf, "_ode_matsol_instance%d(_threadargs_);\n", cvode_num_);
            Lappendstr(procfunc, buf);
            Lappendstr(procfunc, "}}\n");
        }
        /* handle the state_discontinuities  (obsolete in NET_RECEIVE)*/
        if (state_discon_list_)
            ITERATE(q, state_discon_list_) {
                Symbol* s;
                int sindex;
                q1 = ITM(q);
                s = SYM(q1);
                if (q1->itemtype == SYMBOL && (s->subtype & STAT)) {
                    sindex = slist_search(cvode_num_, s);
                    Sprintf(buf, "_cvode_ieq + %d, &", sindex);
                    replacstr(q1->prev, buf);
                }
            }
    }
}

void cvode_proced_emit() {
    Sprintf(buf,
            "\n\
static void _ode_spec(Node* _nd, double* _pp, Datum* _ppd) {\n\
	_p = _pp; _ppvar = _ppd; v = NODEV(_nd);\n\
	%s();\n}\n",
            cvode_fun_->name);

    Lappendstr(procfunc, buf);
    Sprintf(buf,
            "\n\
static void _ode_map(int _ieq, double** _pv, doubl** _pvdot, double* _pp){}\n");
    Lappendstr(procfunc, buf);

    Lappendstr(procfunc,
               "\n\
static void _ode_matsol(Node* _nd, double* _pp, Datum* _ppd){}\n");
}

void cvode_interface(Symbol* fun, int num, int neq) {
    /* if only one then allowed and emit */
    cvode_valid_ = 1;
    cvode_not_allowed = (using_cvode++) ? 1 : 0;
    cvode_emit = !cvode_not_allowed;
    cvode_num_ = num;
    cvode_neq_ = neq;
    cvode_fun_ = fun;
    if (cvode_fun_->subtype == PROCED) {
        cvode_emit = 0;
        return;
    }
    Sprintf(buf,
            "\n\
static int _ode_spec%d(_internalthreadargsproto_);\n\
/*static int _ode_matsol%d(_internalthreadargsproto_);*/\n\
",
            num,
            num);
    Linsertstr(procfunc, buf);
}

void cvode_valid() {
    static int once;
    if (!cvode_valid_ && !once++) {
        Fprintf(stderr, "Notice: This mechanism cannot be used with CVODE\n");
        cvode_not_allowed = 1;
    }
    cvode_valid_ = 0;
}

void cvode_rw_cur(char (&b)[NRN_BUFSIZE]) {
    /* if a current is READ and WRITE then call the correct _ode_spec
       since it may compute some aspect of the current */
    Item *q, *q1;
    int type;
    b[0] = '\0';
    ITERATE(q, useion) {
        q = q->next;
        ITERATE(q1, LST(q)) {
            type = SYM(q1)->nrntype;
            if ((type & NRNCURIN) && (type & NRNCUROUT)) {
                if (!cvode_not_allowed && cvode_emit) {
                    if (vectorize) {
                        Sprintf(b, "if (_nt->_vcv) { _ode_spec%d(_threadargs_); }\n", cvode_num_);
                    } else {
                        Sprintf(b, "if (_nt->_vcv) { _ode_spec%d(); }\n", cvode_num_);
                    }
                    return;
                }
            }
        }
        q = q->next;
        q = q->next;
    }
}

void net_receive(Item* qarg, Item* qp1, Item* qp2, Item* qstmt, Item* qend) {
    Item *q, *q1;
    Symbol* s;
    int i;
    char snew[256];
    if (net_receive_) {
        diag("Only one NET_RECEIVE block allowed", (char*) 0);
    }
    if (!point_process) {
        diag("NET_RECEIVE can only exist in a POINT_PROCESS", (char*) 0);
    }
    net_receive_ = 1;
    deltokens(qp1, qp2);
    insertstr(qstmt, "(Point_process* _pnt, double* _args, double _lflag)");
    i = 0;
    ITERATE(q1, qarg) if (q1->next != qarg) { /* skip last "flag" arg */
        s = SYM(q1);
        Sprintf(snew, "_args[%d]", i);
        ++i;
        for (q = qstmt; q != qend; q = q->next) {
            if (q->itemtype == SYMBOL && SYM(q) == s) {
                replacstr(q, snew);
            }
        }
    }
    net_send_delivered_ = qstmt;
    q = insertstr(qstmt, "\n{");
    vectorize_substitute(q, "\n{  Prop* _p; Datum* _ppvar; Datum* _thread; NrnThread* _nt;\n");
    if (watch_seen_) {
        insertstr(qstmt, "  int _watch_rm = 0;\n");
    }
    vectorize_substitute(
        insertstr(qstmt,
                  "  neuron::legacy::set_globals_from_prop(_pnt->_prop, _ml_real, _ml, _iml);\n"),
        "  _nrn_mechanism_cache_instance _ml_real{_pnt->_prop};\n"
        "  auto* const _ml = &_ml_real;\n"
        "  size_t const _iml{};\n");
    q = insertstr(qstmt, "  _ppvar = _nrn_mechanism_access_dparam(_pnt->_prop);\n");
    vectorize_substitute(insertstr(q, ""), "  _thread = nullptr; _nt = (NrnThread*)_pnt->_vnt;");
    if (debugging_) {
        if (0) {
            insertstr(qstmt, " assert(_tsav <= t); _tsav = t;");
        } else {
            insertstr(qstmt,
                      " if (_tsav > t){ hoc_execerror(hoc_object_name(_pnt->ob), \":Event arrived "
                      "out of order. Must call ParallelContext.set_maxstep AFTER assigning minimum "
                      "NetCon.delay\");}\n _tsav = t;");
        }
    }
    insertstr(qend, "}");
    if (!artificial_cell) {
        Symbol* ions[10];
        int j, nion = 0;
        /* v can be changed in the NET_RECEIVE block since it is
           called between integrator steps and before a re_init
           But no need to do so if it is not used.
        */
        Symbol* vsym = lookup("v");
        netrec_need_v = 1;
        for (q = qstmt; q != qend; q = q->next) {
            if (q->itemtype == SYMBOL && SYM(q) == vsym) {
                insertstr(qstmt, " v = NODEV(_pnt->node);\n");
                insertstr(qend, "\n NODEV(_pnt->node) = v;\n");
                netrec_need_v = 0;
                break;
            }
        }
        /* if an ion concentration
        is mentioned then we need to get the relevant value
        on entry and possibly set a value on exit
        Do not allow mention of reversal potential or current
        */
        for (q = qstmt; q != qend; q = q->next) {
            if (q->itemtype == SYMBOL && SYM(q)->type == NAME) {
                s = SYM(q);
                if ((s->nrntype & (NRNPRANGEIN | NRNPRANGEOUT)) == 0) {
                    continue;
                }
                if ((s->nrntype & IONCONC) == 0) {
                    diag(s->name, ":only concentrations can be mentioned in a NET_RECEIVE block");
                }
                /* distinct only */
                for (j = 0; j < nion; ++j) {
                    if (s == ions[j]) {
                        break;
                    }
                }
                if (j == nion) {
                    if (nion >= 10) {
                        diag("too many ions mentioned in NET_RECEIVE block (limit 10", (char*) 0);
                    }
                    ions[nion] = s;
                    ++nion;
                }
            }
        }
        for (j = 0; j < nion; ++j) {
            Sprintf(
                buf, "%s   %s = _ion_%s;\n", (j == 0) ? "\n" : "", ions[j]->name, ions[j]->name);
            insertstr(qstmt, buf);
        }
        for (j = 0; j < nion; ++j) {
            if (ions[j]->subtype & STAT) {
                Sprintf(buf,
                        "%s   _ion_%s = %s;\n",
                        (j == 0) ? "\n" : "",
                        ions[j]->name,
                        ions[j]->name);
                insertstr(qend, buf);
            }
        }
    }
    if (i > 0) {
        net_receive_ = i;
    }
    if (net_init_q1_) {
        movelist(net_init_q1_, net_init_q2_, procfunc);
    }
}

void net_init(Item* qinit, Item* qp2) {
    /* qinit=INITIAL { stmtlist qp2=} */
    replacstr(qinit, "\nstatic void _net_init(Point_process* _pnt, double* _args, double _lflag)");
    Sprintf(buf, "    _ppvar = _nrn_mechanism_access_dparam(_pnt->_prop);\n");
    vectorize_substitute(insertstr(qinit->next->next, buf),
                         "  _nrn_mechanism_cache_instance _ml_real{_pnt->_prop};\n"
                         "  auto* const _ml = &_ml_real;\n"
                         "  size_t const _iml{};\n"
                         "  Datum* _ppvar = _nrn_mechanism_access_dparam(_pnt->_prop);\n"
                         "  Datum* _thread = (Datum*)0;\n"
                         "  NrnThread* _nt = (NrnThread*)_pnt->_vnt;\n");
    if (net_init_q1_) {
        diag("NET_RECEIVE block can contain only one INITIAL block", (char*) 0);
    }
    net_init_q1_ = qinit;
    net_init_q2_ = qp2;
}

void fornetcon(Item* keyword, Item* par1, Item* args, Item* par2, Item* stmt, Item* qend) {
    Item *q, *q1;
    Symbol* s;
    char snew[256];
    int i;
    /* follows net_receive pretty closely */
    ++for_netcons_;
    deltokens(par1, par2);
    i = for_netcons_;
    Sprintf(buf,
            "{int _ifn%d, _nfn%d; double* _fnargs%d, **_fnargslist%d;\n\
\t_nfn%d = _nrn_netcon_args(_ppvar[_fnc_index].get<void*>(), &_fnargslist%d);\n\
\tfor (_ifn%d = 0; _ifn%d < _nfn%d; ++_ifn%d) {\n",
            i,
            i,
            i,
            i,
            i,
            i,
            i,
            i,
            i,
            i);
    replacstr(keyword, buf);
    Sprintf(buf, "\t _fnargs%d = _fnargslist%d[_ifn%d];\n", i, i, i);
    insertstr(keyword->next, buf);
    insertstr(qend->next, "\t}}\n");
    i = 0;
    ITERATE(q1, args) {
        s = SYM(q1);
        Sprintf(snew, "_fnargs%d[%d]", for_netcons_, i);
        ++i;
        for (q = stmt; q != qend; q = q->next) {
            if (q->itemtype == SYMBOL && SYM(q) == s) {
                replacstr(q, snew);
            }
        }
    }
}

void chk_thread_safe() {
    Symbol* s;
    int i;
    Item* q;
    SYMLISTITER { /* globals are now global with respect to C as well as hoc */
        s = SYM(q);
        if (s->nrntype & (NRNGLOBAL) && s->assigned_to_ == 1) {
            Sprintf(buf, "Assignment to the GLOBAL variable, \"%s\", is not thread safe", s->name);
            threadsafe(buf);
        }
    }
}


void chk_global_state() {
    int i;
    Item* q;
    SYMLISTITER {
        Symbol* s = SYM(q);
        if (s->nrntype & NRNGLOBAL && s->subtype & STAT) {
            diag(s->name, " is a STATE variable and hence cannot be declared as GLOBAL");
        }
    }
}

void conductance_hint(int blocktype, Item* q1, Item* q2) {
    if (blocktype != BREAKPOINT) {
        diag("CONDUCTANCE can only appear in BREAKPOINT block", (char*) 0);
    }
    if (!conductance_) {
        conductance_ = newlist();
    }
    lappendsym(conductance_, SYM(q1->next));
    if (q2 != q1->next) {
        Symbol* s = SYM(q2);
        if (!ion_declared(s)) {
            diag(s->name, "not declared as USEION in NEURON block");
        }
        lappendsym(conductance_, s);
    } else {
        lappendsym(conductance_, SYM0);
    }
    deltokens(q1, q2);
}

void possible_local_current(int blocktype, List* symlist) {
    Item* q;
    Item* q2;
    if (blocktype != BREAKPOINT) {
        return;
    }
    ITERATE(q, currents) {
        ITERATE(q2, symlist) {
            char* n = SYM(q2)->name + 2; /* start after the _l */
            if (strcmp(SYM(q)->name, n) == 0) {
                if (!breakpoint_local_current_) {
                    breakpoint_local_current_ = newlist();
                }
                lappendsym(breakpoint_local_current_, SYM(q));
                lappendsym(breakpoint_local_current_, SYM(q2));
            }
        }
    }
}

Symbol* breakpoint_current(Symbol* s) {
    if (breakpoint_local_current_) {
        Item* q;
        ITERATE(q, breakpoint_local_current_) {
            if (SYM(q) == s) {
                return SYM(q->next);
            }
        }
    }
    return s;
}

// Determine if setdata is required to call FUNCTION or PROCEDURE
// setdata is required if RANGE var used. For safety, also VERBATIM.
// Deal with nested calls, via maintaining a list for each func.
// Note that the nest can be recursive and called function may not
// yet be defined til entire text is processed.

#include <unordered_set>
#include <unordered_map>

struct Info {
    std::unordered_set<Symbol*> func_calls;
    bool need_setdata{false};
    bool is_being_looked_at{false};  // avoid recursion loops
    Item* q{nullptr};                // To be modified if need_setdata.
};

static std::unordered_map<Symbol*, Info> funcs;
static Symbol* in_func_;

void check_range_in_func(Symbol* s) {
    if (in_func_) {
        // If s is a RANGE variable or nullptr (VERBATIM)
        // then mark the current function as needing setdata
        // If s is FUNCTION or PROCEDURE, then add to list
        Info& i = funcs[in_func_];
        if (!s) {  // VERBATIM
            i.need_setdata = true;
        } else if (s->nrntype & (NRNRANGE | NRNPOINTER)) {
            i.need_setdata = true;
        } else if (s->usage & FUNCT) {
            i.func_calls.insert(s);
        }
    }
}

void set_inside_func(Symbol* s) {
    in_func_ = s;
    if (s) {
        assert(funcs.count(s) == 0);
        funcs[s] = {};
    }
}

// Make sure need_setdata is properly marked for all funcs.
// I.e on entry, only ones marked are those that use RANGE or VERBATIM.
// Need to recursively look through func_calls but watch out for loops.
// If there are no RANGE then VERBATIM is ok and set all need_setdata to false.

static bool check_func(Symbol* s);  // recursive

void func_needs_setdata() {
    if (suffix[0] == '\0') {
        return;
    }
    for (auto& f: funcs) {
        f.second.is_being_looked_at = false;
    }

    // if there are no RANGE then set all need_setdata to false.
    bool norange{true};
    Item* q;
    int i;
    SYMLISTITER {
        Symbol* s = SYM(q);
        if (s->type == NAME && s->nrntype & (NRNRANGE | NRNPOINTER)) {
            norange = false;
            break;
        }
    }
    if (norange) {
        for (auto& f: funcs) {
            f.second.need_setdata = false;
        }
    }

    for (auto& f: funcs) {
        check_func(f.first);
    }
    for (auto& f: funcs) {  // update the hocfunc item if need_setdata
        auto& q = f.second.q;
        if (q && f.second.need_setdata) {
            // error if not valid id
            Symbol* s = f.first;
            Sprintf(buf,
                    "\n"
                    "  if(!_prop_id) {\n"
                    "    hoc_execerror(\""
                    "No data for %s_%s. Requires prior call to setdata_%s"
                    " and that the specified mechanism instance still be in existence.\","
                    " NULL);\n",
                    s->name,
                    mechname,
                    mechname);
            insertstr(q, buf);
            if (vectorize) {
                insertstr(q,
                          " }\n"
                          "  Prop* _local_prop = _extcall_prop;\n");
            } else {
                // ensure current instance matches _extcall_prop
                insertstr(q,
                          " } else {\n"
                          "    _setdata(_extcall_prop);\n"
                          "  }\n");
            }

        } else if (q) {
            if (vectorize) {
                // if id not valid then _local_prop must be nullptr
                // because of later _ppvar = _local_prop ? ...
                insertstr(q, "\n  Prop* _local_prop = _prop_id ? _extcall_prop : nullptr;\n");
            }
        }
    }
}

static bool check_func(Symbol* s) {  // recursive
    if (funcs.count(s) == 0) {
        return false;
    }
    Info& i = funcs[s];
    if (i.need_setdata) {
        return true;
    }
    if (i.is_being_looked_at) {
        return false;
    }
    i.is_being_looked_at = true;
    for (auto& s1: i.func_calls) {
        if (check_func(s1)) {
            i.need_setdata = true;
            return true;
        }
    }
    return false;
}

// If the function needs setdata, then q can be changed to
// perform the check on _extcall_prop
// Not called for POINT_PROCESS functions.
void hocfunc_setdata_item(Symbol* s, Item* q) {
    auto& i = funcs[s];
    i.q = q;
}
