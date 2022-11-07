#pragma once
#include "hocdec.h"
#include "membfunc.h"  // nrn_bamech_t
struct Extnode;
struct hoc_Item;
struct HocParmLimits;
struct HocParmUnits;
struct HocStateTolerance;
struct Node;
struct Object;
struct Point_process;
struct Prop;
struct Section;
struct Symbol;

// nocpout.cpp
extern void hoc_register_limits(int, HocParmLimits*);
extern void hoc_register_units(int, HocParmUnits*);
extern void hoc_register_dparam_semantics(int, int, const char*);
extern void add_nrn_fornetcons(int, int);
extern void hoc_register_tolerance(int, HocStateTolerance*, Symbol***);

extern void oc_save_cabcode(int* a1, int* a2);
extern void oc_restore_cabcode(int* a1, int* a2);

extern "C" void modl_reg(void);

// nrnmech stuff
extern void _nrn_free_fornetcon(void**);
extern double nrn_call_mech_func(Symbol*, int narg, Prop*, int type);
extern Prop* nrn_mechanism(int type, Node*);

// mod stuff
extern void _nrn_free_watch(Datum*, int, int);
extern void _nrn_watch_activate(Datum*,
                                double (*)(Point_process*),
                                int,
                                Point_process*,
                                int,
                                double);
extern void _nrn_watch_allocate(Datum*,
                                double (*)(Point_process*),
                                int,
                                Point_process*,
                                double nrflag);
extern void hoc_reg_ba(int, nrn_bamech_t, int);
extern int nrn_pointing(double*);

extern void nrn_pushsec(Section*);
extern void nrn_popsec(void);
extern Section* chk_access(void);

extern Node* node_exact(Section*, double);

extern int state_discon_allowed_;
extern int section_object_seen;

extern int nrn_isecstack(void);
extern void nrn_secstack(int);
extern void new_sections(Object* ob, Symbol* sym, hoc_Item** pitm, int size);
extern void cable_prop_assign(Symbol* sym, double* pd, int op);
extern void nrn_parent_info(Section* s);
extern void nrn_relocate_old_points(Section* oldsec, Node* oldnode, Section* sec, Node* node);
extern int nrn_at_beginning(Section* sec);
extern void nrn_node_destruct1(Node*);
extern void mech_insert1(Section*, int);
extern void extcell_2d_alloc(Section* sec);
extern int nrn_is_ion(int);
extern void single_prop_free(Prop*);
extern void prop_free(Prop**);
extern int can_change_morph(Section*);
extern void nrn_area_ri(Section* sec);
extern void nrn_diam_change(Section*);
extern void sec_free(hoc_Item*);
extern int node_index(Section* sec, double x);
extern void extcell_node_create(Node*);
extern void extnode_free_elements(Extnode*);
extern const char* sec_and_position(Section* sec, Node* nd);
extern void section_order(void);
extern Section* nrn_sec_pop(void);
extern Node* node_ptr(Section* sec, double x, double* parea);
extern double* nrn_vext_pd(Symbol* s, int indx, Node* nd);
extern double* nrnpy_dprop(Symbol* s, int indx, Section* sec, short inode, int* err);
extern void nrn_disconnect(Section*);
extern void mech_uninsert1(Section* sec, Symbol* s);
extern Object* nrn_sec2cell(Section*);
extern int nrn_sec2cell_equals(Section*, Object*);
extern double* dprop(Symbol* s, int indx, Section* sec, short inode);
extern void nrn_initcode();
extern int segment_limits(double*);
extern void second_order_cur(NrnThread*);
extern void hoc_register_dparam_size(int, int);
extern void setup_topology(void);
extern int nrn_errno_check(int);
extern void long_difus_solve(int method, NrnThread* nt);
extern void nrn_fihexec(int);
extern int special_pnt_call(Object*, Symbol*, int);
extern void ob_sec_access_push(hoc_Item*);
extern void nrn_mk_prop_pools(int);
extern void SectionList_reg(void);
extern void SectionRef_reg(void);

