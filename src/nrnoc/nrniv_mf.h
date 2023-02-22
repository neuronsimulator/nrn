#ifndef nrniv_mf_h
#define nrniv_mf_h
#include "hoc_membf.h"
#include "hocdec.h"
#include "membfunc.h"

struct Memb_list;
struct NrnThread;
struct Point_process;

using ldifusfunc3_t = double (*)(int,
                                 Memb_list*,
                                 std::size_t,
                                 Datum*,
                                 double*,
                                 double*,
                                 Datum*,
                                 NrnThread*,
                                 neuron::model_sorted_token const&);
using ldifusfunc2_t =
    void(int, ldifusfunc3_t, void**, int, int, int, neuron::model_sorted_token const&, NrnThread&);
using ldifusfunc_t = void (*)(ldifusfunc2_t, neuron::model_sorted_token const&, NrnThread&);
typedef void (*pnt_receive_t)(Point_process*, double*, double);
typedef void (*pnt_receive_init_t)(Point_process*, double*, double);

extern Prop* need_memb_cl(Symbol*, int*, int*);
extern Prop* prop_alloc(Prop**, int, Node*);
void prop_update_ion_variables(Prop*, Node*);

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
void register_mech(const char**, Pvmp, nrn_cur_t, nrn_jacob_t, nrn_state_t, nrn_init_t, int, int);
int point_register_mech(const char**,
                        Pvmp,
                        nrn_cur_t,
                        nrn_jacob_t,
                        nrn_state_t,
                        nrn_init_t,
                        int,
                        int,
                        void* (*) (Object*),
                        void (*)(void*),
                        Member_func*);
extern int nrn_get_mechtype(const char*);
extern void nrn_writes_conc(int, int);
extern void add_nrn_has_net_event(int);
void hoc_register_cvode(int, nrn_ode_count_t, nrn_ode_map_t, nrn_ode_spec_t, nrn_ode_matsol_t);
void hoc_register_synonym(int, nrn_ode_synonym_t);
extern void register_destructor(Pvmp);
extern void ion_reg(const char*, double);
extern void nrn_promote(Prop*, int, int);
extern void add_nrn_artcell(int, int);
extern void hoc_register_ldifus1(ldifusfunc_t);
extern void nrn_check_conc_write(Prop*, Prop*, int);
void nrn_wrote_conc(Symbol*, double& erev, double ci, double co, int);

extern Prop* need_memb(Symbol*);

extern void* create_point_process(int, Object*);
extern void destroy_point_process(void*);
extern double has_loc_point(void*);
extern double get_loc_point_process(void*);
extern double loc_point_process(int, void*);
extern Prop* nrn_point_prop_;
neuron::container::data_handle<double> point_process_pointer(Point_process*, Symbol*, int);
void steer_point_process(void* v);

bool at_time(NrnThread*, double);

void artcell_net_move(Datum*, Point_process*, double);

extern int ifarg(int);
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
