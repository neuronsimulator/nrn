/******************************************************************
Author: Austin Lachance
Date: 10/28/16
Description: Header File for grids.cpp. Allows access to Grid_node
and Flux_pair structs and their respective functions
******************************************************************/
#include <stdio.h>
#include <assert.h>
#include <nrnmpi.h>

#include "nrn_pyhocobject.h"
#include "nrnwrap_Python.h"

#define SAFE_FREE(ptr)     \
    {                      \
        if ((ptr) != NULL) \
            free(ptr);     \
    }
#define IDX(x, y, z)    ((z) + (y) *g->size_z + (x) *g->size_z * g->size_y)
#define INDEX(x, y, z)  ((z) + (y) *grid->size_z + (x) *grid->size_z * grid->size_y)
#define ALPHA(x, y, z)  (g->get_alpha(g->alpha, IDX(x, y, z)))
#define VOLFRAC(idx)    (g->get_alpha(g->alpha, idx))
#define TORT(idx)       (g->get_permeability(g->permeability, idx))
#define PERM(x, y, z)   (g->get_permeability(g->permeability, IDX(x, y, z)))
#define SQ(x)           ((x) * (x))
#define CU(x)           ((x) * (x) * (x))
#define TRUE            1
#define FALSE           0
#define TORTUOSITY      2
#define VOLUME_FRACTION 3
#define ICS_ALPHA       4

#define NEUMANN   0
#define DIRICHLET 1

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct Hybrid_data {
    long num_1d_indices;
    long* indices1d;
    long* num_3d_indices_per_1d_seg;
    long* indices3d;
    double* rates;
    double* volumes1d;
    double* volumes3d;
} Hybrid_data;

typedef struct Flux_pair {
    double* flux;    // Value of flux
    int grid_index;  // Location in grid
} Flux;

struct Concentration_Pair {
    neuron::container::data_handle<double> destination; /* memory loc to transfer concentration to
                                                         */
    long source;                                        /* index in grid for source */
};

struct Current_Triple {
    long destination;                              /* index in grid */
    neuron::container::data_handle<double> source; /* memory loc of e.g. ica */
    double scale_factor;
};

typedef void (*ReactionRate)(double**,
                             double**,
                             double**,
                             double*,
                             double*,
                             double*,
                             double*,
                             double**,
                             double);
typedef void (*ECSReactionRate)(double*, double*, double*, double*);
typedef struct Reaction {
    struct Reaction* next;
    ECSReactionRate reaction;
    unsigned int num_species_involved;
    unsigned int num_params_involved;
    double** species_states;
    unsigned char* subregion;
    unsigned int region_size;
    uint64_t* mc3d_indices_offsets;
    double** mc3d_mults;
} Reaction;

typedef struct {
    double* copyFrom;
    long copyTo;
} AdiLineData;

typedef struct {
    /*TODO: Support for mixed boundaries and by edge (maybe even by voxel)*/
    unsigned char type;
    double value;
} BoundaryConditions;

class Grid_node {
  public:
    Grid_node* next;

    double* states;  // Array of doubles representing Grid space
    double* states_x;
    double* states_y;
    double* states_z;  // TODO: This is only used by ICS, is it necessary?
    double* states_cur;
    int size_x;   // Size of X dimension
    int size_y;   // Size of Y dimension
    int size_z;   // Size of Z dimension
    double dc_x;  // X diffusion constant
    double dc_y;  // Y diffusion constant
    double dc_z;  // Z diffusion constant
    double dx;    // ∆X
    double dy;    // ∆Y
    double dz;    // ∆Z
    bool diffusable;
    bool hybrid;
    BoundaryConditions* bc;
    Hybrid_data* hybrid_data;
    Concentration_Pair* concentration_list;
    Current_Triple* current_list;
    ssize_t num_concentrations, num_currents;

    /*used for MPI implementation*/
    int num_all_currents;
    int* proc_offsets;
    int* proc_num_currents;
    int* proc_flux_offsets;
    int* proc_num_fluxes;
    long* current_dest;
    double* all_currents;

    /*Extension to handle a variable diffusion characteristics of a grid*/
    unsigned char VARIABLE_ECS_VOLUME; /*FLAG which variable volume fraction
                                       methods should be used*/
    /*diffusion characteristics are arrays of a single value or
     * the number of voxels (size_x*size_y*size_z)*/
    double* permeability; /* 1/tortuosities^2 squared
                             D_eff = D_free*permeability */
    double* alpha;        /* volume fractions */
    /*Function that will be assigned when the grid is created to either return
     * the single value or the value at a given index*/
    double (*get_alpha)(double*, int);
    double (*get_permeability)(double*, int);
    double atolscale;

    int64_t* ics_surface_nodes_per_seg;
    int64_t* ics_surface_nodes_per_seg_start_indices;
    std::vector<neuron::container::data_handle<double>> ics_concentration_seg_handles;
    double** ics_current_seg_ptrs;
    double* ics_scale_factors;

