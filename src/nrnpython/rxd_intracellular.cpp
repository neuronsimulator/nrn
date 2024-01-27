#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
#include <nrnwrap_Python.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cmath>

extern int NUM_THREADS;
extern TaskQueue* AllTasks;
extern double* states;
const int ICS_PREFETCH = 3;

/*
 * Sets the data to be used by the grids for 1D/3D hybrid models
 */
extern "C" void set_hybrid_data(int64_t* num_1d_indices_per_grid,
                                int64_t* num_3d_indices_per_grid,
                                int64_t* hybrid_indices1d,
                                int64_t* hybrid_indices3d,
                                int64_t* num_3d_indices_per_1d_seg,
                                int64_t* hybrid_grid_ids,
                                double* rates,
                                double* volumes1d,
                                double* volumes3d,
                                double* dxs) {
    Grid_node* grid;
    int i, j, k, id;
    int grid_id_check = 0;

    int index_ctr_1d = 0;
    int index_ctr_3d = 0;

    int num_grid_3d_indices;
    int num_grid_1d_indices;

    double dx;

    // loop over grids
    for (id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid->next, id++) {
        // if the grid we are on is the next grid in the hybrid grids
        if (id == hybrid_grid_ids[grid_id_check]) {
            num_grid_1d_indices = num_1d_indices_per_grid[grid_id_check];
            num_grid_3d_indices = num_3d_indices_per_grid[grid_id_check];
            // Allocate memory for hybrid data
            grid->hybrid = true;
            grid->hybrid_data->indices1d = (long*) malloc(sizeof(long) * num_grid_1d_indices);
            grid->hybrid_data->num_3d_indices_per_1d_seg = (long*) malloc(sizeof(long) *
                                                                          num_grid_1d_indices);
            grid->hybrid_data->volumes1d = (double*) malloc(sizeof(double) * num_grid_1d_indices);


            grid->hybrid_data->indices3d = (long*) malloc(sizeof(long) * num_grid_3d_indices);
            grid->hybrid_data->rates = (double*) malloc(sizeof(double) * num_grid_3d_indices);
            grid->hybrid_data->volumes3d = (double*) malloc(sizeof(double) * num_grid_3d_indices);

            dx = *dxs++;

            // Assign grid data
            grid->hybrid_data->num_1d_indices = num_grid_1d_indices;
            for (i = 0, k = 0; i < num_grid_1d_indices; i++, index_ctr_1d++) {
                grid->hybrid_data->indices1d[i] = hybrid_indices1d[index_ctr_1d];
                grid->hybrid_data->num_3d_indices_per_1d_seg[i] =
                    num_3d_indices_per_1d_seg[index_ctr_1d];
                grid->hybrid_data->volumes1d[i] = volumes1d[index_ctr_1d];

                for (j = 0; j < num_3d_indices_per_1d_seg[index_ctr_1d]; j++, index_ctr_3d++, k++) {
                    grid->hybrid_data->indices3d[k] = hybrid_indices3d[index_ctr_3d];
                    grid->hybrid_data->rates[k] = rates[index_ctr_3d];
                    grid->hybrid_data->volumes3d[k] = volumes3d[index_ctr_3d];
                    ((ICS_Grid_node*) grid)->_ics_alphas[hybrid_indices3d[index_ctr_3d]] =
                        volumes3d[index_ctr_3d] / dx;
                }
            }
            grid_id_check++;
        }
    }
}

/* solve Ax=b where A is a diagonally dominant tridiagonal matrix
 * N	-	length of the matrix
 * l_diag	-	pointer to the lower diagonal (length N-1)
 * diag	-	pointer to  diagonal	(length N)
 * u_diag	-	pointer to the upper diagonal (length N-1)
 * B	-	pointer to the RHS	(length N)
 * The solution (x) will be stored in B.
 * l_diag, diag and u_diag are not changed.
 * c	-	scratch pad, preallocated memory for (N-1) doubles
 */
static int solve_dd_tridiag(int N,
                            const double* l_diag,
                            const double* diag,
                            const double* u_diag,
                            double* b,
                            double* c) {
    int i;

    c[0] = u_diag[0] / diag[0];
    b[0] = b[0] / diag[0];

    for (i = 1; i < N - 1; i++) {
        c[i] = u_diag[i] / (diag[i] - l_diag[i - 1] * c[i - 1]);
        b[i] = (b[i] - l_diag[i - 1] * b[i - 1]) / (diag[i] - l_diag[i - 1] * c[i - 1]);
    }
    b[N - 1] = (b[N - 1] - l_diag[N - 2] * b[N - 2]) / (diag[N - 1] - l_diag[N - 2] * c[N - 2]);


    /*back substitution*/
    for (i = N - 2; i >= 0; i--) {
        b[i] = b[i] - c[i] * b[i + 1];
    }
    return 0;
}

// Homogeneous diffusion coefficient
void ics_find_deltas(long start,
                     long stop,
                     long node_start,
                     double* delta,
                     long* line_defs,
                     long* ordered_nodes,
                     double* states,
                     double dc,
                     double* alphas) {
    long ordered_index = node_start;
    long current_index = -1;
    long next_index = -1;
    double prev_state;
    double current_state;
    double next_state;
    double prev_alpha;
    double current_alpha;
    double next_alpha;
    for (int i = start; i < stop - 1; i += 2) {
        long line_start = ordered_nodes[ordered_index];
        int line_length = line_defs[i + 1];
        if (line_length > 1) {
            current_index = line_start;
            ordered_index++;
            next_index = ordered_nodes[ordered_index];
            current_state = states[current_index];
            next_state = states[next_index];
            current_alpha = alphas[current_index];
            next_alpha = alphas[next_index];
            delta[current_index] = dc * next_alpha * current_alpha * (next_state - current_state) /
                                   (next_alpha + current_alpha);
            ordered_index++;
            for (int j = 1; j < line_length - 1; j++) {
                prev_state = current_state;
                current_index = next_index;
                next_index = ordered_nodes[ordered_index];
                current_state = next_state;
                next_state = states[next_index];
                prev_alpha = current_alpha;
                current_alpha = next_alpha;
                next_alpha = alphas[next_index];
                delta[current_index] =
                    dc * ((next_alpha * current_alpha / (next_alpha + current_alpha)) *
                              (next_state - current_state) -
                          (prev_alpha * current_alpha / (prev_alpha + current_alpha)) *
                              (current_state - prev_state));
                ordered_index++;
            }
            // Here next_state is actually current_state and current_state is prev_state
            delta[next_index] = dc * (next_alpha * current_alpha) * (current_state - next_state) /
                                (next_alpha + current_alpha);
        } else {
            ordered_index++;
            delta[line_start] = 0.0;
        }
    }
}

