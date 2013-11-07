/* Created by Language version: 6.2.0 */
/* VECTORIZED */
#include "md1redef.h"
#include "nrnconf.h"
#include "md2redef.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#undef PI
 
#if METHOD3
extern int _method3;
#endif

#define _threadargscomma_ _p, _ppvar, _thread, _nt,
#define _threadargs_ _p, _ppvar, _thread, _nt
 	/*SUPPRESS 761*/
	/*SUPPRESS 762*/
	/*SUPPRESS 763*/
	/*SUPPRESS 765*/
	 extern double *getarg();
 /* Thread safe. No static _p or _ppvar. */
 
#define t _nt->_t
#define dt _nt->_dt
#define gnabar _p[0]
#define gkbar _p[1]
#define gl _p[2]
#define el _p[3]
#define gna _p[4]
#define gk _p[5]
#define il _p[6]
#define m _p[7]
#define h _p[8]
#define n _p[9]
#define Dm _p[10]
#define Dh _p[11]
#define Dn _p[12]
#define ena _p[13]
#define ek _p[14]
#define ina _p[15]
#define ik _p[16]
#define v _p[17]
#define _g _p[18]
#define _ion_ena	_nt->_data[_ppvar[0]]
#define _ion_ina	_nt->_data[_ppvar[1]]
#define _ion_dinadv	_nt->_data[_ppvar[2]]
#define _ion_ek	_nt->_data[_ppvar[3]]
#define _ion_ik	_nt->_data[_ppvar[4]]
#define _ion_dikdv	_nt->_data[_ppvar[5]]
 
 static int hoc_nrnpointerindex =  -1;
 static ThreadDatum* _extcall_thread;
 /* external NEURON variables */
 extern double celsius;
 /* declaration of user functions */
 static int _mechtype;
extern int nrn_get_mechtype();
#define vtrap vtrap_hh
 extern double vtrap();
 
static void _check_rates(double*, Datum*, ThreadDatum*, _NrnThread*); 
static void _check_table_thread(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, int _type) {
   _check_rates(_p, _ppvar, _thread, _nt);
 }
 /* declare global and static user variables */
 static int _thread1data_inuse = 0;
static double _thread1data[6];
#define _gth 0
#define htau_hh _thread1data[0]
#define htau _thread[_gth]._pval[0]
#define hinf_hh _thread1data[1]
#define hinf _thread[_gth]._pval[1]
#define mtau_hh _thread1data[2]
#define mtau _thread[_gth]._pval[2]
#define minf_hh _thread1data[3]
#define minf _thread[_gth]._pval[3]
#define ntau_hh _thread1data[4]
#define ntau _thread[_gth]._pval[4]
#define ninf_hh _thread1data[5]
#define ninf _thread[_gth]._pval[5]
#define usetable usetable_hh
 double usetable = 1;

 static double delta_t = 0.01;
 static double h0 = 0;
 static double m0 = 0;
 static double n0 = 0;

 static double _sav_indep;
 static void nrn_alloc(), nrn_init(), nrn_state();
 static void nrn_cur(), nrn_jacob();
 
 /* connect range variables in _p that hoc is supposed to know about */
 static char *_mechanism[] = {
 "6.2.0",
"hh",
 "gnabar_hh",
 "gkbar_hh",
 "gl_hh",
 "el_hh",
 0,
 "gna_hh",
 "gk_hh",
 "il_hh",
 0,
 "m_hh",
 "h_hh",
 "n_hh",
 0,
 0};
 static Symbol* _na_sym;
 static Symbol* _k_sym;
 
static void nrn_alloc(double* _p, Datum* _ppvar, int _type) {
 	/*initialize range parameters*/
 	gnabar = 0.12;
 	gkbar = 0.036;
 	gl = 0.0003;
 	el = -54.3;
 	/*connect ionic variables to this model*/
	/* already done */
}
 static _initlists();
 static void _thread_mem_init(ThreadDatum*);
 static void _thread_cleanup(ThreadDatum*);
 _hh_reg_() {
	int _vectorized = 1;
  _initlists();
 	register_mech(_mechanism, nrn_alloc,nrn_cur, nrn_jacob, nrn_state, nrn_init, hoc_nrnpointerindex, 2);
  _extcall_thread = (ThreadDatum*)ecalloc(1, sizeof(ThreadDatum));
  _thread_mem_init(_extcall_thread);
  _thread1data_inuse = 0;
 _mechtype = nrn_get_mechtype(_mechanism[1]);
     _nrn_thread_reg1(_mechtype, _thread_mem_init);
     _nrn_thread_reg0(_mechtype, _thread_cleanup);
     _nrn_thread_table_reg(_mechtype, _check_table_thread);
  hoc_register_prop_size(_mechtype, 7, 19);
 }
 static double *_t_minf;
 static double *_t_mtau;
 static double *_t_hinf;
 static double *_t_htau;
 static double *_t_ninf;
 static double *_t_ntau;
