#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#define SPECIES_ABSENT -1
#define PREFETCH       4

using fptr = void (*)(void);

extern "C" NB_EXPORT void rxd_set_no_diffusion();
extern "C" NB_EXPORT void free_curr_ptrs();
extern "C" NB_EXPORT void free_conc_ptrs();
extern "C" NB_EXPORT void rxd_setup_curr_ptrs(int num_currents,
                                              int* curr_index,
                                              double* curr_scale,
                                              PyHocObject** curr_ptrs);
extern "C" NB_EXPORT void rxd_setup_conc_ptrs(int conc_count,
                                              int* conc_index,
                                              PyHocObject** conc_ptrs);
extern "C" NB_EXPORT void rxd_include_node_flux3D(int grid_count,
                                                  int* grid_counts,
                                                  int* grids,
                                                  long* index,
                                                  double* scales,
                                                  PyObject** sources);
extern "C" NB_EXPORT void rxd_include_node_flux1D(int n,
                                                  long* index,
                                                  double* scales,
                                                  PyObject** sources);
extern "C" NB_EXPORT void rxd_set_euler_matrix(int nrow,
                                               int nnonzero,
                                               long* nonzero_i,
                                               long* nonzero_j,
                                               double* nonzero_values,
                                               double* c_diagonal);
extern "C" NB_EXPORT void set_setup(const fptr setup_fn);
extern "C" NB_EXPORT void set_initialize(const fptr initialize_fn);
extern "C" NB_EXPORT void set_setup_matrices(fptr setup_matrices);
extern "C" NB_EXPORT void set_setup_units(fptr setup_units);
extern "C" NB_EXPORT void setup_currents(int num_currents,
                                         int num_fluxes,
                                         int* num_species,
                                         int* node_idxs,
                                         double* scales,
                                         PyHocObject** ptrs,
                                         int* mapped,
                                         int* mapped_ecs);
extern "C" NB_EXPORT int rxd_nonvint_block(int method, int size, double* p1, double* p2, int);
extern "C" NB_EXPORT void register_rate(int nspecies,
                                        int nparam,
                                        int nregions,
                                        int nseg,
                                        int* sidx,
                                        int necs,
                                        int necsparam,
                                        int* ecs_ids,
                                        int* ecsidx,
                                        int nmult,
                                        double* mult,
                                        PyHocObject** vptrs,
                                        ReactionRate f);
extern "C" NB_EXPORT void clear_rates();
extern "C" NB_EXPORT void species_atolscale(int id, double scale, int len, int* idx);
extern "C" NB_EXPORT void remove_species_atolscale(int id);
extern "C" NB_EXPORT void setup_solver(double* my_states,
                                       int my_num_states,
                                       long* zvi,
                                       int num_zvi);

// @olupton 2022-09-16: deleted a declaration of OcPtrVector that did not match
// the one in ocptrvector.h

typedef struct {
    Reaction* reaction;
    int idx;
} ReactSet;

typedef struct {
    ReactSet* onset;
    ReactSet* offset;
} ReactGridData;


typedef struct {
    Grid_node* g;
    int onset, offset;
    double* val;
} CurrentData;


typedef struct SpeciesIndexList {
    int id;
    double atolscale;
    int* indices;
    int length;
    struct SpeciesIndexList* next;
} SpeciesIndexList;

typedef struct ICSReactions {
    ReactionRate reaction;
    int num_species;
    int num_regions;
    int num_params;
    int num_segments;
    int*** state_idx; /*[segment][species][region]*/
    int icsN;         /*total number species*regions per segment*/
    /*NOTE: icsN != num_species*num_regions as every species may not be defined
     *on every region - the missing elements of state_idx are SPECIES_ABSENT*/

    /*ECS for MultiCompartment reactions*/
    int num_ecs_species;
    int num_ecs_params;
    double*** ecs_state; /*[segment][ecs_species]*/
    int* ecs_offset_index;
    ECS_Grid_node** ecs_grid;
    int** ecs_index;
    int ecsN; /*total number of ecs species*regions per segment*/

    int num_mult;
    double** mc_multiplier;
    int* mc_flux_idx;
    double** vptrs;
    struct ICSReactions* next;
} ICSReactions;

typedef struct TaskList {
    void* (*task)(void*);
    void* args;
    void* result;
    struct TaskList* next;
} TaskList;

typedef struct TaskQueue {
    std::condition_variable task_cond, waiting_cond;
    std::mutex task_mutex, waiting_mutex;
    std::vector<bool> exit;
    int length{};
    struct TaskList* first;
    struct TaskList* last;
} TaskQueue;

extern "C" NB_EXPORT void set_num_threads(const int);
void _fadvance(void);
void _fadvance_fixed_step_3D(void);

