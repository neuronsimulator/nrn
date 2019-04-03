/******************************************************************
Author: Austin Lachance
Date: 10/28/16
Description: Header File for grids.c. Allows access to Grid_node
and Flux_pair structs and their respective functions
******************************************************************/
#include <stdio.h>
#include <assert.h>
#include <nrnmpi.h>

#include <nrnwrap_Python.h>

#define DIE(msg) exit(fprintf(stderr, "%s\n", msg))
#define SAFE_FREE(ptr){if((ptr)!=NULL) free(ptr);}
#define IDX(x,y,z)  ((z) + (y) * g->size_z + (x) * g->size_z * g->size_y)
#define INDEX(x,y,z)  ((z) + (y) * grid->size_z + (x) * grid->size_z * grid->size_y)
#define ALPHA(x,y,z) (g->get_alpha(g->alpha,IDX(x,y,z)))
#define VOLFRAC(idx) (g->get_alpha(g->alpha,idx))
#define TORT(idx) (g->get_lambda(g->lambda,idx))
#define LAMBDA(x,y,z) (g->get_lambda(g->lambda,IDX(x,y,z)))
#define SQ(x)       ((x)*(x))
#define CU(x)       ((x)*(x)*(x))
#define TRUE 				1
#define FALSE				0
#define TORTUOSITY			2
#define VOLUME_FRACTION 	3

#define NEUMANN             0
#define DIRICHLET           1

#define MAX(a,b)	((a)>(b)?(a):(b))
#define MIN(a,b)	((a)<(b)?(a):(b))

typedef struct {
    PyObject_HEAD
    void* ho_;
    union {
        double x_;
        char* s_;
        void* ho_;
        double* px_;
    }u;
    void* sym_; // for functions and arrays
    void* iteritem_; // enough info to carry out Iterator protocol
    int nindex_; // number indices seen so far (or narg)
    int* indices_; // one fewer than nindex_
    int type_; // 0 HocTopLevelInterpreter, 1 HocObject
        // 2 function (or TEMPLATE)
        // 3 array
        // 4 reference to number
        // 5 reference to string
        // 6 reference to hoc object
        // 7 forall section iterator
        // 8 pointer to a hoc scalar
        // 9 incomplete pointer to a hoc array (similar to 3)
} PyHocObject;


typedef struct Flux_pair {
    double *flux;           // Value of flux
    int grid_index;         // Location in grid
} Flux;

typedef struct {
    double* destination;    /* memory loc to transfer concentration to */
    long source;            /* index in grid for source */
} Concentration_Pair;

typedef struct {
    long destination;       /* index in grid */
    double* source;         /* memory loc of e.g. ica */
    double scale_factor;
} Current_Triple;

typedef void (*ReactionRate)(double**, double**, double*, double**, double**);
typedef void (*ECSReactionRate)(double*, double*);
typedef struct Reaction {
	struct Reaction* next;
	ECSReactionRate reaction;
	unsigned int num_species_involved;
	double** species_states;
	unsigned char* subregion;
	unsigned int region_size;
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
    Grid_node *next;

    double *states;         // Array of doubles representing Grid space
    double *states_x;
    double *states_y;
    double *states_cur;
    int size_x;          // Size of X dimension
    int size_y;          // Size of Y dimension
    int size_z;          // Size of Z dimension
    double dc_x;            // X diffusion constant
    double dc_y;            // Y diffusion constant
    double dc_z;            // Z diffusion constant
    double dx;              // ∆X
    double dy;              // ∆Y
    double dz;              // ∆Z
    BoundaryConditions* bc;
    Concentration_Pair* concentration_list;
    Current_Triple* current_list;
    ssize_t num_concentrations, num_currents;
    
    /*used for MPI implementation*/
    int num_all_currents;
    int* proc_offsets;
    int* proc_num_currents;
    long* current_dest;
    double* all_currents;
    /*Extension to handle a variable diffusion characteristics of a grid*/
    unsigned char	VARIABLE_ECS_VOLUME;	/*FLAG which variable volume fraction
                                            methods should be used*/
    /*diffusion characteristics are arrays of a single value or
    * the number of voxels (size_x*size_y*size_z)*/
    double *lambda;		/* tortuosities squared D_eff=D_free/lambda */
    double *alpha;		/* volume fractions */
    /*Function that will be assigned when the grid is created to either return
    * the single value or the value at a given index*/ 
    double (*get_alpha)(double*,int);
    double (*get_lambda)(double*,int);
    double atolscale;

    int64_t* ics_nodes_per_seg;
    int64_t* ics_nodes_per_seg_start_indices;
    double** ics_seg_ptrs;
    int ics_num_segs;

    int insert(int grid_list_index);
    virtual void set_num_threads(const int n) = 0;
    virtual void do_grid_currents(double dt, int id) = 0;
    virtual void volume_setup() = 0;
    virtual int dg_adi() = 0;
    virtual void scatter_grid_concentrations() = 0;
    virtual void free_Grid() = 0;
};

class ECS_Grid_node : public Grid_node{
    public:
        //Data for DG-ADI
        struct ECSAdiGridData* ecs_tasks;
        struct ECSAdiDirection* ecs_adi_dir_x;
        struct ECSAdiDirection* ecs_adi_dir_y;
        struct ECSAdiDirection* ecs_adi_dir_z;

