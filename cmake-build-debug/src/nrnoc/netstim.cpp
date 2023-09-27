/* Created by Language version: 7.7.0 */
/* VECTORIZED */
#define NRN_VECTORIZED 1
#include "mech_api.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#undef PI
#define nil   0
#define _pval pval
// clang-format off
#include "md1redef.h"
#include "section_fwd.hpp"
#include "nrniv_mf.h"
#include "md2redef.h"
// clang-format on
#include "neuron/cache/mechanism_range.hpp"
static constexpr auto number_of_datum_variables = 3;
static constexpr auto number_of_floating_point_variables = 9;
namespace {
template <typename T>
using _nrn_mechanism_std_vector = std::vector<T>;
using _nrn_model_sorted_token = neuron::model_sorted_token;
using _nrn_mechanism_cache_range =
    neuron::cache::MechanismRange<number_of_floating_point_variables, number_of_datum_variables>;
using _nrn_mechanism_cache_instance =
    neuron::cache::MechanismInstance<number_of_floating_point_variables, number_of_datum_variables>;
template <typename T>
using _nrn_mechanism_field = neuron::mechanism::field<T>;
template <typename... Args>
void _nrn_mechanism_register_data_fields(Args&&... args) {
    neuron::mechanism::register_data_fields(std::forward<Args>(args)...);
}
}  // namespace

#if !NRNGPU
#undef exp
#define exp hoc_Exp
#endif

#define nrn_init           _nrn_init__NetStim
#define _nrn_initial       _nrn_initial__NetStim
#define nrn_cur            _nrn_cur__NetStim
#define _nrn_current       _nrn_current__NetStim
#define nrn_jacob          _nrn_jacob__NetStim
#define nrn_state          _nrn_state__NetStim
#define _net_receive       _net_receive__NetStim
#define init_sequence      init_sequence__NetStim
#define next_invl          next_invl__NetStim
#define noiseFromRandom123 noiseFromRandom123__NetStim

#define _threadargscomma_ _ml, _iml, _ppvar, _thread, _nt,
#define _threadargsprotocomma_ \
    Memb_list *_ml, size_t _iml, Datum *_ppvar, Datum *_thread, NrnThread *_nt,
#define _internalthreadargsprotocomma_ \
    _nrn_mechanism_cache_range *_ml, size_t _iml, Datum *_ppvar, Datum *_thread, NrnThread *_nt,
#define _threadargs_      _ml, _iml, _ppvar, _thread, _nt
#define _threadargsproto_ Memb_list *_ml, size_t _iml, Datum *_ppvar, Datum *_thread, NrnThread *_nt
#define _internalthreadargsproto_ \
    _nrn_mechanism_cache_range *_ml, size_t _iml, Datum *_ppvar, Datum *_thread, NrnThread *_nt
/*SUPPRESS 761*/
/*SUPPRESS 762*/
/*SUPPRESS 763*/
/*SUPPRESS 765*/
extern double* hoc_getarg(int);

#define t                    _nt->_t
#define dt                   _nt->_dt
#define interval             _ml->template fpfield<0>(_iml)
#define interval_columnindex 0
#define number               _ml->template fpfield<1>(_iml)
#define number_columnindex   1
#define start                _ml->template fpfield<2>(_iml)
#define start_columnindex    2
#define noise                _ml->template fpfield<3>(_iml)
#define noise_columnindex    3
#define event                _ml->template fpfield<4>(_iml)
#define event_columnindex    4
#define on                   _ml->template fpfield<5>(_iml)
#define on_columnindex       5
#define ispike               _ml->template fpfield<6>(_iml)
#define ispike_columnindex   6
#define v                    _ml->template fpfield<7>(_iml)
#define v_columnindex        7
#define _tsav                _ml->template fpfield<8>(_iml)
#define _tsav_columnindex    8
#define _nd_area             *_ml->dptr_field<0>(_iml)
#define donotuse             *_ppvar[2].get<double*>()
#define _p_donotuse          _ppvar[2].literal_value<void*>()

//RNG:: Emit this for each new RANDOM variable
#define rng_donotuse             *_ppvar[3].get<double*>()
#define _p_rng_donotuse          _ppvar[3].literal_value<void*>()
//

/* Thread safe. No static _ml, _iml or _ppvar. */
static int hoc_nrnpointerindex = 2;
static _nrn_mechanism_std_vector<Datum> _extcall_thread;
static Prop* _extcall_prop;
/* external NEURON variables */
/* declaration of user functions */
static double _hoc_bbsavestate(void*);
static double _hoc_erand(void*);
static double _hoc_init_sequence(void*);
static double _hoc_invl(void*);
static double _hoc_next_invl(void*);
static double _hoc_noiseFromRandom123(void*);
static double _hoc_init_rng_NetStim(void*);
static double _hoc_sample_rng_NetStim(void*);
static int _mechtype;
extern void _nrn_cacheloop_reg(int, int);
extern void hoc_register_limits(int, HocParmLimits*);
extern void hoc_register_units(int, HocParmUnits*);
extern void nrn_promote(Prop*, int, int);
extern Memb_func* memb_func;

