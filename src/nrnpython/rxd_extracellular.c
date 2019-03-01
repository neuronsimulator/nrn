#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <matrix2.h>
#include <pthread.h>
#include <nrnwrap_Python.h>

#define loc(x,y,z)((z) + (y) *  grid->size_z + (x) *  grid->size_z *  grid->size_y)

/*
    Globals
*/

/*Store the onset/offset for each threads reaction tasks*/
ReactGridData* threaded_reactions_tasks;

extern int NUM_THREADS;
extern pthread_t* Threads;
extern TaskQueue* AllTasks;
extern double* t_ptr;
extern double* states;

Reaction* ecs_reactions = NULL;

/*Current from multicompartment reations*/
extern unsigned char _membrane_flux;
extern int _memb_curr_total;
extern int* _rxd_induced_currents_grid;
extern int* _rxd_induced_currents_ecs_idx;
extern double* _rxd_induced_currents_ecs;
extern double* _rxd_induced_currents_scale;


static int states_cvode_offset;

/*Update the global array of reaction tasks when the number of reactions 
 *or threads change.
 *n - the old number of threads - use to free the old threaded_reactions_tasks*/
static void ecs_refresh_reactions(const int n)
{
	int k;
	if(threaded_reactions_tasks != NULL)
	{
		for(k = 0; k<n; k++)
		{
			SAFE_FREE(threaded_reactions_tasks[k].onset);
			SAFE_FREE(threaded_reactions_tasks[k].offset);
		}
        SAFE_FREE(threaded_reactions_tasks);

	}
	threaded_reactions_tasks = create_threaded_reactions(n);
}


void set_num_threads_ecs(const int n)
{
    int i;
    Grid_node *grid;
    if(n != NUM_THREADS)
    {
        for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
        {
            if(grid->tasks != NULL)
            {
                for(i = 0; i<NUM_THREADS; i++)
                {
                    free(grid->tasks[i].scratchpad);
                }
            }
            free(grid->tasks);
            grid->tasks = (AdiGridData*)malloc(NUM_THREADS*sizeof(AdiGridData));
            for(i=0; i<n; i++)
            {
                grid->tasks[i].scratchpad = (double*)malloc(sizeof(double) * MAX(grid->size_x,MAX(grid->size_y,grid->size_z)));
                grid->tasks[i].g = grid;

            }
        }
    }
    ecs_refresh_reactions(n);
}


/*Removal all reactions*/
void clear_rates_ecs(void)
{
	Reaction *r, *tmp;
	for (r = ecs_reactions; r != NULL; r = tmp)
	{
		SAFE_FREE(r->species_states);
		if(r->subregion)
		{
			SAFE_FREE(r->subregion);
		}
	
		tmp = r->next;
		SAFE_FREE(r);
	}
	ecs_reactions = NULL;
	
	ecs_refresh_reactions(NUM_THREADS);
}

/*Create a reaction
 * list_idx - the index for the linked list in the global array Parallel_grids
 * grid_id - the grid id within the linked list
 * ECSReactionRate - the reaction function
 * subregion - either NULL or a boolean array the same size as the grid
 * 	indicating if reaction occurs at a specific voxel
 */
Reaction* ecs_create_reaction(int list_idx, int num_species, int* species_ids, ECSReactionRate f, unsigned char* subregion)
{
	Grid_node *grid;
	Reaction* r;
	int i, j;
	
	r = (Reaction*)malloc(sizeof(Reaction));
	assert(r);
	r->reaction = f;
	/*place reaction on the top of the stack of reactions*/
	r->next = ecs_reactions;
	ecs_reactions = r;
	
	for(grid = Parallel_grids[list_idx], i = 0; grid != NULL; grid = grid -> next, i++)
	{
		/* Assume all species have the same grid */ 
		if(i==species_ids[0])
		{
			if(subregion == NULL)
			{
				r->subregion = NULL;
				r->region_size = grid->size_x * grid->size_y * grid->size_z;
			}
			else
			{
				for(r->region_size=0, j=0; j < grid->size_x * grid->size_y * grid->size_z; j++)
					r->region_size += subregion[j];
				r->subregion = subregion;
			}	
		}	
	}

	r->num_species_involved=num_species;
	r->species_states = (double**)malloc(sizeof(Grid_node*)*(r->num_species_involved));
	assert(r->species_states);

	for(i = 0; i < num_species; i++)
	{
		/*Assumes grids are the same size (no interpolation) 
		 *Assumes all the species will be in the same Parallel_grids list*/
		for(grid = Parallel_grids[list_idx], j = 0; grid != NULL; grid = grid -> next, j++)
		{
			if(j==species_ids[i])
				r->species_states[i] = grid->states;
		}
	}
	
	return r;
}

