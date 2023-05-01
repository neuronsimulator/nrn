#ifndef nrniv_mf_h
#define nrniv_mf_h
#include "hoc_membf.h"
#include "hocdec.h"
#include "membfunc.h"

struct NrnThread;
struct Point_process;

typedef double (*ldifusfunc3_t)(int, double*, Datum*, double*, double*, Datum*, NrnThread*);
typedef void ldifusfunc2_t(int, ldifusfunc3_t, void**, int, int, int, NrnThread*);
typedef void (*ldifusfunc_t)(ldifusfunc2_t, NrnThread*);
typedef void (*pnt_receive_t)(Point_process*, double*, double);
typedef void (*pnt_receive_init_t)(Point_process*, double*, double);

extern Prop* need_memb_cl(Symbol*, int*, int*);
extern Prop* prop_alloc(Prop**, int, Node*);

[[deprecated("non-void* overloads are preferred")]] void artcell_net_send(void* v,
                                                                          double* weight,
                                                                          Point_process* pnt,
                                                                          double td,
                                                                          double flag);
void artcell_net_send(Datum* v, double* weight, Point_process* pnt, double td, double flag);
[[deprecated("non-void* overloads are preferred")]] void nrn_net_send(void* v,
                                                                      double* weight,
                                                                      Point_process* pnt,
                                                                      double td,
                                                                      double flag);
void nrn_net_send(Datum* v, double* weight, Point_process* pnt, double td, double flag);

extern double nrn_ion_charge(Symbol*);
extern Point_process* ob2pntproc(Object*);
extern Point_process* ob2pntproc_0(Object*);

extern void register_mech(const char**, Pvmp, Pvmi, Pvmi, Pvmi, Pvmi, int, int);
extern int point_register_mech(const char**,
                               Pvmp,
                               Pvmi,
                               Pvmi,
                               Pvmi,
                               Pvmi,
                               int,
                               int,
                               void* (*) (Object*),
                               void (*)(void*),
                               Member_func*);
extern int nrn_get_mechtype(const char*);
extern void nrn_writes_conc(int, int);
extern void add_nrn_has_net_event(int);
extern void hoc_register_cvode(int, nrn_ode_count_t, nrn_ode_map_t, Pvmi, Pvmi);
extern void hoc_register_synonym(int, void (*)(int, double**, Datum**));
extern void register_destructor(Pvmp);
extern void ion_reg(const char*, double);
extern void nrn_promote(Prop*, int, int);
extern void add_nrn_artcell(int, int);
extern void hoc_register_ldifus1(ldifusfunc_t);
extern void nrn_check_conc_write(Prop*, Prop*, int);
extern void nrn_wrote_conc(Symbol*, double*, int);
extern void nrn_update_ion_pointer(Symbol*, Datum*, int, int);

extern Prop* need_memb(Symbol*);

extern void* create_point_process(int, Object*);
extern void destroy_point_process(void*);
extern double has_loc_point(void*);
extern double get_loc_point_process(void*);
extern double loc_point_process(int, void*);
extern Prop* nrn_point_prop_;
void steer_point_process(void* v);

bool at_time(NrnThread*, double);

void artcell_net_move(Datum*, Point_process*, double);

extern int ifarg(int);

extern void nrn_complain(double*);

extern void set_seed(double);
extern int nrn_matrix_cnt_;       // defined in treeset.cpp
extern int diam_changed;          // defined in cabcode.cpp
extern int diam_change_cnt;       // defined in treeset.cpp
extern int structure_change_cnt;  // defined in treeset.cpp
extern int tree_changed;          // defined in cabcode.cpp
extern int v_structure_change;    // defined in treeset.cpp

// nrnmech stuff
extern pnt_receive_t* pnt_receive;
extern pnt_receive_init_t* pnt_receive_init;
extern short* pnt_receive_size;
extern void nrn_net_event(Point_process*, double);
void nrn_net_move(Datum*, Point_process*, double);

typedef void (*NrnWatchAllocateFunc_t)(Datum*);
extern NrnWatchAllocateFunc_t* nrn_watch_allocate_;

void* nrn_pool_alloc(void* pool);
void* nrn_pool_create(long count, int itemsize);
void nrn_pool_delete(void* pool);
void nrn_pool_freeall(void* pool);

#endif /* nrniv_mf_h */
