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

#include "corebluron/nrnoc/md2redef.h"
#if METHOD3
extern int _method3;
#endif

#if !NRNGPU
#undef exp
#define exp hoc_Exp
extern double hoc_Exp(double);
#endif
 
#define _threadargscomma_ _p, _ppvar, _thread, _nt,
#define _threadargsprotocomma_ double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt,
#define _threadargs_ _p, _ppvar, _thread, _nt
#define _threadargsproto_ double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt
 	/*SUPPRESS 761*/
	/*SUPPRESS 762*/
	/*SUPPRESS 763*/
	/*SUPPRESS 765*/
	 extern double *getarg();
 /* Thread safe. No static _p or _ppvar. */
 
#define t _nt->_t
#define dt _nt->_dt
#define gCabar _p[0]
#define ica _p[1]
#define gCa _p[2]
#define m _p[3]
#define h _p[4]
#define eca _p[5]
#define mInf _p[6]
#define mTau _p[7]
#define mAlpha _p[8]
#define mBeta _p[9]
#define hInf _p[10]
#define hTau _p[11]
#define hAlpha _p[12]
#define hBeta _p[13]
#define Dm _p[14]
#define Dh _p[15]
#define v _p[16]
#define _g _p[17]
#define _ion_eca		_nt->_data[_ppvar[0]]
#define _ion_ica	_nt->_data[_ppvar[1]]
#define _ion_dicadv	_nt->_data[_ppvar[2]]
 
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
 "setdata_Ca", _hoc_setdata,
 "rates_Ca", _hoc_rates,
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
 "gCabar_Ca", "S/cm2",
 "ica_Ca", "mA/cm2",
 "gCa_Ca", "S/cm2",
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
"Ca",
 "gCabar_Ca",
 0,
 "ica_Ca",
 "gCa_Ca",
 0,
 "m_Ca",
 "h_Ca",
 0,
 0};
 static int _ca_type;
 
static void nrn_alloc(double* _p, Datum* _ppvar, int _type) {
 	/*initialize range parameters*/
 	gCabar = 1e-05;
 
#if 0 /*BBCORE*/
 prop_ion = need_memb(_ca_sym);
 nrn_promote(prop_ion, 0, 1);
 	_ppvar[0]._pval = &prop_ion->param[0]; /* eca */
 	_ppvar[1]._pval = &prop_ion->param[3]; /* ica */
 	_ppvar[2]._pval = &prop_ion->param[4]; /* _ion_dicadv */
 
#endif /* BBCORE */
 
}
 static void _initlists();
 static void _update_ion_pointer(Datum*);
 
#define _psize 18
#define _ppsize 3
 extern Symbol* hoc_lookup(const char*);
extern void _nrn_thread_reg(int, int, void(*f)(Datum*));
extern void _nrn_thread_table_reg(int, void(*)(double*, Datum*, ThreadDatum*, _NrnThread*, int));
extern void _cvode_abstol( Symbol**, double*, int);

 void _Ca_reg() {
	int _vectorized = 1;
  _initlists();
 _ca_type = nrn_get_mechtype("ca_ion"); 
#if 0 /*BBCORE*/
 	ion_reg("ca", -10000.);
 	_ca_sym = hoc_lookup("ca_ion");
 
#endif /*BBCORE*/
 	register_mech(_mechanism, nrn_alloc,nrn_cur, nrn_jacob, nrn_state, nrn_init, hoc_nrnpointerindex, 1);
 _mechtype = nrn_get_mechtype(_mechanism[1]);
  hoc_register_prop_size(_mechtype, _psize, _ppsize);
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
 static int _ode_spec1 (double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {int _reset = 0; {
   rates ( _threadargs_ ) ;
   Dm = ( mInf - m ) / mTau ;
   Dh = ( hInf - h ) / hTau ;
   }
 return _reset;
}
 static int _ode_matsol1 (double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
 rates ( _threadargs_ ) ;
 Dm = Dm  / (1. - dt*( ( ( ( - 1.0 ) ) ) / mTau )) ;
 Dh = Dh  / (1. - dt*( ( ( ( - 1.0 ) ) ) / hTau )) ;
 return 0;
}
 /*END CVODE*/
 static int states (double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) { {
   rates ( _threadargs_ ) ;
    m = m + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / mTau)))*(- ( ( ( mInf ) ) / mTau ) / ( ( ( ( - 1.0) ) ) / mTau ) - m) ;
    h = h + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / hTau)))*(- ( ( ( hInf ) ) / hTau ) / ( ( ( ( - 1.0) ) ) / hTau ) - h) ;
   }
  return 0;
}
 