/*register_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * ECSReactionRate - the reaction function
 */
void ecs_register_reaction(int list_idx, int num_species, int* species_id, ECSReactionRate f)
{
	ecs_create_reaction(list_idx, num_species, species_id, f, NULL);
	ecs_refresh_reactions(NUM_THREADS);
}


/*register_subregion_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * my_subregion - a boolean array indicating the voxels where a reaction occurs
 * ECSReactionRate - the reaction function
 */
void ecs_register_subregion_reaction_ecs(int list_idx, int num_species, int* species_id, unsigned char* my_subregion, ECSReactionRate f)
{
	ecs_create_reaction(list_idx, num_species, species_id, f,  my_subregion);
	ecs_refresh_reactions(NUM_THREADS);
}


/*create_threaded_reactions is used to generate evenly distribution of reactions
 * across NUM_THREADS
 * returns ReactGridData* which can be used as the global
 * 	threaded_reactions_tasks 
 */
ReactGridData* create_threaded_reactions(const int n)
{
	int i,k,load,react_count = 0,tasks_per_thread;
	Grid_node* grid;
	Reaction* react;

	for(react = ecs_reactions; react != NULL; react = react -> next)
		react_count += react->region_size;

	if(react_count == 0)
		return NULL;

	/*Divide the number of reactions between the threads*/
	tasks_per_thread = (react_count + NUM_THREADS - 1) / n;
	ReactGridData* tasks = (ReactGridData*)calloc(sizeof(ReactGridData), n);
	
	tasks[0].onset = (ReactSet*)malloc(sizeof(ReactSet));
	tasks[0].onset->reaction = ecs_reactions;
	tasks[0].onset->idx = 0;

	for(k = 0, load = 0, react = ecs_reactions; react != NULL; react = react -> next)
	{
		for(i = 0; i < react->region_size; i++)
		{
			if(!react->subregion || react->subregion[i])
				load++;
			if(load >= tasks_per_thread)
			{
				tasks[k].offset = (ReactSet*)malloc(sizeof(ReactSet));
				tasks[k].offset->reaction = react;
				tasks[k].offset->idx = i;

				if(++k < NUM_THREADS)
				{
					tasks[k].onset = (ReactSet*)malloc(sizeof(ReactSet));
					tasks[k].onset->reaction = react;
					tasks[k].onset->idx = i + 1;
					load = 0;
				}
			}
            if(k == NUM_THREADS - 1 && react -> next == NULL)
            {
                tasks[k].offset = (ReactSet*)malloc(sizeof(ReactSet));
				tasks[k].offset->reaction = react;
				tasks[k].offset->idx = i-1;
            }
		}
	}
	return tasks;
}

/*ecs_do_reactions takes ReactGridData which defines the set of reactions to be
 * performed. It calculate the reaction based on grid->old_states and updates
 * grid->states
 */
