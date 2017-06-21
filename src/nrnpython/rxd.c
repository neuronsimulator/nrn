#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <matrix.h>
#include <pthread.h>
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif


/* a macro to access _indices by segment, region and species index*/
#define INDICES(seg,reg,spe) (_indices[(seg)][_species_idx[(reg)][(spe)]])

/*
    Globals
*/

/* TODO: remove MAX_REACTIONS; use dynamic storage instead */
#define MAX_REACTIONS 100
int NUM_THREADS = 1;


fptr _setup, _initialize, _setup_matrices;

/*intracellular diffusion*/
int _rxd_euler_nrow, _rxd_euler_nnonzero, _rxd_num_zvi;
long* _rxd_euler_nonzero_i;
long* _rxd_euler_nonzero_j;
double* _rxd_euler_nonzero_values;
long* _rxd_zero_volume_indices;
double* _rxd_a;
double* _rxd_b;
double* _rxd_c;
double* _rxd_d;
int* _rxd_p;    


/*intracellular reactions*/
double* states;
double* dt_ptr;
int num_states;
ReactionRate reactions[MAX_REACTIONS];
int num_reactions = 0;
int* _num_species = NULL;
int _max_species_per_location = 0;
int _num_locations = 0;
int** _indices = NULL;
int** _species_idx = NULL;
int* _regions;
int _curr_count;
int* _curr_indices = NULL;
double* _curr_scales = NULL; 
PyHocObject** _curr_ptrs = NULL;
int  _conc_count;
int* _conc_indices = NULL;
PyHocObject** _conc_ptrs = NULL;
double ***_mult = NULL;
int * _mc_mult_count = NULL;


static void tranfer_to_legacy()
{
	/*TODO: support 3D*/
	int i;
	for(i = 0; i < _conc_count; i++)
	{
		*(double*)_conc_ptrs[i]->u.px_ = states[_conc_indices[i]];
	}
}

/* TODO: remove MAX_REACTIONS; use dynamic storage instead */
#define MAX_REACTIONS 100


void rxd_set_euler_matrix(int nrow, int nnonzero, long* nonzero_i,
                          long* nonzero_j, double* nonzero_values,
                          long* zero_volume_indices, int num_zero_volume_indices,
                          double* diffusion_a_base, double* diffusion_b_base,
                          double* diffusion_d_base, int* diffusion_p, 
                          double* c_diagonal,
						  int num_currents, int* curr_index, double* curr_scale,
						  PyHocObject** curr_ptrs, int conc_count, 
						  int* conc_index, PyHocObject** conc_ptrs) {

    /* TODO: is it better to use a pointer or do a copy */
	_rxd_euler_nrow = nrow;
    _rxd_euler_nnonzero = nnonzero;
    _rxd_euler_nonzero_i = nonzero_i;
    _rxd_euler_nonzero_j = nonzero_j;
    _rxd_euler_nonzero_values = nonzero_values;
    
    _rxd_num_zvi = num_zero_volume_indices;
    _rxd_zero_volume_indices = zero_volume_indices; 

    _rxd_a = diffusion_a_base;
    _rxd_b = diffusion_b_base;
    _rxd_d = diffusion_d_base;
    _rxd_p = diffusion_p;
    _rxd_c = c_diagonal;
	
	/*Info for NEURON currents - to update states*/
	_curr_count = num_currents;
	if(_curr_indices != NULL)
		free(_curr_indices);
	_curr_indices = malloc(sizeof(int)*num_currents);
	memcpy(_curr_indices,curr_index, sizeof(int)*num_currents); 
	
	if(_curr_scales != NULL)
		free(_curr_scales);
	_curr_scales = malloc(sizeof(double)*num_currents);
	memcpy(_curr_scales, curr_scale, sizeof(double)*num_currents); 

	if(_curr_ptrs != NULL)
		free(_curr_ptrs);
	_curr_ptrs = malloc(sizeof(PyHocObject)*num_currents);
	memcpy(_curr_ptrs, curr_ptrs, sizeof(PyHocObject*)*num_currents);

	/*Info for NEURON concentration - to transfer to legacy*/
	_conc_count = conc_count;

	if(_conc_indices != NULL)
		free(_conc_indices);
	_conc_indices = malloc(sizeof(int)*conc_count);
	memcpy(_conc_indices, conc_index, sizeof(int)*conc_count); 
	
	if(_conc_ptrs != NULL)
		free(_conc_ptrs);
	_conc_ptrs = malloc(sizeof(PyHocObject)*conc_count);
	memcpy(_conc_ptrs, conc_ptrs, sizeof(PyHocObject*)*conc_count);
}

