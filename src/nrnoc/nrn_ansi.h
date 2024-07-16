#pragma once
#include "hocdec.h"
#include "membfunc.h"  // nrn_bamech_t
#include "cabcode.h"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/generic_data_handle.hpp"
#include <memory>

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

extern "C" void modl_reg(void);

// nrnmech stuff
extern void _nrn_free_fornetcon(void**);
extern double nrn_call_mech_func(Symbol*, int narg, Prop*, int type);

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
[[nodiscard]] inline int nrn_pointing(double* p) {
    return static_cast<bool>(p);
}

extern void nrn_popsec(void);

extern int state_discon_allowed_;
extern int section_object_seen;

extern int nrn_isecstack(void);
extern void nrn_secstack(int);
extern void nrn_relocate_old_points(Section* oldsec, Node* oldnode, Section* sec, Node* node);
extern void extcell_2d_alloc(Section* sec);
extern int nrn_is_ion(int);
extern void single_prop_free(Prop*);
extern void prop_free(Prop**);
extern int can_change_morph(Section*);
extern void nrn_area_ri(Section* sec);
extern void nrn_diam_change(Section*);
extern void sec_free(hoc_Item*);
extern void extcell_node_create(Node*);
extern void extnode_free_elements(Extnode*);
extern void section_order(void);
neuron::container::data_handle<double> dprop(Symbol* s, int indx, Section* sec, short inode);
extern "C" void nrn_random_play();
extern void fixed_play_continuous(NrnThread*);
extern void setup_tree_matrix(neuron::model_sorted_token const& sorted_token, NrnThread& nt);
extern void nrn_solve(NrnThread*);
extern void second_order_cur(NrnThread*);
void nrn_update_voltage(neuron::model_sorted_token const& sorted_token, NrnThread& nt);
extern void nrn_fixed_step_lastpart(neuron::model_sorted_token const& sorted_token, NrnThread& nt);
extern void hoc_register_dparam_size(int, int);
extern int nrn_errno_check(int);
void long_difus_solve(neuron::model_sorted_token const&, int method, NrnThread& nt);
extern void nrn_fihexec(int);
extern int special_pnt_call(Object*, Symbol*, int);
extern void nrn_mk_prop_pools(int);
extern void SectionList_reg(void);
extern void SectionRef_reg(void);

extern void hoc_symbol_tolerance(Symbol*, double);
extern void node_destruct(Node**, int);
extern void nrn_sec_ref(Section**, Section*);
extern void hoc_level_pushsec(Section*);
void nrn_ba(neuron::model_sorted_token const&, NrnThread&, int);
extern void nrn_rhs_ext(NrnThread*);
extern void nrn_setup_ext(NrnThread*);
void nrn_cap_jacob(neuron::model_sorted_token const&, NrnThread*, Memb_list*);
void nrn_div_capacity(neuron::model_sorted_token const&, NrnThread*, Memb_list*);
void nrn_mul_capacity(neuron::model_sorted_token const&, NrnThread*, Memb_list*);
extern void clear_point_process_struct(Prop* p);
extern void ext_con_coef(void);
extern void nrn_multisplit_ptr_update(void);
extern void nrn_use_daspk(int);
extern void nrn_update_ps2nt(void);
neuron::model_sorted_token nrn_ensure_model_data_are_sorted();
extern void activstim_rhs(void);
extern void activclamp_rhs(void);
extern void activclamp_lhs(void);
extern void activsynapse_rhs(void);
extern void activsynapse_lhs(void);
extern void stim_prepare(void);
extern void clamp_prepare(void);
extern void synapse_prepare(void);

void nrn_fixed_step(neuron::model_sorted_token const&);
void nrn_fixed_step_group(neuron::model_sorted_token const&, int n);
void nrn_lhs(neuron::model_sorted_token const&, NrnThread&);
void nrn_rhs(neuron::model_sorted_token const&, NrnThread&);
extern void v_setup_vectors(void);
extern void section_ref(Section*);
extern void section_unref(Section*);
extern const char* nrn_sec2pysecname(Section*);
extern double node_dist(Section*, Node*);  // distance of node to parent position
extern double topol_distance(Section*, Node*, Section*, Node*, Section**, Node**);
extern void nrn_clear_mark(void);
extern short nrn_increment_mark(Section*);
extern short nrn_value_mark(Section*);
extern int is_point_process(Object*);
extern int nrn_vartype(const Symbol*);  // nrnocCONST, DEP, STATE
extern void recalc_diam(void);
extern bool nrn_use_fast_imem;
void nrn_fast_imem_alloc();
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

/**
  In mechanism libraries, cannot use
      auto const token = nrn_ensure_model_data_are_sorted();
  because the return type is incomplete (from include/neuron/model_data.hpp).
  And we do not want to fix by installing more *.hpp files in the
  include/neuron directory because of potential ABI incompatibility (anything
  with std::string anywhere in it).
  The work around is to provide an extra layer of indirection via unique_ptr
  so the opaque token has a definite size (one pointer) and declaration.

  The "trick" is just that you have to make sure the parts of the opaque
  token that need the definition of the non-opaque token are defined in
  the right place. That's why the constructor and destructor are defined
  in fadvance.cpp

  Instead, use
    auto const token = nrn_ensure_model_data_are_sorted_opaque();
  This file is already included in all translated mod files.
**/
namespace neuron {
struct model_sorted_token;
struct opaque_model_sorted_token {
    opaque_model_sorted_token(model_sorted_token&&);
    ~opaque_model_sorted_token();
    operator model_sorted_token const &() const {
        return *m_ptr;
    }

  private:
    std::unique_ptr<model_sorted_token> m_ptr;
};
}  // namespace neuron
neuron::opaque_model_sorted_token nrn_ensure_model_data_are_sorted_opaque();