void* ecs_do_reactions(void* dataptr)
{
	ReactGridData task = *(ReactGridData*)dataptr;
	unsigned char started = FALSE, stop = FALSE;
	int i, j, k, start_idx, stop_idx;
	double dt = *dt_ptr;
	Reaction* react;

	double* states_cache = NULL;
    double* states_cache_dx = NULL;
	double* results_array = NULL;
	double* results_array_dx = NULL;
    double dx = FLT_EPSILON;
	double pd;
	MAT *jacobian;
    VEC *x;
    VEC *b;
    PERM *pivot;

	for(react = ecs_reactions; react != NULL; react = react -> next)
	{
		/*find start location for this thread*/
		if(started || react == task.onset->reaction)
		{
			if(!started)
			{
				started = TRUE;
				start_idx = task.onset->idx;
			}
			else
			{
				start_idx = 0;
			}

			if(task.offset->reaction == react)
			{
				stop_idx = task.offset->idx;
				stop = TRUE;
			}
			else
			{
				stop_idx = react->region_size-1;
				stop = FALSE;
			}
			if(react->num_species_involved == 0)
                return NULL;
            /*allocate data structures*/
			jacobian = m_get(react->num_species_involved,react->num_species_involved);
        	b = v_get(react->num_species_involved);
        	x = v_get(react->num_species_involved);
			pivot = px_get(jacobian->m);
			states_cache = (double*)malloc(sizeof(double)*react->num_species_involved);
			states_cache_dx = (double*)malloc(sizeof(double)*react->num_species_involved);
			results_array = (double*)malloc(react->num_species_involved*sizeof(double));
			results_array_dx = (double*)malloc(react->num_species_involved*sizeof(double));

			for(i = start_idx; i <= stop_idx; i++)
			{
				if(!react->subregion || react->subregion[i])
				{
					//TODO: this assumes grids are the same size/shape 
	 				//      add interpolation in case they aren't      
					for(j = 0; j < react->num_species_involved; j++)
					{
						states_cache[j] = react->species_states[j][i];
						states_cache_dx[j] = react->species_states[j][i];
					}
                    MEM_ZERO(results_array,react->num_species_involved*sizeof(double));
					react->reaction(states_cache, results_array);

					for(j = 0; j < react->num_species_involved; j++)
					{
						states_cache_dx[j] += dx;
                        MEM_ZERO(results_array_dx,react->num_species_involved*sizeof(double));
						react->reaction(states_cache_dx, results_array_dx);
						v_set_val(b, j, dt*results_array[j]);

						for(k = 0; k < react->num_species_involved; k++)
						{
							pd = (results_array_dx[k] - results_array[k])/dx;
							m_set_val(jacobian, k, j, (j==k) - dt*pd);
						}
						states_cache_dx[j] -= dx;
					}
					// solve for x, destructively
        			LUfactor(jacobian, pivot); //Conditional jump or move depends on uninitialised value(s)
        			LUsolve(jacobian, pivot, b, x);
					for(j = 0; j < react->num_species_involved; j++)
					{
						react->species_states[j][i] += v_get_val(x,j);
					}
				}
			}
			m_free(jacobian);
        	v_free(b);
        	v_free(x);
        	px_free(pivot);

			SAFE_FREE(states_cache);
			SAFE_FREE(states_cache_dx);
			SAFE_FREE(results_array);
			SAFE_FREE(results_array_dx);

			if(stop)
				return NULL;
		}
	}
	return NULL;
}

/* run_threaded_reactions
 * Array ReactGridData tasks length NUM_THREADS and calls ecs_do_reactions and
 * executes each task with a separate thread
 */
