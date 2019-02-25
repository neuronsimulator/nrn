#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <matrix2.h>
#include <pthread.h>
#include <../nrnoc/section.h>
#include <../nrnoc/nrn_ansi.h>
#include <../nrnoc/multicore.h>
#include <nrnwrap_Python.h>

extern int structure_change_cnt;
int prev_structure_change_cnt = 0;
unsigned char initialized = 0;

/*
    Globals
*/
extern NrnThread* nrn_threads;
int NUM_THREADS = 1;
pthread_t* Threads = NULL;
TaskQueue* AllTasks = NULL;


extern double *dt_ptr;
extern double *t_ptr;
extern double *h_dt_ptr;
extern double *h_t_ptr;


fptr _setup, _initialize, _setup_matrices;
extern NrnThread* nrn_threads;

/*intracellular diffusion*/
unsigned char diffusion = FALSE;
int _rxd_euler_nrow=0, _rxd_euler_nnonzero=0, _rxd_num_zvi=0;
long* _rxd_euler_nonzero_i = NULL;
long* _rxd_euler_nonzero_j = NULL;
double* _rxd_euler_nonzero_values = NULL;
long* _rxd_zero_volume_indices = NULL;
double* _rxd_a = NULL;
double* _rxd_b = NULL;
double* _rxd_c = NULL;
double* _rxd_d = NULL;
long* _rxd_p = NULL; 
unsigned int*  _rxd_zvi_child_count = NULL;
long** _rxd_zvi_child = NULL;
static int _cvode_offset;
static int _ecs_count;

/*intracellular reactions*/
ICSReactions* _reactions = NULL;

/*Indices used to set atol scale*/
SpeciesIndexList* species_indices = NULL;

/*intracellular reactions*/
double* states;
int num_states=0;
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

/*membrane fluxes*/
int _memb_curr_total = 0;   /*number of membrane currents (sum of 
                              _memb_species_count)*/
int _memb_curr_nodes = 0;   /*corresponding number of nodes
                              equal to _memb_curr_total if Extracellular is not used*/
int _memb_count = 0;        /*number of membrane currents*/
double* _rxd_flux_scale;    /*array length _memb_count to scale fluxes*/
double* _rxd_induced_currents_scale;    /*array of Extracellular current scales*/
int* _memb_net_charges;     /*array length _memb_count of charges*/
int* _cur_node_indices;      /*array length _memb_count into nodes index*/

int* _cur_indices;          /*array length _memb_curr_total into _rxd_induced_currents index*/

int* _memb_species_count;   /*array of length _memb_count
                             number of species involved in each membrane
                             current*/

/*arrays of size _memb_count by _memb_species_count*/
PyHocObject*** _memb_cur_ptrs; /*hoc pointers TODO: replace with index for _curr_ptrs*/
int** _memb_cur_charges;    
int*** _memb_cur_mapped;      /*array of pairs of indices*/
int*** _memb_cur_mapped_ecs;  /*array of pointer into ECS grids*/

double* _rxd_induced_flux = NULL;       /*set when calculating reactions*/
double* _rxd_induced_currents = NULL;
double* _rxd_induced_currents_ecs = NULL;
int* _rxd_induced_currents_grid = NULL;
int* _rxd_induced_currents_ecs_idx = NULL;

unsigned char _membrane_flux = FALSE;   /*TRUE if any membrane fluxes are in the model*/
int* _membrane_flux_lookup; /*states index -> position in _rxd_induced_flux*/
int* _membrane_scale_lookup; /*sates index -> poisition in _rxd_flux_scale*/


static void transfer_to_legacy()
{
	/*TODO: support 3D*/
	int i;
	for(i = 0; i < _conc_count; i++)
	{
		*(double*)_conc_ptrs[i]->u.px_ = states[_conc_indices[i]];
	}
}

static inline void* allocopy(void* src, size_t size)
{
    void* dst = malloc(size);
    memcpy(dst, src, size);
    return dst;
}

void rxd_set_no_diffusion()
{
    int i;
    prev_structure_change_cnt = structure_change_cnt;
    initialized = 1;
    diffusion = FALSE;
    if(_rxd_a != NULL)
    {
        free(_rxd_a);
        free(_rxd_b);
        free(_rxd_c);
        free(_rxd_d);
        free(_rxd_p);
        free(_rxd_euler_nonzero_i);
        free(_rxd_euler_nonzero_j);
        free(_rxd_euler_nonzero_values);
        _rxd_a = NULL;
    }
    /*Clear previous _rxd_zvi_child*/
    if(_rxd_zvi_child != NULL && _rxd_zvi_child_count != NULL)
    {
        for(i = 0; i < _rxd_num_zvi; i++)
            if(_rxd_zvi_child_count[i]>0) free(_rxd_zvi_child[i]);
        free(_rxd_zvi_child);
        free(_rxd_zvi_child_count);
        _rxd_zvi_child = NULL;
        _rxd_zvi_child_count = NULL;
        
    }
    if(_rxd_zvi_child_count != NULL)
    {
        free(_rxd_zvi_child_count);
        _rxd_zvi_child_count = NULL;
    }
    if(_rxd_zvi_child != NULL)
    {
        free(_rxd_zvi_child_count);
        _rxd_zvi_child_count=NULL;
    }
}

void rxd_setup_curr_ptrs(int num_currents, int* curr_index, double* curr_scale,
						  PyHocObject** curr_ptrs, int conc_count, 
						  int* conc_index, PyHocObject** conc_ptrs)
{
	/* info for NEURON currents - to update states */
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
	_curr_ptrs = malloc(sizeof(PyHocObject*)*num_currents);
	memcpy(_curr_ptrs, curr_ptrs, sizeof(PyHocObject*)*num_currents);
}