// Inhomogeneous diffusion coefficient
void ics_find_deltas(long start,
                     long stop,
                     long node_start,
                     double* delta,
                     long* line_defs,
                     long* ordered_nodes,
                     double* states,
                     double* dc,
                     double* alphas) {
    long ordered_index = node_start;
    long current_index = -1;
    long next_index = -1;
    double prev_state;
    double current_state;
    double next_state;
    double prev_alpha;
    double current_alpha;
    double next_alpha;
    for (int i = start; i < stop - 1; i += 2) {
        long line_start = ordered_nodes[ordered_index];
        int line_length = line_defs[i + 1];
        if (line_length > 1) {
            current_index = line_start;
            ordered_index++;
            next_index = ordered_nodes[ordered_index];
            current_state = states[current_index];
            next_state = states[next_index];
            current_alpha = alphas[current_index];
            next_alpha = alphas[next_index];
            delta[current_index] = dc[next_index] * next_alpha * current_alpha *
                                   (next_state - current_state) / (next_alpha + current_alpha);
            ordered_index++;
            for (int j = 1; j < line_length - 1; j++) {
                prev_state = current_state;
                current_index = next_index;
                next_index = ordered_nodes[ordered_index];
                current_state = next_state;
                next_state = states[next_index];
                prev_alpha = current_alpha;
                current_alpha = next_alpha;
                next_alpha = alphas[next_index];
                delta[current_index] =
                    dc[next_index] * (next_alpha * current_alpha * (next_state - current_state) /
                                      (next_alpha + current_alpha)) -
                    dc[current_index] * (prev_alpha * current_alpha * (current_state - prev_state) /
                                         (prev_alpha + current_alpha));
                ordered_index++;
            }
            // Here next_state is actually current_state and current_state is prev_state
            delta[next_index] = dc[next_index] * (next_alpha * current_alpha) *
                                (current_state - next_state) / (next_alpha + current_alpha);
        } else {
            ordered_index++;
            delta[line_start] = 0.0;
        }
    }
}

static void* do_ics_deltas(void* dataptr) {
    ICSAdiGridData* data = (ICSAdiGridData*) dataptr;
    ICSAdiDirection* ics_adi_dir = data->ics_adi_dir;
    ICS_Grid_node* g = data->g;
    int line_start = data->line_start;
    int line_stop = data->line_stop;
    int node_start = data->ordered_start;
    double* states = g->states;
    double* deltas = ics_adi_dir->deltas;
    long* line_defs = ics_adi_dir->ordered_line_defs;
    long* ordered_nodes = ics_adi_dir->ordered_nodes;
    double* alphas = g->_ics_alphas;

    if (ics_adi_dir->dcgrid == NULL)
        ics_find_deltas(line_start,
                        line_stop,
                        node_start,
                        deltas,
                        line_defs,
                        ordered_nodes,
                        states,
                        ics_adi_dir->dc,
                        alphas);
    else
        ics_find_deltas(line_start,
                        line_stop,
                        node_start,
                        deltas,
                        line_defs,
                        ordered_nodes,
                        states,
                        ics_adi_dir->dcgrid,
                        alphas);


    return NULL;
}

void run_threaded_deltas(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir) {
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        g->ics_tasks[i].line_start = ics_adi_dir->line_start_stop_indices[2 * i];
        g->ics_tasks[i].line_stop = ics_adi_dir->line_start_stop_indices[(2 * i) + 1];
        g->ics_tasks[i].ordered_start =
            ics_adi_dir->ordered_start_stop_indices[(2 * i)];  // Change what I'm storing in
                                                               // ordered_start_stop_indices so
                                                               // index is just i
        g->ics_tasks[i].ics_adi_dir = ics_adi_dir;
    }
    /* launch threads */
    for (i = 0; i < NUM_THREADS - 1; i++) {
        TaskQueue_add_task(AllTasks, &do_ics_deltas, &(g->ics_tasks[i]), NULL);
    }
    /* run one task in the main thread */
    do_ics_deltas(&(g->ics_tasks[NUM_THREADS - 1]));
    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}