    int insert(int grid_list_index);
    int node_flux_count;
    long* node_flux_idx;
    double* node_flux_scale;
    PyObject** node_flux_src;


    virtual ~Grid_node() {}
    virtual void set_diffusion(double*, int) = 0;
    virtual void set_num_threads(const int n) = 0;
    virtual void do_grid_currents(double*, double, int) = 0;
    virtual void apply_node_flux3D(double dt, double* states) = 0;
    virtual void volume_setup() = 0;
    virtual int dg_adi() = 0;
    virtual void variable_step_diffusion(const double* states, double* ydot) = 0;
    virtual void variable_step_ode_solve(double* RHS, double dt) = 0;
    virtual void scatter_grid_concentrations() = 0;
    virtual void hybrid_connections() = 0;
    virtual void variable_step_hybrid_connections(const double* cvode_states_3d,
                                                  double* const ydot_3d,
                                                  const double* cvode_states_1d,
                                                  double* const ydot_1d) = 0;
};

class ECS_Grid_node: public Grid_node {
  public:
    // Data for DG-ADI
    ECS_Grid_node();
    ECS_Grid_node(PyHocObject*,
                  int,
                  int,
                  int,
                  double,
                  double,
                  double,
                  double,
                  double,
                  double,
                  PyHocObject*,
                  PyHocObject*,
                  int,
                  double,
                  double);
    ~ECS_Grid_node();
    struct ECSAdiGridData* ecs_tasks;
    struct ECSAdiDirection* ecs_adi_dir_x;
    struct ECSAdiDirection* ecs_adi_dir_y;
    struct ECSAdiDirection* ecs_adi_dir_z;

    // Data for multicompartment reactions
    int induced_idx;
    int* react_offsets;
    int react_offset_count;
    int* reaction_indices;
    int* all_reaction_indices;
    int* proc_num_reactions;
    int* proc_num_reaction_states;
    int total_reaction_states;
    unsigned char multicompartment_inititalized;
    int* induced_currents_index;
    int induced_current_count;
    int* proc_induced_current_count;
    int* proc_induced_current_offset;
    double* all_reaction_states;
    double* induced_currents;
    double* local_induced_currents;
    double* induced_currents_scale;

    void set_num_threads(const int n);
    void do_grid_currents(double*, double, int);
    void apply_node_flux3D(double dt, double* states);
    void volume_setup();
    int dg_adi();
    void variable_step_diffusion(const double* states, double* ydot);
    void variable_step_ode_solve(double* RHS, double dt);
    void variable_step_hybrid_connections(const double* cvode_states_3d,
                                          double* const ydot_3d,
                                          const double* cvode_states_1d,
                                          double* const ydot_1d);
    void scatter_grid_concentrations();
    void hybrid_connections();
    void set_diffusion(double*, int);
    void set_tortuosity(PyHocObject*);
    void set_volume_fraction(PyHocObject*);
    void do_multicompartment_reactions(double*);
    void initialize_multicompartment_reaction();
    void clear_multicompartment_reaction();
    int add_multicompartment_reaction(int, int*, int);
    double* set_rxd_currents(int, int*, PyHocObject**);
};

typedef struct ECSAdiDirection {
    void (*ecs_dg_adi_dir)(ECS_Grid_node*,
                           const double,
                           const int,
                           const int,
                           double const* const,
                           double* const,
                           double* const);
    double* states_in;
    double* states_out;
    int line_size;
} ECSAdiDirection;

typedef struct ECSAdiGridData {
    int start, stop;
    double* state;
    ECS_Grid_node* g;
    int sizej;
    ECSAdiDirection* ecs_adi_dir;
    double* scratchpad;
} ECSAdiGridData;

class ICS_Grid_node: public Grid_node {
  public:
    ICS_Grid_node();
    ~ICS_Grid_node();
    // fractional volumes
    double* _ics_alphas;
    // stores the positive x,y, and z neighbors for each node. [node0_x, node0_y, node0_z, node1_x
    // ...]
    long* _neighbors;

    /*Line definitions from Python. In pattern of [line_start_node, line_length, ...]
    Array is sorted from longest to shortest line */
    long* _sorted_x_lines;
    long* _sorted_y_lines;
    long* _sorted_z_lines;

    // Lengths of _sorted_lines arrays. Used to find thread start and stop indices
    long _x_lines_length;
    long _y_lines_length;
    long _z_lines_length;

    // maximum line length for scratchpad memory allocation
    long _line_length_max;

    // total number of nodes for this grid
    long _num_nodes;

    // indices for thread start and stop positions
    long* _nodes_per_thread;

    // Data for DG-ADI
    struct ICSAdiGridData* ics_tasks;
    struct ICSAdiDirection* ics_adi_dir_x;
    struct ICSAdiDirection* ics_adi_dir_y;
    struct ICSAdiDirection* ics_adi_dir_z;