void rxd_setup_conc_ptrs(int conc_count, int* conc_index, 
                         PyHocObject** conc_ptrs)
{
	/* info for NEURON concentration - to transfer to legacy */
	_conc_count = conc_count;

	if(_conc_indices != NULL)
		free(_conc_indices);
	_conc_indices = malloc(sizeof(int)*conc_count);
	memcpy(_conc_indices, conc_index, sizeof(int)*conc_count); 
	
	if(_conc_ptrs != NULL)
		free(_conc_ptrs);
	_conc_ptrs = malloc(sizeof(PyHocObject*)*conc_count);
	memcpy(_conc_ptrs, conc_ptrs, sizeof(PyHocObject*)*conc_count);
    
}

void rxd_set_euler_matrix(int nrow, int nnonzero, long* nonzero_i,
                          long* nonzero_j, double* nonzero_values,
                          long* zero_volume_indices,
                          int num_zero_volume_indices, double* c_diagonal)
{
    long i, j, idx;
    double val;
    unsigned int k, ps;
    unsigned int* parent_count;
    /*free old data*/
    if(_rxd_a != NULL)
    {
        free(_rxd_a);
        free(_rxd_b);
        free(_rxd_c);
        free(_rxd_d);
        free(_rxd_p);
        free(_rxd_euler_nonzero_i);
        free(_rxd_euler_nonzero_j);
        free(_rxd_euler_nonzero_values);
    }
    prev_structure_change_cnt = structure_change_cnt;
    initialized = 1;
    diffusion = TRUE;
	_rxd_euler_nrow = nrow;
    _rxd_euler_nnonzero = nnonzero;
    _rxd_euler_nonzero_i = (long*)allocopy(nonzero_i, sizeof(long) * nnonzero);
    _rxd_euler_nonzero_j = (long*)allocopy(nonzero_j, sizeof(long) * nnonzero);
    _rxd_euler_nonzero_values = (double*)allocopy(nonzero_values, sizeof(double) * nnonzero);

    /*Clear previous _rxd_zvi_child*/
    if(_rxd_zvi_child != NULL && _rxd_zvi_child_count != NULL)
    {
        for(i = 0; i < _rxd_num_zvi; i++)
            if(_rxd_zvi_child_count[i]>0) free(_rxd_zvi_child[i]);
        free(_rxd_zvi_child);
        free(_rxd_zvi_child_count);
        _rxd_zvi_child_count = NULL;
        _rxd_zvi_child = NULL;
    }
    _rxd_num_zvi = num_zero_volume_indices; 
    _rxd_zero_volume_indices = zero_volume_indices;
    
    _rxd_a = (double*)calloc(nrow,sizeof(double));
    _rxd_b = (double*)calloc(nrow,sizeof(double));
    _rxd_c = (double*)calloc(nrow,sizeof(double));
    _rxd_d = (double*)calloc(nrow,sizeof(double));
    _rxd_p = (long*)malloc(nrow*sizeof(long));
    parent_count = (unsigned int*)calloc(nrow,sizeof(unsigned int));    

    for(idx = 0; idx < nrow; idx++)
        _rxd_p[idx] = -1;

    for(idx = 0; idx < nnonzero; idx++)
    {
        i = nonzero_i[idx];
        j = nonzero_j[idx];
        val = nonzero_values[idx];
        if(i < j)
        {
            _rxd_p[j] = i;
            parent_count[i]++;
            _rxd_a[j] = val;
        }
        else if(i == j)
        {
            _rxd_d[i] = val;
        }
        else
        {
            _rxd_b[i] = val;
        }
        
    }

    for(idx = 0; idx < nrow; idx++)
    {
        _rxd_c[idx] =  _rxd_d[idx] > 0 ? c_diagonal[idx] : 1.0;
    }
    
    if(_rxd_num_zvi > 0)
    {
        _rxd_zvi_child_count = (unsigned int*)malloc(_rxd_num_zvi*sizeof(unsigned int));
        _rxd_zvi_child = (long**)malloc(_rxd_num_zvi*sizeof(long*));

        /* find children of zero-volume-indices */
        for(i = 0; i < _rxd_num_zvi; i++)
        {
            ps = parent_count[_rxd_zero_volume_indices[i]];
            if(ps == 0) 
            {
                _rxd_zvi_child_count[i] = 0;
                _rxd_zvi_child[i] = NULL;

                continue;
            }
            _rxd_zvi_child[i] = (long*)malloc(ps*sizeof(long));
            _rxd_zvi_child_count[i] = ps;
            for(j = 0, k = 0; k < ps; j++)
            {
                if(_rxd_zero_volume_indices[i] == _rxd_p[j])
                {
                    _rxd_zvi_child[i][k] = j;
                    k++;
                }
            }
        }
    }
    free(parent_count);
}

static void add_currents(double * result)
{
    long k;
	/*Add currents to the result*/
    if(_membrane_flux)
    {
	    for (k = 0; k < _curr_count; k++)
        {
    	    result[_curr_indices[k]] += _curr_scales[k] * (*_curr_ptrs[k]->u.px_ - _rxd_induced_currents[k]);
        }
    }
    else
    {
        for (k =0; k < _curr_count; k++)
            result[_curr_indices[k]] += _curr_scales[k] * (*_curr_ptrs[k]->u.px_);
	}
}
static void mul(int nrow, int nnonzero, long* nonzero_i, long* nonzero_j, const double* nonzero_values, const double* v, double* result) {
    long i, j, k;
    /* now loop through all the nonzero locations */
    /* NOTE: this would be more efficient if not repeatedly doing the result[i] lookup */
    for (k = 0; k < nnonzero; k++) {
        i = *nonzero_i++;
        j = *nonzero_j++;
        result[i] -= (*nonzero_values++) * v[j];
    }
}

void set_setup(const fptr setup_fn) {
	_setup = setup_fn;
}

