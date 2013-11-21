/* Created by Language version: 6.2.0 */
/* VECTORIZED */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simcore/nrnoc/md1redef.h"
#include "simcore/nrnconf.h"
#include "simcore/nrnoc/multicore.h"
#include "simcore/nrnoc/md2redef.h"

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
#define tau _p[0]
#define e _p[1]
#define i _p[2]
#define g _p[3]
#define Dg _p[4]
#define v _p[5]
#define _g _p[6]
#define _tsav _p[7]
#define _nd_area  _nt->_data[_ppvar[0]]
 
 static int hoc_nrnpointerindex =  -1;
 static Datum* _extcall_thread;
 /* external NEURON variables */
 /* declaration of user functions */
 static int _mechtype;
extern int nrn_get_mechtype();
 static int _pointtype;
 static void _hoc_destroy_pnt();
 static double delta_t = 0.01;
 static double g0 = 0;

 static double _sav_indep;
 static void nrn_alloc(), nrn_init(), nrn_state();
 static void nrn_cur(), nrn_jacob();
 
static int _ode_count(), _ode_map(), _ode_spec(), _ode_matsol();
 
 /* connect range variables in _p that hoc is supposed to know about */
 static const char *_mechanism[] = {
 "6.2.0",
"ExpSyn",
 "tau",
 "e",
 0,
 "i",
 0,
 "g",
 0,
 0};
 
static void nrn_alloc(double* _p, Datum* _ppvar, int _type) {
 	tau = 0.1;
 	e = 0;
}
 static _initlists();
  /* some states have an absolute tolerance */
 static Symbol** _atollist;
 static _net_receive();
 typedef (*_Pfrv)();
 extern _Pfrv* pnt_receive;
 extern short* pnt_receive_size;
 _expsyn_reg_() {
	int _vectorized = 1;
  _initlists();
 	_pointtype = point_register_mech(_mechanism,
	 nrn_alloc,nrn_cur, nrn_jacob, nrn_state, nrn_init,
	 hoc_nrnpointerindex,
	NULL, NULL,
	 1);
 _mechtype = nrn_get_mechtype(_mechanism[1]);
  hoc_register_prop_size(_mechtype, 8, 3);
  hoc_register_dparam_semantics(_mechtype, 0, "area");
  hoc_register_dparam_semantics(_mechtype, 1, "pntproc");
  hoc_register_dparam_semantics(_mechtype, 2, "cvodeieq");
 pnt_receive[_mechtype] = _net_receive;
 pnt_receive_size[_mechtype] = 1;
 }
static int _reset;
static char *modelname = "";

static int error;
static int _ninits = 0;
static int _match_recurse=1;
static _modl_cleanup(){ _match_recurse=1;}
 static int _slist1[1], _dlist1[1];
 static int state(); 
 
 static int state (double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) { {
    g = g + (1. - exp(dt*(( - 1.0 ) / tau)))*(- ( 0.0 ) / ( ( - 1.0 ) / tau ) - g) ;
   }
  return 0;
}
 
static _net_receive (_pnt, _args, _lflag) Point_process* _pnt; double* _args; double _lflag; 
{  double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _thread = (ThreadDatum*)0; _nt = (_NrnThread*)_pnt->_vnt;   _p = _pnt->_data; _ppvar = _pnt->_pdata;
  if (_tsav > t){ assert (t <= _tsav);}
 _tsav = t; {
   g = g + _args[0] ;
   } }
 
static void initmodel(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
  int _i; double _save;{
  g = g0;
 {
   g = 0.0 ;
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
 _p = _ml->_data + _iml*8; _ppvar = _ml->_pdata + _iml*3;
 _tsav = -1e20;
#if CACHEVEC
    _v = VEC_V(_ni[_iml]);
#endif
 v = _v;
 initmodel(_p, _ppvar, _thread, _nt);
}}

static double _nrn_current(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _v){double _current=0.;v=_v;{ {
   i = g * ( v - e ) ;
   }
 _current += i;

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
 _p = _ml->_data + _iml*8; _ppvar = _ml->_pdata + _iml*3;
#if CACHEVEC
    _v = VEC_V(_ni[_iml]);
#endif
 _g = _nrn_current(_p, _ppvar, _thread, _nt, _v + .001);
 	{ _rhs = _nrn_current(_p, _ppvar, _thread, _nt, _v);
 	}
 _g = (_g - _rhs)/.001;
 _g *=  1.e2/(_nd_area);
 _rhs *= 1.e2/(_nd_area);
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
 _p = _ml->_data + _iml*8;
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
 _p = _ml->_data + _iml*8; _ppvar = _ml->_pdata + _iml*3;
#if CACHEVEC
    _v = VEC_V(_ni[_iml]);
#endif
 _break = t + .5*dt; _save = t;
 v=_v;
{
 { {
 for (; t < _break; t += dt) {
   state(_p, _ppvar, _thread, _nt);
  
}}
 t = _save;
 }}}

}

static terminal(){}

static _initlists(){
 double _x; double* _p = &_x;
 int _i; static int _first = 1;
  if (!_first) return;
 _slist1[0] = &(g) - _p;  _dlist1[0] = &(Dg) - _p;
_first = 0;
}
