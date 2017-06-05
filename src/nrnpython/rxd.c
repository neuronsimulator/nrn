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


fptr _setup, _initialize;
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

/* TODO: remove MAX_REACTIONS; use dynamic storage instead */
#define MAX_REACTIONS 100

fptr _setup, _initialize;



/* TODO: this will need modified to also support intracellular rxd */

void set_setup(const fptr setup_fn) {
	_setup = setup_fn;
}

void set_initialize(const fptr initialize_fn) {
	_initialize = initialize_fn;

	/*Setup threaded reactions*/
    //ecs_refresh_reactions(NUM_THREADS);
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
	int i;
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


void set_reaction_indices( int num_locations, int* regions, int* num_species, int* species_index,  int* indices) {
	int i, j, r, idx;
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

}

void _fadvance(void) {
    /*double* old_states;*/
    double* states_for_reaction;
    double* states_for_reaction_dx;
	double* result_array;
	double* result_array_dx;
    int i, j, r, index, seg_idx=0;
    double dt = *dt_ptr;
    double dx = FLT_EPSILON;
	double pd;
    ReactionRate current_reaction;
    MAT *jacobian;
    VEC *x;
    VEC *b;
    PERM *pivot;

	/*The following code if for diffusion
	 * at present we are only doing reactions*/
	/*
    old_states = (double*) malloc(sizeof(double) * num_states);

    for (i = 0; i < num_states; i++) {
        old_states[i] = states[i];
    }

    for (i = 0; i < num_states; i++) {
        current = states[i];
        shift = 0;
        for (j = neighbor_index[i]; j < neighbor_index[i + 1]; j++) {
            shift += neighbor_rates[j] * (old_states[neighbor_list[j]] - current);
        }
        states[i] += dt * shift;
    }
    */

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

		for(;seg_idx < _regions[r]; seg_idx++)
		{
			// appropriately set the states_for_reaction arrays up
        	for (i = 0; i < _num_species[r]; i++)
			{
            	states_for_reaction[i] = states[INDICES(seg_idx,r,i)];
            	states_for_reaction_dx[i] = states[INDICES(seg_idx,r,i)];
        	}
       	
			for (i = 0; i < _num_species[r]; i++)
			{
				current_reaction(states_for_reaction, result_array);
				v_set_val(b, i, dt*result_array[i]);

				// set up the changed states array
				states_for_reaction_dx[i] += dx;

				/* TODO: Handle approximating the Jacobian at a function upper
				 * limit, e.g. acos(1)
       	         */
				current_reaction(states_for_reaction_dx, result_array_dx);


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
    /* Code is only used for diffusion */
	/* free(old_states);*/
}


/*****************************************************************************
*
* End intracellular code
*
*****************************************************************************/




