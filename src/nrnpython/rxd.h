#include <pthread.h>
/*borrowed from Meschach Version 1.2b*/
#define	v_get_val(x,i)		((x)->ve[(i)])
#define	m_get_val(A,i,j)	((A)->me[(i)][(j)])

typedef void (*fptr)(void);

typedef struct {
    PyObject_HEAD
	struct OcPtrVector (*newOcPtrVector)();
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
        double* copyFrom;
        long copyTo;
} AdiLineData;

typedef struct {
    int start, stop;
    AdiLineData* vals;
    double* state;
    Grid_node g;
    int sizej;
    AdiLineData (*dg_adi_dir)(Grid_node, double, int, int, double const *, double*);
    double* scratchpad;
} AdiGridData;

void set_num_threads(int);
void _fadvance(void);
void _fadvance_fixed_step_ecs(void);

int get_num_threads(void);
static int dg_adi(Grid_node);
int dg_adi_vol(Grid_node);
int dg_adi_tort(Grid_node);
void dg_transfer_data(AdiLineData * const, double* const, int const, int const, int const);
void run_threaded_dg_adi(AdiGridData*, pthread_t*, const int, const int, Grid_node, double*, AdiLineData*, AdiLineData (*dg_adi_dir)(Grid_node, double, int, int, double const *, double*), const int n);

ReactGridData* create_threaded_reactions(void);
void* do_reactions(void*);

typedef void (*fptr)(void);
void current_reaction(double *states);


/*Variable step function declarations*/
void _rhs_variable_step(const double, const double*, double*);

void _ode_reinit(double*);

int ode_count(const int);

void scatter_concentrations(void);

static void update_boundaries_x(int i, int j, int k, int dj, int dk, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot);


static void update_boundaries_y(int i, int j, int k, int di, int dk, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot);

static void update_boundaries_z(int i, int j, int k, int di, int dj, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot);

static void _rhs_variable_step_helper(Grid_node* grid, const double const* states, double* ydot);

int find(const int, const int, const int, const int, const int);

void _rhs_variable_step_helper_tort(Grid_node*, const double const*, double*);

void _rhs_variable_step_helper_vol(Grid_node*, const double const*, double*);
