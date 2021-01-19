/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nmodl1_redef_h
#define nmodl1_redef_h

#define v _v
#define area _area
#define thisnode _thisnode
#define GC _GC
#define EC _EC
#define extnode _extnode
#define xain _xain
#define xbout _xbout
#define i _i
#define sec _sec

#undef Memb_list
#undef nodelist
#undef nodeindices
#undef data
#undef pdata
#undef prop
#undef nodecount
#undef pval
#undef id
#undef weights
#undef weight_index_

#define nodelist _nodelist
#define nodeindices _nodeindices
#define data _data
#define pdata _pdata
#define prop _prop
#define nodecount _nodecount
#define pval _pval
#define id _id
#define weights _weights
#define weight_index_ _weight_index

#endif