#define NMODL_TEXT 1
#if NMODL_TEXT
static void register_nmodl_text_and_filename(int mechtype);
#endif
extern Prop* nrn_point_prop_;
static int _pointtype;
static void* _hoc_create_pnt(Object* _ho) {
    void* create_point_process(int, Object*);
    return create_point_process(_pointtype, _ho);
}
static void _hoc_destroy_pnt(void*);
static double _hoc_loc_pnt(void* _vptr) {
    double loc_point_process(int, void*);
    return loc_point_process(_pointtype, _vptr);
}
static double _hoc_has_loc(void* _vptr) {
    double has_loc_point(void*);
    return has_loc_point(_vptr);
}
static double _hoc_get_loc_pnt(void* _vptr) {
    double get_loc_point_process(void*);
    return (get_loc_point_process(_vptr));
}
extern void _nrn_setdata_reg(int, void (*)(Prop*));
static void _setdata(Prop* _prop) {
    _extcall_prop = _prop;
}
static void _hoc_setdata(void* _vptr) {
    Prop* _prop;
    _prop = ((Point_process*) _vptr)->_prop;
    _setdata(_prop);
}
/* connect user functions to hoc names */
static VoidFunc hoc_intfunc[] = {{0, 0}};
static Member_func _member_func[] = {{"loc", _hoc_loc_pnt},
                                     {"has_loc", _hoc_has_loc},
                                     {"get_loc", _hoc_get_loc_pnt},
                                     {"bbsavestate", _hoc_bbsavestate},
                                     {"erand", _hoc_erand},
                                     {"init_sequence", _hoc_init_sequence},
                                     {"invl", _hoc_invl},
                                     {"next_invl", _hoc_next_invl},
                                     {"noiseFromRandom123", _hoc_noiseFromRandom123},
                                     {"init_rng", _hoc_init_rng_NetStim},
                                     {"sample_rng", _hoc_sample_rng_NetStim},
                                     {0, 0}};
#define bbsavestate bbsavestate_NetStim
#define erand       erand_NetStim
#define invl        invl_NetStim
extern double bbsavestate(_internalthreadargsproto_);
extern double erand(_internalthreadargsproto_);
extern double invl(_internalthreadargsprotocomma_ double);
/* declare global and static user variables */
/* some parameters have upper and lower limits */
static HocParmLimits _hoc_parm_limits[] = {{"interval", 1e-09, 1e+09},
                                           {"noise", 0, 1},
                                           {"number", 0, 1e+09},
                                           {0, 0, 0}};
static HocParmUnits _hoc_parm_units[] = {{"interval", "ms"}, {"start", "ms"}, {0, 0}};
/* connect global user variables to hoc */
static DoubScal hoc_scdoub[] = {{0, 0}};
static DoubVec hoc_vdoub[] = {{0, 0, 0}};
static double _sav_indep;
static void nrn_alloc(Prop*);
static void nrn_init(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);
static void nrn_state(_nrn_model_sorted_token const&, NrnThread*, Memb_list*, int);
static void _hoc_destroy_pnt(void* _vptr) {
    destroy_point_process(_vptr);
}
static void _destructor(Prop*);
/* connect range variables in _p that hoc is supposed to know about */
//RNG:: Add entry into variable section
static const char* _mechanism[] =
    {"7.7.0", "NetStim", "interval", "number", "start", "noise", 0, 0, 0, "donotuse", "rng_donotuse", 0};

extern Prop* need_memb(Symbol*);
static void nrn_alloc(Prop* _prop) {
    Prop* prop_ion{};
    Datum* _ppvar{};
    if (nrn_point_prop_) {
        _nrn_mechanism_access_alloc_seq(_prop) = _nrn_mechanism_access_alloc_seq(nrn_point_prop_);
        _ppvar = _nrn_mechanism_access_dparam(nrn_point_prop_);
    } else {
        _ppvar = nrn_prop_datum_alloc(_mechtype, 4, _prop);
        _nrn_mechanism_access_dparam(_prop) = _ppvar;
        _nrn_mechanism_cache_instance _ml_real{_prop};
        auto* const _ml = &_ml_real;
        size_t const _iml{};
        assert(_nrn_mechanism_get_num_vars(_prop) == 9);
        /*initialize range parameters*/
        interval = 10;
        number = 10;
        start = 50;
        noise = 0;
    }
    assert(_nrn_mechanism_get_num_vars(_prop) == 9);
    _nrn_mechanism_access_dparam(_prop) = _ppvar;
    /*connect ionic variables to this model*/
}
static void _initlists();

