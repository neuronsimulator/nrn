extern "C" {
    #include <pthread.h>
    #include <matrix2.h>
} // extern "C"
/*borrowed from Meschach Version 1.2b*/
#define	v_get_val(x,i)		((x)->ve[(i)])
#define	m_get_val(A,i,j)	((A)->me[(i)][(j)])
#define SPECIES_ABSENT      -1
#define PREFETCH 4

typedef void (*fptr)(void);

typedef struct OcPtrVector_{
    PyObject_HEAD
	struct OcPtrVector_ (*newOcPtrVector)();
	void (*deleteOcPtrVector)();
	int (*size)(int);
	void (*resize)();
	void (*pset)(int, double*);
	double (*getval)(int);
	void (*setval)(int,double);
	void (*scatter)(double*,int);
	void (*gather)(double*, int);
	void (*ptr_update_cmd)(void*);
	void (*ptr_update)();
	void* update_cmd_;
	int size_;
	double ** pd_;
} OcPtrVector;		
/*
		OcPtrVector* newOcPtrVector();
		} ;
*/

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
    int*** state_idx;   /*[segment][species][region]*/
    int icsN;   /*total number species*regions per segment*/
    /*NOTE: icsN != num_species*num_regions as every species may not be defined
     *on every region - the missing elements of state_idx are SPECIES_ABSENT*/

    /*ECS for MultiCompartment reactions*/
    int num_ecs_species;
    int num_ecs_params;
    double*** ecs_state;    /*[segment][ecs_species]*/
    int* ecs_offset_index;
    ECS_Grid_node** ecs_grid;
    int** ecs_index;
    int ecsN;    /*total number of ecs species*regions per segment*/
    
    int num_mult;
    double **mc_multiplier;
    int* mc_flux_idx;
    double** vptrs;
    struct ICSReactions* next;
} ICSReactions;

typedef struct {
    /*variables for reactions*/
    double* states_for_reaction;
    double* states_for_reaction_dx;
    double* ecs_states_for_reaction;
    double* ecs_states_for_reaction_dx;
	double* result_array;
	double* result_array_dx;
	double* result_ecs;
	double* result_ecs_dx;
    MAT *jacobian;
    VEC *x;
    VEC *b;
    PERM *pivot;

} ReactionVariables;

typedef struct TaskList
{
    void *(*task)(void*);
    void *args;
    void *result;
    struct TaskList* next;
} TaskList;

typedef struct TaskQueue
{
    pthread_mutex_t* task_mutex;
    pthread_cond_t* task_cond;
    pthread_mutex_t* waiting_mutex;
    pthread_cond_t* waiting_cond;
    int length;
    struct TaskList* first;
    struct TaskList* last;
} TaskQueue;

extern "C" void set_num_threads(const int);
void _fadvance(void);
void _fadvance_fixed_step_3D(void);

extern "C" int get_num_threads(void);
void ecs_set_adi_tort(ECS_Grid_node*);
void ecs_set_adi_vol(ECS_Grid_node*);
void ecs_set_adi_homogeneous(ECS_Grid_node *);

void dg_transfer_data(AdiLineData * const, double* const, int const, int const, int const);
void ecs_run_threaded_dg_adi(const int, const int, ECS_Grid_node*, ECSAdiDirection*, const int);
ReactGridData* create_threaded_reactions(const int);
void* do_reactions(void*);

void current_reaction(double *states);

void run_threaded_deltas(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir);
void run_threaded_ics_dg_adi(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir);
void ics_dg_adi_x(ICS_Grid_node* g, int, int, int, double, double*, double*, double*, double*, double*, double*);
void ics_dg_adi_y(ICS_Grid_node* g, int, int, int, double, double*, double*, double*, double*, double*, double*);
void ics_dg_adi_z(ICS_Grid_node* g, int, int, int, double, double*, double*, double*, double*, double*, double*);
void ics_dg_adi_x_inhom(ICS_Grid_node* g, int, int, int, double, double*, double*, double*, double*, double*, double*);
void ics_dg_adi_y_inhom(ICS_Grid_node* g, int, int, int, double, double*, double*, double*, double*, double*, double*);
void ics_dg_adi_z_inhom(ICS_Grid_node* g, int, int, int, double, double*, double*, double*, double*, double*, double*);

/*Variable step function declarations*/
void _rhs_variable_step(const double*, double*);

void _ode_reinit(double*);

int ode_count(const int);

extern "C" void scatter_concentrations(void);


int find(const int, const int, const int, const int, const int);

void _ics_hybrid_helper(ICS_Grid_node*);
void _ics_variable_hybrid_helper(ICS_Grid_node*, const double*, double* const, const double*, double *const);

void _ics_rhs_variable_step_helper(ICS_Grid_node*, double const* const, double*);
void _rhs_variable_step_helper(Grid_node*, double const * const, double*);

void ics_ode_solve(double, double*, const double*);
void ics_ode_solve_helper(ICS_Grid_node*, double, double*);

void _rhs_variable_step_helper_tort(Grid_node*, double const * const, double*);

void _rhs_variable_step_helper_vol(Grid_node*, double const * const, double*);

void set_num_threads_3D(int n);

void _rhs_variable_step_ecs(const double*, double*);

void clear_rates_ecs();
void do_ics_reactions(double*, double*, double*, double*);
void get_all_reaction_rates(double*, double*, double*);
void _ecs_ode_reinit(double*); 
void do_currents(Grid_node*, double*, double, int);
void TaskQueue_add_task(TaskQueue*, void* (*task)(void* args), void*, void*);
void *TaskQueue_exe_tasks(void*);
void start_threads(const int);
void TaskQueue_sync(TaskQueue*);
void ecs_atolscale(double*);
void apply_node_flux3D(Grid_node*, double, double*);

