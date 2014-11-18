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
 
#define nrn_init _nrn_init__InhPoissonStim
#define nrn_cur _nrn_cur__InhPoissonStim
#define _nrn_current _nrn_current__InhPoissonStim
#define nrn_jacob _nrn_jacob__InhPoissonStim
#define nrn_state _nrn_state__InhPoissonStim
#define _net_receive _net_receive__InhPoissonStim 
#define generate_next_event generate_next_event__InhPoissonStim 
#define setRate setRate__InhPoissonStim 
#define setTbins setTbins__InhPoissonStim 
#define setRNGs setRNGs__InhPoissonStim 
#define update_time update_time__InhPoissonStim 
 
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
#define duration _p[0]
#define rmax _p[1]
#define index _p[2]
#define curRate _p[3]
#define start _p[4]
#define event _p[5]
#define v _p[6]
#define _tsav _p[7]
#define _nd_area  _nt->_data[_ppvar[0]]
#define _p_uniform_rng	_nt->_vdata[_ppvar[2]]
#define _p_exp_rng	_nt->_vdata[_ppvar[3]]
#define _p_vecRate	_nt->_vdata[_ppvar[4]]
#define _p_vecTbins	_nt->_vdata[_ppvar[5]]
 
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
 static int hoc_nrnpointerindex =  2;
 static ThreadDatum* _extcall_thread;
 /* external NEURON variables */
 
#if 0 /*BBCORE*/
 /* declaration of user functions */
 static double _hoc_erand();
 static double _hoc_generate_next_event();
 static double _hoc_setRate();
 static double _hoc_setTbins();
 static double _hoc_setRNGs();
 static double _hoc_urand();
 static double _hoc_update_time();
 
#endif /*BBCORE*/
 static int _mechtype;
extern int nrn_get_mechtype();
extern void hoc_register_prop_size(int, int, int);
extern Memb_func* memb_func;
 static int _pointtype;
 
#if 0 /*BBCORE*/
 static void* _hoc_create_pnt(_ho) Object* _ho; { void* create_point_process();
 return create_point_process(_pointtype, _ho);
}
 static void _hoc_destroy_pnt();
 static double _hoc_loc_pnt(_vptr) void* _vptr; {double loc_point_process();
 return loc_point_process(_pointtype, _vptr);
}
 static double _hoc_has_loc(_vptr) void* _vptr; {double has_loc_point();
 return has_loc_point(_vptr);
}
 static double _hoc_get_loc_pnt(_vptr)void* _vptr; {
 double get_loc_point_process(); return (get_loc_point_process(_vptr));
}
 
#endif /*BBCORE*/
 
#if 0 /*BBCORE*/
 /* connect user functions to hoc names */
 static VoidFunc hoc_intfunc[] = {
 0,0
};
 static Member_func _member_func[] = {
 "loc", _hoc_loc_pnt,
 "has_loc", _hoc_has_loc,
 "get_loc", _hoc_get_loc_pnt,
 "erand", _hoc_erand,
 "generate_next_event", _hoc_generate_next_event,
 "setRate", _hoc_setRate,
 "setTbins", _hoc_setTbins,
 "setRNGs", _hoc_setRNGs,
 "urand", _hoc_urand,
 "update_time", _hoc_update_time,
 0, 0
};
 
#endif /*BBCORE*/
#define erand erand_InhPoissonStim
#define urand urand_InhPoissonStim
 extern double erand( _threadargsproto_ );
 extern double urand( _threadargsproto_ );
 /* declare global and static user variables */
#define interval_min interval_min_InhPoissonStim
 double interval_min = 1;
 
#if 0 /*BBCORE*/
 /* some parameters have upper and lower limits */
 static HocParmLimits _hoc_parm_limits[] = {
 "duration", 0, 1e+09,
 0,0,0
};
 static HocParmUnits _hoc_parm_units[] = {
 "duration", "ms",
 0,0
};
 
#endif /*BBCORE*/
 