#define _tqitem &(_ppvar[3])
static void _net_receive(Point_process*, double*, double);
static void bbcore_write(double*, int*, int*, int*, _threadargsproto_);
extern void hoc_reg_bbcore_write(int, void (*)(double*, int*, int*, int*, _threadargsproto_));
static void bbcore_read(double*, int*, int*, int*, _threadargsproto_);
extern void hoc_reg_bbcore_read(int, void (*)(double*, int*, int*, int*, _threadargsproto_));
extern Symbol* hoc_lookup(const char*);
extern void _nrn_thread_reg(int, int, void (*)(Datum*));
void _nrn_thread_table_reg(int, nrn_thread_table_check_t);
extern void hoc_register_tolerance(int, HocStateTolerance*, Symbol***);
extern void _cvode_abstol(Symbol**, double*, int);

extern "C" void _netstim_reg_() {
    int _vectorized = 1;
    _initlists();
    _pointtype = point_register_mech(_mechanism,
                                     nrn_alloc,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nrn_init,
                                     hoc_nrnpointerindex,
                                     1,
                                     _hoc_create_pnt,
                                     _hoc_destroy_pnt,
                                     _member_func);
    register_destructor(_destructor);
    _mechtype = nrn_get_mechtype(_mechanism[1]);
    _nrn_setdata_reg(_mechtype, _setdata);
    hoc_reg_bbcore_write(_mechtype, bbcore_write);
    hoc_reg_bbcore_read(_mechtype, bbcore_read);
#if NMODL_TEXT
    register_nmodl_text_and_filename(_mechtype);
#endif
    _nrn_mechanism_register_data_fields(_mechtype,
                                        _nrn_mechanism_field<double>{"interval"} /* 0 */,
                                        _nrn_mechanism_field<double>{"number"} /* 1 */,
                                        _nrn_mechanism_field<double>{"start"} /* 2 */,
                                        _nrn_mechanism_field<double>{"noise"} /* 3 */,
                                        _nrn_mechanism_field<double>{"event"} /* 4 */,
                                        _nrn_mechanism_field<double>{"on"} /* 5 */,
                                        _nrn_mechanism_field<double>{"ispike"} /* 6 */,
                                        _nrn_mechanism_field<double>{"v"} /* 7 */,
                                        _nrn_mechanism_field<double>{"_tsav"} /* 8 */,
                                        _nrn_mechanism_field<double*>{"_nd_area", "area"} /* 0 */,
                                        _nrn_mechanism_field<Point_process*>{"_pntproc",
                                                                             "pntproc"} /* 1 */,
                                        _nrn_mechanism_field<double*>{"donotuse",
                                                                      "bbcorepointer"} /* 2 */,
    //RNG:: register new variable. NOTE: here type/semantic needs to be changed to random or something similar
                                        _nrn_mechanism_field<double*>{"rng_donotuse",
                                                                      "bbcorepointer"} /* 3 */,

                                        _nrn_mechanism_field<void*>{"_tqitem", "netsend"} /* 4 */);
    //RNG:: count of variable needs to be increased
    hoc_register_prop_size(_mechtype, 9, 5);
    hoc_register_dparam_semantics(_mechtype, 0, "area");
    hoc_register_dparam_semantics(_mechtype, 1, "pntproc");
    hoc_register_dparam_semantics(_mechtype, 2, "bbcorepointer");
    //RNG:: register semantic. NOTE: should be also random or something similar
    hoc_register_dparam_semantics(_mechtype, 3, "bbcorepointer");
    hoc_register_dparam_semantics(_mechtype, 4, "netsend");
    add_nrn_artcell(_mechtype, 3);
    add_nrn_has_net_event(_mechtype);
    pnt_receive[_mechtype] = _net_receive;
    pnt_receive_size[_mechtype] = 1;
    hoc_register_var(hoc_scdoub, hoc_vdoub, hoc_intfunc);
    ivoc_help("help ?1 NetStim /Users/kumbhar/workarena/repos/bbp/nrn/src/nrnoc/netstim.mod\n");
    hoc_register_limits(_mechtype, _hoc_parm_limits);
    hoc_register_units(_mechtype, _hoc_parm_units);
}
static int _reset;
static const char* modelname = "";

static int error;
static int _ninits = 0;
static int _match_recurse = 1;
static void _modl_cleanup() {
    _match_recurse = 1;
}
static int init_sequence(_internalthreadargsprotocomma_ double);
static int next_invl(_internalthreadargsproto_);
static int noiseFromRandom123(_internalthreadargsproto_);

static int init_sequence(_internalthreadargsprotocomma_ double _lt) {
    if (number > 0.0) {
        on = 1.0;
        event = 0.0;
        ispike = 0.0;
    }
    return 0;
}

static double _hoc_init_sequence(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = 1.;
    init_sequence(_threadargscomma_ * getarg(1));
    return (_r);
}