void set_initialize(const fptr initialize_fn) {
	_initialize = initialize_fn;
    set_num_threads(NUM_THREADS);
}

void set_setup_matrices(fptr setup_matrices) {
    _setup_matrices = setup_matrices;
}

/* nrn_tree_solve modified from nrnoc/ldifus.c */
static void nrn_tree_solve(double* a, double* b, double* c, double* dbase, double* rhs, long* pindex, long n, double dt) {
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
    long i, pin;
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
		pin = pindex[i];
		if (pin > -1) {
			double p;
			p = dt * a[i] / d[i];
			d[pin] -= dt * p * b[i];
			rhs[pin] -= p * rhs[i];
		}
	}
	/* bksub */
	for (i = 0; i < n; ++i) {
		pin = pindex[i];
		if (pin > -1) {
			rhs[i] -= dt * b[i] * rhs[pin];
		}
		rhs[i] /= d[i];
	}
	
	free(d);
}



static void ode_solve(double t, double dt, double* p1, double* p2)
{
    unsigned long i, j;
    double* b = p1 + _cvode_offset;
    double* y = p2 + _cvode_offset;
    double* full_b, *full_y;
	long* zvi = _rxd_zero_volume_indices;
   
    if(_rxd_num_zvi > 0)
    {
        full_b = (double*)calloc(sizeof(double), num_states);
        full_y = (double*)calloc(sizeof(double), num_states);
        for(i = 0, j = 0; i < num_states; i++)
        {
            if(i == zvi[j])
            {
                j++;
            }
            else
            {
                full_b[i] = b[i-j];
                full_y[i] = y[i-j];
            }
        }
    }
    else
    {
        full_b = b;
        full_y = y;
    }
	nrn_tree_solve(_rxd_a, _rxd_b, _rxd_c, _rxd_d, full_b, _rxd_p, _rxd_euler_nrow, dt);

    do_ics_reactions(full_y, full_b, y, b);
   
    if(_rxd_num_zvi > 0)
    {
        for(i = 0, j = 0; i < num_states; i++)
        {
            if(i == zvi[j])
                j++;
            else
                b[i-j] = full_b[i]; 
        }
        free(full_b);
        free(full_y);
    }
    
}

static void ode_abs_tol(double* p1)
{
    int i;
    double* y = p1 + _cvode_offset;
    if(species_indices != NULL)
    {
        SpeciesIndexList* list;
        for(list = species_indices; list->next != NULL; list = list->next)
        {
            for(i=0; i<list->length; i++)
                y[list->indices[i]] *= list->atolscale;
        }
    }
}

static void free_currents()
{
    int i, j;
    if(!_membrane_flux)
        return;
    for(i = 0; i < _memb_count; i++)
    {
        for(j = 0; j < _memb_species_count[i]; j++)
        {
            free(_memb_cur_mapped[i][j]);
        }
        free(_memb_cur_mapped[i]);
        free(_memb_cur_charges[i]);
        free(_memb_cur_ptrs[i]);
    }
    free(_memb_cur_charges);
    free(_memb_cur_ptrs);
    free(_memb_cur_mapped);
    free(_memb_species_count);
    free(_cur_indices);
    free(_cur_node_indices);
    free(_rxd_induced_flux);
    free(_rxd_induced_currents);
    free(_rxd_flux_scale);
    free(_membrane_flux_lookup);
    free(_membrane_scale_lookup);
    free(_memb_cur_mapped_ecs);
    free(_rxd_induced_currents_grid);
    free(_rxd_induced_currents_ecs);
    free(_rxd_induced_currents_ecs_idx);
    free(_rxd_induced_currents_scale);
    _membrane_flux = FALSE; 
}

