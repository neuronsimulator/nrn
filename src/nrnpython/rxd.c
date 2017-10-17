#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <matrix2.h>
#include <pthread.h>
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif


/* a macro to access _indices by segment, region and species index*/
#define INDICES(seg,reg,spe) (_indices[(seg)][_species_idx[(reg)][(spe)]])
/* Look-up the required ecs state
 * _mc_ecs_grids is an array of grid nodes by region and the 
 *     ecs species number for that region (not it's position in Parallel_Grids)
 * _ecs_indices is an array of state indices index by the MulitCompartment 
 *     reaction species number, region and segment in the region
 * _ecs_grid_ids gives the species number for a given region and local
 *TODO: replace the ecs_spe index in the reaction with the ecs species number 
 *	for MultiCompartment reactions in rxd.py  i.e. remove the need for 
 *	_ecs_grid_ids
 */
#define ECS_STATE(ecs_spe,reg,seg) ((_mc_ecs_grids[(reg)][(ecs_spe)])->states[_ecs_indices[(_ecs_grid_ids[(reg)][(ecs_spe)])][(reg)][(seg)]])

/*
    Globals
*/

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

static int _cvode_offset;

/*intracellular reactions*/
ICSReactions* _reactions = NULL;

/*intracellular reactions*/
double* states;
int num_states;
int _num_reactions = 0;
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
double*** _mult = NULL;
int* _mc_mult_count = NULL;
int*** _ecs_indices = NULL;
int** _ecs_grid_ids = NULL;
int* _ecs_species_count = NULL;
int _num_ecs_species = 0;
Grid_node*** _mc_ecs_grids;

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
			_fadvance_fixed_step_ecs();
			_fadvance();
            break;
        case 5:
            /* ode_count */
            _cvode_offset = size + ode_count(size);
            return num_states + _cvode_offset - size;
            break;
        case 6:
            /* ode_reinit(y) */
            _ode_reinit(p1);
            _ecs_ode_reinit(p1);
            break;
        case 7:
            /* ode_fun(t, y, ydot); from t and y determine ydot */
            _rhs_variable_step(*t_ptr, p1, p2);
            _rhs_variable_step_ecs(*t_ptr, p1, p2);
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
		for(i=0; i < _num_locations; i++)
			if(_indices[i] != NULL) free(_indices[i]);
		free(_indices);
		_indices = NULL;
	}

	if(_species_idx != NULL)
	{
		for(i=0;i<_num_reactions;i++)
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
				for(i = 0;  i < _num_reactions; i++)
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

	if(_num_ecs_species > 0)
	{
		for(i = 0; i < _num_reactions; i++)
		{
			if(_ecs_species_count[i]>0)
			{
				free(_ecs_grid_ids[i]);
				free(_mc_ecs_grids[i]);
			}
		}
		free(_ecs_species_count);

		for(i = 0; i < _num_ecs_species; i++)
		{
			for(j = 0; j < _num_reactions; j++)
			{
					free(_ecs_indices[i][j]);
			}
			free(_ecs_indices[i]);
		}
		free(_ecs_indices);
	
		_num_ecs_species = 0;
		_ecs_indices = NULL;
		_ecs_species_count = NULL;
		_ecs_grid_ids = NULL;
		_mc_ecs_grids = NULL;
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


void register_rate(int nspecies, int nregions, int nseg, int* sidx, int necs, int* ecs_ids, int* ecsidx, int nmult, double* mult, ReactionRate f)
{
    int i,j,k,idx, ecs_id, ecs_index;
    Grid_node* grid;
    ICSReactions* react = (ICSReactions*)malloc(sizeof(ICSReactions));
    react->reaction = f;
    react->num_species = nspecies;
    react->num_regions = nregions;
    react->num_segments = nseg;
    react->num_ecs_species = necs; 
    react->num_mult = nmult;
    react->icsN = 0;
    react->ecsN = 0;
    react->state_idx = (int***)malloc(nseg*sizeof(double**));
    for(i = 0, idx=0; i < nseg; i++)
    {
        react->state_idx[i] = (int**)malloc(nspecies*sizeof(double*));
        for(j = 0; j < nspecies; j++)
        {
            react->state_idx[i][j] = (int*)malloc(nregions*sizeof(double*));
            for(k = 0; k < nregions; k++, idx++)
            {
                if(sidx[idx] < 0)
                {
                    react->state_idx[i][j][k] = SPECIES_ABSENT;
                }
                else
                {
                    react->state_idx[i][j][k] = sidx[idx];
                    //fprintf(stderr,"state_idx[%i][%i][%i] = %i\n",i,j,k,react->state_idx[i][j][k]);
                    if(i==0) react->icsN++;
                }
            }
        } 
    }
    if( nmult > 0 )
    {
        react->mc_multiplier = (double**)malloc(nmult*sizeof(double*));
        for(i = 0; i < nmult; i++)
        {
            react->mc_multiplier[i] = (double*)malloc(nseg*sizeof(double));
            memcpy(react->mc_multiplier[i], (mult+i*nseg), nseg*sizeof(double));
        }

    }

    if(react->num_ecs_species > 0)
    {
        react->ecs_state = (double****)malloc(nseg*sizeof(double***));
        for(i = 0; i < nseg; i++)
        {
            react->ecs_state[i] = (double***)malloc(nseg*sizeof(double**));
        }

        for(j = 0; j < necs; j++)
        {
            react->ecs_state[i][j] = (double**)malloc(nregions*sizeof(double**));
            for(ecs_id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, ecs_id++)
		    {
	            if (ecs_id == ecs_ids[j])
		    	{
                    for(i = 0; i < nseg; i++)
                    {
                        react->ecs_state[i][j][k] = (double*)malloc(sizeof(double));
                        for(k = 0; k < nregions; k++)
                        {
                            ecs_index = ecsidx[i*nseg + j*nregions + k];
                            if(ecs_index < 0)
                            {
                                react->ecs_state[i][j][k] = NULL;
                            }
                            else
                            {
                                react->ecs_state[i][j][k] = &(grid->states[ecs_index]);
                                if(i==0) react->ecsN++;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        react->ecs_state = NULL;
    }   
    if(_reactions == NULL)
    {
        _reactions = react;
        react -> next = NULL;

    }
    else
    {
        react -> next = _reactions;
        _reactions = react;
    }
}

void clear_rates()
{
    ICSReactions *react, *prev;
    int i, j, k;
    for(react = _reactions; react != NULL;)
    {
        for(i = 0; i < react->num_segments; i++)
        {
            for(j = 0; j < react->num_species; j++)
            {
                free(react->state_idx[i][j]);
            }
            free(react->state_idx[i]);
           
            if(react->num_ecs_species > 0)
            {
                for(j = 0; j < react->num_ecs_species; j++)
                {
                    for(k = 0; k < react->num_regions; k++)
                        free(react->ecs_state[i][j][k]);
                    free(react->ecs_state[i][j]);
                }
                free(react->ecs_state[i]);
            }
        }
        if(react->num_mult > 0)
        {
            for(i = 0; i < react->num_mult; i++)
                free(react->mc_multiplier[i]);
            free(react->mc_multiplier);
        }

        free(react->state_idx);
        SAFE_FREE(react->ecs_state);
        prev = react;
        react = react->next;
        free(prev);
    }
    _reactions = NULL;
    /*clear extracellular reactions*/
    clear_rates_ecs();
}

void setup_solver(double* my_states, PyHocObject* my_dt_ptr, int my_num_states) {
    states = my_states;
    num_states = my_num_states;
    dt_ptr = my_dt_ptr -> u.px_;
}


void set_reaction_indices( int num_locations, int* regions, int* num_species, 
	int* species_index,  int* indices, int* mc_mult_count, double* mc_mult,
	int num_ecs_species, int* ecs_grid_ids, int* ecs_species_counts, 
	int* ecs_species_grid_ids, int* ecs_indices)
{
	int i, j, k, r, idx;
	int max_ecs_species = 0;
	Grid_node* grid;
	unset_reaction_indices();
	_num_locations = num_locations;

	/*Note: assumes each region has exactly one aggregated reaction*/
	_regions = (int*)malloc(sizeof(int)*_num_reactions);
	memcpy(_regions,regions,sizeof(int)*_num_reactions);
	
	_num_species = (int*)malloc(sizeof(int)*_num_reactions);
	memcpy(_num_species,num_species,sizeof(int)*_num_reactions);

	_species_idx = (int**)malloc(sizeof(int)*_num_reactions);
	for(idx=0, i=0; i < _num_reactions; i++)
	{
		_species_idx[i] = malloc(sizeof(int)*_num_species[i]);
		_max_species_per_location = MAX(_max_species_per_location,_num_species[i]);
		for(j=0; j<_num_species[i]; j++)
			_species_idx[i][j] = species_index[idx++];
	}
	
	_indices = (int**)malloc(sizeof(int*)*num_locations);
	for(idx=0, r=0, k=0; r < _num_reactions; r++)
	{
		for(i = 0; i < regions[r]; i++, k++)
		{
			_indices[k] = malloc(sizeof(int)*_num_species[r]);
			for(j=0; j < _num_species[r]; j++)
			{
				_indices[k][j] = indices[idx++];
			}
		}
	}

	_mc_mult_count = (int*)malloc(sizeof(int)*_num_reactions);
	memcpy(_mc_mult_count, mc_mult_count, sizeof(int)*_num_reactions);

	_mult = (double***)malloc(sizeof(double**)*_num_reactions);
	for(idx = 0, i = 0;  i < _num_reactions; i++)
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

	_num_ecs_species = num_ecs_species;
	if(num_ecs_species > 0)
	{
		/*Store an array of pointers to the relevant grids*/
		_mc_ecs_grids = (Grid_node***)malloc(sizeof(Grid_node**)*_num_reactions);

		/*an array that maps the species number in the current reaction to
		 * the relevant index stored in _ecs_indices*/
		_ecs_grid_ids = (int**)malloc(sizeof(int*)*_num_reactions);

		_ecs_species_count = (int*)malloc(sizeof(int)*_num_reactions);


		for(i = 0, idx = 0; i < _num_reactions; i++)
		{
			_ecs_species_count[i] = ecs_species_counts[i];
			if(ecs_species_counts[i]>0)
			{
				_mc_ecs_grids[i] = (Grid_node**)malloc(sizeof(Grid_node*)*ecs_species_counts[i]);
				_ecs_grid_ids[i] = (int*)malloc(sizeof(int)*ecs_species_counts[i]);

				for(j = 0; j<ecs_species_counts[i]; j++, idx++)
				{
					/*TODO: handle multiple Parallel_grids */
					for(k = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, k++)
					{
						if (k == ecs_species_grid_ids[idx])
						{
							_mc_ecs_grids[i][j] = grid;
							break;
						}
					}

					for(k = 0; k < num_ecs_species; k++)
					{
						if(ecs_grid_ids[k] == ecs_species_grid_ids[idx])
							_ecs_grid_ids[i][j] = k;
					}
				}
			}
			else
			{
				_mc_ecs_grids[i] = NULL;
				_ecs_grid_ids[i] = NULL;
			}

			/*Store an array of pointers to the relevant grid states index*/
			_ecs_indices = (int***)malloc(sizeof(int**)*num_ecs_species);
			for(i = 0, idx = 0; i < num_ecs_species; i++)
			{
				/*Store an array of indices to the grid states
			 	* indexed by ecs species number, region and segment*/
				_ecs_indices[i] = (int**)malloc(sizeof(int*)*_num_reactions);
				for(r = 0; r < _num_reactions; r++)
				{
					_ecs_indices[i][r] = (int*)malloc(sizeof(int)*regions[r]);

					for(j = 0; j < regions[r]; j++, idx++)
					{
						_ecs_indices[i][r][j] = ecs_indices[idx];
					}
				}
			}
		}
	}
	else
	{
		_mc_ecs_grids = NULL;
		_ecs_grid_ids = NULL;
		_ecs_indices = NULL;
		_ecs_species_count = NULL;
	}
}

void _fadvance(void) {
	double dt = *dt_ptr;
    double t = *t_ptr;
	int i, j, k;

	/*variables for diffusion*/
	double *rhs = malloc(sizeof(double*) * _rxd_euler_nrow);
	long* zvi = _rxd_zero_volume_indices;

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
    free(rhs);

    /*reactions*/
    do_ics_reactions(t, states, NULL);	
	
	tranfer_to_legacy();
}

void _ode_reinit(double* y) {
    y += _cvode_offset;
    memcpy(y,states,num_states*sizeof(double));
}


void _rhs_variable_step(const double t, const double* p1, double* p2) 
{
	int i, j, k;
    double dt = *dt_ptr;
    const unsigned char calculate_rhs = p2 == NULL ? 0 : 1;
    const double* my_states = p1 + _cvode_offset;
    double* ydot = p2 + _cvode_offset;

	/*variables for diffusion*/
	double *rhs;
	long* zvi = _rxd_zero_volume_indices;

    if(!calculate_rhs)
        return;

    bzero(ydot,num_states*sizeof(double));
    
    
    /*diffusion*/
    rhs = (double*)malloc(sizeof(double*) * _rxd_euler_nrow);
	mul(_rxd_euler_nrow, _rxd_euler_nnonzero, _rxd_euler_nonzero_i, _rxd_euler_nonzero_j, _rxd_euler_nonzero_values, my_states, rhs);
	
	/* multiply rhs vector by dt */
    for (i = 0; i < _rxd_euler_nrow; i++) {
        rhs[i] *= dt;
    }
	nrn_tree_solve(_rxd_a, _rxd_b, _rxd_c, _rxd_d, rhs, _rxd_p, _rxd_euler_nrow, dt);
   
    /* increment states by rhs which is now really deltas */
    for (i = 0; i < _rxd_euler_nrow; i++) {
        ydot[i] = rhs[i]/dt;
        states[i] += rhs[i];
    }

    /* clear zero volume indices (conservation nodes) */
    for (i = 0; i < _rxd_num_zvi; i++) {
        ydot[zvi[i]] = 0;
    }
    free(rhs);

	/*reactions*/
    do_ics_reactions(t, states, ydot);
    tranfer_to_legacy();
}


void solve_reaction(ICSReactions* react, double* states, double *ydot)
{
    int segment;
    int i, j, idx, jac_i, jac_j, jac_idx;
    int N = react->icsN + react->ecsN;
    double pd;
    double dt = *dt_ptr;
    double dx = FLT_EPSILON;
    MAT* jacobian = m_get(N,N);
    VEC* b = v_get(N);
    VEC* x = v_get(N);
	PERM* pivot = px_get(N);
    
    double** states_for_reaction = (double**)malloc(react->num_species*sizeof(double*));
    double** states_for_reaction_dx = (double**)malloc(react->num_species*sizeof(double*));
    double** result_array = (double**)malloc(react->num_species*sizeof(double*));
    double** result_array_dx = (double**)malloc(react->num_species*sizeof(double*));
    double* mc_mult;
    if(react->num_mult > 0)
        mc_mult = (double*)malloc(react->num_mult*sizeof(double));

    double** ecs_states_for_reaction;
    double** ecs_states_for_reaction_dx;
    double** ecs_result;
    double** ecs_result_dx;
    if(react->num_ecs_species>0)
    {
        ecs_states_for_reaction = (double**)malloc(react->num_ecs_species*sizeof(double*));
        ecs_states_for_reaction_dx = (double**)malloc(react->num_ecs_species*sizeof(double*));
        ecs_result = (double**)malloc(react->num_ecs_species*sizeof(double*));
        ecs_result_dx = (double**)malloc(react->num_ecs_species*sizeof(double*));
        for(i = 0; i < react->num_ecs_species; i++)
	    {
	        ecs_states_for_reaction[i] = (double*)malloc(react->num_regions*sizeof(double));
	        ecs_states_for_reaction_dx[i] = (double*)malloc(react->num_regions*sizeof(double));
	        ecs_result[i] = (double*)malloc(react->num_regions*sizeof(double));
	        ecs_result_dx[i] = (double*)malloc(react->num_regions*sizeof(double));
        }

    }

    for(i = 0; i < react->num_species; i++)
    {
        states_for_reaction[i] = (double*)malloc(react->num_regions*sizeof(double));
        states_for_reaction_dx[i] = (double*)malloc(react->num_regions*sizeof(double));
        result_array[i] = (double*)malloc(react->num_regions*sizeof(double));
        result_array_dx[i] = (double*)malloc(react->num_regions*sizeof(double));
        
    }
    for(segment = 0; segment < react->num_segments; segment++)
    {
        for(i = 0; i < react->num_species; i++)
        {

            for(j = 0; j < react->num_regions; j++)
	        {
                if(react->state_idx[segment][i][j] != SPECIES_ABSENT)
                {
            	    states_for_reaction[i][j] = states[react->state_idx[segment][i][j]];
            	    states_for_reaction_dx[i][j] = states_for_reaction[i][j];
        	    }
                else
                {
                    states_for_reaction[i][j] = -1.0;
                    states_for_reaction_dx[i][j] = states_for_reaction[i][j];
                }

	        }
	    }

	    for(i = 0; i < react->num_ecs_species; i++)
	    {
	        for(j = 0; j < react->num_regions; j++)
	        {
	            if(react->ecs_state[segment][i][j] != NULL)
	            {
	            	ecs_states_for_reaction[i][j] = *(react->ecs_state[segment][i][j]);
	            	ecs_states_for_reaction_dx[i][j] = ecs_states_for_reaction[i][j];
	        	}
	        }
	    }
        for(i = 0; i < react->num_mult; i++)
        {
            mc_mult[i] = react->mc_multiplier[i][segment];
        }

	    react->reaction(states_for_reaction, result_array, mc_mult, ecs_states_for_reaction, ecs_result);

	    /*Calculate I - Jacobian for ICS reactions*/
	    for(i = 0, idx = 0; i < react->num_species; i++)
		{
	        for(j = 0; j < react->num_regions; j++)
	        {
	            if(react->state_idx[segment][i][j] != SPECIES_ABSENT)
	            {
	                v_set_val(b, idx, dt*result_array[i][j]);
	
	                // set up the changed states array
				    states_for_reaction_dx[i][j] += dx;
	
				    /* TODO: Handle approximating the Jacobian at a function upper
				    * limit, e.g. acos(1)
	       	        */
				    react->reaction(states_for_reaction_dx, result_array_dx, mc_mult, ecs_states_for_reaction, ecs_result_dx);
	
	       	    	for (jac_i = 0, jac_idx = 0; jac_i < react->num_species; jac_i++)
					{
	                    for (jac_j = 0; jac_j < react->num_regions; jac_j++)
					    {
	                        // pd is our Jacobian approximated
	                        if(react->state_idx[segment][jac_i][jac_j] != SPECIES_ABSENT)
	                        {
	       	                    pd = (result_array_dx[jac_i][jac_j] - result_array[jac_i][jac_j])/dx;
						        m_set_val(jacobian, jac_idx, idx, (idx==jac_idx) - dt*pd);
	                            jac_idx += 1;
	       	                }
	                    }
	                }
					for (jac_i = 0; jac_i < react->num_ecs_species; jac_i++)
					{
	                    for (jac_j = 0; jac_j < react->num_regions; jac_j++)
					    {
	                        // pd is our Jacobian approximated
	                        if(react->ecs_state[segment][jac_i][jac_j] != NULL)
	                        {
	       	                    pd = (ecs_result_dx[jac_i][jac_j] - ecs_result[jac_i][jac_j])/dx;
						        m_set_val(jacobian, jac_idx, idx, -dt*pd);
	                            jac_idx += 1;
	       	                }
	                    }
					}
					// reset dx array
	            	states_for_reaction_dx[i][j] -= dx;
	                idx+=1;
	        	}
	        }
	    }
	
	    /*Calculate I - Jacobian for MultiCompartment ECS reactions*/
	    for(i = 0; i < react->num_ecs_species; i++)
		{
	        for(j =0; j < react->num_regions; j++)
	        {
	            if(react->ecs_state[segment][i][j] != NULL)
	            {
	                v_set_val(b, idx, dt*ecs_result[i][j]);
	
	                // set up the changed states array
				    ecs_states_for_reaction_dx[i][j] += dx;
	
				    /* TODO: Handle approximating the Jacobian at a function upper
				    * limit, e.g. acos(1)
	       	        */
				    react->reaction(states_for_reaction, result_array_dx, mc_mult, ecs_states_for_reaction_dx, ecs_result_dx);
	
	       	    	for (jac_i = 0, jac_idx = 0; jac_i < react->num_ecs_species; jac_i++)
					{
	                    for (jac_j = 0; jac_j < react->num_regions; jac_j++)
					    {
	                        // pd is our Jacobian approximated
	                        if(react->ecs_state[segment][jac_i][jac_j] != NULL)
	                        {
	       	                    pd = (result_array_dx[jac_i][jac_j] - result_array[jac_i][jac_j])/dx;
						        m_set_val(jacobian, jac_idx, idx, - dt*pd);
	                            jac_idx += 1;
	       	                }
	                    }
	                }
					for (jac_i = 0; jac_i < react->num_ecs_species; jac_i++)
					{
	                    for (jac_j = 0; jac_j < react->num_regions; jac_j++)
					    {
	                        // pd is our Jacobian approximated
	                        if(react->ecs_state[segment][jac_i][jac_j] != NULL)
	                        {
	       	                    pd = (ecs_result_dx[jac_i][jac_j] - ecs_result[jac_i][jac_j])/dx;
						        m_set_val(jacobian, jac_idx, idx, (idx==jac_idx) - dt*pd);
	                            jac_idx += 1;
	       	                }
	                    }
					}
					// reset dx array
	            	ecs_states_for_reaction_dx[i][j] -= dx;
	                idx+=1;
	        	}
	        }
	    }
        /*
        if(segment == 0)
        {
            fprintf(stderr,"\n============%p===============\n",react);
            for(i=0;i<react->icsN + react->ecsN;i++)
            {
                for(j=0;j<react->icsN+react->ecsN;j++)
                {
                    fprintf(stderr,"%g\t",m_get_val(jacobian,i,j));
                }
                fprintf(stderr,"\n");
            }
	        fprintf(stderr,"===========================\n");
        }
        */
	    // solve for x, destructively
	    LUfactor(jacobian, pivot);
	    LUsolve(jacobian, pivot, b, x);

	    if(ydot == NULL)    //fixed-step
	    {
	    
		    for(i = 0, jac_idx=0; i < react->num_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                idx = react->state_idx[segment][i][j];
	                if(idx != SPECIES_ABSENT)
	                {
                           /* if(segment == 0)
                            {
                                fprintf(stderr,"%i %i] states[%i] += %g\n",i,j,idx,v_get_val(x,jac_idx++));
                            }*/
                            states[idx] += v_get_val(x,jac_idx++);
	                }
	            }
	        }
	    }
	    else    //variable-step
	    {
	    	for(i = 0, jac_idx=0; i < react->num_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                idx = react->state_idx[segment][i][j];
	                if(idx != SPECIES_ABSENT)
	                {
	                        ydot[idx] += v_get_val(x,jac_idx++)/dt;
	                }
	
	            }
	        }
	    
	    }
	    //TODO: Variable step ICS<->ECS reactions
	    for(i = 0, jac_idx=0; i < react->num_ecs_species; i++)
	    {
	        for(j = 0; j < react->num_regions; j++)
	        {
	            if(react->ecs_state[segment][i][j] != NULL)
	                *(react->ecs_state[segment][i][j]) += v_get_val(x,jac_idx++);
	        }
	    }
    }

    m_free(jacobian);
    v_free(b);
    v_free(x);
    px_free(pivot);
    for(i = 0; i < react->num_species; i++)
    {
        free(states_for_reaction[i]);
        free(states_for_reaction_dx[i]);
        free(result_array[i]);
        free(result_array_dx[i]);
    }
    if(react->num_mult > 0)
        free(mc_mult);
    free(states_for_reaction_dx);
    free(states_for_reaction);
    free(result_array);
    free(result_array_dx);
    if(react->num_ecs_species > 0)
    {
        for(i = 0; i < react->num_ecs_species; i++)
        {
            free(ecs_states_for_reaction[i]);
            free(ecs_states_for_reaction_dx[i]);
            free(ecs_result[i]);
            free(ecs_result_dx[i]);
        }
	    free(ecs_states_for_reaction);
        free(ecs_states_for_reaction_dx);
	    free(ecs_result);
	    free(ecs_result_dx);
    }
}

void do_ics_reactions(const double t, const double* states, double* ydot)
{
    ICSReactions* react;
    for(react = _reactions; react != NULL; react = react->next)
    {
        solve_reaction(react, states, ydot);
    }
}

/*****************************************************************************
*
* End intracellular code
*
*****************************************************************************/