static void run_threaded_reactions(ReactGridData* tasks)
{
	int k;
	 /* launch threads */
    for (k = 0; k < NUM_THREADS-1; k++) 
    {
       TaskQueue_add_task(AllTasks,&ecs_do_reactions, &tasks[k], NULL);
    }
    /* run one task in the main thread */
    ecs_do_reactions(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}

static void* gather_currents(void* dataptr)
{
    CurrentData* d =  (CurrentData*)dataptr;
    Grid_node *grid = d->g;
    double *val = d->val;
    int i, start = d->onset, stop = d->offset;
    Current_Triple* c = grid->current_list;
    if(grid->VARIABLE_ECS_VOLUME == VOLUME_FRACTION) 
    {
        for(i = start; i < stop; i++)
           val[i] = c[i].scale_factor * (*c[i].source)/grid->alpha[c[i].destination];
    }
    else
    {
        for(i = start; i < stop; i++)
            val[i] = c[i].scale_factor * (*c[i].source)/grid->alpha[0];
    }
    return NULL;
}

/*
 * do_current - process the current for a given grid
 * Grid_node* grid - the grid used
 * double* output - for fixed step this is the states for variable ydot
 * double dt - for fixed step the step size, for variable step 1
 */
void do_currents(Grid_node* grid, double* output, double dt, int grid_id)
{
    ssize_t m, n, i;
    Current_Triple* c;
    /*Currents to broadcast via MPI*/
    /*TODO: Handle multiple grids with one pass*/
    /*Maybe TODO: Should check #currents << #voxels and not the other way round*/
    double* val;
    double* all_currents;
    int proc_offset;
    //MEM_ZERO(output,sizeof(double)*grid->size_x*grid->size_y*grid->size_z);
    /* currents, via explicit Euler */
    n = grid->num_all_currents;
    m = grid->num_currents;
    c = grid->current_list;
    CurrentData* tasks = (CurrentData*)malloc(NUM_THREADS*sizeof(CurrentData));
#if NRNMPI    
    val = grid->all_currents + (nrnmpi_use?grid->proc_offsets[nrnmpi_myid]:0);
#else
    val = grid->all_currents;
#endif
    int tasks_per_thread = (m + NUM_THREADS - 1)/NUM_THREADS;

    for(i = 0; i < NUM_THREADS; i++)
    {
        tasks[i].g = grid;
        tasks[i].onset = i * tasks_per_thread;
        tasks[i].offset = MIN((i+1)*tasks_per_thread,m);
        tasks[i].val = val;
    }
    for (i = 0; i < NUM_THREADS-1; i++) 
    {
        TaskQueue_add_task(AllTasks, &gather_currents, &tasks[i], NULL);
    }
    /* run one task in the main thread */
    gather_currents(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
    free(tasks);
#if NRNMPI
    if(nrnmpi_use)
    {
        nrnmpi_dbl_allgatherv_inplace(grid->all_currents, grid->proc_num_currents, grid->proc_offsets);
        for(i = 0; i < n; i++)
            output[grid->current_dest[i]] += grid->all_currents[i];
    }
    else
    {
        for(i = 0; i < n; i++)
        {
            output[grid->current_list[i].destination] += grid->all_currents[i];
        }
    }
#else
    for(i = 0; i < n; i++)
        output[grid->current_list[i].destination] +=  grid->all_currents[i];
#endif
    /*Remove the contribution from membrane currents*/
    if(_membrane_flux)
    {
        for(i = 0; i < _memb_curr_total; i++)
        {
            if(_rxd_induced_currents_grid[i] == grid_id)
                output[_rxd_induced_currents_ecs_idx[i]] += _rxd_induced_currents_ecs[i]*_rxd_induced_currents_scale[i];
        }
    }
}

void _fadvance_fixed_step_ecs(void) {
    Grid_node* grid;
    double* states;
    Current_Triple* c;
    double dt = (*dt_ptr);

    /*Currents to broadcast via MPI*/
    /*TODO: Handle multiple grids with one pass*/
    /*Maybe TODO: Should check #currents << #voxels and not the other way round*/
    double* val;
    double* all_currents;
    int proc_offset, id;

	if(threaded_reactions_tasks != NULL)
	    run_threaded_reactions(threaded_reactions_tasks);

    for (id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, id++) {
        MEM_ZERO(grid->states_cur,sizeof(double)*grid->size_x*grid->size_y*grid->size_z);
        do_currents(grid, grid->states_cur, dt, id);
		switch(grid->VARIABLE_ECS_VOLUME)
		{
			case VOLUME_FRACTION:
				set_adi_vol(grid);
				break;
			case TORTUOSITY:
                set_adi_tort(grid);
				break;
			default:
                set_adi_homogeneous(grid);
		}
        dg_adi(grid);
        
    }
    /* transfer concentrations */
    scatter_concentrations();
}

void scatter_concentrations(void) {
    /* transfer concentrations to classic NEURON */
    Grid_node* grid;
    ssize_t i, n;
    Concentration_Pair* cp;
    double* states;

    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        states = grid->states;
        n = grid->num_concentrations;
        cp = grid->concentration_list;
        for (i = 0; i < n; i++) {
            (*cp[i].destination) = states[cp[i].source];
        }
    }
}

/*****************************************************************************
*
* Begin variable step code
*
*****************************************************************************/

/* count the total number of state variables AND store their offset (passed in) in the cvode vector */
int ode_count(const int offset) {
    int count = 0;
    states_cvode_offset = offset;
    Grid_node* grid;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        count += grid->size_x * grid->size_y * grid->size_z;
    }
    return count;
}


void ecs_atolscale(double* y)
{
    Grid_node* grid;
    ssize_t i;
    int grid_size;
    double* grid_states;
    y += states_cvode_offset;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        grid_size = grid->size_x * grid->size_y * grid->size_z;

        for (i = 0; i < grid_size; i++) {
            y[i] *= grid->atolscale;
        }
        y += grid_size;
    }
}

void _ecs_ode_reinit(double* y) {
    Grid_node* grid;
    ssize_t i;
    int grid_size;
    double* grid_states;
    y += states_cvode_offset;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        grid_states = grid->states;
        grid_size = grid->size_x * grid->size_y * grid->size_z;

        for (i = 0; i < grid_size; i++) {
            y[i] = grid_states[i];
        }
        y += grid_size;
    }
}