double invl(_internalthreadargsprotocomma_ double _lmean) {
    double _linvl;
    if (_lmean <= 0.) {
        _lmean = .01;
    }
    if (noise == 0.0) {
        _linvl = _lmean;
    } else {
        _linvl = (1. - noise) * _lmean + noise * _lmean * erand(_threadargs_);
    }

    return _linvl;
}

static double _hoc_invl(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = invl(_threadargscomma_ * getarg(1));
    return (_r);
}

double erand(_internalthreadargsproto_) {
    double _lerand;

    /*VERBATIM*/
    if (_p_donotuse) {
        double rng = nrnran123_negexp(reinterpret_cast<nrnran123_State*>(_p_donotuse));
    }

    return _lerand;
}

static double _hoc_erand(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = erand(_threadargs_);
    return (_r);
}

static int noiseFromRandom123(_internalthreadargsproto_) {
    /*VERBATIM*/
    auto& r123state = reinterpret_cast<nrnran123_State*&>(_p_donotuse);
    if (r123state) {
        nrnran123_deletestream(r123state);
        r123state = nullptr;
    }
    if (!ifarg(3)) {
        printf("Error: noiseFromRandom123 expects 3 argument\n");
        abort();
    }
    r123state = nrnran123_newstream3(static_cast<uint32_t>(*getarg(1)),
                                     static_cast<uint32_t>(*getarg(2)),
                                     static_cast<uint32_t>(*getarg(3)));
    return 0;
}

static double _hoc_noiseFromRandom123(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = 1.;
    noiseFromRandom123(_threadargs_);
    return (_r);
}

/*VERBATIM*/
static void bbcore_write(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
    if (!noise) {
        return;
    }
    if (!_p_donotuse) {
        fprintf(stderr,
                "NetStim: cannot use the legacy scop_negexp generator for the random stream.\n");
        assert(0);
    }
    if (d) {
        char which;
        uint32_t* di = reinterpret_cast<uint32_t*>(d) + *offset;
        auto& r123state = reinterpret_cast<nrnran123_State*&>(_p_donotuse);
        nrnran123_getids3(r123state, di, di + 1, di + 2);
        nrnran123_getseq(r123state, di + 3, &which);
        di[4] = which;
    }
    *offset += 5;
}

static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
    if (!noise) {
        return;
    }
    /* Generally, CoreNEURON, in the context of psolve, begins with an empty
     * model, so this call takes place in the context of a freshly created
     * instance and _p_donotuse is not NULL.
     * However, this function is also now called from NEURON at the end of
     * coreneuron psolve in order to transfer back the nrnran123 sequence state.
     * That allows continuation with a subsequent psolve within NEURON or
     * properly transfer back to CoreNEURON if we continue the psolve there.
     * So now, extra logic is needed for this call to work in a NEURON context.
     */
    uint32_t* di = reinterpret_cast<uint32_t*>(d) + *offset;
    uint32_t id1, id2, id3;
    assert(_p_donotuse);
    auto* r123state = reinterpret_cast<nrnran123_State*>(_p_donotuse);
    nrnran123_getids3(r123state, &id1, &id2, &id3);
    nrnran123_setseq(r123state, di[3], di[4]);
    /* Random123 on NEURON side has same ids as on CoreNEURON side */
    assert(di[0] == id1 && di[1] == id2 && di[2] == id3);
    *offset += 5;
}

static int next_invl(_internalthreadargsproto_) {
    if (number > 0.0) {
        event = invl(_threadargscomma_ interval);
    }
    if (ispike >= number) {
        on = 0.0;
    }
    return 0;
}

static double _hoc_next_invl(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = 1.;
    next_invl(_threadargs_);
    return (_r);
}

static void _net_receive(Point_process* _pnt, double* _args, double _lflag) {
    Prop* _p;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    _nrn_mechanism_cache_instance _ml_real{_pnt->_prop};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _thread = nullptr;
    _nt = (NrnThread*) _pnt->_vnt;
    _ppvar = _nrn_mechanism_access_dparam(_pnt->_prop);
    if (_tsav > t) {
        hoc_execerror(hoc_object_name(_pnt->ob),
                      ":Event arrived out of order. Must call ParallelContext.set_maxstep AFTER "
                      "assigning minimum NetCon.delay");
    }
    _tsav = t;
    if (_lflag == 1.) {
        *(_tqitem) = nullptr;
    }
    {
        if (_lflag == 0.0) {
            if (_args[0] > 0.0 && on == 0.0) {
                init_sequence(_threadargscomma_ t);
                next_invl(_threadargs_);
                event = event - interval * (1. - noise);
                artcell_net_send(_tqitem, _args, _pnt, t + event, 1.0);
            } else if (_args[0] < 0.0) {
                on = 0.0;
            }
        }
        if (_lflag == 3.0) {
            if (on == 1.0) {
                init_sequence(_threadargscomma_ t);
                artcell_net_send(_tqitem, _args, _pnt, t + 0.0, 1.0);
            }
        }
        if (_lflag == 1.0 && on == 1.0) {
            ispike = ispike + 1.0;
            net_event(_pnt, t);
            next_invl(_threadargs_);
            if (on == 1.0) {
                artcell_net_send(_tqitem, _args, _pnt, t + event, 1.0);
            }
        }
    }
}