void setup_currents(int num_currents, int num_fluxes, int num_nodes, int* num_species, int* net_charges, int* cur_idxs, int* node_idxs, double* scales,  int* charges, PyHocObject** ptrs, int* mapped, int* mapped_ecs)
{
    int i, j, k, id;
    Current_Triple* c;
    Grid_node* grid;
    
    free_currents();
    
    _memb_count = num_currents;
    _memb_curr_total = num_fluxes;
    _memb_curr_nodes = num_nodes;
    _memb_species_count = (int*)malloc(sizeof(int)*num_currents);
    memcpy(_memb_species_count,num_species,sizeof(int)*num_currents);

    _memb_net_charges = (int*)malloc(sizeof(int)*num_currents);
    memcpy(_memb_net_charges,net_charges,sizeof(int)*num_currents);

    _rxd_flux_scale = (double*)malloc(sizeof(double)*num_currents);
    memcpy(_rxd_flux_scale,scales,sizeof(double)*num_currents);

    /*TODO: if memory is an issue - replace with sorted list of cur_idxs*/
    _membrane_flux_lookup = (int*)malloc(sizeof(int)*num_states);
    memset(_membrane_flux_lookup, SPECIES_ABSENT, sizeof(int)*num_states);

    _membrane_scale_lookup = (int*)malloc(sizeof(int)*num_states);
     memset(_membrane_scale_lookup, SPECIES_ABSENT, sizeof(int)*num_states);
    _memb_cur_charges = (int**)malloc(sizeof(int*)*num_currents);
    _memb_cur_ptrs = (PyHocObject***)malloc(sizeof(PyHocObject**)*num_currents);     
    _memb_cur_charges = (int**)malloc(sizeof(int*)*num_currents);
    _memb_cur_mapped_ecs = (int***)malloc(sizeof(int*)*num_currents);        
    _memb_cur_mapped = (int***)malloc(sizeof(int**)*num_currents);
    _rxd_induced_currents_grid = (int*)malloc(sizeof(int)*_memb_curr_total);
    memset(_rxd_induced_currents_grid, SPECIES_ABSENT, sizeof(int)*_memb_curr_total);

    _rxd_induced_currents_ecs_idx = (int*)malloc(sizeof(int)*_memb_curr_total);

    for(i = 0, k = 0; i < num_currents; i++)
    {
        _memb_cur_charges[i] = (int*)malloc(sizeof(int)*num_species[i]);
        memcpy(_memb_cur_charges[i], &charges[k], sizeof(int)*num_species[i]);
        _memb_cur_ptrs[i] = (PyHocObject**)malloc(sizeof(PyHocObject*)*num_species[i]);
        memcpy(_memb_cur_ptrs[i], &ptrs[k], sizeof(PyHocObject*)*num_species[i]);
        _memb_cur_mapped_ecs[i] = (int**)malloc(sizeof(int*)*num_species[i]);
        _memb_cur_mapped[i] =  (int**)malloc(sizeof(int*)*num_species[i]);


        for(j = 0; j < num_species[i]; j++, k++)
        {
            _memb_cur_mapped[i][j] = (int*)malloc(2*sizeof(int));
            memcpy(_memb_cur_mapped[i][j],&mapped[2*k],2*sizeof(int));
            _memb_cur_mapped_ecs[i][j] = (int*)malloc(2*sizeof(int));
            memcpy(_memb_cur_mapped_ecs[i][j],&mapped_ecs[2*k],2*sizeof(int));
            if(_memb_cur_mapped[i][j][0] != SPECIES_ABSENT)
            {
                _membrane_scale_lookup[cur_idxs[_memb_cur_mapped[i][j][0]]] = i;
                _membrane_flux_lookup[cur_idxs[_memb_cur_mapped[i][j][0]]] = i;
            }
            if(_memb_cur_mapped[i][j][1] != SPECIES_ABSENT)
            {
                _membrane_scale_lookup[cur_idxs[_memb_cur_mapped[i][j][1]]] = i;
                _membrane_flux_lookup[cur_idxs[_memb_cur_mapped[i][j][1]]] = i;
            }
            if( _memb_cur_mapped[i][j][0] == SPECIES_ABSENT)
            {
                _rxd_induced_currents_grid[_memb_cur_mapped[i][j][1]] = _memb_cur_mapped_ecs[i][j][0];
                _rxd_induced_currents_ecs_idx[_memb_cur_mapped[i][j][1]] = _memb_cur_mapped_ecs[i][j][1];
            }
            else if(_memb_cur_mapped[i][j][1] == SPECIES_ABSENT)

            {
                _rxd_induced_currents_grid[_memb_cur_mapped[i][j][0]] = _memb_cur_mapped_ecs[i][j][0];
                _rxd_induced_currents_ecs_idx[_memb_cur_mapped[i][j][0]] = _memb_cur_mapped_ecs[i][j][1];
            }
        }
    }
    
    _rxd_induced_currents_scale = (double*)malloc(sizeof(double)*_memb_curr_total);
    /*TODO: Should be passed in from python*/
    for(i = 0; i < _memb_curr_total; i++)
    {
        for (id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, id++)
        {
            if(_rxd_induced_currents_grid[i] != id)
                continue;
            c = grid->current_list;
            for(j = 0; j < grid->num_all_currents; j++)
            {
                if(c[j].destination == _rxd_induced_currents_ecs_idx[i])
                {
                    _rxd_induced_currents_scale[i] = c[j].scale_factor/(grid->VARIABLE_ECS_VOLUME == VOLUME_FRACTION ? grid->alpha[c[i].destination] : grid->alpha[0]);
                    break;
                }
            }
        }
    }

    /*index into arrays of currents current*/
    _cur_indices = (int*)malloc(sizeof(int)*num_fluxes);  
    memcpy(_cur_indices, cur_idxs, sizeof(int)*num_fluxes);

    /*index into arrays of nodes/states*/
    _cur_node_indices = (int*)malloc(sizeof(int)*num_currents);
    memcpy(_cur_node_indices, node_idxs, sizeof(int)*num_currents);

    _membrane_flux = TRUE;
    _rxd_induced_currents = (double*)malloc(sizeof(double)*_memb_curr_total);
    _rxd_induced_flux = (double*)malloc(sizeof(double)*_memb_curr_total);
    _rxd_induced_currents_ecs = (double*)malloc(sizeof(double)*_memb_curr_total);
}

static void _currents(double* rhs)
{
    int i, j, idx, side;
    double current;
    
    if(!_membrane_flux)
        return;

    get_all_reaction_rates(states, NULL, NULL);
    
    MEM_ZERO(_rxd_induced_currents, _memb_curr_total*sizeof(double));
    MEM_ZERO(_rxd_induced_currents_ecs, _memb_curr_total*sizeof(double));

    for(i = 0; i < _memb_count; i++)
    {
        idx = _cur_node_indices[i];
        
        rhs[idx] -= _memb_net_charges[i] * _rxd_induced_flux[i]/2.0;
        for(j = 0; j < _memb_species_count[i]; j++)
        {
            current = (double)_memb_cur_charges[i][j] * _rxd_induced_flux[i]/2.0;

            *(_memb_cur_ptrs[i][j]->u.px_) += current;

            for(side = 0; side < 2; side++)
            {
                if(_memb_cur_mapped[i][j][side] == SPECIES_ABSENT)
                {
                        /*Extracellular region is within the ECS grid*/ 
                        _rxd_induced_currents_ecs[_memb_cur_mapped[i][j][(side+1)%2]] -= current;
                }
                else
                {
                        _rxd_induced_currents[_memb_cur_mapped[i][j][side]] += current;
                }

            }
        }
    }
}