void _rhs_variable_step_ecs(const double t, const double* states, double* ydot) {
	Grid_node *grid;
    ssize_t n, i;
    int grid_size, j, k;
	double dt = *dt_ptr;
    Current_Triple* c;
    double* grid_states;
    double const * const orig_states = states + states_cvode_offset;
    const unsigned char calculate_rhs = ydot == NULL ? 0 : 1;
    double* const orig_ydot = ydot + states_cvode_offset;
    states = orig_states;
    ydot = orig_ydot;

    /* prepare for advance by syncing data with local copy */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        grid_states = grid->states;
        grid_size = grid->size_x * grid->size_y * grid->size_z;

        /* copy the passed in states to local memory (needed to make reaction rates correct) */
        for (i = 0; i < grid_size; i++) {
            grid_states[i] = states[i];
        }
        states += grid_size;
    }
    /* transfer concentrations to classic NEURON states */
    scatter_concentrations();

    if (!calculate_rhs) {
        return;
    }
	
	states = orig_states;
	ydot = orig_ydot;

	/* TODO: reactions contribute to adaptive step-size*/
	if(threaded_reactions_tasks != NULL)
	    run_threaded_reactions(threaded_reactions_tasks);
	
	for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
	{
		grid_states = grid->states;
        grid_size = grid->size_x * grid->size_y * grid->size_z;
		for(i = 0; i < grid_size; i++)
		{
			ydot[i] += (grid_states[i] - states[i])/dt;
			grid_states[i] = states[i];
		}
		states += grid_size;
		ydot += grid_size;
	}

    ydot = orig_ydot;
    states = orig_states;
    /* process currents */
    for (i = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, i++)
    {
        do_currents(grid, ydot, 1.0, i);
        ydot += grid_size;
    }
	ydot = orig_ydot;

    /* do the diffusion rates */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        grid_size = grid->size_x * grid->size_y * grid->size_z;
		switch(grid->VARIABLE_ECS_VOLUME)
		{
			case VOLUME_FRACTION:
				_rhs_variable_step_helper_vol(grid, states, ydot);
				break;
			case TORTUOSITY:
				_rhs_variable_step_helper_tort(grid, states, ydot);
				break;
			default:
        		_rhs_variable_step_helper(grid, states, ydot);
		}

        ydot += grid_size;
        states += grid_size;        
    }
}


static void _rhs_variable_step_helper(Grid_node* g, double const * const states, double* ydot) {
    int num_states_x = g->size_x, num_states_y = g->size_y, num_states_z = g->size_z;
    double dx = g->dx, dy = g->dy, dz = g->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double dc_x = g->dc_x, dc_y = g->dc_y, dc_z = g->dc_z;

    double rate_x = dc_x / (dx * dx);
    double rate_y = dc_y / (dy * dy);
    double rate_z = dc_z / (dz * dz);

	/*indices*/
	int index, prev_i, prev_j, prev_k, next_i ,next_j, next_k;
	int xp, xm, yp, ym, zp, zm;
	double div_x, div_y, div_z;

	/* Euler advance x, y, z (all points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;
    for (i = 0, index = 0, prev_i = num_states_y*num_states_z, next_i = num_states_y*num_states_z; 
         i < num_states_x; i++) {
	    /*Zero flux boundary conditions*/
		div_x = (i==0||i==stop_i)?2.:1.;

        for(j = 0, prev_j = index + num_states_z, next_j = index + num_states_z; j < num_states_y; j++) {
			div_y = (j==0||j==stop_j)?2.:1.;
            
			for(k = 0, prev_k = index + 1, next_k = index + 1; k < num_states_z;
                k++, index++, prev_i++, next_i++, prev_j++, next_j++) {
				div_z = (k==0||k==stop_k)?2.:1.;

                ydot[index] += rate_x * (states[prev_i] -  
                    2.0 * states[index] + states[next_i])/div_x;

                ydot[index] += rate_y * (states[prev_j] - 
                    2.0 * states[index] + states[next_j])/div_y;

                ydot[index] += rate_z * (states[prev_k] - 
                    2.0 * states[index] + states[next_k])/div_z;
                next_k = (k==stop_k-1)?index:index+2;
                prev_k = index;

            }
            prev_j = index - num_states_z;
            next_j = (j==stop_j-1)?prev_j:index + num_states_z;
        }
        prev_i = index - num_states_y*num_states_z;
        next_i = (i==stop_i-1)?prev_i:index+num_states_y*num_states_z;
    }
}


/*****************************************************************************
*
* End variable step code
*
*****************************************************************************/










/* solve_dd_clhs_tridiag uses Thomas Algorithm to solves a 
 * diagonally dominant tridiagonal matrix (Ax=b), where the
 * triple (lower, diagonal and upper) are repeated to form A. 
 * N        -   length of the matrix
 * l_diag   -   lower diagonal 
 * diag     -   diagonal
 * u_diag   -   upper diagonal
 * b        -   pointer to the RHS  (length N)
 * lbc_diag -   the lower boundary diagonal (the first diagonal element)
 * lbc_u_diag - the lower boundary upper diagonal (the first upper 
 *              diagonal element)
 * ubc_l_diag - the upper boundary lower diagonal (the last lower
 *              diagonal element)
 * ubc_diag -   the upper boundary diagonal (the last diagonal element)
 * The solution (x) is stored in b.
 * c          - scratchpad array, N - 1 doubles long
 */
