/* Created by Language version: 6.2.0 */
/* VECTORIZED */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "corebluron/mech/cfile/scoplib.h"
#undef PI
#ifdef _PROF_HPM 
void HPM_Start(const char *); 
void HPM_Stop(const char *); 
#endif 
 
#include "corebluron/nrnoc/md1redef.h"
#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"

#include "corebluron/utils/randoms/nrnran123.h"

#include "corebluron/nrnoc/md2redef.h"
#if METHOD3
extern int _method3;
#endif

#if !NRNGPU
#undef exp
#define exp hoc_Exp
extern double hoc_Exp(double);
#endif
 
#if !defined(LAYOUT)
/* 1 means AoS, >1 means AoSoA, <= 0 means SOA */
#define LAYOUT 1
#endif
#if LAYOUT >= 1
#define _STRIDE LAYOUT
#else
#define _STRIDE _cntml
#endif
 
#define nrn_init _nrn_init__Nap_Et2
#define nrn_cur _nrn_cur__Nap_Et2
#define _nrn_current _nrn_current__Nap_Et2
#define nrn_jacob _nrn_jacob__Nap_Et2
#define nrn_state _nrn_state__Nap_Et2
#define _net_receive _net_receive__Nap_Et2 
#define rates rates__Nap_Et2 
#define states states__Nap_Et2 
 
#if LAYOUT == 0 /*SoA*/
#define _threadargscomma_ _cntml, _p, _ppvar, _thread, _nt,
#define _threadargsprotocomma_ int _cntml, double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt,
#define _threadargs_ _cntml, _p, _ppvar, _thread, _nt
#define _threadargsproto_ int _cntml, double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt
#else
#define _threadargscomma_ _p, _ppvar, _thread, _nt,
#define _threadargsprotocomma_ double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt,
#define _threadargs_ _p, _ppvar, _thread, _nt
#define _threadargsproto_ double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt
#endif
 	/*SUPPRESS 761*/
	/*SUPPRESS 762*/
	/*SUPPRESS 763*/
	/*SUPPRESS 765*/
	 extern double *getarg();
 /* Thread safe. No static _p or _ppvar. */
 
#define t _nt->_t
#define dt _nt->_dt
#define gNap_Et2bar _p[0*_STRIDE]
#define ina _p[1*_STRIDE]
#define gNap_Et2 _p[2*_STRIDE]
#define m _p[3*_STRIDE]
#define h _p[4*_STRIDE]
#define ena _p[5*_STRIDE]
#define mInf _p[6*_STRIDE]
#define mTau _p[7*_STRIDE]
#define mAlpha _p[8*_STRIDE]
#define mBeta _p[9*_STRIDE]
#define hInf _p[10*_STRIDE]
#define hTau _p[11*_STRIDE]
#define hAlpha _p[12*_STRIDE]
#define hBeta _p[13*_STRIDE]
#define Dm _p[14*_STRIDE]
#define Dh _p[15*_STRIDE]
#define v _p[16*_STRIDE]
#define _g _p[17*_STRIDE]
#define _ion_ena		_nt->_data[_ppvar[0*_STRIDE]]
#define _ion_ina	_nt->_data[_ppvar[1*_STRIDE]]
#define _ion_dinadv	_nt->_data[_ppvar[2*_STRIDE]]
 
#if MAC
#if !defined(v)
#define v _mlhv
#endif
#if !defined(h)
#define h _mlhh
#endif
#endif
 
