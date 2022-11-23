#pragma once
#include "hocdec.h"
#include "options.h"  // for CACHEVEC

struct Node;
struct Prop;

struct Memb_list {
    Node** nodelist;
#if CACHEVEC != 0
    /* nodeindices contains all nodes this extension is responsible for,
     * ordered according to the matrix. This allows to access the matrix
     * directly via the nrn_actual_* arrays instead of accessing it in the
     * order of insertion and via the node-structure, making it more
     * cache-efficient */
    int* nodeindices;
#endif /* CACHEVEC */
    double** _data;
    Datum** pdata;
    Prop** prop;
    Datum* _thread; /* thread specific data (when static is no good) */
    int nodecount;
};