static int _reset;
static char *modelname = "hh.mod   squid sodium, potassium, and leak channels";

static int error;
static int _ninits = 0;
static int _match_recurse=1;
static _modl_cleanup(){ _match_recurse=1;}
static _f_rates();
static rates();
 
 static _n_rates();
 static int _slist1[3], _dlist1[3];
 static int states();
 
 static int states (double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) { {
   rates ( _threadargscomma_ v ) ;
    m = m + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / mtau)))*(- ( ( ( minf ) ) / mtau ) / ( ( ( ( - 1.0) ) ) / mtau ) - m) ;
    h = h + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / htau)))*(- ( ( ( hinf ) ) / htau ) / ( ( ( ( - 1.0) ) ) / htau ) - h) ;
    n = n + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / ntau)))*(- ( ( ( ninf ) ) / ntau ) / ( ( ( ( - 1.0) ) ) / ntau ) - n) ;
   }
  return 0;
}
 static double _mfac_rates, _tmin_rates;
  static void _check_rates(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
  static int _maktable=1; int _i, _j, _ix = 0;
  double _xi, _tmax;
  static double _sav_celsius;
  if (!usetable) {return;}
  if (_sav_celsius != celsius) { _maktable = 1;}
  if (_maktable) { double _x, _dx; _maktable=0;
   _tmin_rates =  - 100.0 ;
   _tmax =  100.0 ;
   _dx = (_tmax - _tmin_rates)/200.; _mfac_rates = 1./_dx;
   for (_i=0, _x=_tmin_rates; _i < 201; _x += _dx, _i++) {
    _f_rates(_p, _ppvar, _thread, _nt, _x);
    _t_minf[_i] = minf;
    _t_mtau[_i] = mtau;
    _t_hinf[_i] = hinf;
    _t_htau[_i] = htau;
    _t_ninf[_i] = ninf;
    _t_ntau[_i] = ntau;
   }
   _sav_celsius = celsius;
  }
 }

 static rates(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _lv) { 
#if 0
_check_rates(_p, _ppvar, _thread, _nt);
#endif
 _n_rates(_p, _ppvar, _thread, _nt, _lv);
 return;
 }

 static _n_rates(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _lv){ int _i, _j;
 double _xi, _theta;
 if (!usetable) {
 _f_rates(_p, _ppvar, _thread, _nt, _lv); return; 
}
 _xi = _mfac_rates * (_lv - _tmin_rates);
 _i = (int) _xi;
 if (_xi <= 0.) {
 minf = _t_minf[0];
 mtau = _t_mtau[0];
 hinf = _t_hinf[0];
 htau = _t_htau[0];
 ninf = _t_ninf[0];
 ntau = _t_ntau[0];
 return; }
 if (_i >= 200) {
 minf = _t_minf[200];
 mtau = _t_mtau[200];
 hinf = _t_hinf[200];
 htau = _t_htau[200];
 ninf = _t_ninf[200];
 ntau = _t_ntau[200];
 return; }
 _theta = _xi - (double)_i;
 minf = _t_minf[_i] + _theta*(_t_minf[_i+1] - _t_minf[_i]);
 mtau = _t_mtau[_i] + _theta*(_t_mtau[_i+1] - _t_mtau[_i]);
 hinf = _t_hinf[_i] + _theta*(_t_hinf[_i+1] - _t_hinf[_i]);
 htau = _t_htau[_i] + _theta*(_t_htau[_i+1] - _t_htau[_i]);
 ninf = _t_ninf[_i] + _theta*(_t_ninf[_i+1] - _t_ninf[_i]);
 ntau = _t_ntau[_i] + _theta*(_t_ntau[_i+1] - _t_ntau[_i]);
 }

 
static int  _f_rates ( double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread*_nt, double _lv ) {
   double _lalpha , _lbeta , _lsum , _lq10 ;
  _lq10 = pow( 3.0 , ( ( celsius - 6.3 ) / 10.0 ) ) ;
   _lalpha = .1 * vtrap ( _threadargscomma_ - ( _lv + 40.0 ) , 10.0 ) ;
   _lbeta = 4.0 * exp ( - ( _lv + 65.0 ) / 18.0 ) ;
   _lsum = _lalpha + _lbeta ;
   mtau = 1.0 / ( _lq10 * _lsum ) ;
   minf = _lalpha / _lsum ;
   _lalpha = .07 * exp ( - ( _lv + 65.0 ) / 20.0 ) ;
   _lbeta = 1.0 / ( exp ( - ( _lv + 35.0 ) / 10.0 ) + 1.0 ) ;
   _lsum = _lalpha + _lbeta ;
   htau = 1.0 / ( _lq10 * _lsum ) ;
   hinf = _lalpha / _lsum ;
   _lalpha = .01 * vtrap ( _threadargscomma_ - ( _lv + 55.0 ) , 10.0 ) ;
   _lbeta = .125 * exp ( - ( _lv + 65.0 ) / 80.0 ) ;
   _lsum = _lalpha + _lbeta ;
   ntau = 1.0 / ( _lq10 * _lsum ) ;
   ninf = _lalpha / _lsum ;
    return 0; }
 