static int solve_dd_clhs_tridiag(const int N, const double l_diag, const double diag, 
    const double u_diag, const double lbc_diag, const double lbc_u_diag,
    const double ubc_l_diag, const double ubc_diag, double* const b, double* const c) 
{
    int i;
    
    
    c[0] = lbc_u_diag/lbc_diag;
    b[0] = b[0]/lbc_diag;

    for(i=1;i<N-1;i++)
    {
        c[i] = u_diag/(diag-l_diag*c[i-1]);
        b[i] = (b[i]-l_diag*b[i-1])/(diag-l_diag*c[i-1]);
    }
    b[N-1] = (b[N-1]-ubc_l_diag*b[N-2])/(ubc_diag-ubc_l_diag*c[N-2]);

    /*back substitution*/
    for(i=N-2;i>=0;i--)
    {
        b[i]=b[i]-c[i]*b[i+1];  
    }
    return 0;
}

/*
static int solve_dd_clhs_tridiag_rev(const int N, const double l_diag, const double diag, 
    const double u_diag, const double lbc_diag, const double lbc_u_diag,
    const double ubc_l_diag, const double ubc_diag, double* const d, double* const a) 
{
    int i;
    
    
    a[N-2] = ubc_l_diag/ubc_diag;
    d[N-1] = d[N-1]/ubc_diag;

    for(i=N-2; i>0; i--)
    {
        a[i-1] = l_diag/(diag - u_diag*a[i]);
        d[i] = (d[i] - u_diag*d[i+1])/(diag - u_diag*a[i]);
    }
    d[0] = (d[0] - lbc_u_diag*d[1])/(lbc_diag - lbc_u_diag*a[0]);

    for(i=1; i<N; i++)
    {
        d[i] = d[i] - a[i-1]*d[i-1];

    }
    return 0;
}
unsigned char callcount = TRUE;
static int solve_dd_clhs_tridiag(const int N, const double l_diag, const double diag, 
    const double u_diag, const double lbc_diag, const double lbc_u_diag,
    const double ubc_l_diag, const double ubc_diag, double* const b, double* const c)
{
    if(callcount)
    {
        callcount = FALSE;
        return solve_dd_clhs_tridiag_for(N, l_diag, diag, u_diag, lbc_diag, lbc_u_diag, ubc_l_diag, ubc_diag, b, c);
    }
    else
    {
        callcount = TRUE;
        return solve_dd_clhs_tridiag_rev(N, l_diag, diag, u_diag, lbc_diag, lbc_u_diag, ubc_l_diag, ubc_diag, b, c);       
    }
}
*/



/* dg_adi_x performs the first of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g->size_x - 1
 */
static void dg_adi_x(Grid_node* g, const double dt, const int y, const int z, double const * const state, double* const RHS, double* const scratch)
{
    int yp,ym,zp,zm;
    int x;
	double r = g->dc_x*dt/SQ(g->dx);
	double div_y, div_z;
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (y == 0 || z == 0 || y == g->size_y-1 || z == g->size_z-1))
    {
        for(x=0; x<g->size_x; x++)
            RHS[x] = g->bc->value;
        return;
    }    
    yp = (y==g->size_y-1)?y-1:y+1;
    ym = (y==0)?y+1:y-1;
    zp = (z==g->size_z-1)?z-1:z+1;
    zm = (z==0)?z+1:z-1;
    div_y = (y==0||y==g->size_y-1)?2.:1.;
	div_z = (z==0||z==g->size_z-1)?2.:1.;

    if(g->bc->type == NEUMANN)
    {
        /*zero flux boundary condition*/
        RHS[0] =  g->states[IDX(0,y,z)] 
           + dt*((g->dc_x/SQ(g->dx))*(g->states[IDX(1,y,z)] - 2.*g->states[IDX(0,y,z)] + g->states[IDX(1,y,z)])/4.0
           + (g->dc_y/SQ(g->dy))*(g->states[IDX(0,yp,z)] - 2.*g->states[IDX(0,y,z)] + g->states[IDX(0,ym,z)])/div_y
           + (g->dc_z/SQ(g->dz))*(g->states[IDX(0,y,zp)] - 2.*g->states[IDX(0,y,z)] + g->states[IDX(0,y,zm)])/div_z + g->states_cur[IDX(0,y,z)]);
        x = g->size_x-1;
        RHS[x] = g->states[IDX(x,y,z)] 
           + dt*((g->dc_x/SQ(g->dx))*(g->states[IDX(x-1,y,z)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x-1,y,z)])/4.0
           + (g->dc_y/SQ(g->dy))*(g->states[IDX(x,yp,z)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,ym,z)])/div_y
           + (g->dc_z/SQ(g->dz))*(g->states[IDX(x,y,zp)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,y,zm)])/div_z + g->states_cur[IDX(x,y,z)]);
    }
    else
    {
        RHS[0] = g->bc->value;
        RHS[g->size_x-1] = g->bc->value;
    }

    for(x=1; x<g->size_x-1; x++)
    {
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,y,z)]), 0, 1);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,yp,z)]), 0, 0);
        __builtin_prefetch(&(g->states[IDX(x+PREFETCH,ym,z)]), 0, 0);
        RHS[x] =  g->states[IDX(x,y,z)] 
               + dt*((g->dc_x/SQ(g->dx))*(g->states[IDX(x+1,y,z)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x-1,y,z)])/2.
                   + (g->dc_y/SQ(g->dy))*(g->states[IDX(x,yp,z)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,ym,z)])/div_y
                   + (g->dc_z/SQ(g->dz))*(g->states[IDX(x,y,zp)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,y,zm)])/div_z + g->states_cur[IDX(x,y,z)]);

    }
    if(g->bc->type == NEUMANN)
        solve_dd_clhs_tridiag(g->size_x, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratch);
    else
        solve_dd_clhs_tridiag(g->size_x, -r/2.0, 1.0+r, -r/2.0, 1.0, 0, 0, 1.0, RHS, scratch);
}