static void mul(int nrow, int nnonzero, long* nonzero_i, long* nonzero_j, double* nonzero_values, double* v, double* result) {
    long i, j, k;
	double dt = *dt_ptr;
    bzero(result,sizeof(double)*nrow);

    /* now loop through all the nonzero locations */
    /* NOTE: this would be more efficient if not repeatedly doing the result[i] lookup */
    for (k = 0; k < nnonzero; k++) {
        i = *nonzero_i++;
        j = *nonzero_j++;
        result[i] += (*nonzero_values++) * v[j];
    }

	/*Add currents to the result
	 * TODO: RxD induced currents
	 */
	for (k = 0; k < _curr_count; k++)
	{
		result[_curr_indices[k]] += _curr_scales[k] * (*_curr_ptrs[k]->u.px_);

	}

}

void set_setup(const fptr setup_fn) {
	_setup = setup_fn;
}

void set_initialize(const fptr initialize_fn) {
	_initialize = initialize_fn;

	/*Setup threaded reactions*/
    ecs_refresh_reactions(NUM_THREADS);
}

void set_setup_matrices(fptr setup_matrices) {
    _setup_matrices = setup_matrices;
}

/* nrn_tree_solve modified from nrnoc/ldifus.c */
void nrn_tree_solve(double* a, double* b, double* c, double* dbase, double* rhs, int* pindex, int n, double dt) {
    /*
        treesolver
        
        a      - above the diagonal 
        b      - below the diagonal
        c      - used to define diagonal: diag = c + dt * dbase
        dbase  - together with c, used to define diagonal
        rhs    - right hand side, which is changed to the result
        pindex - parent indices
        n      - number of states
    */
    int i;
    double* d = malloc(sizeof(double) * n);
    double* myd;
    double* myc;
    double* mydbase;

    /* TODO: check that d non-null */
    
    /* construct diagonal */
    myd = d;
    mydbase = dbase;
    myc = c;
    for (i = 0; i < n; i++) {
        *myd++ = *myc++ + dt * (*mydbase++);
    }

	/* triang */
	for (i = n - 1; i > 0; --i) {
		int pin = pindex[i];
		if (pin > -1) {
			double p;
			p = dt * a[i] / d[i];
			d[pin] -= dt * p * b[i];
			rhs[pin] -= p * rhs[i];
		}
	}
	/* bksub */
	for (i = 0; i < n; ++i) {
		int pin = pindex[i];
		if (pin > -1) {
			rhs[i] -= dt * b[i] * rhs[pin];
		}
		rhs[i] /= d[i];
	}
	
	free(d);
}


int rxd_nonvint_block(int method, int size, double* p1, double* p2, int thread_id) {
    switch (method) {
        case 0:
            _setup();
            break;
        case 1:
            _initialize();
            break;
        case 2:
            /* compute outward current to be subtracted from rhs */
            break;
        case 3:
            /* conductance to be added to d */
            break;
        case 4:
            /* fixed step solve */
			_fadvance();
            _fadvance_fixed_step_ecs();
            break;
        case 5:
            /* ode_count */
            return ode_count(size);
            break;
        case 6:
            /* ode_reinit(y) */
            _ode_reinit(p1);
            break;
        case 7:
            /* ode_fun(t, y, ydot); from t and y determine ydot */
            _rhs_variable_step(*t_ptr, p1, p2);
            break;
        case 8:
            /* ode_solve(dt, t, b, y); solve mx=b replace b with x */
            /* TODO: we can probably reuse the dgadi code here... for now, we do nothing, which implicitly approximates the Jacobian as the identity matrix */
            break;
        case 9:
            /* ode_jacobian(dt, t, ypred, fpred); optionally prepare jacobian for fast ode_solve */
            break;
        case 10:
            /* ode_abs_tol(y_abs_tolerance); fill with cvode.atol() * scalefactor */
            break;
        default:
            printf("Unknown rxd_nonvint_block call: %d\n", method);
            break;
    }
	/* printf("method=%d, size=%d, thread_id=%d\n", method, size, thread_id);	 */
    return 0;
}


 
/*****************************************************************************
*
* Begin intracellular code
*
*****************************************************************************/