double vtrap ( double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _lx , double _ly ) {
   double _lvtrap;
 if ( fabs ( _lx / _ly ) < 1e-6 ) {
     _lvtrap = _ly * ( 1.0 - _lx / _ly / 2.0 ) ;
     }
   else {
     _lvtrap = _lx / ( exp ( _lx / _ly ) - 1.0 ) ;
     }
   
return _lvtrap;
 }
 
static void _thread_mem_init(ThreadDatum* _thread) {
  if (_thread1data_inuse) {_thread[_gth]._pval = (double*)ecalloc(6, sizeof(double));
 }else{
 _thread[_gth]._pval = _thread1data; _thread1data_inuse = 1;
 }
 }
 
static void _thread_cleanup(ThreadDatum* _thread) {
  if (_thread[_gth]._pval == _thread1data) {
   _thread1data_inuse = 0;
  }else{
   free((void*)_thread[_gth]._pval);
  }
 }

static void initmodel(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
  int _i; double _save;{
  h = h0;
  m = m0;
  n = n0;
 {
   rates ( _threadargscomma_ v ) ;
   m = minf ;
   h = hinf ;
   n = ninf ;
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
 _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];

#if 0
 _check_rates(_p, _ppvar, _thread, _nt);
#endif
#if CACHEVEC
    _v = VEC_V(_ni[_iml]);
#endif
 v = _v;
  ena = _ion_ena;
  ek = _ion_ek;
 initmodel(_p, _ppvar, _thread, _nt);
  }}

static double _nrn_current(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _v){double _current=0.;v=_v;{ {
   gna = gnabar * m * m * m * h ;
   ina = gna * ( v - ena ) ;
   gk = gkbar * n * n * n * n ;
   ik = gk * ( v - ek ) ;
   il = gl * ( v - el ) ;
   }
 _current += ina;
 _current += ik;
 _current += il;

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
 _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];
#if CACHEVEC
    _v = VEC_V(_ni[_iml]);
#endif
  ena = _ion_ena;
  ek = _ion_ek;
 _g = _nrn_current(_p, _ppvar, _thread, _nt, _v + .001);
 	{ double _dik;
 double _dina;
  _dina = ina;
  _dik = ik;
 _rhs = _nrn_current(_p, _ppvar, _thread, _nt, _v);
  _ion_dinadv += (_dina - ina)/.001 ;
  _ion_dikdv += (_dik - ik)/.001 ;
 	}
 _g = (_g - _rhs)/.001;
  _ion_ina += ina ;
  _ion_ik += ik ;
#if CACHEVEC
	VEC_RHS(_ni[_iml]) -= _rhs;
#endif
}}

static void nrn_jacob(_NrnThread* _nt, _Memb_list* _ml, int _type) {
double* _p; Datum* _ppvar; ThreadDatum* _thread;
int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data[_iml];
#if CACHEVEC
	VEC_D(_ni[_iml]) += _g;
#endif
}}

static void nrn_state(_NrnThread* _nt, _Memb_list* _ml, int _type) {
 double _break, _save;
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v; int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];
#if CACHEVEC
    _v = VEC_V(_ni[_iml]);
#endif
 _break = t + .5*dt; _save = t;
 v=_v;
{
  ena = _ion_ena;
  ek = _ion_ek;
 { {
 for (; t < _break; t += dt) {
   states(_p, _ppvar, _thread, _nt);
  
}}
 t = _save;
 }  }}

}

static terminal(){}

static _initlists(){
 double _x; double* _p = &_x;
 int _i; static int _first = 1;
  if (!_first) return;
 _slist1[0] = &(m) - _p;  _dlist1[0] = &(Dm) - _p;
 _slist1[1] = &(h) - _p;  _dlist1[1] = &(Dh) - _p;
 _slist1[2] = &(n) - _p;  _dlist1[2] = &(Dn) - _p;
   _t_minf = makevector(201*sizeof(double));
   _t_mtau = makevector(201*sizeof(double));
   _t_hinf = makevector(201*sizeof(double));
   _t_htau = makevector(201*sizeof(double));
   _t_ninf = makevector(201*sizeof(double));
   _t_ntau = makevector(201*sizeof(double));
_first = 0;
}