        void set_num_threads(const int n);
        void do_grid_currents(double dt, int id);  
        void volume_setup();
        int dg_adi();
        void scatter_grid_concentrations();
        void free_Grid();
};

typedef struct ECSAdiDirection{
    void (*ecs_dg_adi_dir)(ECS_Grid_node*, const double, const int, const int, double const * const, double* const, double* const);
    double* states_in;
    double* states_out;
    int line_size;
} ECSAdiDirection;

typedef struct ECSAdiGridData{
    int start, stop;
    double* state;
    ECS_Grid_node* g;
    int sizej;
    ECSAdiDirection* ecs_adi_dir;
    double* scratchpad;
} ECSAdiGridData;

class ICS_Grid_node : public Grid_node{
    public:
        //stores the positive x,y, and z neighbors for each node. [node0_x, node0_y, node0_z, node1_x ...]
        long* _neighbors;

        /*Line definitions from Python. In pattern of [line_start_node, line_length, ...]
        Array is sorted from longest to shortest line */
        long* _sorted_x_lines;
        long* _sorted_y_lines;
        long* _sorted_z_lines;

        //Lengths of _sorted_lines arrays. Used to find thread start and stop indices
        long _x_lines_length;
        long _y_lines_length;
        long _z_lines_length;

        //maximum line length for scratchpad memory allocation
        long _line_length_max;

        //total number of nodes for this grid
        long _num_nodes;

        //indices for thread start and stop positions 
        long* _nodes_per_thread;
        
        //Data for DG-ADI
        struct ICSAdiGridData* ics_tasks;
        struct ICSAdiDirection* ics_adi_dir_x;
        struct ICSAdiDirection* ics_adi_dir_y;
        struct ICSAdiDirection* ics_adi_dir_z;
        
        void divide_x_work();
        void divide_y_work();
        void divide_z_work();
        void set_num_threads(const int n);
        void do_grid_currents(double dt, int id);  
        void volume_setup();
        int dg_adi();
        void scatter_grid_concentrations();
        void free_Grid();
};

typedef struct ICSAdiDirection{
    void (*ics_dg_adi_dir)(ICS_Grid_node* g, int, int, int, double, double*, double*, double*);
    double* states_in;
    double* states_out;
    double* deltas;
    long* ordered_line_defs;
    long* ordered_nodes;
    long* ordered_start_stop_indices; 
    long* line_start_stop_indices;
    double dc;
    double d;
}ICSAdiDirection;

typedef struct ICSAdiGridData{
    //Start and stop node indices for lines
    int line_start, line_stop;
    //Where to start in the ordered_nodes array
    int ordered_start;
    double* state;
    ICS_Grid_node* g;
    ICSAdiDirection* ics_adi_dir;
    double* scratchpad;
    double* RHS;
    //double* deltas;
}ICSAdiGridData;


static double get_alpha_scalar(double*, int);
static double get_alpha_array(double*, int);
static double get_lambda_scalar(double*, int);
static double get_lambda_array(double*, int);


/***** GLOBALS *******************************************************************/
extern double *dt_ptr;              // Universal ∆t
extern double *t_ptr;               // Universal t
extern double *h_dt_ptr;              // Universal ∆t
extern double *h_t_ptr;               // Universal t

// static int N = 100;                 // Number of grid_lists (size of Parallel_grids)
extern Grid_node *Parallel_grids[100];// Array of Grid_node * lists
/*********************************************************************************/


// Set the global ∆t
void make_dt_ptr(PyHocObject* my_dt_ptr);


// Create a single Grid_node 
/* Parameters:  Python object that includes array of double pointers,
                size of x, y, and z dimensions
                x, y, and z diffusion constants
                delta x, delta y, and delta z*/
ECS_Grid_node *ECS_make_Grid(PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz,
	PyHocObject* my_alpha, PyHocObject* my_lambda, int, double, double);


// Free a single Grid_node "grid"
//void free_Grid(Grid_node *grid);

// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
extern "C" int ECS_insert(int grid_list_index, PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz, 
	PyHocObject* my_alpha, PyHocObject* my_lambda, int, double, double);

Grid_node *ICS_make_Grid(PyHocObject* my_states, long num_nodes, long* neighbors, 
                long* ordered_x_nodes, long* ordered_y_nodes, long* ordered_z_nodes,
                long* x_line_defs, long x_lines_length, long* y_line_defs, long y_lines_length, long* z_line_defs,
                long z_lines_length, double d, double dx);

// Insert an  ICS_Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
extern "C" int ICS_insert(int grid_list_index, PyHocObject* my_states, long num_nodes, long* neighbors,
                long* ordered_x_nodes, long* ordered_y_nodes, long* ordered_z_nodes,
                long* x_line_defs, long x_lines_length, long* y_line_defs, long y_lines_length, long* z_line_defs,
                long z_lines_length, double d, double dx);



// Set the diffusion coefficients for a given grid_id 
extern "C" int set_diffusion(int grid_list_index, int grid_id, double dc_x, double dc_y, double dc_z);

// Delete a specific Grid_node "find" from the list "head"
int remove(Grid_node **head, Grid_node *find);

// Destroy the list located at list_index and free all memory
void empty_list(int list_index);
