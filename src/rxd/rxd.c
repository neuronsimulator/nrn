#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <pthread.h>
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#define DIE(msg) exit(fprintf(stderr, "%s\n", msg))
#define SAFE_FREE(ptr){if(ptr!=NULL) free(ptr);}

/*
    Globals
*/

/*Store the onset/offset for each threads reaction tasks*/
ReactGridData* threaded_reactions_tasks;

int NUM_THREADS = 1;


/*Update the global array of reaction tasks when the number of reactions 
 *or threads change.
 *n - the old number of threads - use to free the old threaded_reactions_tasks*/
void refresh_reactions(int n)
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
	refresh_reactions(old_num);
}

int get_num_threads(void) {
    return NUM_THREADS;
}


/*Removal all reactions*/
void clear_rates(void)
{
	Grid_node *grid;
	Reaction *r, *tmp;
	for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
	{
		for(r = grid->reactions; r != NULL;)
		{
			free(r->species_states);
			if(r->subregion)
				free(r->subregion);
			tmp = r;
			r = r->next;
			free(tmp);
		}
	}
	refresh_reactions(NUM_THREADS);

}

/*Create a reaction
 * list_idx - the index for the linked list in the global array Parallel_grids
 * grid_id - the grid id within the linked list
 * ReactionRate - the reaction function
 * subregion - either NULL or a boolean array the same size as the grid
 * 	indicating if reaction occurs at a specific voxel
 */
Reaction* create_reaction(int list_idx, int grid_id, ReactionRate f, unsigned char* subregion)
{
	Grid_node *grid;
	int i, j;

	Reaction* r = (Reaction*)malloc(sizeof(Reaction));
	assert(r);
	r->reaction = f;
	for(grid = Parallel_grids[list_idx], i = 0; grid != NULL; grid = grid -> next, i++)
	{
		if(i==grid_id)
		{
			/*Place the reaction at the top of the stack `reactions` in Grid_node*/
			r->next = grid->reactions;
			grid->reactions = r;

			if(subregion == NULL)
			{
				r->subregion = NULL;
				r->subregion_size = 0;
			}
			else
			{
				for(r->subregion_size=0, j=0; j < grid->size_x * grid->size_y * grid->size_z; j++)
					r->subregion_size += subregion[j];
				r->subregion = subregion;
			}	

		}	
	}

	r->num_species_involved=i;
	r->species_states = (double**)malloc(sizeof(Grid_node*)*(r->num_species_involved));
	assert(r->species_states);

	for(grid = Parallel_grids[list_idx], i = 0; grid != NULL; grid = grid -> next, i++)
	{
		/*Assumes grids are the same size (no interpolation) 
		 *Assumes all the species will be in the same Parallel_grids list*/
		r->species_states[i] = grid->old_states;
	}
	
	return r;
}

/*register_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * ReactionRate - the reaction function
 */
void register_reaction(int list_idx, int grid_id, ReactionRate f)
{
	create_reaction(list_idx, grid_id, f, NULL);
	refresh_reactions(NUM_THREADS);
}


/*register_subregion_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * my_subregion - a boolean array indicating the voxels where a reaction occurs
 * ReactionRate - the reaction function
 */
void register_subregion_reaction(int list_idx, int grid_id, unsigned char* my_subregion, ReactionRate f)
{
	create_reaction(list_idx, grid_id, f, my_subregion);
	refresh_reactions(NUM_THREADS);
}

static int states_cvode_offset;
void scatter_concentrations(void);

void update_boundaries_x(int i, int j, int k, int dj, int dk, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot);


void update_boundaries_y(int i, int j, int k, int di, int dk, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot);

void update_boundaries_z(int i, int j, int k, int di, int dj, double rate_x,
 double rate_y, double rate_z, int num_states_x, int num_states_y, int num_states_z,
 const double const* states, double* ydot);

int dg_adi(Grid_node g);
void _rhs_variable_step_helper(Grid_node* grid, const double const* states, double* ydot);

fptr _setup, _initialize;

/*create_threaded_reactions is used to generate evenly distribution of reactions
 * across NUM_THREADS
 * returns ReactGridData* which can be used as the global
 * 	threaded_reactions_tasks 
 */