#if defined(__cplusplus)
extern "C" {
#endif
 static int hoc_nrnpointerindex =  -1;
 static ThreadDatum* _extcall_thread;
 /* external NEURON variables */
 
#if 0 /*BBCORE*/
 /* declaration of user functions */
 static void _hoc_rates(void);
 
#endif /*BBCORE*/
 static int _mechtype;
extern int nrn_get_mechtype();
extern void hoc_register_prop_size(int, int, int);
extern Memb_func* memb_func;
 
#if 0 /*BBCORE*/
 /* connect user functions to hoc names */
 static VoidFunc hoc_intfunc[] = {
 "setdata_Nap_Et2", _hoc_setdata,
 "rates_Nap_Et2", _hoc_rates,
 0, 0
};
 
#endif /*BBCORE*/
 /* declare global and static user variables */
 
#if 0 /*BBCORE*/
 /* some parameters have upper and lower limits */
 static HocParmLimits _hoc_parm_limits[] = {
 0,0,0
};
 static HocParmUnits _hoc_parm_units[] = {
 "gNap_Et2bar_Nap_Et2", "S/cm2",
 "ina_Nap_Et2", "mA/cm2",
 "gNap_Et2_Nap_Et2", "S/cm2",
 0,0
};
 
#endif /*BBCORE*/
 static double delta_t = 0.01;
 static double h0 = 0;
 static double m0 = 0;
 
#if 0 /*BBCORE*/
 /* connect global user variables to hoc */
 static DoubScal hoc_scdoub[] = {
 0,0
};
 static DoubVec hoc_vdoub[] = {
 0,0,0
};
 
#endif /*BBCORE*/
 static double _sav_indep;
 static void nrn_alloc(double*, Datum*, int);
static void  nrn_init(_NrnThread*, _Memb_list*, int);
static void nrn_state(_NrnThread*, _Memb_list*, int);
 static void nrn_cur(_NrnThread*, _Memb_list*, int);
static void  nrn_jacob(_NrnThread*, _Memb_list*, int);
 /* connect range variables in _p that hoc is supposed to know about */
 static const char *_mechanism[] = {
 "6.2.0",
"Nap_Et2",
 "gNap_Et2bar_Nap_Et2",
 0,
 "ina_Nap_Et2",
 "gNap_Et2_Nap_Et2",
 0,
 "m_Nap_Et2",
 "h_Nap_Et2",
 0,
 0};
 static int _na_type;
 
static void nrn_alloc(double* _p, Datum* _ppvar, int _type) {
 
#if 0 /*BBCORE*/
 	/*initialize range parameters*/
 	gNap_Et2bar = 1e-05;
 prop_ion = need_memb(_na_sym);
 nrn_promote(prop_ion, 0, 1);
 	_ppvar[0]._pval = &prop_ion->param[0]; /* ena */
 	_ppvar[1]._pval = &prop_ion->param[3]; /* ina */
 	_ppvar[2]._pval = &prop_ion->param[4]; /* _ion_dinadv */
 
#endif /* BBCORE */
 
}
 static void _initlists();
 static void _update_ion_pointer(Datum*);
 
#define _psize 18
#define _ppsize 3
 extern Symbol* hoc_lookup(const char*);
extern void _nrn_thread_reg(int, int, void(*f)(Datum*));
extern void _nrn_thread_table_reg(int, void(*)(_threadargsproto_, int));
extern void _cvode_abstol( Symbol**, double*, int);

 void _Nap_Et2_reg() {
	int _vectorized = 1;
  _initlists();
 _na_type = nrn_get_mechtype("na_ion"); 
#if 0 /*BBCORE*/
 	ion_reg("na", -10000.);
 	_na_sym = hoc_lookup("na_ion");
 
#endif /*BBCORE*/
 	register_mech(_mechanism, nrn_alloc,nrn_cur, nrn_jacob, nrn_state, nrn_init, hoc_nrnpointerindex, 1);
 _mechtype = nrn_get_mechtype(_mechanism[1]);
 _nrn_layout_reg(_mechtype, LAYOUT);
  hoc_register_prop_size(_mechtype, _psize, _ppsize);
  hoc_register_dparam_semantics(_mechtype, 0, "na_ion");
  hoc_register_dparam_semantics(_mechtype, 1, "na_ion");
  hoc_register_dparam_semantics(_mechtype, 2, "na_ion");
 }
static int _reset;
static char *modelname = "";

static int error;
static int _ninits = 0;
static int _match_recurse=1;
static void _modl_cleanup(){ _match_recurse=1;}
static int rates(_threadargsproto_);
 
static int _ode_spec1(_threadargsproto_);
static int _ode_matsol1(_threadargsproto_);
 static int _slist1[2], _dlist1[2];
 static int states(_threadargsproto_);
 
/*CVODE*/
 static int _ode_spec1 (_threadargsproto_) {int _reset = 0; {
   rates ( _threadargs_ ) ;
   Dm = ( mInf - m ) / mTau ;
   Dh = ( hInf - h ) / hTau ;
   }
 return _reset;
}
 static int _ode_matsol1 (_threadargsproto_) {
 rates ( _threadargs_ ) ;
 Dm = Dm  / (1. - dt*( ( ( ( - 1.0 ) ) ) / mTau )) ;
 Dh = Dh  / (1. - dt*( ( ( ( - 1.0 ) ) ) / hTau )) ;
 return 0;
}
 /*END CVODE*/
 static int states (_threadargsproto_) { {
   rates ( _threadargs_ ) ;
    m = m + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / mTau)))*(- ( ( ( mInf ) ) / mTau ) / ( ( ( ( - 1.0) ) ) / mTau ) - m) ;
    h = h + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / hTau)))*(- ( ( ( hInf ) ) / hTau ) / ( ( ( ( - 1.0) ) ) / hTau ) - h) ;
   }
  return 0;
}
 
static int  rates ( _threadargsproto_ ) {
   double _lqt ;
 _lqt = pow( 2.3 , ( ( 34.0 - 21.0 ) / 10.0 ) ) ;
    mInf = 1.0 / ( 1.0 + exp ( ( v - - 52.6 ) / - 4.6 ) ) ;
   if ( v  == - 38.0 ) {
     v = v + 0.0001 ;
     }
   mAlpha = ( 0.182 * ( v - - 38.0 ) ) / ( 1.0 - ( exp ( - ( v - - 38.0 ) / 6.0 ) ) ) ;
   mBeta = ( 0.124 * ( - v - 38.0 ) ) / ( 1.0 - ( exp ( - ( - v - 38.0 ) / 6.0 ) ) ) ;
   mTau = 6.0 * ( 1.0 / ( mAlpha + mBeta ) ) / _lqt ;
   if ( v  == - 17.0 ) {
     v = v + 0.0001 ;
     }
   if ( v  == - 64.4 ) {
     v = v + 0.0001 ;
     }
   hInf = 1.0 / ( 1.0 + exp ( ( v - - 48.8 ) / 10.0 ) ) ;
   hAlpha = - 2.88e-6 * ( v + 17.0 ) / ( 1.0 - exp ( ( v + 17.0 ) / 4.63 ) ) ;
   hBeta = 6.94e-6 * ( v + 64.4 ) / ( 1.0 - exp ( - ( v + 64.4 ) / 2.63 ) ) ;
   hTau = ( 1.0 / ( hAlpha + hBeta ) ) / _lqt ;
     return 0; }
 
