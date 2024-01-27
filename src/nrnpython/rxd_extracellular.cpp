#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <nrnwrap_Python.h>
#include <cmath>
#include <ocmatrix.h>
#include <cfloat>

#define loc(x, y, z) ((z) + (y) *grid->size_z + (x) *grid->size_z * grid->size_y)

static void ecs_refresh_reactions(int);
/*
    Globals
*/

/*Store the onset/offset for each threads reaction tasks*/
ReactGridData* threaded_reactions_tasks;

extern int NUM_THREADS;
extern TaskQueue* AllTasks;
extern double* t_ptr;
extern double* states;

Reaction* ecs_reactions = NULL;

int states_cvode_offset;

/*Update the global array of reaction tasks when the number of reactions
 *or threads change.
 *n - the old number of threads - use to free the old threaded_reactions_tasks*/
static void ecs_refresh_reactions(const int n) {
    int k;
    if (threaded_reactions_tasks != NULL) {
        for (k = 0; k < NUM_THREADS; k++) {
            SAFE_FREE(threaded_reactions_tasks[k].onset);
            SAFE_FREE(threaded_reactions_tasks[k].offset);
        }
        SAFE_FREE(threaded_reactions_tasks);
    }
    threaded_reactions_tasks = create_threaded_reactions(n);
}

void set_num_threads_3D(const int n) {
    Grid_node* grid;
    if (n != NUM_THREADS) {
        for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
            grid->set_num_threads(n);
        }
    }
    ecs_refresh_reactions(n);
}


/*Removal all reactions*/
void clear_rates_ecs(void) {
    Reaction *r, *tmp;
    Grid_node* grid;
    ECS_Grid_node* g;

    for (r = ecs_reactions; r != NULL; r = tmp) {
        SAFE_FREE(r->species_states);
        if (r->subregion) {
            SAFE_FREE(r->subregion);
        }

        tmp = r->next;
        SAFE_FREE(r);
    }
    ecs_reactions = NULL;

    ecs_refresh_reactions(NUM_THREADS);
    for (Grid_node* grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        g = dynamic_cast<ECS_Grid_node*>(grid);
        if (g)
            g->clear_multicompartment_reaction();
    }
}

/*Create a reaction
 * list_idx - the index for the linked list in the global array Parallel_grids
 * grid_id - the grid id within the linked list
 * ECSReactionRate - the reaction function
 * subregion - either NULL or a boolean array the same size as the grid
 * 	indicating if reaction occurs at a specific voxel
 */
Reaction* ecs_create_reaction(int list_idx,
                              int num_species,
                              int num_params,
                              int* species_ids,
                              ECSReactionRate f,
                              unsigned char* subregion,
                              uint64_t* mc3d_start_indices,
                              int mc3d_region_size,
                              double* mc3d_mults) {
    Grid_node* grid;
    Reaction* r;
    int i, j;

    r = (Reaction*) malloc(sizeof(Reaction));
    assert(r);
    r->reaction = f;
    /*place reaction on the top of the stack of reactions*/
    r->next = ecs_reactions;
    ecs_reactions = r;

    for (grid = Parallel_grids[list_idx], i = 0; grid != NULL; grid = grid->next, i++) {
        /* Assume all species have the same grid */
        if (i == species_ids[0]) {
            if (mc3d_region_size > 0) {
                r->subregion = NULL;
                r->region_size = mc3d_region_size;
                r->mc3d_indices_offsets = (uint64_t*) malloc(sizeof(uint64_t) *
                                                             (num_species + num_params));
                memcpy(r->mc3d_indices_offsets,
                       mc3d_start_indices,
                       sizeof(uint64_t) * (num_species + num_params));
                r->mc3d_mults = (double**) malloc(sizeof(double*) * (num_species + num_params));
                int mult_idx = 0;
                for (int row = 0; row < num_species + num_params; row++) {
                    r->mc3d_mults[row] = (double*) malloc(sizeof(double) * mc3d_region_size);
                    for (int c = 0; c < mc3d_region_size; c++, mult_idx++) {
                        r->mc3d_mults[row][c] = mc3d_mults[mult_idx];
                    }
                }


            } else if (subregion == NULL) {
                r->subregion = NULL;
                r->region_size = grid->size_x * grid->size_y * grid->size_z;
                r->mc3d_indices_offsets = NULL;
            } else {
                for (r->region_size = 0, j = 0; j < grid->size_x * grid->size_y * grid->size_z; j++)
                    r->region_size += subregion[j];
                r->subregion = subregion;
                r->mc3d_indices_offsets = NULL;
            }
        }
    }

    r->num_species_involved = num_species;
    r->num_params_involved = num_params;
    r->species_states = (double**) malloc(sizeof(Grid_node*) * (num_species + num_params));
    assert(r->species_states);

    for (i = 0; i < num_species + num_params; i++) {
        /*Assumes grids are the same size (no interpolation)
         *Assumes all the species will be in the same Parallel_grids list*/
        for (grid = Parallel_grids[list_idx], j = 0; grid != NULL; grid = grid->next, j++) {
            if (j == species_ids[i])
                r->species_states[i] = grid->states;
        }
    }

    return r;
}

/*ics_register_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * ECSReactionRate - the reaction function
 */
extern "C" void ics_register_reaction(int list_idx,
                                      int num_species,
                                      int num_params,
                                      int* species_id,
                                      uint64_t* mc3d_start_indices,
                                      int mc3d_region_size,
                                      double* mc3d_mults,
                                      ECSReactionRate f) {
    ecs_create_reaction(list_idx,
                        num_species,
                        num_params,
                        species_id,
                        f,
                        NULL,
                        mc3d_start_indices,
                        mc3d_region_size,
                        mc3d_mults);
    ecs_refresh_reactions(NUM_THREADS);
}

/*ecs_register_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * ECSReactionRate - the reaction function
 */