double bbsavestate(_internalthreadargsproto_) {
    double _lbbsavestate;
    _lbbsavestate = 0.0;

    /*VERBATIM*/
    auto r123state = reinterpret_cast<nrnran123_State*>(_p_donotuse);
    if (!r123state) {
        return 0.0;
    }
    double* xdir = hoc_pgetarg(1);
    if (*xdir == -1.) {
        *xdir = 2;
        return 0.0;
    }
    double* xval = hoc_pgetarg(2);
    if (*xdir == 0.) {
        char which;
        uint32_t seq;
        nrnran123_getseq(r123state, &seq, &which);
        xval[0] = seq;
        xval[1] = which;
    }
    if (*xdir == 1) {
        nrnran123_setseq(r123state, xval[0], xval[1]);
    }

    return _lbbsavestate;
}

static double _hoc_bbsavestate(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = bbsavestate(_threadargs_);
    return (_r);
}

static void _destructor(Prop* _prop) {
    _nrn_mechanism_cache_instance _ml_real{_prop};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    Datum *_ppvar{_nrn_mechanism_access_dparam(_prop)}, *_thread{};
    {
        {
            /*VERBATIM*/
            if (!noise) {
                return;
            }
            if (_p_donotuse) {
                auto& r123state = reinterpret_cast<nrnran123_State*&>(_p_donotuse);
                nrnran123_deletestream(r123state);
                r123state = nullptr;
            }
        }
    }
}


nrnran123_State*& get_random_var_by_name(const char* name, _internalthreadargsproto_) {
    if(strcmp(name, "rng_donotuse") == 0) {
        return reinterpret_cast<nrnran123_State*&>(_p_rng_donotuse);
    }
    // abort if name doesn't match
    std::cout << "Error: invalid RANDOM name" << std::endl;
    abort();
}


static int init_rng_NetStim(_internalthreadargsproto_) {
    const char *name = gargstr(1);
    uint32_t id1 = uint32_t(*getarg(2));
    uint32_t id2 = uint32_t(*getarg(3));
    uint32_t id3 = uint32_t(*getarg(4));

    // std::cout << "init_rng:: " << name << " " << id1 << " " << id2 << " " << id3 << std::endl;

    auto& r123state = get_random_var_by_name(name, _threadargs_);
    if (r123state) {
        nrnran123_deletestream(r123state);
        r123state = nullptr;
    }

    r123state = nrnran123_newstream3(id1, id2, id3);
    return 0;
}


static double _hoc_init_rng_NetStim(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = 1.;
    init_rng_NetStim(_threadargs_);
    return (_r);
}


static double sample_rng_NetStim(_internalthreadargsproto_) {
    const char *name = gargstr(1);

    auto& r123state = get_random_var_by_name(name, _threadargs_);
   
    if(strcmp(name, "rng_donotuse") == 0) {
        return nrnran123_negexp(r123state); // "1.0" from RANDOM declaration
    }   

    return 0;
}

static double _hoc_sample_rng_NetStim(void* _vptr) {
    double _r;
    Datum* _ppvar;
    Datum* _thread;
    NrnThread* _nt;
    auto* const _pnt = static_cast<Point_process*>(_vptr);
    auto* const _p = _pnt->_prop;
    _nrn_mechanism_cache_instance _ml_real{_p};
    auto* const _ml = &_ml_real;
    size_t const _iml{};
    _ppvar = _nrn_mechanism_access_dparam(_p);
    _thread = _extcall_thread.data();
    _nt = static_cast<NrnThread*>(_pnt->_vnt);
    _r = 1.;
    _r = sample_rng_NetStim(_threadargs_);
    return (_r);
}

static void initmodel(_internalthreadargsproto_) {
    int _i;
    double _save;
    {
        {
            /*VERBATIM*/
            if (_p_donotuse) {
                nrnran123_setseq(reinterpret_cast<nrnran123_State*>(_p_donotuse), 0, 0);
            }
            on = 0.0;
            ispike = 0.0;
            if (noise < 0.0) {
                noise = 0.0;
            }
            if (noise > 1.0) {
                noise = 1.0;
            }
            if (start >= 0.0 && number > 0.0) {
                on = 1.0;
                event = start + invl(_threadargscomma_ interval) - interval * (1. - noise);
                if (event < 0.0) {
                    event = 0.0;
                }
                artcell_net_send(_tqitem, nullptr, _ppvar[1].get<Point_process*>(), t + event, 3.0);
            }
        }
    }
}