/* dg_adi_y performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * x    -   the index for the x plane
 * z    -   the index for the z plane
 * state    -   the values from the first step, which are
 *              overwritten by the output of this step
 * scratch  -   scratchpad array of doubles, length g->size_y - 1
 */
static void dg_adi_y(Grid_node* g, double const dt, int const x, int const z, double const * const state, double* const RHS, double* const scratch)
{
    int y;
	double r = (g->dc_y*dt/SQ(g->dy)); 
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (x == 0 || z == 0 || x == g->size_x-1 || z == g->size_z-1))
    {
        for(y=0; y<g->size_y; y++)
            RHS[y] = g->bc->value;
        return;
    }

    if(g->bc->type == NEUMANN)
    {
        /*zero flux boundary condition*/
        RHS[0] =  state[x + z*g->size_x]
            - (g->dc_y*dt/SQ(g->dy))*(g->states[IDX(x,1,z)] - 2.0*g->states[IDX(x,0,z)] + g->states[IDX(x,1,z)])/4.0;
        y = g->size_y-1;
        RHS[y] = state[x + (z + y*g->size_z)*g->size_x]
            - (g->dc_y*dt/SQ(g->dy))*(g->states[IDX(x,y-1,z)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,y-1,z)])/4.0;
    }
    else
    {
        RHS[0] = g->bc->value;
        RHS[g->size_y-1] = g->bc->value;
    }
    for(y=1; y<g->size_y-1; y++)
    {
        __builtin_prefetch(&state[x + (z + (y+PREFETCH)*g->size_z)*g->size_x], 0, 0);
        __builtin_prefetch(&(g->states[IDX(x,y+PREFETCH,z)]), 0, 1);
        RHS[y] =  state[x + (z + y*g->size_z)*g->size_x]
            - (g->dc_y*dt/SQ(g->dy))*(g->states[IDX(x,y+1,z)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,y-1,z)])/2.0;
    }
    if(g->bc->type == NEUMANN)
        solve_dd_clhs_tridiag(g->size_y, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratch);
    else
        solve_dd_clhs_tridiag(g->size_y, -r/2., 1.+r, -r/2., 1.0, 0, 0, 1.0, RHS, scratch);
}


/* dg_adi_z performs the final step in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * x    -   the index for the x plane
 * y    -   the index for the y plane
 * state    -   the values from the second step, which are
 *              overwritten by the output of this step
 * scratch  -   scratchpad array of doubles, length g->size_z - 1
 */
