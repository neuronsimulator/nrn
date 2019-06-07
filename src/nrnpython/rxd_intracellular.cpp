#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include "rxd.h"
extern "C" {
    #include <matrix2.h>
}
#include <pthread.h>
#include <nrnwrap_Python.h>
#include <unistd.h>

#include<cmath>

extern int NUM_THREADS;
extern TaskQueue* AllTasks;
extern double* states;
const int ICS_PREFETCH = 3;

/*
 * Sets the data to be used by the grids for 1D/3D hybrid models
 */
extern "C" void set_hybrid_data(int64_t* num_1d_indices_per_grid, int64_t* num_3d_indices_per_grid, int64_t* hybrid_indices1d, int64_t* hybrid_indices3d, int64_t* num_3d_indices_per_1d_seg, int64_t* hybrid_grid_ids, double* rates, double* volumes1d, double* volumes3d)
{
    Grid_node* grid;
    int i, j, k, id;
    int grid_id_check = 0;

    int index_ctr_1d = 0;
    int index_ctr_3d = 0;

    int num_grid_3d_indices;
    int num_grid_1d_indices;

    //loop over grids
    for (id = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, id++) {
        //if the grid we are on is the next grid in the hybrid grids
        if(id == hybrid_grid_ids[grid_id_check])
        {   
            num_grid_1d_indices = num_1d_indices_per_grid[grid_id_check];
            num_grid_3d_indices = num_3d_indices_per_grid[grid_id_check];
            //Allocate memory for hybrid data
            grid->hybrid = true;
            grid->hybrid_data->indices1d = (long*)malloc(sizeof(long)*num_grid_1d_indices);
            grid->hybrid_data->num_3d_indices_per_1d_seg = (long*)malloc(sizeof(long)*num_grid_1d_indices);
            grid->hybrid_data->volumes1d = (double*)malloc(sizeof(long)*num_grid_1d_indices);


            grid->hybrid_data->indices3d = (long*)malloc(sizeof(long)*num_grid_3d_indices);
            grid->hybrid_data->rates = (double*)malloc(sizeof(long)*num_grid_3d_indices);
            grid->hybrid_data->volumes3d = (double*)malloc(sizeof(long)*num_grid_3d_indices);


            //Assign grid data
            grid->hybrid_data->num_1d_indices = num_grid_1d_indices;
            for(i = 0, k = 0; i < num_grid_1d_indices; i++, index_ctr_1d++)
            {
                grid->hybrid_data->indices1d[i] = hybrid_indices1d[index_ctr_1d];
                grid->hybrid_data->num_3d_indices_per_1d_seg[i] = num_3d_indices_per_1d_seg[index_ctr_1d];
                grid->hybrid_data->volumes1d[i] = volumes1d[index_ctr_1d];

                for (j = 0; j < num_3d_indices_per_1d_seg[index_ctr_1d]; j++, index_ctr_3d++, k++)
                {
                    grid->hybrid_data->indices3d[k] = hybrid_indices3d[index_ctr_3d];
                    grid->hybrid_data->rates[k] = rates[index_ctr_3d];
                    grid->hybrid_data->volumes3d[k] = volumes3d[index_ctr_3d];
                }
            } 
            grid_id_check++;
        }
    } 
}

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

void ics_find_deltas(long start, long stop, long node_start, double* delta, long* line_defs, long* ordered_nodes, double* states, double r){
    long ordered_index = node_start;
    long current_index = -1;
    long next_index = -1;
    double prev_state;
    double current_state;
    double next_state;
    for(int i = start; i < stop - 1; i += 2)
    {   
        long line_start = ordered_nodes[ordered_index]; 
        int line_length = line_defs[i+1];
        if(line_length > 1)
        {   
            current_index = line_start;
            ordered_index++;
            next_index = ordered_nodes[ordered_index];
            current_state = states[current_index];
            next_state = states[next_index];
            delta[current_index] = r * (next_state - current_state);
            ordered_index++;
            for(int j = 1; j < line_length - 1; j++)
            {
                prev_state = current_state;
                current_index = next_index;
                next_index = ordered_nodes[ordered_index];
                current_state = next_state;
                next_state = states[next_index];
                delta[current_index] = r *(next_state - 2.0 * current_state + prev_state);
                ordered_index++;
            }
            delta[next_index] = r * (current_state - next_state);
        }
        else
        {
            ordered_index++;
            delta[line_start] = 0.0;
        }
    }    
}

