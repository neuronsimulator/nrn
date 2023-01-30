#pragma once
extern void hoc_register_prop_size(int type, int psize, int dpsize);

#include "neuron/container/data_handle.hpp"
#include "neuron/model_data.hpp"
#include "nrnoc_ml.h"

#include <vector>

typedef struct NrnThread NrnThread;
struct Symbol;

typedef Datum* (*Pfrpdat)();
typedef void (*Pvmi)(struct NrnThread*, Memb_list*, int);
typedef void (*Pvmp)(Prop*);
typedef int (*nrn_ode_count_t)(int);
using nrn_bamech_t = void (*)(Node*,
                              Datum*,
                              Datum*,
                              NrnThread*,
                              Memb_list*,
                              std::size_t,
                              neuron::model_sorted_token const&);
using nrn_cur_t = void (*)(neuron::model_sorted_token const&, NrnThread*, Memb_list*, int);
using nrn_init_t = nrn_cur_t;
using nrn_jacob_t = nrn_cur_t;
using nrn_ode_map_t = void (*)(Prop*,
                               int /* ieq */,
                               neuron::container::data_handle<double>* /* pv (std::span) */,
                               neuron::container::data_handle<double>* /* pvdot (std::span) */,
                               double* /* atol */,
                               int /* type */);
using nrn_ode_matsol_t = nrn_cur_t;
using nrn_ode_spec_t = nrn_cur_t;
using nrn_ode_synonym_t = void (*)(neuron::model_sorted_token const&, NrnThread&, Memb_list&, int);
using nrn_state_t = nrn_cur_t;
using nrn_thread_table_check_t = void (*)(Memb_list*,
                                          std::size_t,
                                          Datum*,
                                          Datum*,
                                          NrnThread*,
                                          int,
                                          neuron::model_sorted_token const&);

#define NULL_CUR        (Pfri) 0
#define NULL_ALLOC      (Pfri) 0
#define NULL_STATE      (Pfri) 0
#define NULL_INITIALIZE (Pfri) 0

struct Memb_func {
    Pvmp alloc;
    nrn_cur_t current;
    nrn_jacob_t jacob;
    nrn_state_t state;
    bool has_initialize() const {
        return m_initialize;
    }
    void invoke_initialize(neuron::model_sorted_token const& sorted_token,
                           NrnThread* nt,
                           Memb_list* ml,
                           int type) const;
    void set_initialize(nrn_init_t init) {
        m_initialize = init;
    }
    Pvmp destructor; /* only for point processes */
    Symbol* sym;
    nrn_ode_count_t ode_count;
    nrn_ode_map_t ode_map;
    nrn_ode_spec_t ode_spec;
    nrn_ode_matsol_t ode_matsol;
    nrn_ode_synonym_t ode_synonym;
    Pvmi singchan_; /* managed by kschan for variable step methods */
    int vectorized;
    int thread_size_;                 /* how many Datum needed in Memb_list if vectorized */
    void (*thread_mem_init_)(Datum*); /* after Memb_list._thread is allocated */
    void (*thread_cleanup_)(Datum*);  /* before Memb_list._thread is freed */
    nrn_thread_table_check_t thread_table_check_;
    int is_point;
    void* hoc_mech;
    void (*setdata_)(struct Prop*);
    int* dparam_semantics;  // for nrncore writing.
  private:
    nrn_init_t m_initialize{};
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

extern std::vector<Memb_list> memb_list;
/* for finitialize, order is same up through extracellular, then ions,
then mechanisms that write concentrations, then all others. */
extern short* memb_order_;
#define NRNPOINTER                                                            \
    4 /* added on to list of mechanism variables.These are                    \
pointers which connect variables  from other mechanisms via the _ppval array. \
*/

#define _AMBIGUOUS 5