/*unset_reaction_indices clears the global arrays storing the indices than
 * link reactions to a state variable
 */
static void unset_reaction_indices()
{
	int i, j;
	if(_indices != NULL)
	{
		for(i=0;i<_num_locations;i++)
			if(_indices[i] != NULL) free(_indices[i]);
		free(_indices);
		_indices = NULL;
	}

	if(_species_idx != NULL)
	{
		for(i=0;i<num_reactions;i++)
			if(_species_idx[i] != NULL) free(_species_idx[i]);
		free(_species_idx);
		_species_idx = NULL;
	}
	if(_mult != NULL)
	{
		if(_mc_mult_count != NULL)
		{
			if(_regions != NULL)
			{
				for(i = 0;  i < num_reactions; i++)
				{
					if(_mc_mult_count[i] != 0)
					{
						for(j = 0; j < _regions[i]; j++)
						{
							if(_mult[i][j] != NULL) free(_mult[i][j]);
						}
						free(_mult[i]);
					}
				}
			}
			free(_mc_mult_count);
			_mc_mult_count = NULL;
		}
		free(_mult);
		_mult = NULL;
	}
	
	if(_num_species != NULL)
	{
		free(_num_species);
		_num_species = NULL;
	}
	if(_regions != NULL)
	{
		free(_num_species);
		_num_species = NULL;
	}
	_num_locations = 0;
	_max_species_per_location = 0;
}

void clear_rates(void) {
	int i;
	unset_reaction_indices();	
    num_reactions = 0;

	clear_rates_ecs();
}

void register_rate(ReactionRate f) {
    assert(num_reactions < MAX_REACTIONS);
    reactions[num_reactions++] = f;
}

void setup_solver(double* my_states, PyHocObject* my_dt_ptr, int my_num_states) {
    states = my_states;
    num_states = my_num_states;
    dt_ptr = my_dt_ptr -> u.px_;
}


void set_reaction_indices( int num_locations, int* regions, int* num_species, int* species_index,  int* indices, int* mc_mult_count, double* mc_mult) {
	int i, j, k, r, idx;
	unset_reaction_indices();	
	_num_locations = num_locations;

	/*Note: assumes each region has exactly one aggregated reaction*/
	_regions = (int*)malloc(sizeof(int)*num_reactions);
	memcpy(_regions,regions,sizeof(int)*num_reactions);
	
	_num_species = (int*)malloc(sizeof(int)*num_reactions);
	memcpy(_num_species,num_species,sizeof(int)*num_reactions);

	_species_idx = (int**)malloc(sizeof(int)*num_reactions);
	for(idx=0, i=0; i < num_reactions; i++)
	{
		_species_idx[i] = malloc(sizeof(int)*_num_species[i]);
		_max_species_per_location = MAX(_max_species_per_location,_num_species[i]);
		for(j=0; j<_num_species[i]; j++)
			_species_idx[i][j] = species_index[idx++];
	}
	
	_indices = (int**)malloc(sizeof(int*)*num_locations);
	for(idx=0, r=0; r<num_reactions; r++)
	{
		for(i = 0; i<regions[r]; i++)
		{
			_indices[i] = malloc(sizeof(int)*_num_species[r]);
			for(j=0; j<num_species[r]; j++)
				_indices[i][j] = indices[idx++];
		}
	}

	_mc_mult_count = (int*)malloc(sizeof(int)*num_reactions);
	memcpy(_mc_mult_count, mc_mult_count, sizeof(int)*num_reactions);

	_mult = (double***)malloc(sizeof(double**)*num_reactions);
	for(idx = 0, i = 0;  i < num_reactions; i++)
	{
		if(mc_mult_count[i] == 0)
		{
			_mult[i] = NULL;
		}
		else
		{
			_mult[i] = (double**)malloc(sizeof(double*)*_regions[i]);
			for(j = 0; j < mc_mult_count[i]; j++)
			{
				for(k = 0; k < _regions[i]; k++)
				{
					if(j==0)
						_mult[i][k] = (double*)malloc(sizeof(double)*mc_mult_count[i]);
					_mult[i][k][j] = mc_mult[idx++];
				}	
			}
		}
	}

}