int rxd_nonvint_block(int method, int size, double* p1, double* p2, int thread_id) {
        if(initialized && structure_change_cnt != prev_structure_change_cnt)
        {
            /*TODO: Exclude irrelevant (non-rxd) structural changes*/
            _setup_matrices(); 
        }
        switch (method) {
        case 0:
            _setup();
            break;
        case 1:
            _initialize();
            break;
        case 2:
            /* compute outward current to be subtracted from rhs */
            _currents(p1);
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
            _cvode_offset = size;
            _ecs_count = ode_count(size + num_states - _rxd_num_zvi);
            return _ecs_count + num_states - _rxd_num_zvi;
        case 6:
            /* ode_reinit(y) */
            _ode_reinit(p1); //Invalid read of size 8 
            _ecs_ode_reinit(p1);
            break;
        case 7:
            /* ode_fun(t, y, ydot); from t and y determine ydot */
            _rhs_variable_step(*t_ptr, p1, p2);
            _rhs_variable_step_ecs(*t_ptr, p1, p2);
            break;
        case 8:
            ode_solve(*t_ptr, *dt_ptr, p1, p2); /*solve mx=b replace b with x */
            /* TODO: we can probably reuse the dgadi code here... for now, we do nothing, which implicitly approximates the Jacobian as the identity matrix */
            break;
        case 9:
            //ode_jacobian(*dt_ptr, p1, p2); /* optionally prepare jacobian for fast ode_solve */
            break;
        case 10:
            ode_abs_tol(p1);
            ecs_atolscale(p1);
            /* ode_abs_tol(y_abs_tolerance); fill with cvode.atol() * scalefactor */
            break;
        default:
            printf("Unknown rxd_nonvint_block call: %d\n", method);
            break;
    }
    /*
    int i;
    for(i=0; i<size; i++)
    {
        fprintf(stderr,"%i] \t %1.12e \t %1.12e \n", i, (p1==NULL?-1e9:p1[i]), (p2==NULL?-1e9:p2[i]));
    }*/
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
    int i,j,k,idx, ecs_id, ecs_index, ecs_offset;
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
        react->ecs_index = (int***)malloc(nseg*sizeof(int**));
        for(i = 0; i < nseg; i++)
        {
            react->ecs_state[i] = (double***)malloc(necs*sizeof(double**));
            react->ecs_index[i] = (int**)malloc(necs*sizeof(int*));
            for(j = 0; j < necs; j++)
            {
                react->ecs_state[i][j] = (double**)calloc(nregions,sizeof(double*));
                react->ecs_index[i][j] = (int*)calloc(nregions,sizeof(int));

            }
        }
        for(j = 0; j < necs; j++)
        {
            ecs_offset = num_states - _rxd_num_zvi;
            for(ecs_id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, ecs_id++)
		    {
	            if (ecs_id == ecs_ids[j])
		    	{
                    for(i = 0; i < nseg; i++)
                    {
                        for(k = 0; k < nregions; k++)
                        {
                            //react->ecs_state[i][j][k] = (double*)malloc(sizeof(double));
                            //nseg x nregion x nspecies
                            ecs_index = ecsidx[i*necs*nregions + j*nregions + k];
                            
                            if(ecs_index >= 0)
                            {
                                react->ecs_state[i][j][k] = &(grid->states[ecs_index]);
                                react->ecs_index[i][j][k] = ecs_offset + ecs_index;
                                if(i==0) react->ecsN++;
                            }
                        }
                    }
                }
                ecs_offset += grid->size_x*grid->size_y*grid->size_z;
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


void species_atolscale(int id, double scale, int len, int* idx)
{
    SpeciesIndexList* list;
    if(species_indices != NULL)
    {
        for(list = species_indices; list->next != NULL; list = list->next)
        {
            if(list->id == id)
            {
                list->atolscale = scale;
                return;
            }
        }
        list->next = (SpeciesIndexList*)malloc(sizeof(SpeciesIndexList));
        list = list->next;
    }
    else
    {
        species_indices = (SpeciesIndexList*)malloc(sizeof(SpeciesIndexList));
        list = species_indices;
    }
    list->id = id;
    list->indices = (int*)malloc(sizeof(int)*len);
    memcpy(list->indices,idx,sizeof(int)*len);
    list->length = len;
    list->atolscale = scale;
    list->next = NULL;
}

static void free_SpeciesIndexList()
{
    SpeciesIndexList* list;
    while(species_indices != NULL)
    {
        list = species_indices;
        free(list->indices);
        species_indices = list->next;
        free(list);
    }
}

void setup_solver(double* my_states, int my_num_states, long* zvi, int num_zvi, PyHocObject* h_t_ref, PyHocObject* h_dt_ref) {
    int i;
    states = my_states;
    num_states = my_num_states;
    _rxd_num_zvi = num_zvi;
    _rxd_zero_volume_indices = zvi;
    dt_ptr = &nrn_threads->_dt;
    t_ptr = &nrn_threads->_t;
    h_t_ptr = h_t_ref->u.px_;
    h_dt_ptr = h_dt_ref->u.px_;
    set_num_threads(NUM_THREADS);
}

void start_threads(const int n)
{
    int i;
    if(Threads == NULL)
    {
        AllTasks = (TaskQueue*)calloc(1,sizeof(TaskQueue));
        Threads = (pthread_t*)malloc(sizeof(pthread_t)*(n-1));
        AllTasks->task_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        AllTasks->waiting_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        AllTasks->task_cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
        AllTasks->waiting_cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
        pthread_mutex_init(AllTasks->task_mutex, NULL);
        pthread_cond_init(AllTasks->task_cond, NULL);
        pthread_mutex_init(AllTasks->waiting_mutex, NULL);
        pthread_cond_init(AllTasks->waiting_cond, NULL);
        AllTasks->length = 0;
        for(i = 0; i < n-1; i++)
            pthread_create(&Threads[i], NULL, TaskQueue_exe_tasks, AllTasks);
    }

}

void TaskQueue_add_task(TaskQueue* q, void* (*task)(void*), void* args, void* result)
{
    TaskList *t, *list;
    t = (TaskList*)malloc(sizeof(TaskList));
    t->task = task;
    t->args = args;
    t->result = result;
    t->next = NULL;
    
    //Add task to the queue
    pthread_mutex_lock(q->task_mutex);
    if(q->first == NULL)  //empty queue
    {
        q->first=t;
        q->last=t;
    }
    else                    //non-empty
    {
        q->last->next = t;
        q->last = t;
    }

    pthread_mutex_lock(q->waiting_mutex);
    q->length++;
    pthread_mutex_unlock(q->waiting_mutex);
    pthread_mutex_unlock(q->task_mutex);
    
    //signal waiting threads
    pthread_cond_signal(q->task_cond);
 
}

void* TaskQueue_exe_tasks(void* dat)
{
    TaskList* job;
    TaskQueue* q = (TaskQueue*)dat;
    /*int id;
    for(id=0;id<NUM_THREADS;id++)
    {
        if (pthread_equal(Threads[id],pthread_self()))
        {
            break;
        }
    }
    fprintf(stderr,"%i] ready\n",id);
    */
    while(1)    //loop until thread is killed
    {
        pthread_mutex_lock(q->task_mutex);
        while(q->first == NULL) //no tasks
        {
            //Wait for new tasks
            pthread_cond_wait(q->task_cond, q->task_mutex);
        }
        //fprintf(stderr,"%i] running\n",id); 
        job = q->first;
        q->first = job->next;
        pthread_mutex_unlock(q->task_mutex); 
    
        //execute
        job->result = job->task(job->args);
        free(job);

        //fprintf(stderr,"%i] updating\n",id); 
        pthread_mutex_lock(q->waiting_mutex);
        if(--(q->length) == 0)  //all finished
        {
            pthread_cond_broadcast(q->waiting_cond);
        }
        pthread_mutex_unlock(q->waiting_mutex);
        //fprintf(stderr,"%i] done\n",id);
    } 
    
    return NULL;
}


void set_num_threads(const int n)
{
    int k, old_num = NUM_THREADS;
    if(Threads == NULL)
    {
        start_threads(n);
    }
    else
    {
        if(n<old_num)
        {
            //Kill some threads
            for(k=old_num-1; k>=n; k--)
            {
                TaskQueue_sync(AllTasks);
                pthread_cancel(Threads[k]);
            } 
            Threads = realloc(Threads,sizeof(pthread_t) * n);
            assert(Threads);
        }
        else if(n>old_num)
        {
            //Create some threads
            Threads = realloc(Threads,sizeof(pthread_t) * n);
            assert(Threads);
            
            for (k = old_num-1; k < n; k++) 
            {
                pthread_create(&Threads[k], NULL, TaskQueue_exe_tasks, AllTasks);
            }
        }
    }
    set_num_threads_ecs(n);
    NUM_THREADS = n;

}

void TaskQueue_sync(TaskQueue *q)
{
    //Wait till the queue is empty
    pthread_mutex_lock(q->waiting_mutex);
    while(q->length > 0)
        pthread_cond_wait(q->waiting_cond,q->waiting_mutex);
    pthread_mutex_unlock(q->waiting_mutex);
}

int get_num_threads(void) {
    return NUM_THREADS;
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
	double *rhs; 
	long* zvi = _rxd_zero_volume_indices;
    
    rhs = calloc(num_states,sizeof(double));
    /*diffusion*/
    if(diffusion)
	    mul(_rxd_euler_nrow, _rxd_euler_nnonzero, _rxd_euler_nonzero_i, _rxd_euler_nonzero_j, _rxd_euler_nonzero_values, states, rhs);

    add_currents(rhs);

    /* multiply rhs vector by dt */
    for (i = 0; i < num_states; i++) {
        rhs[i] *= dt;
    }

    if(diffusion)
	    nrn_tree_solve(_rxd_a, _rxd_b, _rxd_c, _rxd_d, rhs, _rxd_p, _rxd_euler_nrow, dt);
   
    /* increment states by rhs which is now really deltas */
    for (i = 0; i < num_states; i++) {
        states[i] += rhs[i];
    }

    /* clear zero volume indices (conservation nodes) */
    for (i = 0; i < _rxd_num_zvi; i++) {
        states[zvi[i]] = 0;
    }
    
    free(rhs);

    /*reactions*/
    do_ics_reactions(states, NULL, NULL, NULL);	

	transfer_to_legacy();
}


void _ode_reinit(double* y)
{
    long i, j;
    long* zvi = _rxd_zero_volume_indices;
    y += _cvode_offset;
    /*Copy states to CVode*/ 
    if(_rxd_num_zvi > 0)
    {
        for(i=0, j=0; i < num_states; i++)
        {
            if(zvi[j] == i)
                j++;
            else
                y[i-j] = states[i];
        }
    }
    else
    {
        memcpy(y,states,sizeof(double)*num_states);
    }
}


void _rhs_variable_step(const double t, const double* p1, double* p2) 
{
	long i, j, p, c;
    unsigned int k;
    double dt = *dt_ptr;
    const unsigned char calculate_rhs = p2 == NULL ? 0 : 1;
    const double* my_states = p1 + _cvode_offset;
    double* ydot = p2 + _cvode_offset;
    double st;
	/*variables for diffusion*/
	double *rhs;
	long* zvi = _rxd_zero_volume_indices;

    /*Copy states from CVode*/ 
    if(_rxd_num_zvi > 0)
    {
        for(i=0, j=0; i < num_states; i++)
        {
            if(zvi[j] == i)
                j++;
            else
                states[i] = my_states[i-j];
        }
    }
    else
    {
        memcpy(states,my_states,sizeof(double)*num_states); //Invalid read & write of size 8
    }

    if(diffusion)
    {
        for(i = 0; i < _rxd_num_zvi; i++)
        {
            j = zvi[i];
            p = _rxd_p[j];
            states[j] = (p>0?-(_rxd_b[j]/_rxd_d[j])*states[p]:0);
            for(k = 0; k < _rxd_zvi_child_count[i]; k++)
            {
                c = _rxd_zvi_child[i][k];
                states[j] -= (_rxd_a[c]/_rxd_d[j])*states[c]; 
            }
        }
    }

    transfer_to_legacy();

    if(!calculate_rhs)
    {
        for(i = 0; i < _rxd_num_zvi; i++)
            states[zvi[i]] = 0;
        return;
    }

    /*diffusion*/
    rhs = (double*)calloc(num_states,sizeof(double));
	
    if(diffusion)
        mul(_rxd_euler_nrow, _rxd_euler_nnonzero, _rxd_euler_nonzero_i, _rxd_euler_nonzero_j, _rxd_euler_nonzero_values, states, rhs);

    /*reactions*/
    MEM_ZERO(&ydot[num_states - _rxd_num_zvi], sizeof(double)*_ecs_count);
    get_all_reaction_rates(states, rhs, ydot);

    /*Add currents to the result*/
    add_currents(rhs);
    
    /* increment states by rhs which is now really deltas */
    if(_rxd_num_zvi > 0)
    {
        for (i = 0, j = 0; i < num_states; i++)
        {
            if(zvi[j] == i)
            {
                states[i] = 0;
                j++;
            }
            else
            {
                ydot[i-j] = rhs[i];
            }
        }
    }
    else
    {
        memcpy(ydot,rhs,sizeof(double)*num_states); //Invalid write of size 8
    }
    free(rhs);
}

void get_reaction_rates(ICSReactions* react, double* states, double* rates, double* ydot)
{
    int segment;
    int i, j, idx, jac_i, jac_j, jac_idx;
    int N = react->icsN + react->ecsN;  /*size of Jacobian (number species*regions for a segments)*/
    double pd;
    double dt = *dt_ptr;
    
    double** states_for_reaction = (double**)malloc(react->num_species*sizeof(double*));
    double** result_array = (double**)malloc(react->num_species*sizeof(double*));
    double* mc_mult;
    double** flux = NULL;
    if(react->num_mult > 0)
        mc_mult = (double*)malloc(react->num_mult*sizeof(double));

    double** ecs_states_for_reaction;
    double** ecs_result;
    if(react->num_ecs_species>0)
    {
        ecs_states_for_reaction = (double**)malloc(react->num_ecs_species*sizeof(double*));
        ecs_result = (double**)malloc(react->num_ecs_species*sizeof(double*));
        for(i = 0; i < react->num_ecs_species; i++)
	    {
	        ecs_states_for_reaction[i] = (double*)calloc(react->num_regions,sizeof(double));
	        ecs_result[i] = (double*)malloc(react->num_regions*sizeof(double));
        }

    }
    
    if(_membrane_flux)
    {
        MEM_ZERO(_rxd_induced_flux,sizeof(double)*_memb_curr_total);
        flux = (double**)malloc(react->icsN*sizeof(double*));
        for(i = 0; i < react->icsN; i++)
            flux[i] = (double*)malloc(react->num_regions*sizeof(double));
    }
    
    for(i = 0; i < react->num_species; i++)
    {
        states_for_reaction[i] = (double*)calloc(react->num_regions,sizeof(double));
        result_array[i] = (double*)malloc(react->num_regions*sizeof(double));
        
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
        	    }
                else
                {
                    states_for_reaction[i][j] = -1.0;
                }
	        }
            MEM_ZERO(result_array[i],react->num_regions*sizeof(double));
	    }

	    for(i = 0; i < react->num_ecs_species; i++)
	    {
	        for(j = 0; j < react->num_regions; j++)
	        {
	            if(react->ecs_state[segment][i][j] != NULL)
	            {
	            	ecs_states_for_reaction[i][j] = *(react->ecs_state[segment][i][j]);
	        	}
	        }
            MEM_ZERO(ecs_result[i],react->num_regions*sizeof(double));
	    }
        for(i = 0; i < react->num_mult; i++)
        {
            mc_mult[i] = react->mc_multiplier[i][segment];
        }

	    react->reaction(states_for_reaction, result_array, mc_mult, ecs_states_for_reaction, ecs_result, flux);
        
        for(i = 0; i < react->num_species; i++)
        {
            for(j = 0; j < react->num_regions; j++)
            {
                idx = react->state_idx[segment][i][j];
                if(idx != SPECIES_ABSENT)
                {
                    if(_membrane_flux && _membrane_flux_lookup[idx] != SPECIES_ABSENT)
                    {
                        _rxd_induced_flux[_membrane_flux_lookup[idx]] -= _rxd_flux_scale[_membrane_scale_lookup[idx]] * flux[i][j];
                    }
                    if(rates != NULL)
                    {
                        rates[idx] += result_array[i][j];
                    }
	            }
	        }
	    }
        if(ydot != NULL)
        {
            for(i = 0;  i < react->num_ecs_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                if(react->ecs_state[segment][i][j] != NULL)
	                    ydot[react->ecs_index[segment][i][j]] += ecs_result[i][j];
	            }
	        }
        }
    }
    /* free allocated memory */
    if(react->num_mult > 0)
        free(mc_mult);
    if(_membrane_flux)
    {
        for(i = 0; i < react->icsN; i++)
            free(flux[i]);
        free(flux);
    }
    if(react->num_ecs_species>0)
    {
        for(i = 0; i < react->num_ecs_species; i++)
        {
            free(ecs_states_for_reaction[i]); 
            free(ecs_result[i]);
        }
        free(ecs_states_for_reaction);
        free(ecs_result);
    }
    for(i = 0; i < react->num_species; i++)
    {
        free(states_for_reaction[i]);
        free(result_array[i]);
    }
    free(states_for_reaction);
    free(result_array);

}