#if 0 /*BBCORE*/
 
static void _hoc_rates(void) {
  double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   if (_extcall_prop) {_p = _extcall_prop->param; _ppvar = _extcall_prop->dparam;}else{ _p = (double*)0; _ppvar = (Datum*)0; }
  _thread = _extcall_thread;
  _nt = nrn_threads;
 _r = 1.;
 rates ( _threadargs_ ;
 hoc_retpushx(_r);
}
 
#endif /*BBCORE*/
 static void _update_ion_pointer(Datum* _ppvar) {
 }

static void initmodel(_threadargsproto_) {
  int _i; double _save;{
  h = h0;
  m = m0;
 {
   rates ( _threadargs_ ) ;
   m = mInf ;
   h = hInf ;
   }
 
}
}

static void nrn_init(_NrnThread* _nt, _Memb_list* _ml, int _type){
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v; int* _ni; int _iml, _cntml;
    _ni = _ml->_nodeindices;
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
#if LAYOUT == 1 /*AoS*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
#endif
#if LAYOUT == 0 /*SoA*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml; _ppvar = _ml->_pdata + _iml;
#endif
#if LAYOUT > 1 /*AoSoA*/
#error AoSoA not implemented.
#endif
    _v = VEC_V(_ni[_iml]);
 v = _v;
  ena = _ion_ena;
 initmodel(_threadargs_);
 }
}

static double _nrn_current(_threadargsproto_, double _v){double _current=0.;v=_v;{ {
   gNap_Et2 = gNap_Et2bar * m * m * m * h ;
   ina = gNap_Et2 * ( v - ena ) ;
   }
 _current += ina;

} return _current;
}

static void nrn_cur(_NrnThread* _nt, _Memb_list* _ml, int _type) {
double* _p; Datum* _ppvar; ThreadDatum* _thread;
int* _ni; double _rhs, _v; int _iml, _cntml;
    _ni = _ml->_nodeindices;
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
#if LAYOUT == 1 /*AoS*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
#endif
#if LAYOUT == 0 /*SoA*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml; _ppvar = _ml->_pdata + _iml;
#endif
#if LAYOUT > 1 /*AoSoA*/
#error AoSoA not implemented.
#endif
    _v = VEC_V(_ni[_iml]);
  ena = _ion_ena;
 _g = _nrn_current(_threadargs_, _v + .001);
 	{ double _dina;
  _dina = ina;
 _rhs = _nrn_current(_threadargs_, _v);
  _ion_dinadv += (_dina - ina)/.001 ;
 	}
 _g = (_g - _rhs)/.001;
  _ion_ina += ina ;
	VEC_RHS(_ni[_iml]) -= _rhs;
 
}
 
}

static void nrn_jacob(_NrnThread* _nt, _Memb_list* _ml, int _type) {
double* _p; Datum* _ppvar; ThreadDatum* _thread;
int* _ni; int _iml, _cntml;
    _ni = _ml->_nodeindices;
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
#if LAYOUT == 1 /*AoS*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
#endif
#if LAYOUT == 0 /*SoA*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml; _ppvar = _ml->_pdata + _iml;
#endif
#if LAYOUT > 1 /*AoSoA*/
#error AoSoA not implemented.
#endif
	VEC_D(_ni[_iml]) += _g;
 
}
 
}

static void nrn_state(_NrnThread* _nt, _Memb_list* _ml, int _type) {
#ifdef _PROF_HPM 
HPM_Start("nrn_state_Nap_Et2"); 
#endif 
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v = 0.0; int* _ni; int _iml, _cntml;
    _ni = _ml->_nodeindices;
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
#if LAYOUT == 1 /*AoS*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
#endif
#if LAYOUT == 0 /*SoA*/
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml; _ppvar = _ml->_pdata + _iml;
#endif
#if LAYOUT > 1 /*AoSoA*/
#error AoSoA not implemented.
#endif
    _v = VEC_V(_ni[_iml]);
 v=_v;
{
  ena = _ion_ena;
 {   states(_threadargs_);
  } }}
#ifdef _PROF_HPM 
HPM_Stop("nrn_state_Nap_Et2"); 
#endif 

}

static void terminal(){}

static void _initlists(){
 double _x; double* _p = &_x;
 int _i; static int _first = 1;
 int _cntml=0; assert(0);
  if (!_first) return;
 _slist1[0] = &(m) - _p;  _dlist1[0] = &(Dm) - _p;
 _slist1[1] = &(h) - _p;  _dlist1[1] = &(Dh) - _p;
_first = 0;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