static void nrn_init(_nrn_model_sorted_token const& _sorted_token,
                     NrnThread* _nt,
                     Memb_list* _ml_arg,
                     int _type) {
    _nrn_mechanism_cache_range _lmr{_sorted_token, *_nt, *_ml_arg, _type};
    auto* const _vec_v = _nt->node_voltage_storage();
    auto* const _ml = &_lmr;
    Datum* _ppvar;
    Datum* _thread;
    Node* _nd;
    double _v;
    int* _ni;
    int _iml, _cntml;
    _ni = _ml_arg->_nodeindices;
    _cntml = _ml_arg->_nodecount;
    _thread = _ml_arg->_thread;
    for (_iml = 0; _iml < _cntml; ++_iml) {
        _ppvar = _ml_arg->_pdata[_iml];
        _tsav = -1e20;
        initmodel(_threadargs_);
    }
}

static double _nrn_current(_internalthreadargsprotocomma_ double _v) {
    double _current = 0.;
    v = _v;
    {}
    return _current;
}

static void nrn_state(_nrn_model_sorted_token const& _sorted_token,
                      NrnThread* _nt,
                      Memb_list* _ml_arg,
                      int _type) {
    _nrn_mechanism_cache_range _lmr{_sorted_token, *_nt, *_ml_arg, _type};
    auto* const _vec_v = _nt->node_voltage_storage();
    auto* const _ml = &_lmr;
    Datum* _ppvar;
    Datum* _thread;
    Node* _nd;
    double _v = 0.0;
    int* _ni;
    _ni = _ml_arg->_nodeindices;
    size_t _cntml = _ml_arg->_nodecount;
    _thread = _ml_arg->_thread;
    for (size_t _iml = 0; _iml < _cntml; ++_iml) {
        _ppvar = _ml_arg->_pdata[_iml];
        _nd = _ml_arg->_nodelist[_iml];
        v = _v;
        {}
    }
}

static void terminal() {}

static void _initlists() {
    int _i;
    static int _first = 1;
    if (!_first)
        return;
    _first = 0;
}

