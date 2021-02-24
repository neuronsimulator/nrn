/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <vector>

#include "coreneuron/mechanism/mechanism.hpp"
namespace coreneuron {

using Pfrpdat = Datum* (*) (void);

struct NrnThread;

using mod_alloc_t = void (*)(double*, Datum*, int);
using mod_f_t = void (*)(NrnThread*, Memb_list*, int);
using pnt_receive_t = void (*)(Point_process*, int, double);

/*
 * Memb_func structure contains all related informations of a mechanism
 */
struct Memb_func {
    mod_alloc_t alloc;
    mod_f_t current;
    mod_f_t jacob;
    mod_f_t state;
    mod_f_t initialize;
    Pfri destructor; /* only for point processes */
    Symbol* sym;
    int vectorized;
    int thread_size_;                       /* how many Datum needed in Memb_list if vectorized */
    void (*thread_mem_init_)(ThreadDatum*); /* after Memb_list._thread is allocated */
    void (*thread_cleanup_)(ThreadDatum*);  /* before Memb_list._thread is freed */
    void (*thread_table_check_)(int, int, double*, Datum*, ThreadDatum*, NrnThread*, int);
    int is_point;
    void (*setdata_)(double*, Datum*);
    int* dparam_semantics; /* for nrncore writing. */
};

#define VINDEX       -1
#define CABLESECTION 1
#define MORPHOLOGY   2
#define CAP          3
#define EXTRACELL    5

#define nrnocCONST 1
#define DEP        2
#define STATE      3 /*See init.c and cabvars.h for order of nrnocCONST, DEP, and STATE */

#define BEFORE_INITIAL    0
#define AFTER_INITIAL     1
#define BEFORE_BREAKPOINT 2
#define AFTER_SOLVE       3
#define BEFORE_STEP       4
#define BEFORE_AFTER_SIZE 5 /* 1 more than the previous */
struct BAMech {
    mod_f_t f;
    int type;
    struct BAMech* next;
};

extern int nrn_ion_global_map_size;
extern double** nrn_ion_global_map;
extern const int ion_global_map_member_size;

#define NRNPOINTER                                                            \
    4 /* added on to list of mechanism variables.These are                    \
pointers which connect variables  from other mechanisms via the _ppval array. \
*/

#define _AMBIGUOUS 5


extern int nrn_get_mechtype(const char*);
extern const char* nrn_get_mechname(int);  // slow. use memb_func[i].sym if posible
extern int register_mech(const char** m,
                         mod_alloc_t alloc,
                         mod_f_t cur,
                         mod_f_t jacob,
                         mod_f_t stat,
                         mod_f_t initialize,
                         int nrnpointerindex,
                         int vectorized);
extern int point_register_mech(const char**,
                               mod_alloc_t alloc,
                               mod_f_t cur,
                               mod_f_t jacob,
                               mod_f_t stat,
                               mod_f_t initialize,
                               int nrnpointerindex,
                               void* (*constructor)(),
                               void (*destructor)(),
                               int vectorized);
using NetBufReceive_t = void (*)(NrnThread*);
extern void hoc_register_net_receive_buffering(NetBufReceive_t, int);

extern void hoc_register_net_send_buffering(int);

using nrn_watch_check_t = void (*)(NrnThread*, Memb_list*);
extern void hoc_register_watch_check(nrn_watch_check_t, int);

extern void nrn_jacob_capacitance(NrnThread*, Memb_list*, int);
extern void nrn_writes_conc(int, int);
#pragma acc routine seq
extern void nrn_wrote_conc(int, double*, int, int, double**, double, int);
#pragma acc routine seq
double nrn_nernst(double ci, double co, double z, double celsius);
#pragma acc routine seq
extern double nrn_ghk(double v, double ci, double co, double z);
extern void hoc_register_prop_size(int, int, int);
extern void hoc_register_dparam_semantics(int type, int, const char* name);

struct DoubScal {
    const char* name;
    double* pdoub;
};
struct DoubVec {
    const char* name;
    double* pdoub;
    int index1;
};
struct VoidFunc {
    const char* name;
    void (*func)(void);
};
extern void hoc_register_var(DoubScal*, DoubVec*, VoidFunc*);

extern void _nrn_layout_reg(int, int);
extern void _nrn_thread_reg0(int i, void (*f)(ThreadDatum*));
extern void _nrn_thread_reg1(int i, void (*f)(ThreadDatum*));

using bbcore_read_t = void (*)(double*,
                               int*,
                               int*,
                               int*,
                               int,
                               int,
                               double*,
                               Datum*,
                               ThreadDatum*,
                               NrnThread*,
                               double);

using bbcore_write_t = void (*)(double*,
                                int*,
                                int*,
                                int*,
                                int,
                                int,
                                double*,
                                Datum*,
                                ThreadDatum*,
                                NrnThread*,
                                double);

extern int nrn_mech_depend(int type, int* dependencies);
extern int nrn_fornetcon_cnt_;
extern int* nrn_fornetcon_type_;
extern int* nrn_fornetcon_index_;
extern void add_nrn_fornetcons(int, int);
extern void add_nrn_has_net_event(int);
extern void net_event(Point_process*, double);
extern void net_send(void**, int, Point_process*, double, double);
extern void net_move(void**, Point_process*, double);
extern void artcell_net_send(void**, int, Point_process*, double, double);
extern void artcell_net_move(void**, Point_process*, double);
extern void nrn2ncs_outputevent(int netcon_output_index, double firetime);
extern bool nrn_use_localgid_;
extern void net_sem_from_gpu(int sendtype, int i_vdata, int, int ith, int ipnt, double, double);
#pragma acc routine seq
extern int at_time(NrnThread*, double);

// _OPENACC and/or NET_RECEIVE_BUFFERING
extern void net_sem_from_gpu(int, int, int, int, int, double, double);

extern void hoc_malchk(void); /* just a stub */
extern void* hoc_Emalloc(size_t);

}  // namespace coreneuron
