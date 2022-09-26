#pragma once
extern void hoc_register_prop_size(int type, int psize, int dpsize);

#include "nrnoc_ml.h"

typedef struct NrnThread NrnThread;
struct Symbol;

typedef Datum* (*Pfrpdat)();
typedef void (*Pvmi)(struct NrnThread*, Memb_list*, int);
typedef void (*Pvmp)(Prop*);
typedef int (*nrn_ode_count_t)(int);
typedef void (*nrn_ode_map_t)(int, double**, double**, double*, Datum*, double*, int);
typedef void (*nrn_ode_synonym_t)(int, double**, Datum**);
/* eventually replace following with Pvmp */
typedef void (*nrn_bamech_t)(Node*, double*, Datum*, Datum*, struct NrnThread*);

#define NULL_CUR        (Pfri) 0
#define NULL_ALLOC      (Pfri) 0
#define NULL_STATE      (Pfri) 0
#define NULL_INITIALIZE (Pfri) 0

struct Memb_func {
    Pvmp alloc;
    Pvmi current;
    Pvmi jacob;
    Pvmi state;
    bool has_initialize() const {
        return m_initialize;
    }
    void invoke_initialize(NrnThread* nt, Memb_list* ml, int type) const;
    void set_initialize(Pvmi init) {
        m_initialize = init;
    }
    Pvmp destructor; /* only for point processes */
    Symbol* sym;
    nrn_ode_count_t ode_count;
    nrn_ode_map_t ode_map;
    Pvmi ode_spec;
    Pvmi ode_matsol;
    nrn_ode_synonym_t ode_synonym;
    Pvmi singchan_; /* managed by kschan for variable step methods */
    int vectorized;
    int thread_size_;                 /* how many Datum needed in Memb_list if vectorized */
    void (*thread_mem_init_)(Datum*); /* after Memb_list._thread is allocated */
    void (*thread_cleanup_)(Datum*);  /* before Memb_list._thread is freed */
    void (*thread_table_check_)(double*, Datum*, Datum*, NrnThread*, int);
    void (*_update_ion_pointers)(Datum*);
    int is_point;
    void* hoc_mech;
    void (*setdata_)(struct Prop*);
    int* dparam_semantics;  // for nrncore writing.
  private:
    Pvmi m_initialize{};
};


#define IMEMFAST     -2
#define VINDEX       -1
#define CABLESECTION 1
#define MORPHOLOGY   2
#define CAP          3
#if EXTRACELLULAR
#define EXTRACELL 5
#endif

#define nrnocCONST 1
#define DEP        2
#define STATE      3 /*See init.cpp and cabvars.h for order of nrnocCONST, DEP, and STATE */

#define BEFORE_INITIAL    0
#define AFTER_INITIAL     1
#define BEFORE_BREAKPOINT 2
#define AFTER_SOLVE       3
#define BEFORE_STEP       4
#define BEFORE_AFTER_SIZE 5 /* 1 more than the previous */
typedef struct BAMech {
    nrn_bamech_t f;
    int type;
    struct BAMech* next;
} BAMech;
extern BAMech** bamech_;

extern Memb_func* memb_func;
extern int n_memb_func;
extern int* nrn_prop_param_size_;
extern int* nrn_prop_dparam_size_;

extern Memb_list* memb_list;
/* for finitialize, order is same up through extracellular, then ions,
then mechanisms that write concentrations, then all others. */
extern short* memb_order_;
#define NRNPOINTER                                                            \
    4 /* added on to list of mechanism variables.These are                    \
pointers which connect variables  from other mechanisms via the _ppval array. \
*/

#define _AMBIGUOUS 5