void _fadvance(void) {
	double dt = *dt_ptr;
	int i, j;

	/*variables for diffusion*/
	double *rhs = malloc(sizeof(double*) * _rxd_euler_nrow);
	long* zvi = _rxd_zero_volume_indices;
	
    /*variables for reactions*/
    double* states_for_reaction;
    double* states_for_reaction_dx;
	double* result_array;
	double* result_array_dx;
   	int r, index, seg_idx=0, reg_seg_idx;
    double dx = FLT_EPSILON;
	double pd;
	double *mc_mult_for_reaction;
    ReactionRate current_reaction;
    MAT *jacobian;
    VEC *x;
    VEC *b;
    PERM *pivot;

    /*diffusion*/
	mul(_rxd_euler_nrow, _rxd_euler_nnonzero, _rxd_euler_nonzero_i, _rxd_euler_nonzero_j, _rxd_euler_nonzero_values, states, rhs);
	
	/* multiply rhs vector by dt */
    for (i = 0; i < _rxd_euler_nrow; i++) {
        rhs[i] *= dt;
    }
	nrn_tree_solve(_rxd_a, _rxd_b, _rxd_c, _rxd_d, rhs, _rxd_p, _rxd_euler_nrow, dt);
   
    /* increment states by rhs which is now really deltas */
    for (i = 0; i < _rxd_euler_nrow; i++) {
        states[i] += rhs[i];
    }

    /* clear zero volume indices (conservation nodes) */
    for (i = 0; i < _rxd_num_zvi; i++) {
        states[zvi[i]] = 0;
    }

	/*reactions*/
    states_for_reaction = (double*) malloc(sizeof(double) * _max_species_per_location);
    states_for_reaction_dx = (double*) malloc(sizeof(double) * _max_species_per_location);
    result_array = (double*) malloc(sizeof(double) * _max_species_per_location);
    result_array_dx = (double*) malloc(sizeof(double) * _max_species_per_location);
	for(r = 0; r < num_reactions; r++)
	{
		// set up jacobian and other relevant matrices
		jacobian = m_get(_num_species[r], _num_species[r]);
        b = v_get(_num_species[r]);
        x = v_get(_num_species[r]);
		pivot = px_get(jacobian->m);

		current_reaction = reactions[r];

		for(reg_seg_idx = 0 ;seg_idx < _regions[r]; reg_seg_idx++, seg_idx++)
		{
			// appropriately set the states_for_reaction arrays up
			mc_mult_for_reaction = (_mult[r]==NULL?NULL:_mult[r][reg_seg_idx]);
        	for (i = 0; i < _num_species[r]; i++)
			{
            	states_for_reaction[i] = states[INDICES(seg_idx,r,i)];
            	states_for_reaction_dx[i] = states[INDICES(seg_idx,r,i)];
        	}
       	
			for (i = 0; i < _num_species[r]; i++)
			{
				current_reaction(states_for_reaction, result_array,mc_mult_for_reaction);
				v_set_val(b, i, dt*result_array[i]);

				// set up the changed states array
				states_for_reaction_dx[i] += dx;

				/* TODO: Handle approximating the Jacobian at a function upper
				 * limit, e.g. acos(1)
       	         */
				current_reaction(states_for_reaction_dx, result_array_dx,mc_mult_for_reaction);


       	    	for (j = 0; j < _num_species[r]; j++)
				{
					// pd is our jacobian approximated
       	            pd = (result_array_dx[j] - result_array[j])/dx;
					m_set_val(jacobian, j, i, (i==j) - dt*pd);
       	        }			
				// reset dx array
            	states_for_reaction_dx[i] -= dx;
        	}
       
	   		// solve for x, destructively
        	LUfactor(jacobian, pivot);
        	LUsolve(jacobian, pivot, b, x);

			for (i = 0; i < _num_species[r]; i++)
           		states[INDICES(seg_idx,r,i)] += v_get_val(x,i);
		}
        m_free(jacobian);
        v_free(b);
        v_free(x);
        px_free(pivot);
    }
    SAFE_FREE(states_for_reaction_dx);
    SAFE_FREE(states_for_reaction);
    SAFE_FREE(result_array);
    SAFE_FREE(result_array_dx);
	SAFE_FREE(rhs);

	tranfer_to_legacy();
}


/*****************************************************************************
*
* End intracellular code
*
*****************************************************************************/