void solve_reaction(ICSReactions* react, double* states, double *bval, double* cvode_states, double* cvode_b)
{
    int segment;
    int i, j, idx, jac_i, jac_j, jac_idx;
    int N = react->icsN + react->ecsN;  /*size of Jacobian (number species*regions for a segments)*/
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
            MEM_ZERO(result_array[i],react->num_regions*sizeof(double));
            MEM_ZERO(result_array_dx[i],react->num_regions*sizeof(double));
	    }

	    for(i = 0; i < react->num_ecs_species; i++)
	    {
	        for(j = 0; j < react->num_regions; j++)
	        {
	            if(react->ecs_state[segment][i][j] != NULL)
	            {
	            	if(cvode_states != NULL)
                        ecs_states_for_reaction[i][j] = cvode_states[react->ecs_index[segment][i][j]];
                    else
                        ecs_states_for_reaction[i][j] = *(react->ecs_state[segment][i][j]);
	            	ecs_states_for_reaction_dx[i][j] = ecs_states_for_reaction[i][j];
	        	}
	        }
            MEM_ZERO(ecs_result[i],react->num_regions*sizeof(double));
            MEM_ZERO(ecs_result_dx[i],react->num_regions*sizeof(double));

	    }
        for(i = 0; i < react->num_mult; i++)
        {
            mc_mult[i] = react->mc_multiplier[i][segment];
        }

	    react->reaction(states_for_reaction, result_array, mc_mult, ecs_states_for_reaction, ecs_result, NULL);

	    /*Calculate I - Jacobian for ICS reactions*/
        for(i = 0, idx = 0; i < react->num_species; i++)
		{
	        for(j = 0; j < react->num_regions; j++)
	        {
	            if(react->state_idx[segment][i][j] != SPECIES_ABSENT)
	            {
                    if(bval == NULL)
    	                v_set_val(b, idx, dt*result_array[i][j]);
                    else
                        v_set_val(b, idx, bval[react->state_idx[segment][i][j]]);

	
	                // set up the changed states array
				    states_for_reaction_dx[i][j] += dx;
	
				    /* TODO: Handle approximating the Jacobian at a function upper
				    * limit, e.g. acos(1)
	       	        */
				    react->reaction(states_for_reaction_dx, result_array_dx, mc_mult, ecs_states_for_reaction, ecs_result_dx, NULL);
	
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
                            result_array_dx[jac_i][jac_j] = 0;
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
                            ecs_result_dx[jac_i][jac_j] = 0;
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
                    if(bval == NULL)
    	                v_set_val(b, idx, dt*ecs_result[i][j]);
                    else
                        v_set_val(b, idx, cvode_b[react->ecs_index[segment][i][j]]);

	                
	                // set up the changed states array
				    ecs_states_for_reaction_dx[i][j] += dx;
	
				    /* TODO: Handle approximating the Jacobian at a function upper
				    * limit, e.g. acos(1)
	       	        */
				    react->reaction(states_for_reaction, result_array_dx, mc_mult, ecs_states_for_reaction_dx, ecs_result_dx, NULL);
	
	       	    	for (jac_i = 0, jac_idx = 0; jac_i < react->num_species; jac_i++)
					{
	                    for (jac_j = 0; jac_j < react->num_regions; jac_j++)
					    {
	                        // pd is our Jacobian approximated
	                        if(react->state_idx[segment][jac_i][jac_j] != SPECIES_ABSENT)
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
        // solve for x, destructively
        tracecatch(LUfactor(jacobian, pivot);
	        LUsolve(jacobian, pivot, b, x);,
            "solve_reaction");  //Conditional jump or move depends on uninitialised value(s)

        if(bval != NULL) //variable-step
        {
		    for(i = 0, jac_idx=0; i < react->num_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                idx = react->state_idx[segment][i][j];
	                if(idx != SPECIES_ABSENT)
	                {
                        bval[idx] = v_get_val(x, jac_idx++);
	                }
	            }
	        }
            for(i = 0; i < react->num_ecs_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                if(react->ecs_state[segment][i][j] != NULL)
                        cvode_b[react->ecs_index[segment][i][j]] = v_get_val(x, jac_idx++);
	            }
	        }
        }
	    else  //fixed-step
	    {
		    for(i = 0, jac_idx=0; i < react->num_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                idx = react->state_idx[segment][i][j];
	                if(idx != SPECIES_ABSENT)
	                {
                        states[idx] += v_get_val(x,jac_idx++);
	                }
	            }
	        }
            for(i = 0; i < react->num_ecs_species; i++)
	        {
	            for(j = 0; j < react->num_regions; j++)
	            {
	                if(react->ecs_state[segment][i][j] != NULL)
	                    *(react->ecs_state[segment][i][j]) += v_get_val(x,jac_idx++);
	            }
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

void do_ics_reactions(double* states, double* b, double* cvode_states, double* cvode_b)
{
    int i;
    ICSReactions* react;
    for(react = _reactions; react != NULL; react = react->next)
    {
        if(react->icsN + react->ecsN > 0)
            solve_reaction(react, states, b, cvode_states, cvode_b);
    }
}

void get_all_reaction_rates(double* states, double* rates, double* ydot)
{
    ICSReactions* react;
    for(react = _reactions; react != NULL; react = react->next)
    {
        if(react->icsN + react->ecsN > 0)
            get_reaction_rates(react, states, rates, ydot);
    }
}

/*****************************************************************************
*
* End intracellular code
*
*****************************************************************************/