ReactGridData* create_threaded_reactions()
{
	int i,k,load,reactions = 0,tasks_per_thread;
	Grid_node* grid;
	Reaction* react;

	/*Count the number of reactions*/
	for(grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
	{
		for(react = grid->reactions; react != NULL; react = react -> next)
		{
			if(react->subregion)
				reactions += react->subregion_size;
			else
				reactions += grid->size_x * grid->size_y * grid->size_z;
		}
	}
	if(reactions == 0)
		return NULL;

	/*Divide the number of reactions between the threads*/
	tasks_per_thread = reactions / NUM_THREADS;
	ReactGridData* tasks = (ReactGridData*)calloc(sizeof(ReactGridData), NUM_THREADS);
	
	tasks[0].onset = (ReactSet*)malloc(sizeof(ReactSet));
	tasks[0].onset->grid = Parallel_grids[0];
	tasks[0].onset->idx = 0;


	for(k = 0, load = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
	{
		for(i = 0; i < grid->size_x * grid->size_y * grid->size_z; i++)
		{
			for(react = grid->reactions; react != NULL; react = react -> next)
			{
				if(!react->subregion || react->subregion[i])
					load++;
				if(load >= tasks_per_thread)
				{
					tasks[k].offset = (ReactSet*)malloc(sizeof(ReactSet));
					tasks[k].offset->grid = grid;
					tasks[k++].offset->idx = i;	

					tasks[k].onset = (ReactSet*)malloc(sizeof(ReactSet));
					tasks[k].onset->grid = grid;
					tasks[k].onset->idx = i + 1;

					load = 0;
				}
			}
		}
	}
	return tasks;
}

/*do_reactions takes ReactGridData which defines the set of reactions to be
 * performed. It calculate the reaction based on grid->old_states and updates
 * grid->states
 */
void* do_reactions(void* dataptr)
{
	ReactGridData task = *(ReactGridData*)dataptr;
	unsigned char started = FALSE, stop = FALSE;
	int i, j, start_idx, stop_idx;
	double* states_cache;
	double dt = *dt_ptr;
	Grid_node* grid;
	Reaction* react;
	for(grid = Parallel_grids[0]; grid != NULL; grid = grid -> next)
	{
		if(started || grid == task.onset->grid)
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
			if(task.offset->grid == grid)
			{
				stop_idx = task.offset->idx;
				stop = TRUE;
			}
			else
			{
				stop_idx = grid->size_x * grid->size_y * grid->size_z;
				stop = FALSE;
			}
			for(i = start_idx; i<stop_idx; i++)
			{
				for(react = grid->reactions; react != NULL; react = react -> next)
				{
					if(!react->subregion || react->subregion[i])
					{
						states_cache = (double*)malloc(sizeof(double)*react->num_species_involved);

						for(j = 0; j < react->num_species_involved; j++)
						{
								/*TODO: this assumes grids are the same size/shape *
		 						 *      add interpolation in case they aren't      */
							states_cache[j] = react->species_states[j][i];
						}
						grid->states[i] += dt*(react->reaction(states_cache));
						free(states_cache);
					}
				}
			}
			if(stop)
				return NULL;
		}
	}
	return NULL;
}

/* run_threaded_reactions
 * Array ReactGridData tasks length NUM_THREADS and calls do_reactions and
 * executes each task with a separate thread
 */
void run_threaded_reactions(ReactGridData* tasks)
{
	int k;
	pthread_t* thread = malloc(sizeof(pthread_t) * NUM_THREADS);
	assert(thread);
	 /* launch threads */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        pthread_create(&thread[k], NULL, &do_reactions, &tasks[k]);
    }
    /* run one task in the main thread */
    do_reactions(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        pthread_join(thread[k], NULL);
    }
	free(thread);
}

void _fadvance_fixed_step(void) {
    Grid_node* grid;
    Py_ssize_t n, i;
    double* states;
    Current_Triple* c;
    double dt = (*dt_ptr);
    /* currents, via explicit Euler */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        states = grid->states;
        n = grid->num_currents;
        c = grid->current_list;
		/*divided the concentration by the volume fraction of the relevant voxel*/ 
		if(grid->VARIABLE_ECS_VOLUME == VOLUME_FRACTION) {
        	for (i = 0; i < n; i++) {
            	states[c[i].destination] += dt * c[i].scale_factor * (*c[i].source)/grid->alpha[c[i].destination];
        	}
		}
		else {
			for (i = 0; i < n; i++) {
            	states[c[i].destination] += dt * c[i].scale_factor * (*c[i].source)/grid->alpha[0];
        	}
		}
		memcpy(grid->old_states, states, sizeof(double) * grid->size_x * grid->size_y * grid->size_z);
    }

    /* TODO: implicit reactions*/
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