static void* do_ics_deltas(void* dataptr){
    ICSAdiGridData* data = (ICSAdiGridData*) dataptr;
    ICSAdiDirection* ics_adi_dir = data -> ics_adi_dir;
    ICS_Grid_node* g = data -> g;
    int line_start =  data->line_start;
    int line_stop = data->line_stop;
    int node_start = data->ordered_start;
    double* states = g->states;
    double dt = *dt_ptr;
    double r = ((ics_adi_dir->dc)*dt)/(ics_adi_dir->d * ics_adi_dir->d);
    double* deltas = ics_adi_dir->deltas;
    long* line_defs = ics_adi_dir->ordered_line_defs;
    long* ordered_nodes = ics_adi_dir->ordered_nodes;

    ics_find_deltas(line_start, line_stop, node_start, deltas, line_defs, ordered_nodes, states, r);
    
    return NULL;    
}

void run_threaded_deltas(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir){
    int i;
    for(i = 0; i < NUM_THREADS; i++){
        g->ics_tasks[i].line_start = ics_adi_dir->line_start_stop_indices[2*i];
        g->ics_tasks[i].line_stop = ics_adi_dir->line_start_stop_indices[(2*i)+1];
        g->ics_tasks[i].ordered_start = ics_adi_dir->ordered_start_stop_indices[(2*i)];  //Change what I'm storing in ordered_start_stop_indices so index is just i
        g->ics_tasks[i].ics_adi_dir = ics_adi_dir;
    }
    /* launch threads */
    for (i = 0; i < NUM_THREADS-1; i++) 
    {
       TaskQueue_add_task(AllTasks, &do_ics_deltas, &(g->ics_tasks[i]), NULL);
    }
    /* run one task in the main thread */
    do_ics_deltas(&(g->ics_tasks[NUM_THREADS - 1]));
    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}