extern "C" void ecs_register_reaction(int list_idx,
                                      int num_species,
                                      int num_params,
                                      int* species_id,
                                      ECSReactionRate f) {
    ecs_create_reaction(list_idx, num_species, num_params, species_id, f, NULL, NULL, 0, NULL);
    ecs_refresh_reactions(NUM_THREADS);
}


/*register_subregion_reaction is called from python it creates a reaction with
 * list_idx - the index for the linked list in the global array Parallel_grids
 * 	(currently this is always 0)
 * grid_id - the grid id within the linked list - this corresponds to species
 * my_subregion - a boolean array indicating the voxels where a reaction occurs
 * ECSReactionRate - the reaction function
 */
void ecs_register_subregion_reaction_ecs(int list_idx,
                                         int num_species,
                                         int num_params,
                                         int* species_id,
                                         unsigned char* my_subregion,
                                         ECSReactionRate f) {
    ecs_create_reaction(
        list_idx, num_species, num_params, species_id, f, my_subregion, NULL, 0, NULL);
    ecs_refresh_reactions(NUM_THREADS);
}


/*create_threaded_reactions is used to generate evenly distribution of reactions
 * across NUM_THREADS
 * returns ReactGridData* which can be used as the global
 * 	threaded_reactions_tasks
 */
ReactGridData* create_threaded_reactions(const int n) {
    unsigned int i;
    int load, k;
    int react_count = 0;
    Reaction* react;

    for (react = ecs_reactions; react != NULL; react = react->next)
        react_count += react->region_size;

    if (react_count == 0)
        return NULL;

    /*Divide the number of reactions between the threads*/
    const int tasks_per_thread = (react_count) / n;
    ReactGridData* tasks = (ReactGridData*) calloc(sizeof(ReactGridData), n);
    const int extra = react_count % n;

    tasks[0].onset = (ReactSet*) malloc(sizeof(ReactSet));
    tasks[0].onset->reaction = ecs_reactions;
    tasks[0].onset->idx = 0;

    for (k = 0, load = 0, react = ecs_reactions; react != NULL; react = react->next) {
        for (i = 0; i < react->region_size; i++) {
            if (!react->subregion || react->subregion[i])
                load++;
            if (load >= tasks_per_thread + (extra > k)) {
                tasks[k].offset = (ReactSet*) malloc(sizeof(ReactSet));
                tasks[k].offset->reaction = react;
                tasks[k].offset->idx = i;

                if (++k < n) {
                    tasks[k].onset = (ReactSet*) malloc(sizeof(ReactSet));
                    tasks[k].onset->reaction = react;
                    tasks[k].onset->idx = i + 1;
                    load = 0;
                }
            }
            if (k == n - 1 && react->next == NULL) {
                tasks[k].offset = (ReactSet*) malloc(sizeof(ReactSet));
                tasks[k].offset->reaction = react;
                tasks[k].offset->idx = i;
            }
        }
    }
    return tasks;
}

/*ecs_do_reactions takes ReactGridData which defines the set of reactions to be
 * performed. It calculate the reaction based on grid->old_states and updates
 * grid->states
 */
