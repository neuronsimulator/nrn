#ifndef nmodl1_redef_h
#define nmodl1_redef_h

#define v        _v
#define area     _area
#define thisnode _thisnode
#define GC       _GC
#define EC       _EC
#define extnode  _extnode
#define xain     _xain
#define xbout    _xbout
#define i        _i
#define sec      _sec

#undef nodelist
#undef nodeindices
#undef pdata
#undef prop
#undef nodecount

#define nodelist    _nodelist
#define nodeindices _nodeindices
#define pdata       _pdata
#define prop        _prop
#define nodecount   _nodecount
#define id          _id

#endif