void ics_dg_adi_x(ICS_Grid_node* g, int line_start, int line_stop, int node_start, double r, double* states, double* RHS, double* scratchpad){
    double* delta_x = g->ics_adi_dir_x->deltas;
    double* delta_y = g->ics_adi_dir_y->deltas;
    double* delta_z = g->ics_adi_dir_z->deltas;
    double* ics_states_cur = g->ics_states_cur;

    long* x_lines = g->ics_adi_dir_x->ordered_line_defs;
    long* ordered_nodes = g->ics_adi_dir_x->ordered_nodes;

    long current_index;
    long ordered_index = node_start;
    long N = 0;
    for(int i = line_start; i < line_stop - 1; i += 2)
    {
        long N = x_lines[i+1];
        long ordered_index_start = ordered_index;
        for(int j = 0; j < N; j++)
        {
            current_index = ordered_nodes[ordered_index];
            RHS[j] = delta_x[current_index]/2.0 + delta_y[current_index] + delta_z[current_index] + states[current_index] + ics_states_cur[current_index];
            ordered_index++;            
        }

        solve_dd_clhs_tridiag(N, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for(int k = 0; k < N; k++)
        {
            current_index = ordered_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
void ics_dg_adi_y(ICS_Grid_node* g, int line_start, int line_stop, int node_start, double r, double* states, double* RHS, double* scratchpad){
    double* delta = g->ics_adi_dir_y->deltas;
    long* lines = g->ics_adi_dir_y->ordered_line_defs;

    long current_index;
    long* ordered_y_nodes = g->ics_adi_dir_y->ordered_nodes;
    long ordered_index = node_start;
    long N = 0;

    for(int i = line_start; i < line_stop - 1; i += 2)
    {
        long N = lines[i+1];
        long ordered_index_start = ordered_index;

        for(int j = 0; j < N; j++)
        {
            current_index = ordered_y_nodes[ordered_index];
            RHS[j] = states[current_index] - delta[current_index]/2.0;
            ordered_index++;
        }

        solve_dd_clhs_tridiag(N, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for(int k = 0; k < N; k++)
        {
            current_index = ordered_y_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }

}
void ics_dg_adi_z(ICS_Grid_node* g, int line_start, int line_stop, int node_start, double r, double* states, double* RHS, double* scratchpad){
    double* delta = g->ics_adi_dir_z->deltas;
    long* lines = g->ics_adi_dir_z->ordered_line_defs;
    long* ordered_z_nodes = g->ics_adi_dir_z->ordered_nodes;

    long current_index;
    long ordered_index = node_start;
    long N = 0;
    for(int i = line_start; i< line_stop - 1; i+=2)
    {
        long N = lines[i+1];
        long ordered_index_start = ordered_index;
        
        for(int j = 0; j < N; j++)
        {
            current_index = ordered_z_nodes[ordered_index];
            RHS[j] = states[current_index] - delta[current_index]/2.0;
            ordered_index++;
        }

        solve_dd_clhs_tridiag(N, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for(int k = 0; k < N; k++)
        {
            current_index = ordered_z_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

static void* do_ics_dg_adi(void* dataptr){
    ICSAdiGridData* data = (ICSAdiGridData*) dataptr;
    ICSAdiDirection* ics_adi_dir = data -> ics_adi_dir;
    ICS_Grid_node* g = data -> g;
    void (*ics_dg_adi_dir)(ICS_Grid_node* g, int, int, int, double, double*, double*, double*) = ics_adi_dir -> ics_dg_adi_dir;
    int line_start = data->line_start;
    int line_stop = data->line_stop;
    int node_start = data->ordered_start;
    double dt = *dt_ptr;
    double r = ((ics_adi_dir->dc)*dt)/(ics_adi_dir->d * ics_adi_dir->d);

    ics_dg_adi_dir(g, line_start, line_stop, node_start, r, g->states, data->RHS, data->scratchpad);
    return NULL;
}

void run_threaded_ics_dg_adi(ICS_Grid_node* g, ICSAdiDirection* ics_adi_dir){
    int i;
    for(i = 0; i < NUM_THREADS; i++){
        g->ics_tasks[i].line_start = ics_adi_dir->line_start_stop_indices[2*i];
        g->ics_tasks[i].line_stop = ics_adi_dir->line_start_stop_indices[(2*i)+1];
        g->ics_tasks[i].ordered_start = ics_adi_dir->ordered_start_stop_indices[(2*i)];  //Change what I'm storing in ordered_start_stop_indices so index is just i
        g->ics_tasks[i].ics_adi_dir = ics_adi_dir;
    }
    /* launch threads */
    for (i = 0; i < NUM_THREADS-1; i++) 
    {
       TaskQueue_add_task(AllTasks, &do_ics_dg_adi, &(g->ics_tasks[i]), NULL);
    }
    /* run one task in the main thread */
    do_ics_dg_adi(&(g->ics_tasks[NUM_THREADS - 1]));
    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
}

/* ------- Variable Step ICS Code ------- */
static void variable_step_delta(long start, long stop, long node_start, double* ydot, long* line_defs, long* ordered_nodes, double const * const states, double r)
{
    long ordered_index = node_start;
    long current_index = -1;
    long next_index = -1;
    double prev_state;
    double current_state;
    double next_state;
    for(int i = start; i < stop - 1; i += 2)
    {   
        long line_start = ordered_nodes[ordered_index]; 
        int line_length = line_defs[i+1];
        if(line_length > 1)
        {   
            current_index = line_start;
            ordered_index++;
            next_index = ordered_nodes[ordered_index];
            current_state = states[current_index];
            next_state = states[next_index];
            ydot[current_index] += r * (next_state - current_state);
            ordered_index++;
            for(int j = 1; j < line_length - 1; j++)
            {
                prev_state = current_state;
                current_index = next_index;
                next_index = ordered_nodes[ordered_index];
                current_state = next_state;
                next_state = states[next_index];
                ydot[current_index] += r *(next_state - 2.0 * current_state + prev_state);
                ordered_index++;
            }
            ydot[next_index] += r * (current_state - next_state);
        }
        else
        {
            ordered_index++;
            ydot[line_start] += 0.0;
        }
    }      
}

//TODO: Can probably thread this relatively easily 
void _ics_rhs_variable_step_helper(ICS_Grid_node* g, double const * const states, double* ydot)
{
    //I don't think I need this since I have the ordered indices
    int num_states = g->_num_nodes;
    //Find rate for each direction
    double dx = g->ics_adi_dir_x->d, dy = g->ics_adi_dir_y->d, dz = g->ics_adi_dir_z->d;
    double dc_x = g->ics_adi_dir_x->dc, dc_y = g->ics_adi_dir_y->dc, dc_z = g->ics_adi_dir_z->dc;
    
    double rate_x = dc_x / (dx * dx);
    double rate_y = dc_y / (dy * dy);
    double rate_z = dc_z / (dz * dz);

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

    //x contribution
    variable_step_delta(x_line_start, x_line_stop, x_node_start, ydot, x_line_defs, x_ordered_nodes, states, rate_x);
    //y contribution
    variable_step_delta(y_line_start, y_line_stop, y_node_start, ydot, y_line_defs, y_ordered_nodes, states, rate_y);
    //z contribution
    variable_step_delta(z_line_start, z_line_stop, z_node_start, ydot, z_line_defs, z_ordered_nodes, states, rate_z);
}


static void variable_step_x(ICS_Grid_node* g, int line_start, int line_stop, int node_start, double* states, double* CVodeRHS, double* RHS, double* scratchpad, double r)
{
    long* x_lines = g->ics_adi_dir_x->ordered_line_defs;
    long* ordered_nodes = g->ics_adi_dir_x->ordered_nodes;

    long current_index;
    long ordered_index = node_start;
    long N = 0;
    for(int i = line_start; i < line_stop - 1; i += 2)
    {
        long N = x_lines[i+1];
        long ordered_index_start = ordered_index;
        for(int j = 0; j < N; j++)
        {
            current_index = ordered_nodes[ordered_index];
            RHS[j] = CVodeRHS[current_index];
            ordered_index++;            
        }

        solve_dd_clhs_tridiag(N, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for(int k = 0; k < N; k++)
        {
            current_index = ordered_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
static void variable_step_y(ICS_Grid_node* g, int line_start, int line_stop, int node_start, double* states, double* RHS, double* scratchpad, double r)
{
    double* delta = g->ics_adi_dir_y->deltas;
    long* lines = g->ics_adi_dir_y->ordered_line_defs;

    long current_index;
    long* ordered_y_nodes = g->ics_adi_dir_y->ordered_nodes;
    long ordered_index = node_start;
    long N = 0;

    for(int i = line_start; i < line_stop - 1; i += 2)
    {
        long N = lines[i+1];
        long ordered_index_start = ordered_index;

        for(int j = 0; j < N; j++)
        {
            current_index = ordered_y_nodes[ordered_index];
            RHS[j] = states[current_index] - delta[current_index]/2.0;
            ordered_index++;
        }

        solve_dd_clhs_tridiag(N, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for(int k = 0; k < N; k++)
        {
            current_index = ordered_y_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}
static void variable_step_z(ICS_Grid_node* g, int line_start, int line_stop, int node_start, double* states, double* RHS, double* scratchpad, double r)
{
    double* delta = g->ics_adi_dir_z->deltas;
    long* lines = g->ics_adi_dir_z->ordered_line_defs;
    long* ordered_z_nodes = g->ics_adi_dir_z->ordered_nodes;

    long current_index;
    long ordered_index = node_start;
    long N = 0;
    for(int i = line_start; i< line_stop - 1; i+=2)
    {
        long N = lines[i+1];
        long ordered_index_start = ordered_index;
        
        for(int j = 0; j < N; j++)
        {
            current_index = ordered_z_nodes[ordered_index];
            RHS[j] = states[current_index] - delta[current_index]/2.0;
            ordered_index++;
        }

        solve_dd_clhs_tridiag(N, -r/2.0, 1.0+r, -r/2.0, 1.0+r/2.0, -r/2.0, -r/2.0, 1.0+r/2.0, RHS, scratchpad);

        ordered_index = ordered_index_start;
        for(int k = 0; k < N; k++)
        {
            current_index = ordered_z_nodes[ordered_index];
            states[current_index] = RHS[k];
            ordered_index++;
        }
    }
}

void ics_ode_solve_helper(ICS_Grid_node* g, double dt, const double* CVode_states, double* CVodeRHS)
{ 
    int num_states = g->_num_nodes;
    //Find rate for each direction
    double dx = g->ics_adi_dir_x->d, dy = g->ics_adi_dir_y->d, dz = g->ics_adi_dir_z->d;
    double dc_x = g->ics_adi_dir_x->dc, dc_y = g->ics_adi_dir_y->dc, dc_z = g->ics_adi_dir_z->dc;

    double rate_x = (dc_x * dt) / (dx * dx);
    double rate_y = (dc_y * dt) / (dy * dy);
    double rate_z = (dc_z * dt) / (dz * dz);

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
    double* delta_y = g->ics_adi_dir_y->deltas;

    long z_line_start = g->ics_adi_dir_z->line_start_stop_indices[0];
    long z_line_stop = g->ics_adi_dir_z->line_start_stop_indices[NUM_THREADS * 2 - 1];
    long z_node_start = g->ics_adi_dir_z->ordered_start_stop_indices[0];
    long* z_line_defs = g->ics_adi_dir_z->ordered_line_defs;
    long* z_ordered_nodes = g->ics_adi_dir_z->ordered_nodes;
    double* delta_z = g->ics_adi_dir_z->deltas;

    double* Grid_RHS = g->ics_tasks->RHS;
    double* Grid_ScratchPad = g->ics_tasks->scratchpad;

    double* CVode_states_copy = (double*)calloc(num_states, sizeof(double));
    memcpy(CVode_states_copy, CVode_states, sizeof(double)*num_states);

    //find deltas for y and z directions
    ics_find_deltas(y_line_start, y_line_stop, y_node_start, delta_y, y_line_defs, y_ordered_nodes, CVode_states_copy, rate_y);
    ics_find_deltas(z_line_start, z_line_stop, z_node_start, delta_z, z_line_defs, z_ordered_nodes, CVode_states_copy, rate_z); 


    variable_step_x(g, x_line_start, x_line_stop, x_node_start, CVode_states_copy, CVodeRHS, Grid_RHS, Grid_ScratchPad, rate_x);
    variable_step_y(g, y_line_start, y_line_stop, y_node_start, CVode_states_copy, Grid_RHS, Grid_ScratchPad, rate_y);
    variable_step_z(g, z_line_start, z_line_stop, z_node_start, CVode_states_copy, Grid_RHS, Grid_ScratchPad, rate_z);

    memcpy(CVodeRHS, CVode_states_copy, sizeof(double)*num_states);
    free(CVode_states_copy);
}

void _ics_hybrid_helper(ICS_Grid_node* g)
{   
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
    for(int i = 0; i < num_1d_indices; i++) {
        total_num_3d_indices_per_1d_seg += num_3d_indices_per_1d_seg[i];
    }

    // store the state values in the order that we need them
    double* old_g_states = (double*) malloc(sizeof(double) * total_num_3d_indices_per_1d_seg);

    vol_3d_index = 0;
    for(int i = 0; i < num_1d_indices; i++) {
        for (int j = 0; j < num_3d_indices_per_1d_seg[i]; j++, vol_3d_index++) {
            old_g_states[vol_3d_index] = g->states[indices3d[vol_3d_index]];
        }
    }

    vol_3d_index = 0;
    for(int i = 0; i<num_1d_indices; i++)
    {
        vol_1d = volumes1d[i];
        my_1d_index = indices1d[i];
        conc_1d = states[my_1d_index];


        //printf("for 1d index %d: volume 1d = %g and conc1d = %g\n", my_1d_index, vol_1d, conc_1d);
        for(int j=0; j<num_3d_indices_per_1d_seg[i]; j++, vol_3d_index++)
        {
            vol_3d = volumes3d[vol_3d_index];
            //rate is rate of change of 3d concentration
            my_3d_index = indices3d[vol_3d_index];
            rate = (rates[vol_3d_index]) * (old_g_states[vol_3d_index] - conc_1d);
            //forward euler coupling
            g->states[my_3d_index] -= dt * rate;
            states[my_1d_index] += dt * rate * vol_3d / vol_1d; 
            //printf("for 3d index %d: volume 3d = %g and states3d = %g and rate3d = %g\n", my_3d_index, vol_3d, g->states[my_3d_index], rates[vol_3d_index]);
        }
    }

    free(old_g_states);
}

void _ics_variable_hybrid_helper(ICS_Grid_node* g, const double* cvode_states_3d, double* const ydot_3d, const double* cvode_states_1d, double *const  ydot_1d)
{   
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

    for(int i = 0; i<num_1d_indices; i++)
    {
        vol_1d = volumes1d[i];
        my_1d_index = indices1d[i];
        conc_1d = cvode_states_1d[my_1d_index];
        for(int j=0; j<num_3d_indices_per_1d_seg[i]; j++, vol_3d_index++)
        {
            vol_3d = volumes3d[vol_3d_index];
            //rate is rate of change of 3d concentration
            my_3d_index = indices3d[vol_3d_index];
            rate = (rates[vol_3d_index]) * (cvode_states_3d[my_3d_index] - conc_1d);
            //forward euler coupling
            ydot_3d[my_3d_index] -= rate;
            ydot_1d[my_1d_index] += rate * vol_3d / vol_1d; 
        }
    }
}