void* ecs_do_reactions(void* dataptr) {
    ReactGridData task = *(ReactGridData*) dataptr;
    unsigned char started = FALSE, stop = FALSE;
    unsigned int i, j, k, n, start_idx, stop_idx, offset_idx;
    double temp, ge_value, val_to_set;
    double dt = *dt_ptr;
    Reaction* react;

    double* states_cache = NULL;
    double* params_cache = NULL;
    double* states_cache_dx = NULL;
    double* results_array = NULL;
    double* results_array_dx = NULL;
    double* mc_mults_array = NULL;
    double dx = FLT_EPSILON;
    double pd;
    std::unique_ptr<OcFullMatrix> jacobian;
    std::vector<double> x{};
    std::vector<double> b{};

    for (react = ecs_reactions; react != NULL; react = react->next) {
        // TODO: This is bad. Need to refactor
        if (react->mc3d_indices_offsets != NULL) {
            /*find start location for this thread*/
            if (started || react == task.onset->reaction) {
                if (!started) {
                    started = TRUE;
                    start_idx = task.onset->idx;
                } else {
                    start_idx = 0;
                }

                if (task.offset->reaction == react) {
                    stop_idx = task.offset->idx;
                    stop = TRUE;
                } else {
                    stop_idx = react->region_size - 1;
                    stop = FALSE;
                }
                if (react->num_species_involved == 0)
                    continue;
                /*allocate data structures*/
                jacobian = std::make_unique<OcFullMatrix>(react->num_species_involved,
                                                          react->num_species_involved);
                b.resize(react->num_species_involved);
                x.resize(react->num_species_involved);
                states_cache = (double*) malloc(sizeof(double) * react->num_species_involved);
                params_cache = (double*) malloc(sizeof(double) * react->num_params_involved);
                states_cache_dx = (double*) malloc(sizeof(double) * react->num_species_involved);
                results_array = (double*) malloc(react->num_species_involved * sizeof(double));
                results_array_dx = (double*) malloc(react->num_species_involved * sizeof(double));
                mc_mults_array = (double*) malloc(react->num_species_involved * sizeof(double));
                for (i = start_idx; i <= stop_idx; i++) {
                    if (!react->subregion || react->subregion[i]) {
                        // TODO: this assumes grids are the same size/shape
                        //      add interpolation in case they aren't
                        for (j = 0; j < react->num_species_involved; j++) {
                            offset_idx = i + react->mc3d_indices_offsets[j];
                            states_cache[j] = react->species_states[j][offset_idx];
                            states_cache_dx[j] = react->species_states[j][offset_idx];
                            mc_mults_array[j] = react->mc3d_mults[j][i];
                        }
                        memset(results_array, 0, react->num_species_involved * sizeof(double));
                        for (k = 0; j < react->num_species_involved + react->num_params_involved;
                             k++, j++) {
                            offset_idx = i + react->mc3d_indices_offsets[j];
                            params_cache[k] = react->species_states[j][offset_idx];
                            mc_mults_array[k] = react->mc3d_mults[j][i];
                        }
                        react->reaction(states_cache, params_cache, results_array, mc_mults_array);

                        for (j = 0; j < react->num_species_involved; j++) {
                            states_cache_dx[j] += dx;
                            memset(results_array_dx,
                                   0,
                                   react->num_species_involved * sizeof(double));
                            react->reaction(states_cache_dx,
                                            params_cache,
                                            results_array_dx,
                                            mc_mults_array);
                            b[j] = dt * results_array[j];

                            for (k = 0; k < react->num_species_involved; k++) {
                                pd = (results_array_dx[k] - results_array[k]) / dx;
                                *jacobian->mep(k, j) = (j == k) - dt * pd;
                            }
                            states_cache_dx[j] -= dx;
                        }
                        // solve for x
                        if (react->num_species_involved == 1) {
                            react->species_states[0][i] += b[0] / jacobian->getval(0, 0);
                        } else {
                            // find entry in leftmost column with largest absolute value
                            // Pivot
                            for (j = 0; j < react->num_species_involved; j++) {
                                for (k = j + 1; k < react->num_species_involved; k++) {
                                    if (abs(jacobian->getval(j, j)) < abs(jacobian->getval(k, j))) {
                                        for (n = 0; n < react->num_species_involved; n++) {
                                            temp = jacobian->getval(j, n);
                                            *jacobian->mep(j, n) = jacobian->getval(k, n);
                                            *jacobian->mep(k, n) = temp;
                                        }
                                    }
                                }
                            }

                            for (j = 0; j < react->num_species_involved - 1; j++) {
                                for (k = j + 1; k < react->num_species_involved; k++) {
                                    ge_value = jacobian->getval(k, j) / jacobian->getval(j, j);
                                    for (n = 0; n < react->num_species_involved; n++) {
                                        val_to_set = jacobian->getval(k, n) -
                                                     ge_value * jacobian->getval(j, n);
                                        *jacobian->mep(k, n) = val_to_set;
                                    }
                                    b[k] = b[k] - ge_value * b[j];
                                }
                            }

                            for (j = react->num_species_involved - 1; j + 1 > 0; j--) {
                                x[j] = b[j];
                                for (k = j + 1; k < react->num_species_involved; k++) {
                                    if (k != j) {
                                        x[j] = x[j] - jacobian->getval(j, k) * x[k];
                                    }
                                }
                                x[j] = x[j] / jacobian->getval(j, j);
                            }
                            for (j = 0; j < react->num_species_involved; j++) {
                                // I think this should be something like
                                // react->species_states[j][mc3d_indices[i]] += v_get_val(x,j);
                                // Since the grid has a uniform discretization, the mc3d_indices
                                // should be the same length. So just need to access the correct
                                // mc3d_indices[i] maybe do two lines?: index =
                                // react->species_indices[j][i] react->species_states[j][index] +=
                                // v_get_val(x,j);
                                offset_idx = i + react->mc3d_indices_offsets[j];
                                react->species_states[j][offset_idx] += x[j];
                            }
                        }
                    }
                }

                SAFE_FREE(states_cache);
                SAFE_FREE(states_cache_dx);
                SAFE_FREE(params_cache);
                SAFE_FREE(results_array);
                SAFE_FREE(results_array_dx);
                SAFE_FREE(mc_mults_array);

                if (stop)
                    return NULL;
            }
        } else {
            /*find start location for this thread*/
            if (started || react == task.onset->reaction) {
                if (!started) {
                    started = TRUE;
                    start_idx = task.onset->idx;
                } else {
                    start_idx = 0;
                }

                if (task.offset->reaction == react) {
                    stop_idx = task.offset->idx;
                    stop = TRUE;
                } else {
                    stop_idx = react->region_size - 1;
                    stop = FALSE;
                }
                if (react->num_species_involved == 0)
                    continue;
                /*allocate data structures*/
                jacobian = std::make_unique<OcFullMatrix>(react->num_species_involved,
                                                          react->num_species_involved);
                b.resize(react->num_species_involved);
                x.resize(react->num_species_involved);
                states_cache = (double*) malloc(sizeof(double) * react->num_species_involved);
                params_cache = (double*) malloc(sizeof(double) * react->num_params_involved);
                states_cache_dx = (double*) malloc(sizeof(double) * react->num_species_involved);
                results_array = (double*) malloc(react->num_species_involved * sizeof(double));
                results_array_dx = (double*) malloc(react->num_species_involved * sizeof(double));
                for (i = start_idx; i <= stop_idx; i++) {
                    if (!react->subregion || react->subregion[i]) {
                        // TODO: this assumes grids are the same size/shape
                        //      add interpolation in case they aren't
                        for (j = 0; j < react->num_species_involved; j++) {
                            states_cache[j] = react->species_states[j][i];
                            states_cache_dx[j] = react->species_states[j][i];
                        }
                        memset(results_array, 0, react->num_species_involved * sizeof(double));
                        for (k = 0; j < react->num_species_involved + react->num_params_involved;
                             k++, j++) {
                            params_cache[k] = react->species_states[j][i];
                        }
                        react->reaction(states_cache, params_cache, results_array, NULL);

                        for (j = 0; j < react->num_species_involved; j++) {
                            states_cache_dx[j] += dx;
                            memset(results_array_dx,
                                   0,
                                   react->num_species_involved * sizeof(double));
                            react->reaction(states_cache_dx, params_cache, results_array_dx, NULL);
                            b[j] = dt * results_array[j];

                            for (k = 0; k < react->num_species_involved; k++) {
                                pd = (results_array_dx[k] - results_array[k]) / dx;
                                *jacobian->mep(k, j) = (j == k) - dt * pd;
                            }
                            states_cache_dx[j] -= dx;
                        }
                        // solve for x
                        if (react->num_species_involved == 1) {
                            react->species_states[0][i] += b[0] / jacobian->getval(0, 0);
                        } else {
                            // find entry in leftmost column with largest absolute value
                            // Pivot
                            for (j = 0; j < react->num_species_involved; j++) {
                                for (k = j + 1; k < react->num_species_involved; k++) {
                                    if (abs(jacobian->getval(j, j)) < abs(jacobian->getval(k, j))) {
                                        for (n = 0; n < react->num_species_involved; n++) {
                                            temp = jacobian->getval(j, n);
                                            *jacobian->mep(j, n) = jacobian->getval(k, n);
                                            *jacobian->mep(k, n) = temp;
                                        }
                                    }
                                }
                            }

                            for (j = 0; j < react->num_species_involved - 1; j++) {
                                for (k = j + 1; k < react->num_species_involved; k++) {
                                    ge_value = jacobian->getval(k, j) / jacobian->getval(j, j);
                                    for (n = 0; n < react->num_species_involved; n++) {
                                        val_to_set = jacobian->getval(k, n) -
                                                     ge_value * jacobian->getval(j, n);
                                        *jacobian->mep(k, n) = val_to_set;
                                    }
                                    b[k] = b[k] - ge_value * b[j];
                                }
                            }

                            for (j = react->num_species_involved - 1; j + 1 > 0; j--) {
                                x[j] = b[j];
                                for (k = j + 1; k < react->num_species_involved; k++) {
                                    if (k != j) {
                                        x[j] = x[j] - jacobian->getval(j, k) * x[k];
                                    }
                                }
                                x[j] = x[j] / jacobian->getval(j, j);
                            }
                            for (j = 0; j < react->num_species_involved; j++) {
                                // I think this should be something like
                                // react->species_states[j][mc3d_indices[i]] += x[j];
                                // Since the grid has a uniform discretization, the mc3d_indices
                                // should be the same length. So just need to access the correct
                                // mc3d_indices[i] maybe do two lines?: index =
                                // react->species_indices[j][i] react->species_states[j][index] +=
                                // x[j];
                                react->species_states[j][i] += x[j];
                            }
                        }
                    }
                }

                SAFE_FREE(states_cache);
                SAFE_FREE(states_cache_dx);
                SAFE_FREE(params_cache);
                SAFE_FREE(results_array);
                SAFE_FREE(results_array_dx);

                if (stop)
                    return NULL;
            }
        }
    }
    return NULL;
}