    ICS_Grid_node(PyHocObject*,
                  long,
                  long*,
                  long*,
                  long,
                  long*,
                  long,
                  long*,
                  long,
                  double*,
                  double*,
                  double,
                  bool,
                  double,
                  double*);
    void divide_x_work(const int nthreads);
    void divide_y_work(const int nthreads);
    void divide_z_work(const int nthreads);
    void set_num_threads(const int n);
    void do_grid_currents(double*, double, int);
    void apply_node_flux3D(double dt, double* states);
    void volume_setup();
    int dg_adi();
    void variable_step_diffusion(const double* states, double* ydot);
    void variable_step_ode_solve(double* RHS, double dt);
    void hybrid_connections();
    void variable_step_hybrid_connections(const double* cvode_states_3d,
                                          double* const ydot_3d,
                                          const double* cvode_states_1d,
                                          double* const ydot_1d);
    void scatter_grid_concentrations();
    void run_threaded_ics_dg_adi(struct ICSAdiDirection*);
    void set_diffusion(double*, int);
    void do_multicompartment_reactions(double*);
    void initialize_multicompartment_reaction();
    void clear_multicompartment_reaction();
    int add_multicompartment_reaction(int, int*, int);
    double* set_rxd_currents(int, int*, PyHocObject**);
};

typedef struct ICSAdiDirection {
    void (*ics_dg_adi_dir)(ICS_Grid_node* g,
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
    double* states_in;
    double* states_out;
    double* deltas;
    long* ordered_line_defs;
    long* ordered_nodes;
    long* ordered_start_stop_indices;
    long* line_start_stop_indices;
    double dc;
    double* dcgrid;
    double d;
} ICSAdiDirection;


typedef struct ICSAdiGridData {
    // Start and stop node indices for lines
    int line_start, line_stop;
    // Where to start in the ordered_nodes array
    int ordered_start;
    double* state;
    ICS_Grid_node* g;
    ICSAdiDirection* ics_adi_dir;
    double* scratchpad;
    double* RHS;
    double* l_diag;
    double* diag;
    double* u_diag;
    // double* deltas;
} ICSAdiGridData;

/***** GLOBALS *******************************************************************/
extern double* dt_ptr;  // Universal ∆t
extern double* t_ptr;   // Universal t

// static int N = 100;                 // Number of grid_lists (size of Parallel_grids)
extern Grid_node* Parallel_grids[100];  // Array of Grid_node * lists
/*********************************************************************************/


// Set the global ∆t
void make_dt_ptr(PyHocObject* my_dt_ptr);


// Create a single Grid_node
/* Parameters:  Python object that includes array of double pointers,
                size of x, y, and z dimensions
                x, y, and z diffusion constants
                delta x, delta y, and delta z*/

// Free a single Grid_node "grid"
// void free_Grid(Grid_node *grid);

// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
extern "C" int ECS_insert(int grid_list_index,
                          PyHocObject* my_states,
                          int my_num_states_x,
                          int my_num_states_y,
                          int my_num_states_z,
                          double my_dc_x,
                          double my_dc_y,
                          double my_dc_z,
                          double my_dx,
                          double my_dy,
                          double my_dz,
                          PyHocObject* my_alpha,
                          PyHocObject* my_permeability,
                          int,
                          double,
                          double);

Grid_node* ICS_make_Grid(PyHocObject* my_states,
                         long num_nodes,
                         long* neighbors,
                         long* x_line_defs,
                         long x_lines_length,
                         long* y_line_defs,
                         long y_lines_length,
                         long* z_line_defs,
                         long z_lines_length,
                         double* dcs,
                         double dx,
                         bool is_diffusable,
                         double atolscale,
                         double* ics_alphas);

// Insert an  ICS_Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
extern "C" int ICS_insert(int grid_list_index,
                          PyHocObject* my_states,
                          long num_nodes,
                          long* neighbors,
                          long* x_line_defs,
                          long x_lines_length,
                          long* y_line_defs,
                          long y_lines_length,
                          long* z_line_defs,
                          long z_lines_length,
                          double* dcs,
                          double dx,
                          bool is_diffusable,
                          double atolscale,
                          double* ics_alphas);

extern "C" int ICS_insert_inhom(int grid_list_index,
                                PyHocObject* my_states,
                                long num_nodes,
                                long* neighbors,
                                long* x_line_defs,
                                long x_lines_length,
                                long* y_line_defs,
                                long y_lines_length,
                                long* z_line_defs,
                                long z_lines_length,
                                double* dcs,
                                double dx,
                                bool is_diffusable,
                                double atolscale,
                                double* ics_alphas);


// Set the diffusion coefficients for a given grid_id
extern "C" int set_diffusion(int, int, double*, int);

// Delete a specific Grid_node "find" from the list "head"
int remove(Grid_node** head, Grid_node* find);

// Destroy the list located at list_index and free all memory
void empty_list(int list_index);

void apply_node_flux(int, long*, double*, PyObject**, double, double*);