extern void hoc_symbol_tolerance(Symbol*, double);
extern void node_destruct(Node**, int);
extern void nrn_sec_ref(Section**, Section*);
extern void hoc_level_pushsec(Section*);
extern double nrn_ra(Section*);
extern int node_index_exact(Section*, double);
void nrn_cachevec(int);
void nrn_ba(NrnThread*, int);
extern void nrniv_recalc_ptrs(void);
extern void nrn_recalc_ptrvector(void);
extern void nrn_recalc_ptrs(double* (*r)(double*) );
extern void nrn_rhs_ext(NrnThread*);
extern void nrn_setup_ext(NrnThread*);
extern void nrn_cap_jacob(NrnThread*, Memb_list*);
extern void clear_point_process_struct(Prop* p);
extern void ext_con_coef(void);
extern void nrn_multisplit_ptr_update(void);
extern void nrn_cache_prop_realloc();
extern void nrn_use_daspk(int);
extern void nrn_update_ps2nt(void);
extern void activstim_rhs(void);
extern void activclamp_rhs(void);
extern void activclamp_lhs(void);
extern void activsynapse_rhs(void);
extern void activsynapse_lhs(void);
extern void stim_prepare(void);
extern void clamp_prepare(void);
extern void synapse_prepare(void);

extern void v_setup_vectors(void);
extern void section_ref(Section*);
extern void section_unref(Section*);
extern const char* secname(Section*);
extern const char* nrn_sec2pysecname(Section*);
void nrn_rangeconst(Section*, Symbol*, double* value, int op);
extern int nrn_exists(Symbol*, Node*);
double* nrn_rangepointer(Section*, Symbol*, double x);
extern double* cable_prop_eval_pointer(Symbol*);  // section on stack will be popped
extern char* hoc_section_pathname(Section*);
extern double nrn_arc_position(Section*, Node*);
extern double node_dist(Section*, Node*);  // distance of node to parent position
extern double nrn_section_orientation(Section*);
extern double nrn_connection_position(Section*);
extern Section* nrn_trueparent(Section*);
extern double topol_distance(Section*, Node*, Section*, Node*, Section**, Node**);
extern int arc0at0(Section*);
extern void nrn_clear_mark(void);
extern short nrn_increment_mark(Section*);
extern short nrn_value_mark(Section*);
extern int is_point_process(Object*);
extern int nrn_vartype(Symbol*);  // nrnocCONST, DEP, STATE
extern void recalc_diam(void);
extern Prop* nrn_mechanism_check(int type, Section* sec, int inode);
extern int nrn_use_fast_imem;
extern void nrn_fast_imem_alloc();
extern void nrn_calc_fast_imem(NrnThread*);
extern Section* nrn_secarg(int iarg);
extern void nrn_seg_or_x_arg(int iarg, Section** psec, double* px);
extern void nrn_seg_or_x_arg2(int iarg, Section** psec, double* px);
extern Section* nrnpy_pysecname2sec(const char*);
extern const char* nrnpy_sec2pysecname(Section* sec);
extern void nrnpy_pysecname2sec_add(Section* sec);
extern void nrnpy_pysecname2sec_remove(Section* sec);
extern void nrn_verify_ion_charge_defined();

extern void nrn_pt3dclear(Section*, int);
extern void nrn_length_change(Section*, double);
extern void stor_pt3d(Section*, double x, double y, double z, double d);
extern int nrn_netrec_state_adjust;
extern int nrn_sparse_partrans;

char* nrn_version(int);

/** @brief Get the number of NEURON configuration items.
 */
[[nodiscard]] std::size_t nrn_num_config_keys();

/** @brief Get the ith NEURON configuration key.
 *
 * @param i Key index, must be less than nrn_num_config_keys().
 */
[[nodiscard]] char* nrn_get_config_key(std::size_t i);

/** @brief Get the ith NEURON configuration value.
 *
 * @param i Key index, must be less than nrn_num_config_keys().
 */
[[nodiscard]] char* nrn_get_config_val(std::size_t i);