/* run_threaded_reactions
 * Array ReactGridData tasks length NUM_THREADS and calls ecs_do_reactions and
 * executes each task with a separate thread
 */
static void run_threaded_reactions(ReactGridData* tasks) {
    int k;
    /* launch threads */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        TaskQueue_add_task(AllTasks, &ecs_do_reactions, &tasks[k], NULL);
    }
    /* run one task in the main thread */
    ecs_do_reactions(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}

void _fadvance_fixed_step_3D(void) {
    Grid_node* grid;
    ECS_Grid_node* g;
    double dt = (*dt_ptr);

    /*Currents to broadcast via MPI*/
    /*TODO: Handle multiple grids with one pass*/
    /*Maybe TODO: Should check #currents << #voxels and not the other way round*/
    int id;

    if (threaded_reactions_tasks != NULL)
        run_threaded_reactions(threaded_reactions_tasks);

    for (id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid->next, id++) {
        memset(grid->states_cur, 0, sizeof(double) * grid->size_x * grid->size_y * grid->size_z);
        g = dynamic_cast<ECS_Grid_node*>(grid);
        if (g)
            g->do_multicompartment_reactions(NULL);
        grid->do_grid_currents(grid->states_cur, dt, id);
        grid->apply_node_flux3D(dt, NULL);

        // grid->volume_setup();
        if (grid->hybrid) {
            grid->hybrid_connections();
        }
        grid->dg_adi();
    }
    /* transfer concentrations */
    scatter_concentrations();
}

extern "C" void scatter_concentrations(void) {
    /* transfer concentrations to classic NEURON */
    Grid_node* grid;

    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid->scatter_grid_concentrations();
    }
}

/*****************************************************************************
 *
 * Begin variable step code
 *
 *****************************************************************************/

/* count the total number of state variables AND store their offset (passed in) in the cvode vector
 */
int ode_count(const int offset) {
    int count = 0;
    states_cvode_offset = offset;
    Grid_node* grid;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        count += grid->size_x * grid->size_y * grid->size_z;
    }
    return count;
}


void ecs_atolscale(double* y) {
    Grid_node* grid;
    ssize_t i;
    int grid_size;
    y += states_cvode_offset;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid_size = grid->size_x * grid->size_y * grid->size_z;
        for (i = 0; i < grid_size; i++) {
            y[i] *= grid->atolscale;
        }
        y += grid_size;
    }
}

void _ecs_ode_reinit(double* y) {
    Grid_node* grid;
    ECS_Grid_node* g;

    ssize_t i;
    int grid_size;
    double* grid_states;
    y += states_cvode_offset;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid_states = grid->states;
        grid_size = grid->size_x * grid->size_y * grid->size_z;

        for (i = 0; i < grid_size; i++) {
            y[i] = grid_states[i];
        }
        y += grid_size;
        g = dynamic_cast<ECS_Grid_node*>(grid);
        if (g)
            g->initialize_multicompartment_reaction();
    }
}