// Inhomogeneous diffusion coefficient
void ics_dg_adi_x_inhom(ICS_Grid_node* g,
                        int line_start,
                        int line_stop,
                        int node_start,
                        double,
                        double* states,
                        double* RHS,
                        double* scratchpad,
                        double* u_diag,
                        double* diag,
                        double* l_diag) {
    double* delta_x = g->ics_adi_dir_x->deltas;
    double* delta_y = g->ics_adi_dir_y->deltas;
    double* delta_z = g->ics_adi_dir_z->deltas;
    double* states_cur = g->states_cur;
    double* alphas = g->_ics_alphas;
    double* dc = g->ics_adi_dir_x->dcgrid;
    double dx = g->ics_adi_dir_x->d;
    double dy = g->ics_adi_dir_y->d;
    double dz = g->ics_adi_dir_z->d;
    double dt = *dt_ptr;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;

    long* x_lines = g->ics_adi_dir_x->ordered_line_defs;
    long* ordered_nodes = g->ics_adi_dir_x->ordered_nodes;

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = x_lines[i + 1];
        long ordered_index_start = ordered_index;
        for (int j = 0; j < N; j++) {
            current_index = ordered_nodes[ordered_index];
            RHS[j] = (dt / alphas[current_index]) *
                         (delta_x[current_index] / SQ(dx) + 2.0 * delta_y[current_index] / SQ(dy) +
                          2.0 * delta_z[current_index] / SQ(dz)) +
                     states[current_index] + states_cur[current_index];
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_nodes[ordered_index];
        next = alphas[next_index] * dc[next_index] / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dx);
        u_diag[0] = -dt * next / SQ(dx);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_nodes[ordered_index];
            prev = alphas[prev_index] * dc[current_index] /
                   (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dc[next_index] /
                   (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dx);
            diag[c] = 1. + dt * (prev + next) / SQ(dx);
            u_diag[c] = -dt * next / SQ(dx);
            ordered_index++;
        }
        prev = alphas[current_index] * dc[next_index] /
               (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dx);
        l_diag[N - 2] = -dt * prev / SQ(dx);


        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

void ics_dg_adi_y_inhom(ICS_Grid_node* g,
                        int line_start,
                        int line_stop,
                        int node_start,
                        double,
                        double* states,
                        double* RHS,
                        double* scratchpad,
                        double* u_diag,
                        double* diag,
                        double* l_diag) {
    double* delta = g->ics_adi_dir_y->deltas;
    long* lines = g->ics_adi_dir_y->ordered_line_defs;
    double* alphas = g->_ics_alphas;
    double* dc = g->ics_adi_dir_y->dcgrid;
    double dy = g->ics_adi_dir_y->d;
    double dt = *dt_ptr;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;

    long current_index;
    long* ordered_y_nodes = g->ics_adi_dir_y->ordered_nodes;
    long ordered_index = node_start;

    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_y_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (alphas[current_index] * SQ(dy));
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_y_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_y_nodes[ordered_index];
        next = alphas[next_index] * dc[next_index] / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dy);
        u_diag[0] = -dt * next / SQ(dy);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_y_nodes[ordered_index];
            prev = alphas[prev_index] * dc[prev_index] /
                   (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dc[next_index] /
                   (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dy);
            diag[c] = 1. + dt * (prev + next) / SQ(dy);
            u_diag[c] = -dt * next / SQ(dy);
            ordered_index++;
        }
        prev = alphas[current_index] * dc[current_index] /
               (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dy);
        l_diag[N - 2] = -dt * prev / SQ(dy);

        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_y_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
void ics_dg_adi_z_inhom(ICS_Grid_node* g,
                        int line_start,
                        int line_stop,
                        int node_start,
                        double,
                        double* states,
                        double* RHS,
                        double* scratchpad,
                        double* u_diag,
                        double* diag,
                        double* l_diag) {
    double* delta = g->ics_adi_dir_z->deltas;
    long* lines = g->ics_adi_dir_z->ordered_line_defs;
    long* ordered_z_nodes = g->ics_adi_dir_z->ordered_nodes;
    double* alphas = g->_ics_alphas;
    double* dc = g->ics_adi_dir_z->dcgrid;
    double dz = g->ics_adi_dir_z->d;
    double dt = *dt_ptr;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_z_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (SQ(dz) * alphas[current_index]);
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_z_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_z_nodes[ordered_index];
        next = alphas[next_index] * dc[next_index] / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dz);
        u_diag[0] = -dt * next / SQ(dz);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_z_nodes[ordered_index];
            prev = alphas[prev_index] * dc[prev_index] /
                   (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dc[next_index] /
                   (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dz);
            diag[c] = 1. + dt * (prev + next) / SQ(dz);
            u_diag[c] = -dt * next / SQ(dz);
            ordered_index++;
        }
        prev = alphas[current_index] * dc[current_index] /
               (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dz);
        l_diag[N - 2] = -dt * prev / SQ(dz);
        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_z_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

// Homogeneous diffusion coefficient
void ics_dg_adi_x(ICS_Grid_node* g,
                  int line_start,
                  int line_stop,
                  int node_start,
                  double,
                  double* states,
                  double* RHS,
                  double* scratchpad,
                  double* u_diag,
                  double* diag,
                  double* l_diag) {
    double* delta_x = g->ics_adi_dir_x->deltas;
    double* delta_y = g->ics_adi_dir_y->deltas;
    double* delta_z = g->ics_adi_dir_z->deltas;
    double* states_cur = g->states_cur;
    double* alphas = g->_ics_alphas;
    double dc = g->ics_adi_dir_x->dc;
    double dx = g->ics_adi_dir_x->d;
    double dy = g->ics_adi_dir_y->d;
    double dz = g->ics_adi_dir_z->d;
    double dt = *dt_ptr;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;

    long* x_lines = g->ics_adi_dir_x->ordered_line_defs;
    long* ordered_nodes = g->ics_adi_dir_x->ordered_nodes;

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = x_lines[i + 1];
        long ordered_index_start = ordered_index;
        for (int j = 0; j < N; j++) {
            current_index = ordered_nodes[ordered_index];
            RHS[j] = (dt / alphas[current_index]) *
                         (delta_x[current_index] / (SQ(dx)) + 2 * delta_y[current_index] / SQ(dy) +
                          2 * delta_z[current_index] / SQ(dz)) +
                     states[current_index] + states_cur[current_index];
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_nodes[ordered_index];
        next = alphas[next_index] * dc / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dx);
        u_diag[0] = -dt * next / SQ(dx);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_nodes[ordered_index];
            prev = alphas[prev_index] * dc / (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dc / (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dx);
            diag[c] = 1. + dt * (prev + next) / SQ(dx);
            u_diag[c] = -dt * next / SQ(dx);
            ordered_index++;
        }
        prev = alphas[current_index] * dc / (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dx);
        l_diag[N - 2] = -dt * prev / SQ(dx);


        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

void ics_dg_adi_y(ICS_Grid_node* g,
                  int line_start,
                  int line_stop,
                  int node_start,
                  double,
                  double* states,
                  double* RHS,
                  double* scratchpad,
                  double* u_diag,
                  double* diag,
                  double* l_diag) {
    double* delta = g->ics_adi_dir_y->deltas;
    long* lines = g->ics_adi_dir_y->ordered_line_defs;
    double* alphas = g->_ics_alphas;
    double dc = g->ics_adi_dir_y->dc;
    double dy = g->ics_adi_dir_y->d;
    double dt = *dt_ptr;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;

    long current_index;
    long* ordered_y_nodes = g->ics_adi_dir_y->ordered_nodes;
    long ordered_index = node_start;

    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_y_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (alphas[current_index] * SQ(dy));
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_y_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_y_nodes[ordered_index];
        next = alphas[next_index] * dc / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dy);
        u_diag[0] = -dt * next / SQ(dy);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_y_nodes[ordered_index];
            prev = alphas[prev_index] * dc / (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dc / (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dy);
            diag[c] = 1. + dt * (prev + next) / SQ(dy);
            u_diag[c] = -dt * next / SQ(dy);
            ordered_index++;
        }
        prev = alphas[current_index] * dc / (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dy);
        l_diag[N - 2] = -dt * prev / SQ(dy);

        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_y_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
void ics_dg_adi_z(ICS_Grid_node* g,
                  int line_start,
                  int line_stop,
                  int node_start,
                  double,
                  double* states,
                  double* RHS,
                  double* scratchpad,
                  double* u_diag,
                  double* diag,
                  double* l_diag) {
    double* delta = g->ics_adi_dir_z->deltas;
    long* lines = g->ics_adi_dir_z->ordered_line_defs;
    long* ordered_z_nodes = g->ics_adi_dir_z->ordered_nodes;
    double* alphas = g->_ics_alphas;
    double dc = g->ics_adi_dir_z->dc;
    double dz = g->ics_adi_dir_z->d;
    double dt = *dt_ptr;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_z_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (SQ(dz) * alphas[current_index]);
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_z_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_z_nodes[ordered_index];
        next = alphas[next_index] * dc / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dz);
        u_diag[0] = -dt * next / SQ(dz);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_z_nodes[ordered_index];
            prev = alphas[prev_index] * dc / (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dc / (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dz);
            diag[c] = 1. + dt * (prev + next) / SQ(dz);
            u_diag[c] = -dt * next / SQ(dz);
            ordered_index++;
        }
        prev = alphas[current_index] * dc / (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dz);
        l_diag[N - 2] = -dt * prev / SQ(dz);
        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_z_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

void* do_ics_dg_adi(void* dataptr) {
    ICSAdiGridData* data = (ICSAdiGridData*) dataptr;
    ICSAdiDirection* ics_adi_dir = data->ics_adi_dir;
    ICS_Grid_node* g = data->g;
    void (*ics_dg_adi_dir)(ICS_Grid_node * g,
                           int,
                           int,
                           int,
                           double,
                           double*,
                           double*,
                           double*,
                           double*,
                           double*,
                           double*) = ics_adi_dir->ics_dg_adi_dir;
    int line_start = data->line_start;
    int line_stop = data->line_stop;
    int node_start = data->ordered_start;
    double dt = *dt_ptr;
    double r = dt / (ics_adi_dir->d * ics_adi_dir->d);
    ics_dg_adi_dir(g,
                   line_start,
                   line_stop,
                   node_start,
                   r,
                   g->states,
                   data->RHS,
                   data->scratchpad,
                   data->u_diag,
                   data->diag,
                   data->l_diag);
    return NULL;
}

void ICS_Grid_node::run_threaded_ics_dg_adi(ICSAdiDirection* ics_adi_dir) {
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        ics_tasks[i].line_start = ics_adi_dir->line_start_stop_indices[2 * i];
        ics_tasks[i].line_stop = ics_adi_dir->line_start_stop_indices[(2 * i) + 1];
        ics_tasks[i].ordered_start =
            ics_adi_dir->ordered_start_stop_indices[(2 * i)];  // Change what I'm storing in
                                                               // ordered_start_stop_indices so
                                                               // index is just i
        ics_tasks[i].ics_adi_dir = ics_adi_dir;
    }
    /* launch threads */
    for (i = 0; i < NUM_THREADS - 1; i++) {
        TaskQueue_add_task(AllTasks, &do_ics_dg_adi, &ics_tasks[i], NULL);
    }
    /* run one task in the main thread */
    do_ics_dg_adi(&ics_tasks[NUM_THREADS - 1]);
    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}


/* ------- Variable Step ICS Code ------- */
static inline double flux(const double alphai,
                          const double alphaj,
                          const double statei,
                          const double statej) {
    return 2.0 * alphai * alphaj * (statei - statej) / ((alphai + alphaj));
}

static void variable_step_delta(long start,
                                long stop,
                                long node_start,
                                double* ydot,
                                long* line_defs,
                                long* ordered_nodes,
                                double const* const states,
                                double r,
                                double* alphas) {
    long ordered_index = node_start;
    long current_index = -1;
    long next_index = -1;
    double prev_state;
    double current_state;
    double next_state;
    double prev_alpha;
    double current_alpha;
    double next_alpha;
    for (int i = start; i < stop - 1; i += 2) {
        long line_start = ordered_nodes[ordered_index];
        int line_length = line_defs[i + 1];
        if (line_length > 1) {
            current_index = line_start;
            ordered_index++;
            next_index = ordered_nodes[ordered_index];
            current_state = states[current_index];
            next_state = states[next_index];
            current_alpha = alphas[current_index];
            next_alpha = alphas[next_index];
            ydot[current_index] += (r / current_alpha) *
                                   flux(next_alpha, current_alpha, next_state, current_state);
            ordered_index++;
            for (int j = 1; j < line_length - 1; j++) {
                prev_state = current_state;
                current_index = next_index;
                next_index = ordered_nodes[ordered_index];
                current_state = next_state;
                next_state = states[next_index];
                prev_alpha = current_alpha;
                current_alpha = next_alpha;
                next_alpha = alphas[next_index];
                ydot[current_index] += (r / current_alpha) *
                                       (flux(prev_alpha, current_alpha, prev_state, current_state) +
                                        flux(next_alpha, current_alpha, next_state, current_state));
                ordered_index++;
            }
            ydot[next_index] += r * flux(current_alpha, next_alpha, current_state, next_state) /
                                next_alpha;
        } else {
            ordered_index++;
            // ydot[line_start] += 0.0;
        }
    }
}

static void variable_step_delta(long start,
                                long stop,
                                long node_start,
                                double* ydot,
                                long* line_defs,
                                long* ordered_nodes,
                                double const* const states,
                                double r,
                                double* dcs,
                                double* alphas) {
    long ordered_index = node_start;
    long current_index = -1;
    long next_index = -1;
    double prev_state;
    double current_state;
    double next_state;
    double prev_alpha;
    double current_alpha;
    double next_alpha;
    double current_dc, next_dc;
    for (int i = start; i < stop - 1; i += 2) {
        long line_start = ordered_nodes[ordered_index];
        int line_length = line_defs[i + 1];
        if (line_length > 1) {
            current_index = line_start;
            ordered_index++;
            next_index = ordered_nodes[ordered_index];
            current_state = states[current_index];
            next_state = states[next_index];
            current_alpha = alphas[current_index];
            next_alpha = alphas[next_index];
            current_dc = dcs[current_index];
            next_dc = dcs[next_index];
            ydot[current_index] += (r / current_alpha) * next_dc *
                                   flux(next_alpha, current_alpha, next_state, current_state);
            ordered_index++;
            for (int j = 1; j < line_length - 1; j++) {
                prev_state = current_state;
                current_index = next_index;
                next_index = ordered_nodes[ordered_index];
                current_state = next_state;
                next_state = states[next_index];
                prev_alpha = current_alpha;
                current_alpha = next_alpha;
                next_alpha = alphas[next_index];
                current_dc = next_dc;
                next_dc = dcs[next_index];
                ydot[current_index] +=
                    (r / current_alpha) *
                    (current_dc * flux(prev_alpha, current_alpha, prev_state, current_state) +
                     next_dc * flux(next_alpha, current_alpha, next_state, current_state));
                ordered_index++;
            }
            ydot[next_index] += r * next_dc *
                                flux(current_alpha, next_alpha, current_state, next_state) /
                                next_alpha;
        } else {
            ordered_index++;
            // ydot[line_start] += 0.0;
        }
    }
}
void _ics_rhs_variable_step_helper(ICS_Grid_node* g, double const* const states, double* ydot) {
    double rate_x, rate_y, rate_z;
    // Find rate for each direction
    double dx = g->ics_adi_dir_x->d, dy = g->ics_adi_dir_y->d, dz = g->ics_adi_dir_z->d;

    long x_line_start = g->ics_adi_dir_x->line_start_stop_indices[0];
    long x_line_stop = g->ics_adi_dir_x->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long x_node_start = g->ics_adi_dir_x->ordered_start_stop_indices[0];
    long* x_line_defs = g->ics_adi_dir_x->ordered_line_defs;
    long* x_ordered_nodes = g->ics_adi_dir_x->ordered_nodes;

    long y_line_start = g->ics_adi_dir_y->line_start_stop_indices[0];
    long y_line_stop = g->ics_adi_dir_y->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long y_node_start = g->ics_adi_dir_y->ordered_start_stop_indices[0];
    long* y_line_defs = g->ics_adi_dir_y->ordered_line_defs;
    long* y_ordered_nodes = g->ics_adi_dir_y->ordered_nodes;

    long z_line_start = g->ics_adi_dir_z->line_start_stop_indices[0];
    long z_line_stop = g->ics_adi_dir_z->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long z_node_start = g->ics_adi_dir_z->ordered_start_stop_indices[0];
    long* z_line_defs = g->ics_adi_dir_z->ordered_line_defs;
    long* z_ordered_nodes = g->ics_adi_dir_z->ordered_nodes;

    double* alphas = g->_ics_alphas;

    if (g->ics_adi_dir_x->dcgrid == NULL) {
        rate_x = g->ics_adi_dir_x->dc / (dx * dx);
        rate_y = g->ics_adi_dir_y->dc / (dy * dy);
        rate_z = g->ics_adi_dir_z->dc / (dz * dz);
        // x contribution
        variable_step_delta(x_line_start,
                            x_line_stop,
                            x_node_start,
                            ydot,
                            x_line_defs,
                            x_ordered_nodes,
                            states,
                            rate_x,
                            alphas);
        // y contribution
        variable_step_delta(y_line_start,
                            y_line_stop,
                            y_node_start,
                            ydot,
                            y_line_defs,
                            y_ordered_nodes,
                            states,
                            rate_y,
                            alphas);
        // z contribution
        variable_step_delta(z_line_start,
                            z_line_stop,
                            z_node_start,
                            ydot,
                            z_line_defs,
                            z_ordered_nodes,
                            states,
                            rate_z,
                            alphas);
    } else {
        rate_x = 1.0 / (dx * dx);
        rate_y = 1.0 / (dy * dy);
        rate_z = 1.0 / (dz * dz);
        // x contribution
        variable_step_delta(x_line_start,
                            x_line_stop,
                            x_node_start,
                            ydot,
                            x_line_defs,
                            x_ordered_nodes,
                            states,
                            rate_x,
                            g->ics_adi_dir_x->dcgrid,
                            alphas);
        // y contribution
        variable_step_delta(y_line_start,
                            y_line_stop,
                            y_node_start,
                            ydot,
                            y_line_defs,
                            y_ordered_nodes,
                            states,
                            rate_y,
                            g->ics_adi_dir_y->dcgrid,
                            alphas);
        // z contribution
        variable_step_delta(z_line_start,
                            z_line_stop,
                            z_node_start,
                            ydot,
                            z_line_defs,
                            z_ordered_nodes,
                            states,
                            rate_z,
                            g->ics_adi_dir_z->dcgrid,
                            alphas);
    }
}

// Inhomogeneous diffusion coefficient
static void variable_step_x(ICS_Grid_node* g,
                            int line_start,
                            int line_stop,
                            int node_start,
                            double* states,
                            double* CVodeRHS,
                            double* RHS,
                            double* scratchpad,
                            double* alphas,
                            double* dcs,
                            double dt) {
    long* x_lines = g->ics_adi_dir_x->ordered_line_defs;
    long* ordered_nodes = g->ics_adi_dir_x->ordered_nodes;
    double* l_diag = g->ics_tasks->l_diag;
    double* diag = g->ics_tasks->diag;
    double* u_diag = g->ics_tasks->u_diag;
    double* delta_x = g->ics_adi_dir_x->deltas;
    double* delta_y = g->ics_adi_dir_y->deltas;
    double* delta_z = g->ics_adi_dir_z->deltas;

    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;
    double dx = g->ics_adi_dir_x->d;
    double dy = g->ics_adi_dir_y->d;
    double dz = g->ics_adi_dir_z->d;

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = x_lines[i + 1];
        long ordered_index_start = ordered_index;
        for (int j = 0; j < N; j++) {
            current_index = ordered_nodes[ordered_index];
            RHS[j] = CVodeRHS[current_index] -
                     dt *
                         (delta_x[current_index] / SQ(dx) + delta_y[current_index] / SQ(dy) +
                          delta_z[current_index] / SQ(dz)) /
                         alphas[current_index];
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_nodes[ordered_index];
        next = alphas[next_index] * dcs[next_index] / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dx);
        u_diag[0] = -dt * next / SQ(dx);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_nodes[ordered_index];
            prev = alphas[prev_index] * dcs[current_index] /
                   (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dcs[next_index] /
                   (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dx);
            diag[c] = 1. + dt * (prev + next) / SQ(dx);
            u_diag[c] = -dt * next / SQ(dx);
            ordered_index++;
        }
        prev = alphas[current_index] * dcs[current_index] /
               (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dx);
        l_diag[N - 2] = -dt * prev / SQ(dx);

        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
static void variable_step_y(ICS_Grid_node* g,
                            int line_start,
                            int line_stop,
                            int node_start,
                            double* states,
                            double* RHS,
                            double* scratchpad,
                            double* alphas,
                            double* dcs,
                            double dt) {
    double* delta = g->ics_adi_dir_y->deltas;
    long* lines = g->ics_adi_dir_y->ordered_line_defs;

    double* l_diag = g->ics_tasks->l_diag;
    double* diag = g->ics_tasks->diag;
    double* u_diag = g->ics_tasks->u_diag;

    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;
    double dy = g->ics_adi_dir_y->d;

    long current_index;
    long* ordered_y_nodes = g->ics_adi_dir_y->ordered_nodes;
    long ordered_index = node_start;

    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_y_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (alphas[current_index] * SQ(dy));
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_y_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_y_nodes[ordered_index];
        next = alphas[next_index] * dcs[next_index] / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dy);
        u_diag[0] = -dt * next / SQ(dy);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_y_nodes[ordered_index];
            prev = alphas[prev_index] * dcs[current_index] /
                   (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dcs[next_index] /
                   (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dy);
            diag[c] = 1. + dt * (prev + next) / SQ(dy);
            u_diag[c] = -dt * next / SQ(dy);
            ordered_index++;
        }
        prev = alphas[current_index] * dcs[current_index] /
               (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dy);
        l_diag[N - 2] = -dt * prev / SQ(dy);

        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_y_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
static void variable_step_z(ICS_Grid_node* g,
                            int line_start,
                            int line_stop,
                            int node_start,
                            double* states,
                            double* RHS,
                            double* scratchpad,
                            double* alphas,
                            double* dcs,
                            double dt) {
    double* delta = g->ics_adi_dir_z->deltas;
    long* lines = g->ics_adi_dir_z->ordered_line_defs;
    long* ordered_z_nodes = g->ics_adi_dir_z->ordered_nodes;

    double* l_diag = g->ics_tasks->l_diag;
    double* diag = g->ics_tasks->diag;
    double* u_diag = g->ics_tasks->u_diag;

    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;
    double dz = g->ics_adi_dir_z->d;

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_z_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (alphas[current_index] * SQ(dz));
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_z_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_z_nodes[ordered_index];
        next = alphas[next_index] * dcs[next_index] / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + dt * next / SQ(dz);
        u_diag[0] = -dt * next / SQ(dz);
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_z_nodes[ordered_index];
            prev = alphas[prev_index] * dcs[current_index] /
                   (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * dcs[next_index] /
                   (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -dt * prev / SQ(dz);
            diag[c] = 1. + dt * (prev + next) / SQ(dz);
            u_diag[c] = -dt * next / SQ(dz);
            ordered_index++;
        }
        prev = alphas[current_index] * dcs[current_index] /
               (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + dt * prev / SQ(dz);
        l_diag[N - 2] = -dt * prev / SQ(dz);
        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_z_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

// Homogeneous diffusion coefficient
static void variable_step_x(ICS_Grid_node* g,
                            int line_start,
                            int line_stop,
                            int node_start,
                            double* states,
                            double* CVodeRHS,
                            double* RHS,
                            double* scratchpad,
                            double* alphas,
                            double dt) {
    long* x_lines = g->ics_adi_dir_x->ordered_line_defs;
    long* ordered_nodes = g->ics_adi_dir_x->ordered_nodes;
    double* l_diag = g->ics_tasks->l_diag;
    double* diag = g->ics_tasks->diag;
    double* u_diag = g->ics_tasks->u_diag;
    double* delta_x = g->ics_adi_dir_x->deltas;
    double* delta_y = g->ics_adi_dir_y->deltas;
    double* delta_z = g->ics_adi_dir_z->deltas;
    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;
    double dx = g->ics_adi_dir_x->d;
    double dy = g->ics_adi_dir_y->d;
    double dz = g->ics_adi_dir_z->d;
    double dc = g->ics_adi_dir_x->dc;

    double r = dt * dc / SQ(dx);

    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = x_lines[i + 1];
        long ordered_index_start = ordered_index;
        for (int j = 0; j < N; j++) {
            current_index = ordered_nodes[ordered_index];
            RHS[j] = CVodeRHS[current_index] -
                     dt *
                         (delta_x[current_index] / SQ(dx) + delta_y[current_index] / SQ(dy) +
                          delta_z[current_index] / SQ(dz)) /
                         alphas[current_index];
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_nodes[ordered_index];
        next = alphas[next_index] * r / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + next;
        u_diag[0] = -next;
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_nodes[ordered_index];
            prev = alphas[prev_index] * r / (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * r / (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -prev;
            diag[c] = 1. + prev + next;
            u_diag[c] = -next;
            ordered_index++;
        }
        prev = alphas[current_index] * r / (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + prev;
        l_diag[N - 2] = -prev;

        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
static void variable_step_y(ICS_Grid_node* g,
                            int line_start,
                            int line_stop,
                            int node_start,
                            double* states,
                            double* RHS,
                            double* scratchpad,
                            double* alphas,
                            double dt) {
    double* delta = g->ics_adi_dir_y->deltas;
    long* lines = g->ics_adi_dir_y->ordered_line_defs;

    double* l_diag = g->ics_tasks->l_diag;
    double* diag = g->ics_tasks->diag;
    double* u_diag = g->ics_tasks->u_diag;

    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;
    double dy = g->ics_adi_dir_y->d;
    double dc = g->ics_adi_dir_y->dc;

    double r = dt * dc / SQ(dy);

    long current_index;
    long* ordered_y_nodes = g->ics_adi_dir_y->ordered_nodes;
    long ordered_index = node_start;

    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_y_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (alphas[current_index] * SQ(dy));
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_y_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_y_nodes[ordered_index];
        next = alphas[next_index] * r / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + next;
        u_diag[0] = -next;
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_y_nodes[ordered_index];
            prev = alphas[prev_index] * r / (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * r / (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -prev;
            diag[c] = 1. + prev + next;
            u_diag[c] = -next;
            ordered_index++;
        }
        prev = alphas[current_index] * r / (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + prev;
        l_diag[N - 2] = -prev;

        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_y_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
static void variable_step_z(ICS_Grid_node* g,
                            int line_start,
                            int line_stop,
                            int node_start,
                            double* states,
                            double* RHS,
                            double* scratchpad,
                            double* alphas,
                            double dt) {
    double* delta = g->ics_adi_dir_z->deltas;
    long* lines = g->ics_adi_dir_z->ordered_line_defs;
    long* ordered_z_nodes = g->ics_adi_dir_z->ordered_nodes;

    double* l_diag = g->ics_tasks->l_diag;
    double* diag = g->ics_tasks->diag;
    double* u_diag = g->ics_tasks->u_diag;

    long next_index = -1;
    long prev_index = -1;
    double next;
    double prev;
    double dz = g->ics_adi_dir_z->d;
    double dc = g->ics_adi_dir_z->dc;
    double r = dt * dc / SQ(dz);


    long current_index;
    long ordered_index = node_start;
    for (int i = line_start; i < line_stop - 1; i += 2) {
        long N = lines[i + 1];
        long ordered_index_start = ordered_index;

        for (int j = 0; j < N; j++) {
            current_index = ordered_z_nodes[ordered_index];
            RHS[j] = states[current_index] -
                     dt * delta[current_index] / (alphas[current_index] * SQ(dz));
            ordered_index++;
        }

        ordered_index = ordered_index_start;
        current_index = ordered_z_nodes[ordered_index];
        ordered_index++;
        next_index = ordered_z_nodes[ordered_index];
        next = alphas[next_index] * r / (alphas[next_index] + alphas[current_index]);
        diag[0] = 1.0 + next;
        u_diag[0] = -next;
        ordered_index++;
        for (int c = 1; c < N - 1; c++) {
            prev_index = current_index;
            current_index = next_index;
            next_index = ordered_z_nodes[ordered_index];
            prev = alphas[prev_index] * r / (alphas[prev_index] + alphas[current_index]);
            next = alphas[next_index] * r / (alphas[next_index] + alphas[current_index]);
            l_diag[c - 1] = -prev;
            diag[c] = 1. + prev + next;
            u_diag[c] = -next;
            ordered_index++;
        }
        prev = alphas[current_index] * r / (alphas[current_index] + alphas[next_index]);
        diag[N - 1] = 1.0 + prev;
        l_diag[N - 2] = -prev;
        solve_dd_tridiag(N, l_diag, diag, u_diag, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for (int k = 0; k < N; k++) {
            current_index = ordered_z_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

void ics_ode_solve_helper(ICS_Grid_node* g, double dt, double* CVodeRHS) {
    int num_states = g->_num_nodes;

    long x_line_start = g->ics_adi_dir_x->line_start_stop_indices[0];
    long x_line_stop = g->ics_adi_dir_x->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long x_node_start = g->ics_adi_dir_x->ordered_start_stop_indices[0];
    long* x_line_defs = g->ics_adi_dir_x->ordered_line_defs;
    long* x_ordered_nodes = g->ics_adi_dir_x->ordered_nodes;
    double* delta_x = g->ics_adi_dir_x->deltas;

    long y_line_start = g->ics_adi_dir_y->line_start_stop_indices[0];
    long y_line_stop = g->ics_adi_dir_y->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long y_node_start = g->ics_adi_dir_y->ordered_start_stop_indices[0];
    long* y_line_defs = g->ics_adi_dir_y->ordered_line_defs;
    long* y_ordered_nodes = g->ics_adi_dir_y->ordered_nodes;
    double* delta_y = g->ics_adi_dir_y->deltas;

    long z_line_start = g->ics_adi_dir_z->line_start_stop_indices[0];
    long z_line_stop = g->ics_adi_dir_z->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long z_node_start = g->ics_adi_dir_z->ordered_start_stop_indices[0];
    long* z_line_defs = g->ics_adi_dir_z->ordered_line_defs;
    long* z_ordered_nodes = g->ics_adi_dir_z->ordered_nodes;
    double* delta_z = g->ics_adi_dir_z->deltas;

    double* Grid_RHS = g->ics_tasks->RHS;
    double* Grid_ScratchPad = g->ics_tasks->scratchpad;

    double* CVode_states_copy = (double*) calloc(num_states, sizeof(double));
    memcpy(CVode_states_copy, CVodeRHS, sizeof(double) * num_states);

    double* alphas = g->_ics_alphas;

    if (g->ics_adi_dir_x->dcgrid == NULL) {
        // find deltas for x, y and z directions
        ics_find_deltas(x_line_start,
                        x_line_stop,
                        x_node_start,
                        delta_x,
                        x_line_defs,
                        x_ordered_nodes,
                        CVode_states_copy,
                        g->ics_adi_dir_x->dc,
                        alphas);
        ics_find_deltas(y_line_start,
                        y_line_stop,
                        y_node_start,
                        delta_y,
                        y_line_defs,
                        y_ordered_nodes,
                        CVode_states_copy,
                        g->ics_adi_dir_y->dc,
                        alphas);
        ics_find_deltas(z_line_start,
                        z_line_stop,
                        z_node_start,
                        delta_z,
                        z_line_defs,
                        z_ordered_nodes,
                        CVode_states_copy,
                        g->ics_adi_dir_z->dc,
                        alphas);

        variable_step_x(g,
                        x_line_start,
                        x_line_stop,
                        x_node_start,
                        CVode_states_copy,
                        CVodeRHS,
                        Grid_RHS,
                        Grid_ScratchPad,
                        alphas,
                        dt);
        variable_step_y(g,
                        y_line_start,
                        y_line_stop,
                        y_node_start,
                        CVode_states_copy,
                        Grid_RHS,
                        Grid_ScratchPad,
                        alphas,
                        dt);
        variable_step_z(g,
                        z_line_start,
                        z_line_stop,
                        z_node_start,
                        CVode_states_copy,
                        Grid_RHS,
                        Grid_ScratchPad,
                        alphas,
                        dt);
    } else {
        // find deltas for x, y and z directions
        ics_find_deltas(x_line_start,
                        x_line_stop,
                        x_node_start,
                        delta_x,
                        x_line_defs,
                        x_ordered_nodes,
                        CVode_states_copy,
                        g->ics_adi_dir_x->dcgrid,
                        alphas);
        ics_find_deltas(y_line_start,
                        y_line_stop,
                        y_node_start,
                        delta_y,
                        y_line_defs,
                        y_ordered_nodes,
                        CVode_states_copy,
                        g->ics_adi_dir_y->dcgrid,
                        alphas);
        ics_find_deltas(z_line_start,
                        z_line_stop,
                        z_node_start,
                        delta_z,
                        z_line_defs,
                        z_ordered_nodes,
                        CVode_states_copy,
                        g->ics_adi_dir_z->dcgrid,
                        alphas);

        variable_step_x(g,
                        x_line_start,
                        x_line_stop,
                        x_node_start,
                        CVode_states_copy,
                        CVodeRHS,
                        Grid_RHS,
                        Grid_ScratchPad,
                        alphas,
                        g->ics_adi_dir_x->dcgrid,
                        dt);
        variable_step_y(g,
                        y_line_start,
                        y_line_stop,
                        y_node_start,
                        CVode_states_copy,
                        Grid_RHS,
                        Grid_ScratchPad,
                        alphas,
                        g->ics_adi_dir_y->dcgrid,
                        dt);
        variable_step_z(g,
                        z_line_start,
                        z_line_stop,
                        z_node_start,
                        CVode_states_copy,
                        Grid_RHS,
                        Grid_ScratchPad,
                        alphas,
                        g->ics_adi_dir_z->dcgrid,
                        dt);
    }

    memcpy(CVodeRHS, CVode_states_copy, sizeof(double) * num_states);
    free(CVode_states_copy);
}

void _ics_hybrid_helper(ICS_Grid_node* g) {
    double dt = *dt_ptr;
    long num_1d_indices = g->hybrid_data->num_1d_indices;
    long* indices1d = g->hybrid_data->indices1d;
    long* num_3d_indices_per_1d_seg = g->hybrid_data->num_3d_indices_per_1d_seg;
    long* indices3d = g->hybrid_data->indices3d;
    double* rates = g->hybrid_data->rates;
    double* volumes1d = g->hybrid_data->volumes1d;
    double* volumes3d = g->hybrid_data->volumes3d;

    double vol_1d, vol_3d, rate, conc_1d;
    int my_3d_index, my_1d_index;
    int vol_3d_index;

    int total_num_3d_indices_per_1d_seg = 0;
    for (int i = 0; i < num_1d_indices; i++) {
        total_num_3d_indices_per_1d_seg += num_3d_indices_per_1d_seg[i];
    }

    // store the state values in the order that we need them
    double* old_g_states = (double*) malloc(sizeof(double) * total_num_3d_indices_per_1d_seg);

    vol_3d_index = 0;
    for (int i = 0; i < num_1d_indices; i++) {
        for (int j = 0; j < num_3d_indices_per_1d_seg[i]; j++, vol_3d_index++) {
            old_g_states[vol_3d_index] = g->states[indices3d[vol_3d_index]];
        }
    }

    vol_3d_index = 0;
    for (int i = 0; i < num_1d_indices; i++) {
        vol_1d = volumes1d[i];
        my_1d_index = indices1d[i];
        conc_1d = states[my_1d_index];


        // printf("for 1d index %d: volume 1d = %g and conc1d = %g\n", my_1d_index, vol_1d,
        // conc_1d);
        for (int j = 0; j < num_3d_indices_per_1d_seg[i]; j++, vol_3d_index++) {
            vol_3d = volumes3d[vol_3d_index];
            // rate is rate of change of 3d concentration
            my_3d_index = indices3d[vol_3d_index];
            rate = (rates[vol_3d_index]) * (old_g_states[vol_3d_index] - conc_1d);
            // forward euler coupling
            g->states[my_3d_index] -= dt * rate;
            states[my_1d_index] += dt * rate * vol_3d / vol_1d;
            // printf("for 3d index %d: volume 3d = %g and states3d = %g and rate3d = %g\n",
            // my_3d_index, vol_3d, g->states[my_3d_index], rates[vol_3d_index]);
        }
    }

    free(old_g_states);
}

void _ics_variable_hybrid_helper(ICS_Grid_node* g,
                                 const double* cvode_states_3d,
                                 double* const ydot_3d,
                                 const double* cvode_states_1d,
                                 double* const ydot_1d) {
    long num_1d_indices = g->hybrid_data->num_1d_indices;
    long* indices1d = g->hybrid_data->indices1d;
    long* num_3d_indices_per_1d_seg = g->hybrid_data->num_3d_indices_per_1d_seg;
    long* indices3d = g->hybrid_data->indices3d;
    double* rates = g->hybrid_data->rates;
    double* volumes1d = g->hybrid_data->volumes1d;
    double* volumes3d = g->hybrid_data->volumes3d;

    double vol_1d, vol_3d, rate, conc_1d;
    int my_3d_index, my_1d_index;
    int vol_3d_index = 0;

    for (int i = 0; i < num_1d_indices; i++) {
        vol_1d = volumes1d[i];
        my_1d_index = indices1d[i];
        conc_1d = cvode_states_1d[my_1d_index];
        for (int j = 0; j < num_3d_indices_per_1d_seg[i]; j++, vol_3d_index++) {
            vol_3d = volumes3d[vol_3d_index];
            // rate is rate of change of 3d concentration
            my_3d_index = indices3d[vol_3d_index];
            rate = (rates[vol_3d_index]) * (cvode_states_3d[my_3d_index] - conc_1d);
            // forward euler coupling
            ydot_3d[my_3d_index] -= rate;
            ydot_1d[my_1d_index] += rate * vol_3d / vol_1d;
        }
    }
}