void _ode_reinit(double* y) {
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


void _rhs_variable_step(const double t, const double* states, double* ydot) {
    Grid_node *grid;
    Py_ssize_t n, i;
    int grid_size;
    Current_Triple* c;
    double* grid_states;
    const double const* orig_states = states + states_cvode_offset;
    const int calculate_rhs = ydot == NULL ? 0 : 1;
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

    ydot = orig_ydot;
    states = orig_states;

    /* process currents */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        grid_states = grid->states;
        n = grid->num_currents;
        c = grid->current_list;
        grid_size = grid->size_x * grid->size_y * grid->size_z;

        for (i = 0; i < n; i++) {
            ydot[c[i].destination] += c[i].scale_factor * (*c[i].source);
        }
        ydot += grid_size;
        states += grid_size;        
    }

    /* TODO: reactions */

    ydot = orig_ydot;
    states = orig_states;

    /* do the diffusion rates */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid -> next) {
        _rhs_variable_step_helper(grid, states, ydot);
        ydot += grid_size;
        states += grid_size;        
    }
}

void _rhs_variable_step_helper(Grid_node* grid, const double const* states, double* ydot) {
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
void update_boundaries_x(int i, int j, int k, int dj, int dk, double rate_x,
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
void update_boundaries_y(int i, int j, int k, int di, int dk, double rate_x,
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
void update_boundaries_z(int i, int j, int k, int di, int dj, double rate_x,
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
int solve_dd_clhs_tridiag(const int N, const double l_diag, const double diag, 
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
AdiLineData dg_adi_x(Grid_node g, const double dt, const int y, const int z, double const * const state, double* const scratch)
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
AdiLineData dg_adi_y(Grid_node g, double const dt, int const x, int const z, double const * const state, double* const scratch)
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
AdiLineData dg_adi_z(Grid_node g, double const dt, int const x, int const y, double const * const state, double* const scratch)
{
    int z;
    double *RHS = malloc(sizeof(double)*g.size_z);
	double r = g.dc_y*dt/SQ(g.dz);
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

void* do_dg_adi(void* dataptr) {
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
        tasks[k].scratchpad = malloc(sizeof(double) * ((g.VARIABLE_ECS_VOLUME==VOLUME_FRACTION?2*n-1:n-1)));
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
int dg_adi(Grid_node g)
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


/* TODO: this will need modified to also support intracellular rxd */
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
            _fadvance_fixed_step();
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

void set_setup(const fptr setup_fn) {
	_setup = setup_fn;
}

void set_initialize(const fptr initialize_fn) {
	_initialize = initialize_fn;

	/*Setup threaded reactions*/
    refresh_reactions(NUM_THREADS);
	printf("SET INITIALIZE\n");
}

/* verbatim from nrnoc/ldifus.c; included as a copy for convenience
   TODO: if we don't ever need this, remove it
 */
void nrn_tree_solve(double* a, double* d, double* b, double* rhs, int* pindex, int n) {
    /*
        treesolver
        
        a      - above the diagonal 
        d      - diagonal
        b      - below the diagonal
        rhs    - right hand side, which is changed to the result
        pindex - parent indices
        n      - number of states
    */

    int i;

	/* triang */
	for (i = n - 1; i > 0; --i) {
		int pin = pindex[i];
		if (pin > -1) {
			double p;
			p = a[i] / d[i];
			d[pin] -= p * b[i];
			rhs[pin] -= p * rhs[i];
		}
	}
	/* bksub */
	for (i = 0; i < n; ++i) {
		int pin = pindex[i];
		if (pin > -1) {
			rhs[i] -= b[i] * rhs[pin];
		}
		rhs[i] /= d[i];
	}
}

// double* states1;
// double* states2;
// double* states3;
int num_states;

