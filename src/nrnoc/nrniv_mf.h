#ifndef nrniv_mf_h
#define nrniv_mf_h

#include "membfunc.h"
#include <hoc_membf.h>

struct NrnThread;
typedef struct SparseObj SparseObj;
union Datum;	

typedef double (*ldifusfunc3_t)(int, double*, Datum*, double*, double*, Datum*, NrnThread*);
typedef void ldifusfunc2_t(int, ldifusfunc3_t, void**, int, int, int, NrnThread*);
typedef void (*ldifusfunc_t)(ldifusfunc2_t, NrnThread*);
typedef void (*pnt_receive_t)(Point_process*, double*, double);
typedef void (*pnt_receive_init_t)(Point_process*, double*, double);

extern Prop* need_memb_cl(Symbol*,int*,int*);
extern Prop* prop_alloc(Prop**, int, Node*);

extern double nrn_ion_charge(Symbol*);
#if defined(__cplusplus)
extern "C" {
#endif
extern Point_process *ob2pntproc(Object *);
extern Point_process* ob2pntproc_0(Object*);
extern double* _getelm(int, int);
extern double* _nrn_thread_getelm(SparseObj*, int, int);
extern int sparse(void**, int, int*, int*, double*, double*, double,
  int(*)(), double**, int);
extern int sparse_thread(void**, int, int*, int*, double*, double*, double,
  int(*)(void*, double*, double*, Datum*, Datum*, NrnThread*), int, Datum*, Datum*, NrnThread*);
extern int _ss_sparse(void**, int, int*, int*, double*, double*, double,
  int(*)(), double**, int);
extern int _ss_sparse_thread(void**, int, int*, int*, double*, double*, double,
  int(*)(void*, double*, double*, Datum*, Datum*, NrnThread*), int, void*, void*, void*);
extern int _cvode_sparse(void**, int, int*, double*,
  int(*)(), double**);
extern int _cvode_sparse_thread(void**, int, int*, double*,
  int(*)(void*, double*, double*, Datum*, Datum*, NrnThread*), void*, void*, void*);
extern int _nrn_destroy_sparseobj_thread(void*);
extern int derivimplicit(int, int, int*, int*, double*, double*, double, int(*)(), double**);
extern int derivimplicit_thread(int, int*, int*, double*, int(*)(double*, union Datum*, union Datum*, struct NrnThread*), void*, void*, void*);
extern int _ss_derivimplicit(int, int, int*, int*, double*, double*, double,
  int(*)(), double**);
extern int _ss_derivimplicit_thread(int, int*, int*, double*,
   int(*)(double*, union Datum*, union Datum*, struct NrnThread*), void*, void*, void*);

extern int euler_thread(int, int*, int*, double*,
                        int(*)(double*, union Datum*, union Datum*, struct NrnThread*), union Datum*, union Datum*, struct NrnThread*);

extern void register_mech(const char**, Pvmp, Pvmi, Pvmi, Pvmi, Pvmi, int, int);
extern int point_register_mech(const char**, Pvmp, Pvmi, Pvmi, Pvmi, Pvmi, int, int,
                               void*(*)(Object*), void(*)(void*), Member_func*);
extern int nrn_get_mechtype(const char*);
extern void nrn_writes_conc(int, int);
extern void add_nrn_has_net_event(int);
extern void hoc_register_cvode(int, nrn_ode_count_t, nrn_ode_map_t, Pvmi, Pvmi);
extern void hoc_register_synonym(int, void(*)(int, double**, Datum**));
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

extern int at_time(NrnThread*, double);

extern void artcell_net_send(void**, double*, Point_process*, double, double);
extern void artcell_net_move(void**, Point_process*, double);

extern int ifarg(int);

extern void nrn_complain(double*);

extern void set_seed(double);
extern int v_structure_change;

//nrnmech stuff
extern pnt_receive_t* pnt_receive;
extern pnt_receive_init_t* pnt_receive_init;
extern short* pnt_receive_size;
extern void nrn_net_send(void**, double*, Point_process*, double, double);
extern void nrn_net_event(Point_process*, double);
extern void nrn_net_move(void**, Point_process*, double);

typedef void(*NrnWatchAllocateFunc_t)(Datum*);
extern NrnWatchAllocateFunc_t* nrn_watch_allocate_;

#if defined(__cplusplus)
}
#endif

#endif /* nrniv_mf_h */