#if 0 /*BBCORE*/
 /* connect global user variables to hoc */
 static DoubScal hoc_scdoub[] = {
 "interval_min_InhPoissonStim", &interval_min_InhPoissonStim,
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
 
#if 0 /*BBCORE*/
 static void _hoc_destroy_pnt(_vptr) void* _vptr; {
   destroy_point_process(_vptr);
}
 
#endif /*BBCORE*/
 /* connect range variables in _p that hoc is supposed to know about */
 static const char *_mechanism[] = {
 "6.2.0",
"InhPoissonStim",
 "duration",
 0,
 "rmax",
 0,
 0,
 "uniform_rng",
 "exp_rng",
 "vecRate",
 "vecTbins",
 0};
 
static void nrn_alloc(double* _p, Datum* _ppvar, int _type) {
 	/*initialize range parameters*/
 	duration = 1e+06;
 
#if 0 /*BBCORE*/
 
#endif /* BBCORE */
 
}
 static void _initlists();
 
#define _tqitem &(_nt->_vdata[_ppvar[6]])
 static void _net_receive(Point_process*, double*, double);
 
#define _psize 8
#define _ppsize 7
 static void bbcore_read(double *, int*, int*, int*, _threadargsproto_);
 extern void hoc_reg_bbcore_read(int, void(*)(double *, int*, int*, int*, _threadargsproto_));
 extern Symbol* hoc_lookup(const char*);
extern void _nrn_thread_reg(int, int, void(*f)(Datum*));
extern void _nrn_thread_table_reg(int, void(*)(double*, Datum*, ThreadDatum*, _NrnThread*, int));
extern void _cvode_abstol( Symbol**, double*, int);

 void _netstim_inhpoisson_reg() {
	int _vectorized = 1;
  _initlists();
 
#if 0 /*BBCORE*/
 
#endif /*BBCORE*/
 	_pointtype = point_register_mech(_mechanism,
	 nrn_alloc,(void*)0, (void*)0, (void*)0, nrn_init,
	 hoc_nrnpointerindex,
	 NULL/*_hoc_create_pnt*/, NULL/*_hoc_destroy_pnt*/, /*_member_func,*/
	 1);
 _mechtype = nrn_get_mechtype(_mechanism[1]);
   hoc_reg_bbcore_read(_mechtype, bbcore_read);
  hoc_register_prop_size(_mechtype, _psize, _ppsize);
 add_nrn_artcell(_mechtype, 6);
 add_nrn_has_net_event(_mechtype);
 pnt_receive[_mechtype] = _net_receive;
 pnt_receive_size[_mechtype] = 1;
 }
static char *modelname = "";

static int error;
static int _ninits = 0;
static int _match_recurse=1;
static void _modl_cleanup(){ _match_recurse=1;}
static int generate_next_event(_threadargsproto_);
static int setRate(_threadargsproto_);
static int setTbins(_threadargsproto_);
static int setRNGs(_threadargsproto_);
static int update_time(_threadargsproto_);
 
/*VERBATIM*/
extern int ifarg(int iarg);
extern double* vector_vec(void* vv);
extern void* vector_new1(int _i);
extern int vector_capacity(void* vv);
extern void* vector_arg(int iarg);
double nrn_random_pick(void* r);
void* nrn_random_arg(int argpos);
 
/*VERBATIM*/
#include "nrnran123.h"
 
static int  generate_next_event ( _threadargsproto_ ) {
   event = 1000.0 / rmax * erand ( _threadargs_ ) ;
   if ( event < 0.0 ) {
     event = 0.0 ;
     }
    return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_generate_next_event(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 generate_next_event ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
static int  setRNGs ( _threadargsproto_ ) {
   
/*VERBATIM*/
{
#if !NRNBBCORE
        nrnran123_State** pv = (nrnran123_State**)(&_p_exp_rng);
        if (*pv) {
                nrnran123_deletestream(*pv);
                *pv = (nrnran123_State*)0;
        } 
        if (ifarg(2)) {
                *pv = nrnran123_newstream((uint32_t)*getarg(1), (uint32_t)*getarg(2));
        }

        pv = (nrnran123_State**)(&_p_uniform_rng);
        if (*pv) {
                nrnran123_deletestream(*pv);
                *pv = (nrnran123_State*)0;
        } 
        if (ifarg(4)) {
                *pv = nrnran123_newstream((uint32_t)*getarg(3), (uint32_t)*getarg(4));
        }
#endif
}
  return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_setRNGs(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 setRNGs ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
double urand ( _threadargsproto_ ) {
   double _lurand;
 
/*VERBATIM*/
	if (_p_uniform_rng) {
		/*
		:Supports separate independent but reproducible streams for
		: each instance. However, the corresponding hoc Random
		: distribution MUST be set to Random.uniform(0,1)
		*/
		_lurand = nrnran123_dblpick((nrnran123_State*)_p_uniform_rng);
	}else{
          _lurand = 0.0;
  	  hoc_execerror("multithread random in NetStim"," only via hoc Random");
	}
 
return _lurand;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_urand(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  urand ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
double erand ( _threadargsproto_ ) {
   double _lerand;
 
/*VERBATIM*/
	if (_p_exp_rng) {
		/*
		:Supports separate independent but reproducible streams for
		: each instance. However, the corresponding hoc Random
		: distribution MUST be set to Random.negexp(1)
		*/
		_lerand = nrnran123_negexp((nrnran123_State*)_p_exp_rng);
	}else{
          _lerand = 0.0;
  	  hoc_execerror("multithread random in NetStim"," only via hoc Random");
	}
 
return _lerand;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_erand(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  erand ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
/*VERBATIM*/
static void bbcore_write(double* dArray, int* iArray, int* doffset, int* ioffset, _threadargsproto_) {
        uint32_t dsize = 0;
        if (_p_vecRate)
        {
          dsize = (uint32_t)vector_capacity(_p_vecRate);
        }
        if (iArray) {
                uint32_t* ia = ((uint32_t*)iArray) + *ioffset;
                nrnran123_State** pv = (nrnran123_State**)(&_p_exp_rng);
                nrnran123_getids(*pv, ia, ia+1);

                ia = ia + 2;
                pv = (nrnran123_State**)(&_p_uniform_rng);
                nrnran123_getids(*pv, ia, ia+1);

                ia = ia + 2;
                void* vec = _p_vecRate;
                ia[0] = dsize;

                double *da = dArray + *doffset;
                double *dv;
                if(dsize)
                {
                  dv = vector_vec(vec);
                }
                int iInt;
                for (iInt = 0; iInt < dsize; ++iInt)
                {
                  da[iInt] = dv[iInt];
                }

                vec = _p_vecTbins;
                da = dArray + *doffset + dsize;
                if(dsize)
                {
                  dv = vector_vec(vec);
                }
                for (iInt = 0; iInt < dsize; ++iInt)
                {
                  da[iInt] = dv[iInt];
                }
        }
        *ioffset += 5;
        *doffset += 2*dsize;

}

static void bbcore_read(double* dArray, int* iArray, int* doffset, int* ioffset, _threadargsproto_) {
        assert(!_p_exp_rng);
        assert(!_p_uniform_rng);
        assert(!_p_vecRate);
        assert(!_p_vecTbins);
        uint32_t* ia = ((uint32_t*)iArray) + *ioffset;
        nrnran123_State** pv;
        if (ia[0] != 0 || ia[1] != 0)
        {
          pv = (nrnran123_State**)(&_p_exp_rng);
          *pv = nrnran123_newstream(ia[0], ia[1]);
        }

        ia = ia + 2;
        if (ia[0] != 0 || ia[1] != 0)
        {
          pv = (nrnran123_State**)(&_p_uniform_rng);
          *pv = nrnran123_newstream(ia[0], ia[1]);
        }

        ia = ia + 2;
        int dsize = ia[0];
        *ioffset += 5;

        double *da = dArray + *doffset;
        _p_vecRate = vector_new1(dsize);  /* works for dsize=0 */
        double *dv = vector_vec(_p_vecRate);
        int iInt;
        for (iInt = 0; iInt < dsize; ++iInt)
        {
          dv[iInt] = da[iInt];
        }
        *doffset += dsize;

        da = dArray + *doffset;
        _p_vecTbins = vector_new1(dsize);
        dv = vector_vec(_p_vecTbins);
        for (iInt = 0; iInt < dsize; ++iInt)
        {     
          dv[iInt] = da[iInt];
        }
        *doffset += dsize; 
}
 
static int  setTbins ( _threadargsproto_ ) {
   
/*VERBATIM*/
#if !NRNBBCORE
  void** vv;
  vv = &_p_vecTbins;
  *vv = (void*)0;

  if (ifarg(1)) {
    *vv = vector_arg(1);
  }
#endif
  return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_setTbins(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 setTbins ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
static int  setRate ( _threadargsproto_ ) {
   
/*VERBATIM*/
#if !NRNBBCORE
  void** vv;
  vv = &_p_vecRate;
  *vv = (void*)0;

  if (ifarg(1)) {
    *vv = vector_arg(1);

    int size = vector_capacity(*vv);
    int i;
    double max=0.0;
    double* px = vector_vec(*vv);
    for (i=0;i<size;i++) {
    	if (px[i]>max) max = px[i];
    }
    rmax = max;
  }
#endif
  return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_setRate(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 setRate ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
static int  update_time ( _threadargsproto_ ) {
   
/*VERBATIM*/
  void* vv; int i, i_prev, size; double* px;
  i = (int)index;
  i_prev = i;

  if (i >= 0) { // are we disabled?
    vv = _p_vecTbins;
    if (vv) {
      size = vector_capacity(vv);
      px = vector_vec(vv);
      /* advance to current tbins without exceeding array bounds */
      while ((i+1 < size) && (t>=px[i+1])) {
	index += 1.;
	i += 1;
      }
      /* did the index change? */
      if (i!=i_prev) {
        /* advance curRate to next vecRate if possible */
        void *vvRate = _p_vecRate;
        if (vvRate && vector_capacity(vvRate)>i) {
          px = vector_vec(vvRate);
          curRate = px[i];
        }
        else curRate = 1.0;
      }

      /* have we hit last bin? ... disable time advancing leaving curRate as it is*/
      if (i==size)
        index = -1.;

    } else { /* no vecTbins, use some defaults */
      rmax = 1.0;
      curRate = 1.0;
      index = -1.; /* no vecTbins ... disable time advancing & Poisson unit rate. */
    }
  }

  return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_update_time(void* _vptr) {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 update_time ( _p, _ppvar, _thread, _nt );
 return(_r);
}
 
#endif /*BBCORE*/
 
static void _net_receive (_pnt, _args, _lflag) Point_process* _pnt; double* _args; double _lflag; 
{  double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _thread = (ThreadDatum*)0; _nt = (_NrnThread*)_pnt->_vnt;   _p = _pnt->_data; _ppvar = _pnt->_pdata;
  assert(_tsav <= t); _tsav = t;   if (_lflag == 1. ) {*(_tqitem) = 0;}
 {
   if ( _lflag  == 0.0 ) {
     update_time ( _threadargs_ ) ;
     generate_next_event ( _threadargs_ ) ;
     if ( t + event < start + duration ) {
       artcell_net_send ( _tqitem, _args, _pnt, t +  event , 0.0 ) ;
       }
     
/*VERBATIM*/
    
          double u = (double)urand(_threadargs_);
	  /*printf("InhPoisson: spike time at time %g urand=%g curRate=%g, rmax=%g, curRate/rmax=%g \n",t, u, curRate, rmax, curRate/rmax);*/
	  if (u<curRate/rmax) {
 net_event ( _pnt, t ) ;
     
/*VERBATIM*/
    
          }
 }
   } }

static void initmodel(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
  int _i; double _save;{
 {
   index = 0. ;
   
/*VERBATIM*/
   if (_p_uniform_rng && _p_exp_rng)
   {
     nrnran123_setseq((nrnran123_State*)_p_uniform_rng, 0, 0);
     nrnran123_setseq((nrnran123_State*)_p_exp_rng, 0, 0);
   }

   void *vvTbins = _p_vecTbins;
   double* px;

   if (vvTbins && vector_capacity(vvTbins)>=1) {
     px = vector_vec(vvTbins);
     start = px[0];
     if (start < 0.0) start=0.0;
   }
   else start = 0.0;

   /* first event is at the start 
   TODO: This should draw from a more appropriate dist
   that has the surrogate process starting a t=-inf
   */
   event = start;

   /* set curRate */
   void *vvRate = _p_vecRate;
   px = vector_vec(vvRate);

   /* set rmax */
   rmax = 0.0;
   int i;
   for (i=0;i<vector_capacity(vvRate);i++) {
      if (px[i]>rmax) rmax = px[i];
   }

   if (vvRate && vector_capacity(vvRate)>0) {
     curRate = px[0];
   }
   else {
      curRate = 1.0;
      rmax = 1.0;
   }

 update_time ( _threadargs_ ) ;
   erand ( _threadargs_ ) ;
   generate_next_event ( _threadargs_ ) ;
   if ( t + event < start + duration ) {
     artcell_net_send ( _tqitem, (double*)0, _nt->_vdata[_ppvar[1]], t +  event , 0.0 ) ;
     }
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
 _tsav = -1e20;
 initmodel(_p, _ppvar, _thread, _nt);
}
}

static double _nrn_current(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _v){double _current=0.;v=_v;{
} return _current;
}

static void nrn_state(_NrnThread* _nt, _Memb_list* _ml, int _type) {
#ifdef _PROF_HPM 
HPM_Start("nrn_state_netstim_inhpoisson"); 
#endif 
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v = 0.0; int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
 v=_v;
{
}}
#ifdef _PROF_HPM 
HPM_Stop("nrn_state_netstim_inhpoisson"); 
#endif 

}

static void terminal(){}

static void _initlists(){
 double _x; double* _p = &_x;
 int _i; static int _first = 1;
  if (!_first) return;
_first = 0;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