static void dg_adi_z(Grid_node* g, double const dt, int const x, int const y, double const * const state, double* const RHS, double* const scratch)
{
    int z;
	double r = g->dc_z*dt/SQ(g->dz);
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if(g->bc->type == DIRICHLET && (x == 0 || y == 0 || x == g->size_x-1 || y == g->size_y-1))
    {
        for(z=0; z<g->size_z; z++)
            RHS[z] = g->bc->value;
        return;
    }

    if(g->bc->type == NEUMANN)
    { 
        /*zero flux boundary condition*/
        RHS[0] = state[y + g->size_y*(x*g->size_z)]
            - (g->dc_z*dt/SQ(g->dz))*(g->states[IDX(x,y,1)] - 2.0*g->states[IDX(x,y,0)] + g->states[IDX(x,y,1)])/4.0;
        z = g->size_z-1;
        RHS[z] =  state[y + g->size_y*(x*g->size_z + z)]
                - (g->dc_z*dt/SQ(g->dz))*(g->states[IDX(x,y,z-1)] - 2.0*g->states[IDX(x,y,z)] + g->states[IDX(x,y,z-1)])/4.0;

    }
    else
    {
        RHS[0] = g->bc->value;
        RHS[g->size_z-1] = g->bc->value;
    }
    for(z=1; z<g->size_z-1; z++)
    {
        RHS[z] =  state[y + g->size_y*(x*g->size_z + z)]
                - (g->dc_z*dt/SQ(g->dz))*(g->states[IDX(x,y,z+1)] - 2.*g->states[IDX(x,y,z)] + g->states[IDX(x,y,z-1)])/2.;
    }

    if(g->bc->type == NEUMANN)
        solve_dd_clhs_tridiag(g->size_z, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratch);
    else
        solve_dd_clhs_tridiag(g->size_z, -r/2., 1.+r, -r/2., 1.0, 0, 0, 1.0, RHS, scratch);
}

static void* do_dg_adi(void* dataptr) {
    AdiGridData* data = (AdiGridData*) dataptr;
    int start = data -> start;
    int stop = data -> stop;
    int i, j, k;
    AdiDirection* adi_dir = data -> adi_dir;
    double dt = *dt_ptr;
    int sizej = data -> sizej;
    Grid_node* g = data -> g;
    double* state_in = adi_dir-> states_in;
    double* state_out = adi_dir-> states_out;
    int offset = adi_dir -> line_size;
    double* scratchpad = data -> scratchpad;
    void (*dg_adi_dir)(Grid_node*, double, int, int, double const * const, double* const, double* const) = adi_dir -> dg_adi_dir;
    for (k = start; k < stop; k++)
    {
        i = k / sizej;
        j = k % sizej;
        dg_adi_dir(g, dt, i, j, state_in, &state_out[k*offset], scratchpad);
    }

    return NULL;
}

void run_threaded_dg_adi(const int i, const int j, Grid_node* g, AdiDirection* adi_dir, const int n) {
    int k;  
    /* when doing any given direction, the number of tasks is the product of the other two, so multiply everything then divide out the current direction */
    const int tasks_per_thread = (g->size_x * g->size_y * g->size_z / n) / NUM_THREADS;
    const int extra = (g->size_x * g->size_y * g->size_z / n) % NUM_THREADS;
    
    g->tasks[0].start = 0; 
    g->tasks[0].stop = tasks_per_thread + (extra>0);
    g->tasks[0].sizej = j;
    g->tasks[0].adi_dir = adi_dir;
    for (k = 1; k < NUM_THREADS; k++)
    {
        g->tasks[k].start = g->tasks[k-1].stop;
        g->tasks[k].stop = g->tasks[k].start + tasks_per_thread + (extra>k);
        g->tasks[k].sizej = j;
        g->tasks[k].adi_dir = adi_dir;
    }
    g->tasks[NUM_THREADS - 1].stop = i * j;
     /* launch threads */
    for (k = 0; k < NUM_THREADS-1; k++) 
    {
       TaskQueue_add_task(AllTasks, &do_dg_adi, &(g->tasks[k]), NULL);
    }
    /* run one task in the main thread */
    do_dg_adi(&(g->tasks[NUM_THREADS - 1]));
    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}

void set_adi_homogeneous(Grid_node *g)
{
    g->adi_dir_x->dg_adi_dir = dg_adi_x;
    g->adi_dir_y->dg_adi_dir = dg_adi_y;
    g->adi_dir_z->dg_adi_dir = dg_adi_z;
}

/*DG-ADI implementation the 3 step process to diffusion species in grid g by time step *dt_ptr
 * g    -   the state and parameters
 */
static int dg_adi(Grid_node *g)
{
    //double* tmp;
    /* first step: advance the x direction */
    run_threaded_dg_adi(g->size_y, g->size_z, g, g->adi_dir_x, g->size_x);

    /* second step: advance the y direction */
    run_threaded_dg_adi(g->size_x, g->size_z, g, g->adi_dir_y, g->size_y);

    /* third step: advance the z direction */
    run_threaded_dg_adi(g->size_x, g->size_y, g, g->adi_dir_z, g->size_z);

    /* transfer data */
    /*TODO: Avoid copy by switching pointers and updating Python copy
    tmp = g->states;
    g->states = g->adi_dir_z->states_out;
    g->adi_dir_z->states_out = tmp;
    */
    memcpy(g->states, g->adi_dir_z->states_out, sizeof(double)*g->size_x*g->size_y*g->size_z);
    return 0;
}