void _rhs_variable_step_ecs(const double* states, double* ydot) {
    Grid_node* grid;
    ECS_Grid_node* g;
    ssize_t i;
    int grid_size;
    double dt = *dt_ptr;
    double* grid_states;
    double* const orig_ydot = ydot + states_cvode_offset;
    double const* const orig_states = states + states_cvode_offset;
    const unsigned char calculate_rhs = ydot == NULL ? 0 : 1;
    states = orig_states;
    ydot = orig_ydot;
    /* prepare for advance by syncing data with local copy */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
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
    if (threaded_reactions_tasks != NULL) {
        run_threaded_reactions(threaded_reactions_tasks);
    }
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid_states = grid->states;
        grid_size = grid->size_x * grid->size_y * grid->size_z;
        for (i = 0; i < grid_size; i++) {
            ydot[i] += (grid_states[i] - states[i]) / dt;
            grid_states[i] = states[i];
        }
        states += grid_size;
        ydot += grid_size;
    }

    ydot = orig_ydot;
    states = orig_states;
    /* process currents */
    for (i = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid->next, i++) {
        g = dynamic_cast<ECS_Grid_node*>(grid);
        if (g)
            g->do_multicompartment_reactions(ydot);
        grid->do_grid_currents(ydot, 1.0, i);

        /*Add node fluxes to the result*/
        grid->apply_node_flux3D(1.0, ydot);

        ydot += grid_size;
    }
    ydot = orig_ydot;

    /* do the diffusion rates */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid_size = grid->size_x * grid->size_y * grid->size_z;
        grid->variable_step_diffusion(states, ydot);

        ydot += grid_size;
        states += grid_size;
    }
}


void _rhs_variable_step_helper(Grid_node* g, double const* const states, double* ydot) {
    int num_states_x = g->size_x, num_states_y = g->size_y, num_states_z = g->size_z;
    double dx = g->dx, dy = g->dy, dz = g->dz;
    int i, j, k, stop_i, stop_j, stop_k;
    double dc_x = g->dc_x, dc_y = g->dc_y, dc_z = g->dc_z;

    double rate_x = dc_x / (dx * dx);
    double rate_y = dc_y / (dy * dy);
    double rate_z = dc_z / (dz * dz);

    /*indices*/
    int index, prev_i, prev_j, prev_k, next_i, next_j, next_k;
    double div_x, div_y, div_z;

    /* Euler advance x, y, z (all points) */
    stop_i = num_states_x - 1;
    stop_j = num_states_y - 1;
    stop_k = num_states_z - 1;
    if (g->bc->type == NEUMANN) {
        for (i = 0,
            index = 0,
            prev_i = num_states_y * num_states_z,
            next_i = num_states_y * num_states_z;
             i < num_states_x;
             i++) {
            /*Zero flux boundary conditions*/
            div_x = (i == 0 || i == stop_i) ? 2. : 1.;

            for (j = 0, prev_j = index + num_states_z, next_j = index + num_states_z;
                 j < num_states_y;
                 j++) {
                div_y = (j == 0 || j == stop_j) ? 2. : 1.;

                for (k = 0, prev_k = index + 1, next_k = index + 1; k < num_states_z;
                     k++, index++, prev_i++, next_i++, prev_j++, next_j++) {
                    div_z = (k == 0 || k == stop_k) ? 2. : 1.;

                    if (stop_i > 0) {
                        ydot[index] += rate_x *
                                       (states[prev_i] - 2.0 * states[index] + states[next_i]) /
                                       div_x;
                    }
                    if (stop_j > 0) {
                        ydot[index] += rate_y *
                                       (states[prev_j] - 2.0 * states[index] + states[next_j]) /
                                       div_y;
                    }
                    if (stop_k > 0) {
                        ydot[index] += rate_z *
                                       (states[prev_k] - 2.0 * states[index] + states[next_k]) /
                                       div_z;
                    }
                    next_k = (k == stop_k - 1) ? index : index + 2;
                    prev_k = index;
                }
                prev_j = index - num_states_z;
                next_j = (j == stop_j - 1) ? prev_j : index + num_states_z;
            }
            prev_i = index - num_states_y * num_states_z;
            next_i = (i == stop_i - 1) ? prev_i : index + num_states_y * num_states_z;
        }
    } else {
        for (i = 0, index = 0, prev_i = 0, next_i = num_states_y * num_states_z; i < num_states_x;
             i++) {
            for (j = 0, prev_j = index - num_states_z, next_j = index + num_states_z;
                 j < num_states_y;
                 j++) {
                for (k = 0, prev_k = index - 1, next_k = index + 1; k < num_states_z;
                     k++, index++, prev_i++, next_i++, prev_j++, next_j++, next_k++, prev_k++) {
                    if (i == 0 || i == stop_i || j == 0 || j == stop_j || k == 0 || k == stop_k) {
                        // set to zero to prevent currents altering concentrations at the boundary
                        ydot[index] = 0;
                    } else {
                        ydot[index] += rate_x *
                                       (states[prev_i] - 2.0 * states[index] + states[next_i]);

                        ydot[index] += rate_y *
                                       (states[prev_j] - 2.0 * states[index] + states[next_j]);

                        ydot[index] += rate_z *
                                       (states[prev_k] - 2.0 * states[index] + states[next_k]);
                    }
                }
                prev_j = index - num_states_z;
                next_j = index + num_states_z;
            }
            prev_i = index - num_states_y * num_states_z;
            next_i = index + num_states_y * num_states_z;
        }
    }
    /*
            for (i = 1, index = (num_states_y+1)*num_states_z + 1,
                 prev_i = num_states_z + 1, next_i = (2*num_states_y+1)*num_states_z + 1;
                 i < stop_i; i++) {
                for (j = 1, prev_j = index - num_states_z, next_j = index + num_states_z; j <
       stop_j; j++) {

                    for(k = 1, prev_k = index - 1, next_k = index + 1; k < stop_k;
                        k++, index++, prev_i++, next_i++, prev_j++, next_j++, next_k++) {
                        if(index != i*num_states_y*num_states_z + j*num_states_z + k)
                        {
                            fprintf(stderr,"%i \t %i %i %i\n", index,i,j,k);
                        }
                        if(prev_i != (i-1)*num_states_y*num_states_z + j*num_states_z + k)
                        {
                            fprintf(stderr,"prev_i %i %i \t %i %i %i\n",
       prev_i,(i-1)*num_states_y*num_states_z + j*num_states_z + k,i,j,k);
                        }
                        if(next_i != (i+1)*num_states_y*num_states_z + j*num_states_z + k)
                        {
                            fprintf(stderr,"next_i %i %i \t %i %i %i\n",
       next_i,(i+1)*num_states_y*num_states_z + j*num_states_z + k,i,j,k);
                        }
                        if(prev_j != i*num_states_y*num_states_z + (j-1)*num_states_z + k)
                        {
                            fprintf(stderr,"prev_j %i %i \t %i %i %i\n",
       prev_j,i*num_states_y*num_states_z + (j-1)*num_states_z + k,i,j,k);
                        }
                        if(next_j != i*num_states_y*num_states_z + (j+1)*num_states_z + k)
                        {
                            fprintf(stderr,"next_j %i %i \t %i %i %i\n",
       next_j,i*num_states_y*num_states_z + (j+1)*num_states_z + k,i,j,k);
                        }
                        if(prev_k != i*num_states_y*num_states_z + j*num_states_z + k-1)
                        {
                            fprintf(stderr,"prev_k %i %i \t %i %i %i\n",
       prev_k,i*num_states_y*num_states_z + j*num_states_z + k-1,i,j,k);
                        }
                        if(next_k != i*num_states_y*num_states_z + j*num_states_z + k+1)
                        {
                            fprintf(stderr,"next_k %i %i \t %i %i %i\n",
       next_k,i*num_states_y*num_states_z + j*num_states_z + k+1,i,j,k);
                        }

                        ydot[index] += rate_x * (states[prev_i] -
                            2.0 * states[index] + states[next_i]);

                        ydot[index] += rate_y * (states[prev_j] -
                            2.0 * states[index] + states[next_j]);

                        ydot[index] += rate_z * (states[prev_k] -
                            2.0 * states[index] + states[next_k]);

                    }
                    index += 2; //skip z boundaries
                    //prev_j = index - num_states_z;
                    //next_j = index + num_states_z;
                }
                index += 2*num_states_z; //skip y boundaries
                //prev_i = index - num_states_y*num_states_z;
                //next_i = index + num_states_y*num_states_z;
            }
        }
        */
}

