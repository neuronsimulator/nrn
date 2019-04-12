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

extern int NUM_THREADS;
extern TaskQueue* AllTasks;
const int ICS_PREFETCH = 3;

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