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


/*
    Globals
*/

/*Store the onset/offset for each threads reaction tasks*/
ReactGridData* threaded_reactions_tasks;

extern int NUM_THREADS;

extern double* states;

Reaction* ecs_reactions = NULL;

static int states_cvode_offset;

/*Update the global array of reaction tasks when the number of reactions 
 *or threads change.
 *n - the old number of threads - use to free the old threaded_reactions_tasks*/
void ecs_refresh_reactions(int n)
{
	int k;
	if(threaded_reactions_tasks != NULL)
	{
		for(k = 0; k<n; k++)
		{
			SAFE_FREE(threaded_reactions_tasks[k].onset);
			SAFE_FREE(threaded_reactions_tasks[k].offset);
		}
	}
	threaded_reactions_tasks = create_threaded_reactions();
}

void set_num_threads(int n) {
    int old_num = NUM_THREADS;
	NUM_THREADS = n;
	ecs_refresh_reactions(old_num);
}

int get_num_threads(void) {
    return NUM_THREADS;
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
			if(j==i)
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
ReactGridData* create_threaded_reactions()
{
	int i,k,load,react_count = 0,tasks_per_thread;
	Grid_node* grid;
	Reaction* react;

	for(react = ecs_reactions; react != NULL; react = react -> next)
		react_count += react->region_size;

	if(react_count == 0)
		return NULL;

	/*Divide the number of reactions between the threads*/
	tasks_per_thread = react_count / NUM_THREADS;
	ReactGridData* tasks = (ReactGridData*)calloc(sizeof(ReactGridData), NUM_THREADS);
	
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

	double* states_cache;
    double* states_cache_dx;
	double* results_array;
	double* results_array_dx;
    double dx = FLT_EPSILON;
	double pd;
	MAT *jacobian;
    VEC *x;
    VEC *b;
    PERM *pivot;

	int rcalls=0;
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

			/*allocate data structures*/
			jacobian = m_get(react->num_species_involved,react->num_species_involved);
        	b = v_get(react->num_species_involved);
        	x = v_get(react->num_species_involved);
			pivot = px_get(jacobian->m);
			states_cache = (double*)malloc(sizeof(double)*react->num_species_involved);
			states_cache_dx = (double*)malloc(sizeof(double)*react->num_species_involved);
			results_array = (double*)malloc(sizeof(double)*react->num_species_involved);
			results_array_dx = (double*)malloc(sizeof(double)*react->num_species_involved);


			for(i = start_idx; i <= stop_idx; i++)
			{
				if(!react->subregion || react->subregion[i])
				{
					
					/*TODO: this assumes grids are the same size/shape *
	 				 *      add interpolation in case they aren't      */
					for(j = 0; j < react->num_species_involved; j++)
					{
						states_cache[j] = react->species_states[j][i];
						states_cache_dx[j] = react->species_states[j][i];
					}

					react->reaction(states_cache, results_array, NULL, NULL, NULL);

					for(j = 0; j < react->num_species_involved; j++)
					{
						states_cache_dx[j] += dx;
						react->reaction(states_cache_dx, results_array_dx, NULL, NULL, NULL);
						v_set_val(b, j, dt*results_array[j]);

						for(k = 0; k < react->num_species_involved; k++)
						{
							pd = (results_array_dx[k] - results_array[k])/dx;
							m_set_val(jacobian, k, j, (j==k) - dt*pd);
						}
						states_cache_dx[j] -= dx;
					}
					// solve for x, destructively
        			LUfactor(jacobian, pivot);
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
	pthread_t* thread = malloc(sizeof(pthread_t) * NUM_THREADS);
	assert(thread);
	 /* launch threads */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        pthread_create(&thread[k], NULL, &ecs_do_reactions, &tasks[k]);
   }
    /* run one task in the main thread */
    ecs_do_reactions(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        pthread_join(thread[k], NULL);
    }
	free(thread);
}

/*
 * do_current - process the current for a given grid
 * Grid_node* grid - the grid used
 * double* output - for fixed step this is the states for variable ydot
 * double dt - for fixed step the step size, for variable step 1
 */
void do_currents(Grid_node* grid, double* output, double dt)
{
    Py_ssize_t m, n, i;
    Current_Triple* c;

    /*Currents to broadcast via MPI*/
    /*TODO: Handle multiple grids with one pass*/
    /*Maybe TODO: Should check #currents << #voxels and not the other way round*/
    double* val;
    double* all_currents;
    int proc_offset;

    /* currents, via explicit Euler */
    n = grid->num_all_currents;
    m = grid->num_currents;
    c = grid->current_list;
    if(nrnmpi_use)
    {
        proc_offset = grid->proc_offsets[nrnmpi_myid_world];
        val = (grid->all_currents + proc_offset*sizeof(double*));

        for(i = 0; i < m; i++)
        {
            if(grid->VARIABLE_ECS_VOLUME == VOLUME_FRACTION) 
                val[i] = dt * c[i].scale_factor * (*c[i].source)/grid->alpha[c[i].destination];
            else
                val[i] = dt * c[i].scale_factor * (*c[i].source)/grid->alpha[0];
        }

        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, grid->all_currents, grid->proc_num_currents, grid->proc_offsets, MPI_DOUBLE,nrnmpi_world_comm);
        for(i = 0; i < n; i++)
            output[grid->current_dest[i]] += grid->all_currents[i];
    }
    else
    {
        /*divided the concentration by the volume fraction of the relevant voxel*/ 
        if(grid->VARIABLE_ECS_VOLUME == VOLUME_FRACTION)
        {
       	    for (i = 0; i < n; i++)
          	    output[c[i].destination] += dt * c[i].scale_factor * (*c[i].source)/grid->alpha[c[i].destination];
	    }
	    else 
        {
		    for (i = 0; i < n; i++) 
        	    output[c[i].destination] += dt * c[i].scale_factor * (*c[i].source)/grid->alpha[0];
		}
    }
}


void _fadvance_fixed_step_ecs(void) {
    Grid_node* grid;
    Py_ssize_t m, n, i;
    double* states;
    Current_Triple* c;
    double dt = (*dt_ptr);

    /*Currents to broadcast via MPI*/
    /*TODO: Handle multiple grids with one pass*/
    /*Maybe TODO: Should check #currents << #voxels and not the other way round*/
    double* val;
    double* all_currents;
    int proc_offset;

    /* currents, via explicit Euler */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
    {
        do_currents(grid, grid->states, dt);
        memcpy(grid->old_states, grid->states, sizeof(double) * grid->size_x * grid->size_y * grid->size_z);
    }

	if(threaded_reactions_tasks != NULL)
	    run_threaded_reactions(threaded_reactions_tasks);

    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
		switch(grid->VARIABLE_ECS_VOLUME)
		{
			case VOLUME_FRACTION:
				dg_adi_vol(*grid);
				break;
			case TORTUOSITY:
				dg_adi_tort(*grid);
				break;
			default:
        		dg_adi(*grid);
		}

    }
    /* transfer concentrations */
    scatter_concentrations();
}