// p1 = b  p2 = states
void ics_ode_solve(double dt, double* RHS, const double* states) {
    Grid_node* grid;
    ssize_t i;
    int grid_size = 0;
    double* grid_states;
    double const* const orig_states = states + states_cvode_offset;
    const unsigned char calculate_rhs = RHS == NULL ? 0 : 1;
    double* const orig_RHS = RHS + states_cvode_offset;
    states = orig_states;
    RHS = orig_RHS;

    /* prepare for advance by syncing data with local copy */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
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
    RHS = orig_RHS;

    /* TODO: reactions contribute to adaptive step-size*/
    if (threaded_reactions_tasks != NULL) {
        run_threaded_reactions(threaded_reactions_tasks);
    }
    /* do the diffusion rates */
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid->variable_step_ode_solve(RHS, dt);
        RHS += grid_size;
        states += grid_size;
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
static int solve_dd_clhs_tridiag(const int N,
                                 const double l_diag,
                                 const double diag,
                                 const double u_diag,
                                 const double lbc_diag,
                                 const double lbc_u_diag,
                                 const double ubc_l_diag,
                                 const double ubc_diag,
                                 double* const b,
                                 double* const c) {
    int i;


    c[0] = lbc_u_diag / lbc_diag;
    b[0] = b[0] / lbc_diag;

    for (i = 1; i < N - 1; i++) {
        c[i] = u_diag / (diag - l_diag * c[i - 1]);
        b[i] = (b[i] - l_diag * b[i - 1]) / (diag - l_diag * c[i - 1]);
    }
    b[N - 1] = (b[N - 1] - ubc_l_diag * b[N - 2]) / (ubc_diag - ubc_l_diag * c[N - 2]);

    /*back substitution*/
    for (i = N - 2; i >= 0; i--) {
        b[i] = b[i] - c[i] * b[i + 1];
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
        return solve_dd_clhs_tridiag_for(N, l_diag, diag, u_diag, lbc_diag, lbc_u_diag, ubc_l_diag,
ubc_diag, b, c);
    }
    else
    {
        callcount = TRUE;
        return solve_dd_clhs_tridiag_rev(N, l_diag, diag, u_diag, lbc_diag, lbc_u_diag, ubc_l_diag,
ubc_diag, b, c);
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
static void ecs_dg_adi_x(ECS_Grid_node* g,
                         const double dt,
                         const int y,
                         const int z,
                         double const* const state,
                         double* const RHS,
                         double* const scratch) {
    int yp, ym, zp, zm;
    int x;
    double r = g->dc_x * dt / SQ(g->dx);
    double div_y, div_z;
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (y == 0 || z == 0 || y == g->size_y - 1 || z == g->size_z - 1)) {
        for (x = 0; x < g->size_x; x++)
            RHS[x] = g->bc->value;
        return;
    }

    if (g->size_y > 1) {
        yp = (y == g->size_y - 1) ? y - 1 : y + 1;
        ym = (y == 0) ? y + 1 : y - 1;
        div_y = (y == 0 || y == g->size_y - 1) ? 2. : 1.;
    } else {
        yp = 0;
        ym = 0;
        div_y = 1;
    }
    if (g->size_z > 1) {
        zp = (z == g->size_z - 1) ? z - 1 : z + 1;
        zm = (z == 0) ? z + 1 : z - 1;
        div_z = (z == 0 || z == g->size_z - 1) ? 2. : 1.;
    } else {
        zp = 0;
        zm = 0;
        div_z = 1;
    }

    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        RHS[0] = state[IDX(0, y, z)] + g->states_cur[IDX(0, y, z)] +
                 dt *
                     ((g->dc_y / SQ(g->dy)) *
                          (state[IDX(0, yp, z)] - 2. * state[IDX(0, y, z)] + state[IDX(0, ym, z)]) /
                          div_y +
                      (g->dc_z / SQ(g->dz)) *
                          (state[IDX(0, y, zp)] - 2. * state[IDX(0, y, z)] + state[IDX(0, y, zm)]) /
                          div_z);
        if (g->size_x > 1) {
            RHS[0] += dt * (g->dc_x / SQ(g->dx)) * (state[IDX(1, y, z)] - state[IDX(0, y, z)]);
            x = g->size_x - 1;
            RHS[x] =
                state[IDX(x, y, z)] + g->states_cur[IDX(x, y, z)] +
                dt * ((g->dc_x / SQ(g->dx)) * (state[IDX(x - 1, y, z)] - state[IDX(x, y, z)]) +
                      (g->dc_y / SQ(g->dy)) *
                          (state[IDX(x, yp, z)] - 2. * state[IDX(x, y, z)] + state[IDX(x, ym, z)]) /
                          div_y +
                      (g->dc_z / SQ(g->dz)) *
                          (state[IDX(x, y, zp)] - 2. * state[IDX(x, y, z)] + state[IDX(x, y, zm)]) /
                          div_z);
        }
    } else {
        RHS[0] = g->bc->value;
        RHS[g->size_x - 1] = g->bc->value;
    }
    for (x = 1; x < g->size_x - 1; x++) {
#ifndef __PGI
        __builtin_prefetch(&(state[IDX(x + PREFETCH, y, z)]), 0, 1);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, yp, z)]), 0, 0);
        __builtin_prefetch(&(state[IDX(x + PREFETCH, ym, z)]), 0, 0);