#if NMODL_TEXT
static void register_nmodl_text_and_filename(int mech_type) {
    const char* nmodl_filename = "/Users/kumbhar/workarena/repos/bbp/nrn/src/nrnoc/netstim.mod";
    const char* nmodl_file_text =
        ": $Id: netstim.mod 2212 2008-09-08 14:32:26Z hines $\n"
        ": comments at end\n"
        "\n"
        ": the Random idiom has been extended to support CoreNEURON.\n"
        "\n"
        ": For backward compatibility, noiseFromRandom(hocRandom) can still be used\n"
        ": as well as the default low-quality scop_exprand generator.\n"
        ": However, CoreNEURON will not accept usage of the low-quality generator,\n"
        ": and, if noiseFromRandom is used to specify the random stream, that stream\n"
        ": must be using the Random123 generator.\n"
        "\n"
        ": The recommended idiom for specfication of the random stream is to use\n"
        ": noiseFromRandom123(id1, id2[, id3])\n"
        "\n"
        ": If any instance uses noiseFromRandom123, then no instance can use noiseFromRandom\n"
        ": and vice versa.\n"
        "\n"
        "NEURON	{ \n"
        "    ARTIFICIAL_CELL NetStim\n"
        "    RANGE interval, number, start\n"
        "    RANGE noise\n"
        "    THREADSAFE : only true if every instance has its own distinct Random\n"
        "    BBCOREPOINTER donotuse\n"
        "    RANDOM NEGEXP(1.0) rng_donotuse\n"
        "}\n"
        "\n"
        "PARAMETER {\n"
        "    interval    = 10 (ms) <1e-9,1e9>    : time between spikes (msec)\n"
        "    number      = 10 <0,1e9>            : number of spikes (independent of noise)\n"
        "    start       = 50 (ms)               : start of first spike\n"
        "    noise       = 0 <0,1>               : amount of randomness (0.0 - 1.0)\n"
        "}\n"
        "\n"
        "ASSIGNED {\n"
        "    event (ms)\n"
        "    on\n"
        "    ispike\n"
        "    donotuse\n"
        "}\n"
        "\n"
        "INITIAL {\n"
        "    VERBATIM\n"
        "        if(_p_donotuse) {\n"
        "            nrnran123_setseq(reinterpret_cast<nrnran123_State*>(_p_donotuse), 0, 0);\n"
        "        }\n"
        "    ENDVERBATIM\n"
        "    :setseq(rng_donotuse, 0, 0)\n"
        "\n"
        "    on = 0 : off\n"
        "    ispike = 0\n"
        "    if (noise < 0) {\n"
        "        noise = 0\n"
        "    }\n"
        "    if (noise > 1) {\n"
        "        noise = 1\n"
        "    }\n"
        "    if (start >= 0 && number > 0) {\n"
        "        on = 1\n"
        "        : randomize the first spike so on average it occurs at\n"
        "        : start + noise*interval\n"
        "        event = start + invl(interval) - interval*(1. - noise)\n"
        "        : but not earlier than 0\n"
        "        if (event < 0) {\n"
        "            event = 0\n"
        "        }\n"
        "        net_send(event, 3)\n"
        "    }\n"
        "}	\n"
        "\n"
        "PROCEDURE init_sequence(t(ms)) {\n"
        "    if (number > 0) {\n"
        "        on = 1\n"
        "        event = 0\n"
        "        ispike = 0\n"
        "    }\n"
        "}\n"
        "\n"
        "FUNCTION invl(mean (ms)) (ms) {\n"
        "    if (mean <= 0.) {\n"
        "        mean = .01 (ms) : I would worry if it were 0.\n"
        "    }\n"
        "    if (noise == 0) {\n"
        "        invl = mean\n"
        "    } else {\n"
        "        invl = (1. - noise)*mean + noise*mean*erand()\n"
        "    }\n"
        "}\n"
        "\n"
        "FUNCTION erand() {\n"
        "VERBATIM\n"
        "        if (_p_donotuse) {\n"
        "            double rng = "
        "nrnran123_negexp(reinterpret_cast<nrnran123_State*>(_p_donotuse));\n"
        "        }\n"
        "ENDVERBATIM\n"
        "}\n"
        "\n"
        "PROCEDURE noiseFromRandom123() {\n"
        "VERBATIM\n"
        "    auto& r123state = reinterpret_cast<nrnran123_State*&>(_p_donotuse);\n"
        "    if (r123state) {\n"
        "        nrnran123_deletestream(r123state);\n"
        "        r123state = nullptr;\n"
        "    }\n"
        "    if (!ifarg(3)) {\n"
        "        printf(\"Error: noiseFromRandom123 expects 3 argument\\n\");\n"
        "        abort();\n"
        "    }\n"
        "    r123state = nrnran123_newstream3(static_cast<uint32_t>(*getarg(1)), "
        "static_cast<uint32_t>(*getarg(2)), static_cast<uint32_t>(*getarg(3)));\n"
        "ENDVERBATIM\n"
        "}\n"
        "\n"
        "DESTRUCTOR {\n"
        "VERBATIM\n"
        "    if (!noise) { return; }\n"
        "    if (_p_donotuse) {\n"
        "        auto& r123state = reinterpret_cast<nrnran123_State*&>(_p_donotuse);\n"
        "        nrnran123_deletestream(r123state);\n"
        "        r123state = nullptr;\n"
        "    }\n"
        "ENDVERBATIM\n"
        "}\n"
        "\n"
        "VERBATIM\n"
        "static void bbcore_write(double* x, int* d, int* xx, int *offset, _threadargsproto_) {\n"
        "    if (!noise) { return; }\n"
        "    if (!_p_donotuse) {\n"
        "        fprintf(stderr, \"NetStim: cannot use the legacy scop_negexp generator for the "
        "random stream.\\n\");\n"
        "        assert(0);\n"
        "    }\n"
        "    if (d) {\n"
        "        char which;\n"
        "        uint32_t* di = reinterpret_cast<uint32_t*>(d) + *offset;\n"
        "            auto& r123state = reinterpret_cast<nrnran123_State*&>(_p_donotuse);\n"
        "            nrnran123_getids3(r123state, di, di+1, di+2);\n"
        "            nrnran123_getseq(r123state, di+3, &which);\n"
        "            di[4] = which;\n"
        "    }\n"
        "    *offset += 5;\n"
        "}\n"
        "\n"
        "static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_) {\n"
        "    if (!noise) { return; }\n"
        "    /* Generally, CoreNEURON, in the context of psolve, begins with an empty\n"
        "     * model, so this call takes place in the context of a freshly created\n"
        "     * instance and _p_donotuse is not NULL.\n"
        "     * However, this function is also now called from NEURON at the end of\n"
        "     * coreneuron psolve in order to transfer back the nrnran123 sequence state.\n"
        "     * That allows continuation with a subsequent psolve within NEURON or\n"
        "     * properly transfer back to CoreNEURON if we continue the psolve there.\n"
        "     * So now, extra logic is needed for this call to work in a NEURON context.\n"
        "     */\n"
        "    uint32_t* di = reinterpret_cast<uint32_t*>(d) + *offset;\n"
        "    uint32_t id1, id2, id3;\n"
        "    assert(_p_donotuse);\n"
        "        auto* r123state = reinterpret_cast<nrnran123_State*>(_p_donotuse);\n"
        "        nrnran123_getids3(r123state, &id1, &id2, &id3);\n"
        "        nrnran123_setseq(r123state, di[3], di[4]);\n"
        "    /* Random123 on NEURON side has same ids as on CoreNEURON side */\n"
        "    assert(di[0] == id1 && di[1] == id2 && di[2] == id3);\n"
        "    *offset += 5;\n"
        "}\n"
        "ENDVERBATIM\n"
        "\n"
        "PROCEDURE next_invl() {\n"
        "    if (number > 0) {\n"
        "        event = invl(interval)\n"
        "    }\n"
        "    if (ispike >= number) {\n"
        "        on = 0\n"
        "    }\n"
        "}\n"
        "\n"
        "NET_RECEIVE (w) {\n"
        "    if (flag == 0) { : external event\n"
        "        if (w > 0 && on == 0) { : turn on spike sequence\n"
        "            : but not if a netsend is on the queue\n"
        "            init_sequence(t)\n"
        "            : randomize the first spike so on average it occurs at\n"
        "            : noise*interval (most likely interval is always 0)\n"
        "            next_invl()\n"
        "            event = event - interval*(1. - noise)\n"
        "            net_send(event, 1)\n"
        "        }else if (w < 0) { : turn off spiking definitively\n"
        "            on = 0\n"
        "        }\n"
        "    }\n"
        "    if (flag == 3) { : from INITIAL\n"
        "        if (on == 1) { : but ignore if turned off by external event\n"
        "            init_sequence(t)\n"
        "            net_send(0, 1)\n"
        "        }\n"
        "    }\n"
        "    if (flag == 1 && on == 1) {\n"
        "        ispike = ispike + 1\n"
        "        net_event(t)\n"
        "        next_invl()\n"
        "        if (on == 1) {\n"
        "            net_send(event, 1)\n"
        "        }\n"
        "    }\n"
        "}\n"
        "\n"
        "FUNCTION bbsavestate() {\n"
        "    bbsavestate = 0\n"
        "    : limited to noiseFromRandom123\n"
        "VERBATIM\n"
        "        auto r123state = reinterpret_cast<nrnran123_State*>(_p_donotuse);\n"
        "        if (!r123state) { return 0.0; }\n"
        "        double* xdir = hoc_pgetarg(1);\n"
        "        if (*xdir == -1.) {\n"
        "            *xdir = 2;\n"
        "            return 0.0;\n"
        "        }\n"
        "        double* xval = hoc_pgetarg(2);\n"
        "        if (*xdir == 0.) {\n"
        "            char which;\n"
        "            uint32_t seq;\n"
        "            nrnran123_getseq(r123state, &seq, &which);\n"
        "            xval[0] = seq;\n"
        "            xval[1] = which;\n"
        "        }\n"
        "        if (*xdir == 1) {\n"
        "            nrnran123_setseq(r123state, xval[0], xval[1]);\n"
        "        }\n"
        "ENDVERBATIM\n"
        "}\n"
        "\n"
        "\n"
        "COMMENT\n"
        "Presynaptic spike generator\n"
        "---------------------------\n"
        "\n"
        "This mechanism has been written to be able to use synapses in a single\n"
        "neuron receiving various types of presynaptic trains.  This is a \"fake\"\n"
        "presynaptic compartment containing a spike generator.  The trains\n"
        "of spikes can be either periodic or noisy (Poisson-distributed)\n"
        "\n"
        "Parameters;\n"
        "   noise: 	between 0 (no noise-periodic) and 1 (fully noisy)\n"
        "   interval: 	mean time between spikes (ms)\n"
        "   number: 	number of spikes (independent of noise)\n"
        "\n"
        "Written by Z. Mainen, modified by A. Destexhe, The Salk Institute\n"
        "\n"
        "Modified by Michael Hines for use with CVode\n"
        "The intrinsic bursting parameters have been removed since\n"
        "generators can stimulate other generators to create complicated bursting\n"
        "patterns with independent statistics (see below)\n"
        "\n"
        "Modified by Michael Hines to use logical event style with NET_RECEIVE\n"
        "This stimulator can also be triggered by an input event.\n"
        "If the stimulator is in the on==0 state (no net_send events on queue)\n"
        " and receives a positive weight\n"
        "event, then the stimulator changes to the on=1 state and goes through\n"
        "its entire spike sequence before changing to the on=0 state. During\n"
        "that time it ignores any positive weight events. If, in an on!=0 state,\n"
        "the stimulator receives a negative weight event, the stimulator will\n"
        "change to the on==0 state. In the on==0 state, it will ignore any ariving\n"
        "net_send events. A change to the on==1 state immediately fires the first spike of\n"
        "its sequence.\n"
        "\n"
        "ENDCOMMENT\n"
        "\n";
    hoc_reg_nmodl_filename(mech_type, nmodl_filename);
    hoc_reg_nmodl_text(mech_type, nmodl_file_text);
}
#endif
