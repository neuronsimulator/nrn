#ifndef nrnoc_decl_h
#define nrnoc_decl_h

#if defined(__cplusplus)
extern "C" {
#endif
	
extern int v_structure_change;
extern int diam_changed;
extern int structure_change_cnt;

extern const char* nrn_version(int);
extern void nrn_exit(int);
extern void deliver_net_events(NrnThread*);
extern void nrn_deliver_events(NrnThread*);
extern void init_net_events(void);
extern void nrn_random_play(NrnThread*);
extern void nrn_play_init(void);
extern void nrn_record_init(void);
extern void fixed_play_continuous(NrnThread*);
extern void nrn_solve_minimal(NrnThread*);
extern void second_order_cur(NrnThread*);
extern void nrn_ba(NrnThread*, int);
extern void fixed_record_continuous(NrnThread*);
extern void dt2thread(double);
extern void clear_event_queue(void);
extern void nrn_spike_exchange_init(void);
extern void nrn_mk_prop_pools(int);
extern void modl_reg(void);
extern int nrn_is_ion(int);
extern int nrn_modeltype(void);
#define nrn_fixed_step_group nrn_fixed_step_group_minimal
#define nrn_fixed_step nrn_fixed_step_minimal
#define setup_tree_matrix setup_tree_matrix_minimal
extern void nrn_finitialize(int setv, double v);
extern void nrn_fixed_step_group(int n);
extern void nrn_fixed_step(void);
extern void* setup_tree_matrix(NrnThread*);
extern void alloc_mech(int);
extern void hoc_last_init(void);
extern void ion_reg(const char*, double);
extern void nrn_mk_table_check(void);
extern void initnrn(void);

#if defined(__cplusplus)
}
#endif
#endif