#endif
        RHS[x] = state[IDX(x, y, z)] +
                 dt *
                     ((g->dc_x / SQ(g->dx)) *
                          (state[IDX(x + 1, y, z)] - 2. * state[IDX(x, y, z)] +
                           state[IDX(x - 1, y, z)]) /
                          2. +
                      (g->dc_y / SQ(g->dy)) *
                          (state[IDX(x, yp, z)] - 2. * state[IDX(x, y, z)] + state[IDX(x, ym, z)]) /
                          div_y +
                      (g->dc_z / SQ(g->dz)) *
                          (state[IDX(x, y, zp)] - 2. * state[IDX(x, y, z)] + state[IDX(x, y, zm)]) /
                          div_z) +
                 g->states_cur[IDX(x, y, z)];
    }
    if (g->size_x > 1) {
        if (g->bc->type == NEUMANN)
            solve_dd_clhs_tridiag(g->size_x,
                                  -r / 2.0,
                                  1.0 + r,
                                  -r / 2.0,
                                  1.0 + r / 2.0,
                                  -r / 2.0,
                                  -r / 2.0,
                                  1.0 + r / 2.0,
                                  RHS,
                                  scratch);
        else
            solve_dd_clhs_tridiag(
                g->size_x, -r / 2.0, 1.0 + r, -r / 2.0, 1.0, 0, 0, 1.0, RHS, scratch);
    }
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
static void ecs_dg_adi_y(ECS_Grid_node* g,
                         double const dt,
                         int const x,
                         int const z,
                         double const* const state,
                         double* const RHS,
                         double* const scratch) {
    int y;
    double r = (g->dc_y * dt / SQ(g->dy));
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (x == 0 || z == 0 || x == g->size_x - 1 || z == g->size_z - 1)) {
        for (y = 0; y < g->size_y; y++)
            RHS[y] = g->bc->value;
        return;
    }
    if (g->size_y == 1) {
        if (g->bc->type == NEUMANN)
            RHS[0] = state[x + z * g->size_x];
        else
            RHS[0] = g->bc->value;
        return;
    }
    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        RHS[0] = state[x + z * g->size_x] -
                 (g->dc_y * dt / SQ(g->dy)) *
                     (g->states[IDX(x, 1, z)] - 2.0 * g->states[IDX(x, 0, z)] +
                      g->states[IDX(x, 1, z)]) /
                     4.0;
        y = g->size_y - 1;
        RHS[y] = state[x + (z + y * g->size_z) * g->size_x] -
                 (g->dc_y * dt / SQ(g->dy)) *
                     (g->states[IDX(x, y - 1, z)] - 2. * g->states[IDX(x, y, z)] +
                      g->states[IDX(x, y - 1, z)]) /
                     4.0;
    } else {
        RHS[0] = g->bc->value;
        RHS[g->size_y - 1] = g->bc->value;
    }
    for (y = 1; y < g->size_y - 1; y++) {
#ifndef __PGI
        __builtin_prefetch(&state[x + (z + (y + PREFETCH) * g->size_z) * g->size_x], 0, 0);
        __builtin_prefetch(&(g->states[IDX(x, y + PREFETCH, z)]), 0, 1);
#endif
        RHS[y] = state[x + (z + y * g->size_z) * g->size_x] -
                 (g->dc_y * dt / SQ(g->dy)) *
                     (g->states[IDX(x, y + 1, z)] - 2. * g->states[IDX(x, y, z)] +
                      g->states[IDX(x, y - 1, z)]) /
                     2.0;
    }
    if (g->bc->type == NEUMANN)
        solve_dd_clhs_tridiag(g->size_y,
                              -r / 2.0,
                              1.0 + r,
                              -r / 2.0,
                              1.0 + r / 2.0,
                              -r / 2.0,
                              -r / 2.0,
                              1.0 + r / 2.0,
                              RHS,
                              scratch);
    else
        solve_dd_clhs_tridiag(g->size_y, -r / 2., 1. + r, -r / 2., 1.0, 0, 0, 1.0, RHS, scratch);
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
static void ecs_dg_adi_z(ECS_Grid_node* g,
                         double const dt,
                         int const x,
                         int const y,
                         double const* const state,
                         double* const RHS,
                         double* const scratch) {
    int z;
    double r = g->dc_z * dt / SQ(g->dz);
    /*TODO: Get rid of this by not calling dg_adi when on the boundary for DIRICHLET conditions*/
    if (g->bc->type == DIRICHLET &&
        (x == 0 || y == 0 || x == g->size_x - 1 || y == g->size_y - 1)) {
        for (z = 0; z < g->size_z; z++)
            RHS[z] = g->bc->value;
        return;
    }

    if (g->size_z == 1) {
        if (g->bc->type == NEUMANN)
            RHS[0] = state[y + g->size_y * x];
        else
            RHS[0] = g->bc->value;
        return;
    }

    if (g->bc->type == NEUMANN) {
        /*zero flux boundary condition*/
        RHS[0] = state[y + g->size_y * (x * g->size_z)] -
                 (g->dc_z * dt / SQ(g->dz)) *
                     (g->states[IDX(x, y, 1)] - 2.0 * g->states[IDX(x, y, 0)] +
                      g->states[IDX(x, y, 1)]) /
                     4.0;
        z = g->size_z - 1;
        RHS[z] = state[y + g->size_y * (x * g->size_z + z)] -
                 (g->dc_z * dt / SQ(g->dz)) *
                     (g->states[IDX(x, y, z - 1)] - 2.0 * g->states[IDX(x, y, z)] +
                      g->states[IDX(x, y, z - 1)]) /
                     4.0;

    } else {
        RHS[0] = g->bc->value;
        RHS[g->size_z - 1] = g->bc->value;
    }
    for (z = 1; z < g->size_z - 1; z++) {
        RHS[z] = state[y + g->size_y * (x * g->size_z + z)] -
                 (g->dc_z * dt / SQ(g->dz)) *
                     (g->states[IDX(x, y, z + 1)] - 2. * g->states[IDX(x, y, z)] +
                      g->states[IDX(x, y, z - 1)]) /
                     2.;
    }

    if (g->bc->type == NEUMANN)
        solve_dd_clhs_tridiag(g->size_z,
                              -r / 2.0,
                              1.0 + r,
                              -r / 2.0,
                              1.0 + r / 2.0,
                              -r / 2.0,
                              -r / 2.0,
                              1.0 + r / 2.0,
                              RHS,
                              scratch);
    else
        solve_dd_clhs_tridiag(g->size_z, -r / 2., 1. + r, -r / 2., 1.0, 0, 0, 1.0, RHS, scratch);
}