extern "C" NB_EXPORT int get_num_threads(void);
void ecs_set_adi_tort(ECS_Grid_node*);
void ecs_set_adi_vol(ECS_Grid_node*);
void ecs_set_adi_homogeneous(ECS_Grid_node*);

void dg_transfer_data(AdiLineData* const, double* const, int const, int const, int const);
void ecs_run_threaded_dg_adi(const int, const int, ECS_Grid_node*, ECSAdiDirection*, const int);
ReactGridData* create_threaded_reactions(const int);
void* do_reactions(void*);

void current_reaction(double* states);

void run_threaded_deltas(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir);
void run_threaded_ics_dg_adi(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir);
void ics_dg_adi_x(ICS_Grid_node* g,
                  int,
                  int,
                  int,
                  double,
                  double*,
                  double*,
                  double*,
                  double*,
                  double*,
                  double*);
void ics_dg_adi_y(ICS_Grid_node* g,
                  int,
                  int,
                  int,
                  double,
                  double*,
                  double*,
                  double*,
                  double*,
                  double*,
                  double*);
void ics_dg_adi_z(ICS_Grid_node* g,
                  int,
                  int,
                  int,
                  double,
                  double*,
                  double*,
                  double*,
                  double*,
                  double*,
                  double*);
void ics_dg_adi_x_inhom(ICS_Grid_node* g,
                        int,
                        int,
                        int,
                        double,
                        double*,
                        double*,
                        double*,
                        double*,
                        double*,
                        double*);
void ics_dg_adi_y_inhom(ICS_Grid_node* g,
                        int,
                        int,
                        int,
                        double,
                        double*,
                        double*,
                        double*,
                        double*,
                        double*,
                        double*);
void ics_dg_adi_z_inhom(ICS_Grid_node* g,
                        int,
                        int,
                        int,
                        double,
                        double*,
                        double*,
                        double*,
                        double*,
                        double*,
                        double*);

/*Variable step function declarations*/
void _rhs_variable_step(const double*, double*);

void _ode_reinit(double*);

int ode_count(const int);

extern "C" NB_EXPORT void ics_register_reaction(int list_idx,
                                                int num_species,
                                                int num_params,
                                                int* species_id,
                                                uint64_t* mc3d_start_indices,
                                                int mc3d_region_size,
                                                double* mc3d_mults,
                                                ECSReactionRate f);
extern "C" NB_EXPORT void ecs_register_reaction(int list_idx,
                                                int num_species,
                                                int num_params,
                                                int* species_id,
                                                ECSReactionRate f);
extern "C" NB_EXPORT void scatter_concentrations(void);

extern "C" NB_EXPORT double llgramarea(double* p0, double* p1, double* p2);
extern "C" NB_EXPORT double llpipedfromoriginvolume(double* p0, double* p1, double* p2);


int find(const int, const int, const int, const int, const int);

void _ics_hybrid_helper(ICS_Grid_node*);
void _ics_variable_hybrid_helper(ICS_Grid_node*,
                                 const double*,
                                 double* const,
                                 const double*,
                                 double* const);

void _ics_rhs_variable_step_helper(ICS_Grid_node*, double const* const, double*);
void _rhs_variable_step_helper(Grid_node*, double const* const, double*);

void ics_ode_solve(double, double*, const double*);
void ics_ode_solve_helper(ICS_Grid_node*, double, double*);

void _rhs_variable_step_helper_tort(Grid_node*, double const* const, double*);

void _rhs_variable_step_helper_vol(Grid_node*, double const* const, double*);

void set_num_threads_3D(int n);

void _rhs_variable_step_ecs(const double*, double*);

void clear_rates_ecs();
void do_ics_reactions(double*, double*, double*, double*);
void get_all_reaction_rates(double*, double*, double*);
void _ecs_ode_reinit(double*);
void do_currents(Grid_node*, double*, double, int);
void TaskQueue_add_task(TaskQueue*, void* (*task)(void* args), void*, void*);
void TaskQueue_exe_tasks(std::size_t, TaskQueue*);
void TaskQueue_sync(TaskQueue*);
void ecs_atolscale(double*);
void apply_node_flux3D(Grid_node*, double, double*);

extern "C" NB_EXPORT double factorial(const double x);
extern "C" NB_EXPORT double degrees(const double radians);
extern "C" NB_EXPORT void radians(const double degrees, double* radians);
extern "C" NB_EXPORT double log1p(const double x);
extern "C" NB_EXPORT double vtrap(const double x, const double y);

extern "C" NB_EXPORT void set_hybrid_data(int64_t* num_1d_indices_per_grid,
                                          int64_t* num_3d_indices_per_grid,
                                          int64_t* hybrid_indices1d,
                                          int64_t* hybrid_indices3d,
                                          int64_t* num_3d_indices_per_1d_seg,
                                          int64_t* hybrid_grid_ids,
                                          double* rates,
                                          double* volumes1d,
                                          double* volumes3d,
                                          double* dxs);
