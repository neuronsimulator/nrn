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

typedef void (*ReactionRate)(double**, double**, double*, double**, double**, double**);
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

typedef struct Grid_node {
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
    struct Grid_node *next;
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
    struct AdiGridData* tasks;
    struct AdiDirection* adi_dir_x;
    struct AdiDirection* adi_dir_y;
    struct AdiDirection* adi_dir_z;
    double atolscale;
} Grid_node;

typedef struct AdiDirection{
    void (*dg_adi_dir)(Grid_node*, const double, const int, const int, double const * const, double* const, double* const);
    double* states_in;
    double* states_out;
    int line_size;
} AdiDirection;

typedef struct AdiGridData{
    int start, stop;
    double* state;
    Grid_node* g;
    int sizej;
    AdiDirection* adi_dir;
    double* scratchpad;
} AdiGridData;



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
Grid_node *make_Grid(PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz,
	PyHocObject* my_alpha, PyHocObject* my_lambda, int, double, double);


// Free a single Grid_node "grid"
void free_Grid(Grid_node *grid);

// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
int insert(int grid_list_index, PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz, 
	PyHocObject* my_alpha, PyHocObject* my_lambda, int, double, double);

// Set the diffusion coefficients for a given grid_id 
int set_diffusion(int grid_list_index, int grid_id, double dc_x, double dc_y, double dc_z);

// Delete a specific Grid_node "find" from the list "head"
int delete(Grid_node **head, Grid_node *find);

// Destroy the list located at list_index and free all memory
void empty_list(int list_index);