static void* ecs_do_dg_adi(void* dataptr) {
    ECSAdiGridData* data = (ECSAdiGridData*) dataptr;
    int start = data->start;
    int stop = data->stop;
    int i, j, k;
    ECSAdiDirection* ecs_adi_dir = data->ecs_adi_dir;
    double dt = *dt_ptr;
    int sizej = data->sizej;
    ECS_Grid_node* g = data->g;
    double* state_in = ecs_adi_dir->states_in;
    double* state_out = ecs_adi_dir->states_out;
    int offset = ecs_adi_dir->line_size;
    double* scratchpad = data->scratchpad;
    void (*ecs_dg_adi_dir)(
        ECS_Grid_node*, double, int, int, double const* const, double* const, double* const) =
        ecs_adi_dir->ecs_dg_adi_dir;
    for (k = start; k < stop; k++) {
        i = k / sizej;
        j = k % sizej;
        ecs_dg_adi_dir(g, dt, i, j, state_in, &state_out[k * offset], scratchpad);
    }

    return NULL;
}

void ecs_run_threaded_dg_adi(const int i,
                             const int j,
                             ECS_Grid_node* g,
                             ECSAdiDirection* ecs_adi_dir,
                             const int n) {
    int k;
    /* when doing any given direction, the number of tasks is the product of the other two, so
     * multiply everything then divide out the current direction */
    const int tasks_per_thread = (g->size_x * g->size_y * g->size_z / n) / NUM_THREADS;
    const int extra = (g->size_x * g->size_y * g->size_z / n) % NUM_THREADS;

    g->ecs_tasks[0].start = 0;
    g->ecs_tasks[0].stop = tasks_per_thread + (extra > 0);
    g->ecs_tasks[0].sizej = j;
    g->ecs_tasks[0].ecs_adi_dir = ecs_adi_dir;
    for (k = 1; k < NUM_THREADS; k++) {
        g->ecs_tasks[k].start = g->ecs_tasks[k - 1].stop;
        g->ecs_tasks[k].stop = g->ecs_tasks[k].start + tasks_per_thread + (extra > k);
        g->ecs_tasks[k].sizej = j;
        g->ecs_tasks[k].ecs_adi_dir = ecs_adi_dir;
    }
    g->ecs_tasks[NUM_THREADS - 1].stop = i * j;
    /* launch threads */
    for (k = 0; k < NUM_THREADS - 1; k++) {
        TaskQueue_add_task(AllTasks, &ecs_do_dg_adi, &(g->ecs_tasks[k]), NULL);
    }
    /* run one task in the main thread */
    ecs_do_dg_adi(&(g->ecs_tasks[NUM_THREADS - 1]));
    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}

void ecs_set_adi_homogeneous(ECS_Grid_node* g) {
    g->ecs_adi_dir_x->ecs_dg_adi_dir = ecs_dg_adi_x;
    g->ecs_adi_dir_y->ecs_dg_adi_dir = ecs_dg_adi_y;
    g->ecs_adi_dir_z->ecs_dg_adi_dir = ecs_dg_adi_z;
}