static int  rates ( _threadargsproto_ ) {
    if ( ( v  == - 27.0 ) ) {
     v = v + 0.0001 ;
     }
   mAlpha = ( 0.055 * ( - 27.0 - v ) ) / ( exp ( ( - 27.0 - v ) / 3.8 ) - 1.0 ) ;
   mBeta = ( 0.94 * exp ( ( - 75.0 - v ) / 17.0 ) ) ;
   mInf = mAlpha / ( mAlpha + mBeta ) ;
   mTau = 1.0 / ( mAlpha + mBeta ) ;
   hAlpha = ( 0.000457 * exp ( ( - 13.0 - v ) / 50.0 ) ) ;
   hBeta = ( 0.0065 / ( exp ( ( - v - 15.0 ) / 28.0 ) + 1.0 ) ) ;
   hInf = hAlpha / ( hAlpha + hBeta ) ;
   hTau = 1.0 / ( hAlpha + hBeta ) ;
     return 0; }
 
#if 0 /*BBCORE*/
 
static void _hoc_rates(void) {
  double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   if (_extcall_prop) {_p = _extcall_prop->param; _ppvar = _extcall_prop->dparam;}else{ _p = (double*)0; _ppvar = (Datum*)0; }
  _thread = _extcall_thread;
  _nt = nrn_threads;
 _r = 1.;
 rates ( _p, _ppvar, _thread, _nt ;
 hoc_retpushx(_r);
}
 
#endif /*BBCORE*/
 static void _update_ion_pointer(Datum* _ppvar) {
 }

static void initmodel(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
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
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
    _v = VEC_V(_ni[_iml]);
 v = _v;
  eca = _ion_eca;
 initmodel(_p, _ppvar, _thread, _nt);
 }
}

static double _nrn_current(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _v){double _current=0.;v=_v;{ {
   gCa = gCabar * m * m * h ;
   ica = gCa * ( v - eca ) ;
   }
 _current += ica;

} return _current;
}

static void nrn_cur(_NrnThread* _nt, _Memb_list* _ml, int _type) {
double* _p; Datum* _ppvar; ThreadDatum* _thread;
int* _ni; double _rhs, _v; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
    _v = VEC_V(_ni[_iml]);
  eca = _ion_eca;
 _g = _nrn_current(_p, _ppvar, _thread, _nt, _v + .001);
 	{ double _dica;
  _dica = ica;
 _rhs = _nrn_current(_p, _ppvar, _thread, _nt, _v);
  _ion_dicadv += (_dica - ica)/.001 ;
 	}
 _g = (_g - _rhs)/.001;
  _ion_ica += ica ;
	VEC_RHS(_ni[_iml]) -= _rhs;
 
}
 
}

static void nrn_jacob(_NrnThread* _nt, _Memb_list* _ml, int _type) {
double* _p; Datum* _ppvar; ThreadDatum* _thread;
int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize;
	VEC_D(_ni[_iml]) += _g;
 
}
 
}

static void nrn_state(_NrnThread* _nt, _Memb_list* _ml, int _type) {
#ifdef _PROF_HPM 
HPM_Start("nrn_state_Ca"); 
#endif 
 double _break, _save;
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v; int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
    _v = VEC_V(_ni[_iml]);
 _break = t + .5*dt; _save = t;
 v=_v;
{
  eca = _ion_eca;
 { {
 for (; t < _break; t += dt) {
   states(_p, _ppvar, _thread, _nt);
  
}}
 t = _save;
 } }}
#ifdef _PROF_HPM 
HPM_Stop("nrn_state_Ca"); 
#endif 

}

static void terminal(){}

static void _initlists(){
 double _x; double* _p = &_x;
 int _i; static int _first = 1;
  if (!_first) return;
 _slist1[0] = &(m) - _p;  _dlist1[0] = &(Dm) - _p;
 _slist1[1] = &(h) - _p;  _dlist1[1] = &(Dh) - _p;
_first = 0;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
