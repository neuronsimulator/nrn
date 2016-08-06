#ifndef mod2c_core_thread_h
#define mod2c_core_thread_h

#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_ml.h"

#if !defined(LAYOUT)
/* 1 means AoS, >1 means AoSoA, <= 0 means SOA */
#define LAYOUT 1
#endif
#if LAYOUT >= 1
#define _STRIDE LAYOUT
#else
#define _STRIDE _cntml_padded + _iml
#endif

#define _threadargscomma_ _iml, _cntml_padded, \
  _p, _ppvar, _thread, _nt, _v,
#define _threadargsprotocomma_ int _iml, int _cntml_padded, \
  double* _p, Datum* _ppvar, ThreadDatum* _thread, NrnThread* _nt, double _v,
#define _threadargs_ _iml, _cntml_padded, \
  _p, _ppvar, _thread, _nt, _v
#define _threadargsproto_ int _iml, int _cntml_padded, \
  double* _p, Datum* _ppvar, ThreadDatum* _thread, NrnThread* _nt, double _v

#endif