void scatter_concentrations(void) {
    /* transfer concentrations to classic NEURON */
    Grid_node* grid;
    Py_ssize_t i, n;
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

int find(const int x, const int y, const int z, const int size_y, const int size_z) {
    int index = z + y * size_z + x * size_z * size_y;
    return index;

}

void _ecs_ode_reinit(double* y) {
    Grid_node* grid;
    Py_ssize_t i;
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
    Py_ssize_t n, i;
    int grid_size;
	double dt = *dt_ptr;
    Current_Triple* c;
    double* grid_states;
    const double const* orig_states = states + states_cvode_offset;
    const unsigned char calculate_rhs = ydot == NULL ? 0 : 1;
    double* const orig_ydot = ydot + states_cvode_offset;
    states = orig_states;
    ydot = orig_ydot;

    /* prepare for advance by syncing data with local copy */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        grid_states = grid->states;
        grid_size = grid->size_x * grid->size_y * grid->size_z;
        /* start by clearing our part of ydot */

        if (calculate_rhs) {
            for (i = 0; i < grid_size; i++) {
                ydot[i] = 0;
            }
            ydot += grid_size;
        }

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
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
    {
        do_currents(grid, ydot, 1.0);
        ydot += grid_size;
    }
	ydot = orig_ydot;

    /* do the diffusion rates */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
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

static void _rhs_variable_step_helper(Grid_node* grid, const double const* states, double* ydot) {
    int num_states_x = grid->size_x, num_states_y = grid->size_y, num_states_z = grid->size_z;
    double dc_x = grid->dc_x, dc_y = grid->dc_y, dc_z = grid->dc_z;
    double dx = grid->dx, dy = grid->dy, dz = grid->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double rate_x = dc_x / (dx * dx);
    double rate_y = dc_y / (dy * dy);
    double rate_z = dc_z / (dz * dz);

    /* Euler advance x, y, z (all internal points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;
    for (i = 1; i < stop_i; i++) {
        for(j = 1; j < stop_j; j++) {
            for(k = 1; k < stop_k; k++) {
                int index = find(i, j, k, num_states_y, num_states_z);
                int prev_i = find(i-1, j, k, num_states_y, num_states_z);
                int prev_j = find(i, j-1, k, num_states_y, num_states_z);
                int prev_k = find(i, j, k-1, num_states_y, num_states_z);
                int next_i = find(i+1, j, k, num_states_y, num_states_z);
                int next_j = find(i, j+1, k, num_states_y, num_states_z);
                int next_k = find(i, j, k+1, num_states_y, num_states_z);

                ydot[index] += rate_x * (states[prev_i] - 2 * 
                    states[index] + states[next_i]);

                ydot[index] += rate_y * (states[prev_j] - 2 * 
                    states[index] + states[next_j]);

                ydot[index] += rate_z * (states[prev_k] - 2 * 
                    states[index] + states[next_k]);
            }   
        }
    }

    /* boundary conditions for Z faces */
    for(i = 1; i < stop_i; i++) {
        for(j = 1; j < stop_j; j++) {
            int index = find(i, j, 0, num_states_y, num_states_z);
            int prev_i = find(i-1, j, 0, num_states_y, num_states_z);
            int prev_j = find(i, j-1, 0, num_states_y, num_states_z);
            int next_i = find(i+1, j, 0, num_states_y, num_states_z);
            int next_j = find(i, j+1, 0, num_states_y, num_states_z);
            int next_k = find(i, j, 1, num_states_y, num_states_z);

            ydot[index] += rate_x * (states[prev_i] - 2 * 
                states[index] + states[next_i]);

            ydot[index] += rate_y * (states[prev_j] - 2 * 
                states[index] + states[next_j]);

            ydot[index] += rate_z * (states[next_k] - states[index]);

            index = find(i, j, stop_k, num_states_y, num_states_z);
            prev_i = find(i-1, j, stop_k, num_states_y, num_states_z);
            prev_j = find(i, j-1, stop_k, num_states_y, num_states_z);
            next_i = find(i+1, j, stop_k, num_states_y, num_states_z);
            next_j = find(i, j+1, stop_k, num_states_y, num_states_z);
            next_k = find(i, j, stop_k-1, num_states_y, num_states_z);

            ydot[index] += rate_x * (states[prev_i] - 2 * 
                states[index] + states[next_i]);

            ydot[index] += rate_y * (states[prev_j] - 2 * 
                states[index] + states[next_j]);

            ydot[index] += rate_z * (states[next_k] - states[index]);
        }   
    }   

    /* boundary conditions for X faces */
    for(j = 1; j < stop_j; j++) {
        for(k = 1; k < stop_k; k++) {
            int index = find(0, j, k, num_states_y, num_states_z);
            int prev_j = find(0, j-1, k, num_states_y, num_states_z);
            int prev_k = find(0, j, k-1, num_states_y, num_states_z);
            int next_j = find(0, j+1, k, num_states_y, num_states_z);
            int next_k = find(0, j, k+1, num_states_y, num_states_z);
            int next_i = find(1, j, k, num_states_y, num_states_z);

            ydot[index] += rate_x * (states[next_i] - states[index]);

            ydot[index] += rate_y * (states[prev_j] - 2 * 
                states[index] + states[next_j]);

            ydot[index] += rate_z * (states[prev_k] - 2 * 
                states[index] + states[next_k]);

            index = find(stop_i, j, k, num_states_y, num_states_z);
            prev_j = find(stop_i, j-1, k, num_states_y, num_states_z);
            prev_k = find(stop_i, j, k-1, num_states_y, num_states_z);
            next_j = find(stop_i, j+1, k, num_states_y, num_states_z);
            next_k = find(stop_i, j, k+1, num_states_y, num_states_z);
            next_i = find(stop_i-1, j, k, num_states_y, num_states_z);

            ydot[index] += rate_x * (states[next_i] - states[index]);

            ydot[index] += rate_y * (states[prev_j] - 2 * 
                states[index] + states[next_j]);

            ydot[index] += rate_z * (states[prev_k] - 2 * 
                states[index] + states[next_k]);
        }   
    }   

    /* boundary conditions for Y faces */
    for(i = 1; i < stop_i; i++) {
        for(k = 1; k < stop_k; k++) {
            int index = find(i, 0, k, num_states_y, num_states_z);
            int prev_i = find(i-1, 0, k, num_states_y, num_states_z);
            int prev_k = find(i, 0, k-1, num_states_y, num_states_z);
            int next_i = find(i+1, 0, k, num_states_y, num_states_z);
            int next_k = find(i, 0, k+1, num_states_y, num_states_z);
            int next_j = find(i, 1, k, num_states_y, num_states_z);

            ydot[index] += rate_x * (states[prev_i] - 2 * 
                states[index] + states[next_i]);

            ydot[index] += rate_y * (states[next_j] - states[index]);

            ydot[index] += rate_z * (states[prev_k] - 2 * 
                states[index] + states[next_k]);

            index = find(i, stop_j, k, num_states_y, num_states_z);
            prev_i = find(i-1, stop_j, k, num_states_y, num_states_z);
            prev_k = find(i, stop_j, k-1, num_states_y, num_states_z);
            next_i = find(i+1, stop_j, k, num_states_y, num_states_z);
            next_k = find(i, stop_j, k+1, num_states_y, num_states_z);
            next_j = find(i, stop_j-1, k, num_states_y, num_states_z);

            ydot[index] += rate_x * (states[prev_i] - 2 * 
                states[index] + states[next_i]);

            ydot[index] += rate_y * (states[next_j] - states[index]);

            ydot[index] += rate_z * (states[prev_k] - 2 * 
                states[index] + states[prev_k]);
        }   
    }

    /* boundary conditions for edges */
    for(i = 1; i < stop_i; i++) {
        update_boundaries_x(i, 0, 0, 1, 1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_x(i, stop_j, 0, -1, 1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_x(i, 0, stop_k, 1, -1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_x(i, stop_j, stop_k, -1, -1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
    }
    for(j = 1; j < stop_j; j++) {
        update_boundaries_y(0, j, 0, 1, 1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_y(stop_i, j, 0, -1, 1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_y(0, j, stop_k, 1, -1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_y(stop_i, j, stop_k, -1, -1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
    }
    for(k = 1; k < stop_k; k++) {
        update_boundaries_z(0, 0, k, 1, 1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_z(stop_i, 0, k, -1, 1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_z(0, stop_j, k, 1, -1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
        update_boundaries_z(stop_i, stop_j, k, -1, -1, rate_x, rate_y, rate_z, num_states_x, num_states_y, num_states_z, states, ydot);
    }

    /* boundary conditions for corners */
    int next_i, next_j, next_k;
    for(i = 0; i <= stop_i; i += stop_i) {
        for(j = 0; j <= stop_j; j += stop_j) {
            for(k = 0; k <= stop_k; k += stop_k) {
                int corner = find(i, j, k, num_states_y, num_states_z);
                if(!i) next_i = find(i+1, j, k, num_states_y, num_states_z);
                else next_i = find(i-1, j, k, num_states_y, num_states_z);
                if(!j) next_j = find(i, j+1, k, num_states_y, num_states_z);
                else next_j = find(i, j-1, k, num_states_y, num_states_z);
                if(!k) next_k = find(i, j, k+1, num_states_y, num_states_z);
                else next_k = find(i, j, k-1, num_states_y, num_states_z);
                ydot[corner] += rate_x * (states[next_i] - states[corner]);
                ydot[corner] += rate_y * (states[next_j] - states[corner]);
                ydot[corner] += rate_z * (states[next_k] - states[corner]);
            }
        }
    }

}

/* update x-dimension edge points */
/* function takes in combinations of j and k to represent the edges as well as
1 or -1 for dj and dk to indicate which adjacent location is still in bounds */
static void update_boundaries_x(int i, int j, int k, int dj, int dk, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot) {

    int edge = find(i, j, k, num_states_y, num_states_z);
    int prev_i = find(i-1, j, k, num_states_y, num_states_z);
    int next_i = find(i+1, j, k, num_states_y, num_states_z);
    int next_j = find(i, j + dj, k, num_states_y, num_states_z);
    int next_k = find(i, j, k + dk, num_states_y, num_states_z);

    ydot[edge] += rate_x * (states[prev_i] - 2 * states[edge]
     + states[next_i]);
    ydot[edge] += rate_y * (states[next_j] - states[edge]);
    ydot[edge] += rate_z * (states[next_k] - states[edge]);
}

/* update y-dimension edge points */
static void update_boundaries_y(int i, int j, int k, int di, int dk, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot) {

    int edge = find(i, j, k, num_states_y, num_states_z);
    int prev_j = find(i, j-1, k, num_states_y, num_states_z);
    int next_j = find(i, j+1, k, num_states_y, num_states_z);
    int next_i = find(i + di, j, k, num_states_y, num_states_z);
    int next_k = find(i, j, k + dk, num_states_y, num_states_z);

    ydot[edge] += rate_x * (states[next_i] - states[edge]);
    ydot[edge] += rate_y * (states[prev_j] - 2 * states[edge]
     + states[next_j]);
    ydot[edge] += rate_z * (states[next_k] - states[edge]);
}

/* update z-dimension edge points */
static void update_boundaries_z(int i, int j, int k, int di, int dj, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot) {

    int edge = find(i, j, k, num_states_y, num_states_z);
    int prev_k = find(i, j, k-1, num_states_y, num_states_z);
    int next_k = find(i, j, k+1, num_states_y, num_states_z);
    int next_i = find(i + di, j, k, num_states_y, num_states_z);
    int next_j = find(i, j + dj, k, num_states_y, num_states_z);

    ydot[edge] += rate_x * (states[next_i] - states[edge]);
    ydot[edge] += rate_y * (states[next_j] - states[edge]);
    ydot[edge] += rate_z * (states[prev_k] - 2 * states[edge] + 
        states[next_k]);
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
    /*could solve the difference equation rather than allocate memory*/ 
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


/* dg_adi_x performs the first of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * y    -   the index for the y plane
 * z    -   the index for the z plane
 * state    -   where the output of this step is stored
 * scratch  - scratchpad array of doubles, length g.size_x - 1
 */
static AdiLineData dg_adi_x(Grid_node g, const double dt, const int y, const int z, double const * const state, double* const scratch)
{
    int yp,ym,zp,zm;
    int x;
    double *RHS = malloc(sizeof(double)*g.size_x);
	double r = g.dc_x*dt/SQ(g.dx);
	double div_y = (y==0||y==g.size_y-1)?2.:1.,
		   div_z = (z==0||z==g.size_z-1)?2.:1.;
	AdiLineData result;
    result.copyFrom = RHS;
    result.copyTo = IDX(0, y, z);

    /*zero flux boundary condition*/
    yp = (y==g.size_y-1)?y-1:y+1;
    ym = (y==0)?y+1:y-1;
    zp = (z==g.size_z-1)?z-1:z+1;
    zm = (z==0)?z+1:z-1;


    RHS[0] =  g.states[IDX(0,y,z)] 
           + dt*((g.dc_x/SQ(g.dx))*(g.states[IDX(1,y,z)] - 2.*g.states[IDX(0,y,z)] + g.states[IDX(1,y,z)])/4.
           + (g.dc_y/SQ(g.dy))*(g.states[IDX(0,yp,z)] - 2.*g.states[IDX(0,y,z)] + g.states[IDX(0,ym,z)])/div_y
           + (g.dc_z/SQ(g.dz))*(g.states[IDX(0,y,zp)] - 2.*g.states[IDX(0,y,z)] + g.states[IDX(0,y,zm)])/div_z);

    for(x=1; x<g.size_x-1; x++)
    {
        RHS[x] =  g.states[IDX(x,y,z)] 
               + dt*((g.dc_x/SQ(g.dx))*(g.states[IDX(x+1,y,z)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x-1,y,z)])/2.
                   + (g.dc_y/SQ(g.dy))*(g.states[IDX(x,yp,z)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,ym,z)])/div_y
                   + (g.dc_z/SQ(g.dz))*(g.states[IDX(x,y,zp)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,y,zm)])/div_z);

    }
    RHS[x] = g.states[IDX(x,y,z)] 
           + dt*((g.dc_x/SQ(g.dx))*(g.states[IDX(x-1,y,z)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x-1,y,z)])/4.
           + (g.dc_y/SQ(g.dy))*(g.states[IDX(x,yp,z)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,ym,z)])/div_y
           + (g.dc_z/SQ(g.dz))*(g.states[IDX(x,y,zp)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,y,zm)])/div_z);
    
    solve_dd_clhs_tridiag(g.size_x, -r/2., 1.+r, -r/2., 1.+r/2., -r/2., -r/2., 1.+r/2., RHS, scratch);

    return result;
}


/* dg_adi_y performs the second of 3 steps in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * x    -   the index for the x plane
 * z    -   the index for the z plane
 * state    -   the values from the first step, which are
 *              overwritten by the output of this step
 * scratch  -   scratchpad array of doubles, length g.size_y - 1
 */
static AdiLineData dg_adi_y(Grid_node g, double const dt, int const x, int const z, double const * const state, double* const scratch)
{
    int y;
	double r = (g.dc_y*dt/SQ(g.dy)); 
    double *RHS = malloc(sizeof(double)*g.size_y);
    AdiLineData result;
    result.copyFrom = RHS;
    result.copyTo = IDX(x, 0, z);


    /*zero flux boundary condition*/
    RHS[0] =  state[IDX(x,0,z)]
            - (g.dc_y*dt/SQ(g.dy))*(g.states[IDX(x,1,z)] - 2.*g.states[IDX(x,0,z)] + g.states[IDX(x,1,z)])/4.;

    for(y=1; y<g.size_y-1; y++)
    {
        RHS[y] =  state[IDX(x,y,z)]
            - (g.dc_y*dt/SQ(g.dy))*(g.states[IDX(x,y+1,z)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,y-1,z)])/2.;

    }
    RHS[y] =  state[IDX(x,y,z)]
            - (g.dc_y*dt/SQ(g.dy))*(g.states[IDX(x,y-1,z)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,y-1,z)])/4.;


    solve_dd_clhs_tridiag(g.size_y, -r/2., 1.+r, -r/2., 1.+r/2., -r/2., -r/2., 1.+r/2., RHS, scratch);

    return result;
}


/* dg_adi_z performs the final step in DG-ADI
 * g    -   the parameters and state of the grid
 * dt   -   the time step
 * x    -   the index for the x plane
 * y    -   the index for the y plane
 * state    -   the values from the second step, which are
 *              overwritten by the output of this step
 * scratch  -   scratchpad array of doubles, length g.size_z - 1
 */
static AdiLineData dg_adi_z(Grid_node g, double const dt, int const x, int const y, double const * const state, double* const scratch)
{
    int z;
    double *RHS = malloc(sizeof(double)*g.size_z);
	double r = g.dc_z*dt/SQ(g.dz);
    AdiLineData result;
    result.copyFrom = RHS;
    result.copyTo = IDX(x, y, 0);
    
    /*zero flux boundary condition*/
    RHS[0] =  state[IDX(x,y,0)]
            - (g.dc_z*dt/SQ(g.dz))*(g.states[IDX(x,y,1)] - 2.*g.states[IDX(x,y,0)] + g.states[IDX(x,y,1)])/4.;

    for(z=1; z<g.size_z-1; z++)
    {
        RHS[z] =  state[IDX(x,y,z)]
                - (g.dc_z*dt/SQ(g.dz))*(g.states[IDX(x,y,z+1)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,y,z-1)])/2.;

    }
    RHS[z] =  state[IDX(x,y,z)]
                - (g.dc_z*dt/SQ(g.dz))*(g.states[IDX(x,y,z-1)] - 2.*g.states[IDX(x,y,z)] + g.states[IDX(x,y,z-1)])/4.;

    solve_dd_clhs_tridiag(g.size_z, -r/2., 1.+r, -r/2., 1.+r/2., -r/2., -r/2., 1.+r/2., RHS, scratch);

    return result;
}

void dg_transfer_data(AdiLineData * const vals, double* const state, int const num_each_line, int const num_lines, int const offset) {
    AdiLineData* myVal;
    int i, j, k;

    for(i = 0; i < num_lines; i++) {
        myVal = vals + i;
        for(j = myVal -> copyTo, k = 0; k < num_each_line; k++, j += offset) {
            state[j] = (myVal -> copyFrom)[k];
        }
        free(myVal -> copyFrom);
    }
}

static void* do_dg_adi(void* dataptr) {
    AdiGridData* data = (AdiGridData*) dataptr;
    int start = data -> start;
    int stop = data -> stop;
    AdiLineData* vals = data -> vals;
    int i, j, k;
    double* state = data -> state;
    double dt = *dt_ptr;
    int sizej = data -> sizej;
    Grid_node g = data -> g;
    double* scratchpad = data -> scratchpad;
    AdiLineData (*dg_adi_dir)(Grid_node, double, int, int, double const *, double*) = data -> dg_adi_dir;

    for (k = start; k < stop; k++) {
        i = k / sizej;
        j = k % sizej;
        vals[k] = dg_adi_dir(g, dt, i, j, state, scratchpad);
    }
    return NULL;
}

void run_threaded_dg_adi(AdiGridData* tasks, pthread_t* thread, const int i, const int j, Grid_node g, double* state, AdiLineData* vals, AdiLineData (*dg_adi_dir)(Grid_node, double, int, int, double const *, double*), const int n) {
    int k;
    /* when doing any given direction, the number of tasks is the product of the other two, so multiply everything then divide out the current direction */
    const int tasks_per_thread = (g.size_x * g.size_y * g.size_z / n + NUM_THREADS - 1) / NUM_THREADS;

    for (k = 0; k < NUM_THREADS; k++) {
        tasks[k].start = k * tasks_per_thread;
        tasks[k].stop = (k + 1) * tasks_per_thread;
        tasks[k].g = g;
        tasks[k].state = state;
        tasks[k].vals = vals;
        tasks[k].sizej = j;
        tasks[k].dg_adi_dir = dg_adi_dir;
        tasks[k].scratchpad = malloc(sizeof(double) * (n-1));
		/*with variable volume fraction there are at most n additional points*/
     }
    tasks[NUM_THREADS - 1].stop = i * j;
    /* launch threads */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        pthread_create(&thread[k], NULL, do_dg_adi, &tasks[k]);
    }
    /* run one task in the main thread */
    do_dg_adi(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish, free scratch space as they do */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        pthread_join(thread[k], NULL);
        free(tasks[k].scratchpad);
    }
}

/*DG-ADI implementation the 3 step process to diffusion species in grid g by time step *dt_ptr
 * g    -   the state and parameters
 */
static int dg_adi(Grid_node g)
{
    double* state = malloc(sizeof(double) * g.size_x * g.size_y * g.size_z);
    AdiLineData* vals = malloc(sizeof(AdiLineData) * g.size_y * g.size_z);
    pthread_t* thread = malloc(sizeof(pthread_t) * NUM_THREADS);
    AdiGridData* tasks = malloc(sizeof(AdiGridData) * NUM_THREADS);

	assert(vals);
	assert(thread);
	assert(tasks);

    /* first step: advance the x direction */
    run_threaded_dg_adi(tasks, thread, g.size_y, g.size_z, g, state, vals, dg_adi_x, g.size_x);

    /* transfer data */
    dg_transfer_data(vals, state, g.size_x, g.size_y * g.size_z, g.size_y * g.size_z);

    /* Adjust memory */
    free(vals);
    vals = malloc(sizeof(AdiLineData) * g.size_x * g.size_z);

    /* second step: advance the y direction */
    run_threaded_dg_adi(tasks, thread, g.size_x, g.size_z, g, state, vals, dg_adi_y, g.size_y);

    /* transfer data */
    dg_transfer_data(vals, state, g.size_y, g.size_x * g.size_z, g.size_z);

    /* Adjust memory */
    free(vals);
    vals = malloc(sizeof(AdiLineData) * g.size_x * g.size_y);

    /* third step: advance the z direction */
    run_threaded_dg_adi(tasks, thread, g.size_x, g.size_y, g, state, vals, dg_adi_z, g.size_z);

    /* transfer data directly into Grid_node values (g.states) */
    dg_transfer_data(vals, g.states, g.size_z, g.size_x * g.size_y, 1);

    free(state);
    free(vals);
    free(thread);
    free(tasks);

    return 0;
}


