/******************************************************************
Author: Austin Lachance
Date: 10/28/16
Description: Defines the functions for implementing and manipulating
a linked list of Grid_nodes
******************************************************************/
#include <stdio.h>
#include <assert.h>
#include "nrnpython.h"
#include "grids.h"
#include "rxd.h"

extern int NUM_THREADS;
double *dt_ptr;
double *t_ptr;
double *h_dt_ptr;
double *h_t_ptr;
Grid_node *Parallel_grids[100] = {NULL};

// Set dt, t pointers
extern "C" void make_time_ptr(PyHocObject* my_dt_ptr, PyHocObject* my_t_ptr) {
    dt_ptr = my_dt_ptr -> u.px_;
    t_ptr = my_t_ptr -> u.px_;
}

// Make a new Grid_node given required Grid_node parameters
ECS_Grid_node *ECS_make_Grid(PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz, PyHocObject* my_alpha,
	PyHocObject* my_lambda, int bc, double bc_value, double atolscale) {
    int k;
    ECS_Grid_node *new_Grid = new ECS_Grid_node();
    assert(new_Grid);

    new_Grid->states = my_states->u.px_;
    
    /*TODO: When there are multiple grids share the largest intermediate arrays to save memory*/
    /*intermediate states for DG-ADI*/
    new_Grid->states_x = (double*)malloc(sizeof(double)*my_num_states_x*my_num_states_y*my_num_states_z);
    new_Grid->states_y = (double*)malloc(sizeof(double)*my_num_states_x*my_num_states_y*my_num_states_z);
    new_Grid->states_cur = (double*)malloc(sizeof(double)*my_num_states_x*my_num_states_y*my_num_states_z);

    new_Grid->size_x = my_num_states_x;
    new_Grid->size_y = my_num_states_y;
    new_Grid->size_z = my_num_states_z;
    
    new_Grid->dc_x = my_dc_x;
    new_Grid->dc_y = my_dc_y;
    new_Grid->dc_z = my_dc_z;

    new_Grid->dx = my_dx;
    new_Grid->dy = my_dy;
    new_Grid->dz = my_dz;

    new_Grid->concentration_list = NULL;
    new_Grid->num_concentrations = 0;
    new_Grid->current_list = NULL;
    new_Grid->num_currents = 0;

    new_Grid->next = NULL;
	new_Grid->VARIABLE_ECS_VOLUME = FALSE;

	/*Check to see if variable tortuosity/volume fraction is used*/
	if(PyFloat_Check(my_lambda))
	{
		/*apply the tortuosity*/
		new_Grid->dc_x = my_dc_x/SQ(PyFloat_AsDouble((PyObject*)my_lambda));
    	new_Grid->dc_y = my_dc_y/SQ(PyFloat_AsDouble((PyObject*)my_lambda));
	    new_Grid->dc_z = my_dc_z/SQ(PyFloat_AsDouble((PyObject*)my_lambda));
		
		new_Grid->lambda = (double*)malloc(sizeof(double));
		new_Grid->lambda[0] = SQ(PyFloat_AsDouble((PyObject*)my_lambda));
		new_Grid->get_lambda = &get_lambda_scalar;
	}
	else
	{
		new_Grid->lambda = my_lambda->u.px_;
		new_Grid->VARIABLE_ECS_VOLUME = TORTUOSITY;
		new_Grid->get_lambda = &get_lambda_array;
	}
	
	if(PyFloat_Check(my_alpha))
	{
		new_Grid->alpha = (double*)malloc(sizeof(double));
		new_Grid->alpha[0] = PyFloat_AsDouble((PyObject*)my_alpha);
		new_Grid->get_alpha = &get_alpha_scalar;

	}
	else
	{
		new_Grid->alpha = my_alpha->u.px_;
		new_Grid->VARIABLE_ECS_VOLUME = VOLUME_FRACTION;
		new_Grid->get_alpha = &get_alpha_array;	

	}
#if NRNMPI
    if(nrnmpi_use)
    {
        new_Grid->proc_offsets = (int*)malloc(nrnmpi_numprocs*sizeof(int));
        new_Grid->proc_num_currents = (int*)malloc(nrnmpi_numprocs*sizeof(int));
    }
#endif
    new_Grid->num_all_currents = 0;
    new_Grid->current_dest = NULL;
    new_Grid->all_currents = NULL;
    

    new_Grid->bc = (BoundaryConditions*)malloc(sizeof(BoundaryConditions));
    new_Grid->bc->type=bc;
    new_Grid->bc->value=bc_value;

    new_Grid->ecs_tasks = NULL;
    new_Grid->ecs_tasks = (ECSAdiGridData*)malloc(NUM_THREADS*sizeof(ECSAdiGridData));
    for(k=0; k<NUM_THREADS; k++)
    {
        new_Grid->ecs_tasks[k].scratchpad = (double*)malloc(sizeof(double) * MAX(my_num_states_x,MAX(my_num_states_y,my_num_states_z)));
        new_Grid->ecs_tasks[k].g = new_Grid;
    }


    new_Grid->ecs_adi_dir_x = (ECSAdiDirection*)malloc(sizeof(ECSAdiDirection));
    new_Grid->ecs_adi_dir_x->states_in = new_Grid->states;
    new_Grid->ecs_adi_dir_x->states_out = new_Grid->states_x;
    new_Grid->ecs_adi_dir_x->line_size = my_num_states_x;


    new_Grid->ecs_adi_dir_y = (ECSAdiDirection*)malloc(sizeof(ECSAdiDirection));
    new_Grid->ecs_adi_dir_y->states_in = new_Grid->states_x;
    new_Grid->ecs_adi_dir_y->states_out = new_Grid->states_y;
    new_Grid->ecs_adi_dir_y->line_size = my_num_states_y;



    new_Grid->ecs_adi_dir_z = (ECSAdiDirection*)malloc(sizeof(ECSAdiDirection));
    new_Grid->ecs_adi_dir_z->states_in = new_Grid->states_y;
    new_Grid->ecs_adi_dir_z->states_out = new_Grid->states_x;
    new_Grid->ecs_adi_dir_z->line_size = my_num_states_z;

    new_Grid->atolscale = atolscale;


    return new_Grid;
}


// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
/* returns the grid number
   TODO: change this to returning the pointer */
int ECS_insert(int grid_list_index, PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz, 
	PyHocObject* my_alpha, PyHocObject* my_lambda, int bc, double bc_value,
    double atolscale) {
    int i = 0;

    Grid_node *new_Grid = ECS_make_Grid(my_states, my_num_states_x, my_num_states_y, 
            my_num_states_z, my_dc_x, my_dc_y, my_dc_z, my_dx, my_dy, my_dz, 
			my_alpha, my_lambda, bc, bc_value, atolscale);

    return new_Grid->insert(grid_list_index);
}

Grid_node *ICS_make_Grid(PyHocObject* my_states, long num_nodes, long* neighbors, 
                long* ordered_x_nodes, long* ordered_y_nodes, long* ordered_z_nodes,
                long* x_line_defs, long x_lines_length, long* y_line_defs, long y_lines_length, long* z_line_defs,
                long z_lines_length, double d, double dx) {

    int k;
    ICS_Grid_node *new_Grid = new ICS_Grid_node();
    assert(new_Grid);

    new_Grid->_num_nodes = num_nodes;

    new_Grid->states = my_states->u.px_;
    new_Grid->states_x = (double*)malloc(sizeof(double)*new_Grid->_num_nodes);
    new_Grid->states_y = (double*)malloc(sizeof(double)*new_Grid->_num_nodes);
    new_Grid->states_cur = (double*)malloc(sizeof(double)*new_Grid->_num_nodes);
    new_Grid->next = NULL;

    new_Grid->size_x = new_Grid->_num_nodes;
    new_Grid->size_y = 1;
    new_Grid->size_z = 1;

    new_Grid->concentration_list = NULL;
    new_Grid->num_concentrations = 0;
    new_Grid->current_list = NULL;
    new_Grid->num_currents = 0;

    new_Grid->ics_nodes_per_seg = NULL;
    new_Grid->ics_nodes_per_seg_start_indices = NULL;
    new_Grid->ics_seg_ptrs = NULL;
    new_Grid->ics_num_segs = NULL;

    #if NRNMPI
        if(nrnmpi_use)
        {
            new_Grid->proc_offsets = (int*)malloc(nrnmpi_numprocs*sizeof(int));
            new_Grid->proc_num_currents = (int*)malloc(nrnmpi_numprocs*sizeof(int));
        }
    #endif

    new_Grid->num_all_currents = 0;
    new_Grid->current_dest = NULL;
    new_Grid->all_currents = NULL;
    
    //stores the positive x,y, and z neighbors for each node. [node0_x, node0_y, node0_z, node1_x ...]
    new_Grid->_neighbors = neighbors;
    
    /*Line definitions from Python. In pattern of [line_start_node, line_length, ...]
      Array is sorted from longest to shortest line */
    new_Grid->_sorted_x_lines = x_line_defs;
    new_Grid->_sorted_y_lines = y_line_defs;
    new_Grid->_sorted_z_lines = z_line_defs;

    //Lengths of _sorted_lines arrays. Used to find thread start and stop indices
    new_Grid->_x_lines_length = x_lines_length;
    new_Grid->_y_lines_length = y_lines_length;
    new_Grid->_z_lines_length = z_lines_length;

    //Find the maximum line length for scratchpad memory allocation
    long x_max = x_line_defs[1];
    long y_max = y_line_defs[1];
    long z_max = z_line_defs[1];
    long xy_max = (x_max > y_max) ? x_max : y_max;
    new_Grid->_line_length_max = (z_max > xy_max) ? z_max : xy_max;

    new_Grid->ics_tasks = NULL;
    new_Grid->ics_tasks = (ICSAdiGridData*)malloc(NUM_THREADS*sizeof(ICSAdiGridData));
    
    for(int k=0; k<NUM_THREADS; k++)
    {
        new_Grid->ics_tasks[k].RHS = (double*)malloc(sizeof(double) * (new_Grid->_line_length_max));
        new_Grid->ics_tasks[k].scratchpad = (double*)malloc(sizeof(double) * (new_Grid->_line_length_max-1));
        new_Grid->ics_tasks[k].g = new_Grid;
    }    
    new_Grid->ics_adi_dir_x = (ICSAdiDirection*)malloc(sizeof(ICSAdiDirection));
    new_Grid->ics_adi_dir_x->states_in = new_Grid->states_x;
    new_Grid->ics_adi_dir_x->states_out = new_Grid->states;
    new_Grid->ics_adi_dir_x->ordered_start_stop_indices = (long*)malloc(sizeof(long)*NUM_THREADS*2);
    new_Grid->ics_adi_dir_x->line_start_stop_indices = (long*)malloc(sizeof(long)*NUM_THREADS*2);
    new_Grid->ics_adi_dir_x->ordered_nodes = (long*)malloc(sizeof(long)*new_Grid->_num_nodes);
    new_Grid->ics_adi_dir_x->ordered_line_defs = (long*)malloc(sizeof(long)*new_Grid->_x_lines_length);
    new_Grid->ics_adi_dir_x->deltas = (double*)malloc(sizeof(double)*new_Grid->_num_nodes);
    new_Grid->ics_adi_dir_x->dc = d;
    new_Grid->ics_adi_dir_x->d = dx;

    new_Grid->ics_adi_dir_y = (ICSAdiDirection*)malloc(sizeof(ICSAdiDirection));
    new_Grid->ics_adi_dir_y->states_in = new_Grid->states_y;
    new_Grid->ics_adi_dir_y->states_out = new_Grid->states;
    new_Grid->ics_adi_dir_y->ordered_start_stop_indices = (long*)malloc(sizeof(long)*NUM_THREADS*2);
    new_Grid->ics_adi_dir_y->line_start_stop_indices = (long*)malloc(sizeof(long)*NUM_THREADS*2);
    new_Grid->ics_adi_dir_y->ordered_nodes = (long*)malloc(sizeof(long)*new_Grid->_num_nodes);
    new_Grid->ics_adi_dir_y->ordered_line_defs = (long*)malloc(sizeof(long)*new_Grid->_y_lines_length);
    new_Grid->ics_adi_dir_y->deltas = (double*)malloc(sizeof(double)*new_Grid->_num_nodes);
    new_Grid->ics_adi_dir_y->dc = d;
    new_Grid->ics_adi_dir_y->d = dx;

    new_Grid->ics_adi_dir_z = (ICSAdiDirection*)malloc(sizeof(ICSAdiDirection));
    new_Grid->ics_adi_dir_z->states_in = new_Grid->states_cur;
    new_Grid->ics_adi_dir_z->states_out = new_Grid->states;
    new_Grid->ics_adi_dir_z->ordered_start_stop_indices = (long*)malloc(sizeof(long)*NUM_THREADS*2);
    new_Grid->ics_adi_dir_z->line_start_stop_indices = (long*)malloc(sizeof(long)*NUM_THREADS*2);
    new_Grid->ics_adi_dir_z->ordered_nodes = (long*)malloc(sizeof(long)*new_Grid->_num_nodes);
    new_Grid->ics_adi_dir_z->ordered_line_defs = (long*)malloc(sizeof(long)*new_Grid->_z_lines_length);
    new_Grid->ics_adi_dir_z->deltas = (double*)malloc(sizeof(double)*new_Grid->_num_nodes);
    new_Grid->ics_adi_dir_z->dc = d;
    new_Grid->ics_adi_dir_z->d = dx;

    new_Grid->divide_x_work();
    new_Grid->divide_y_work();
    new_Grid->divide_z_work();

    return new_Grid;
}


// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
/* returns the grid number
   TODO: change this to returning the pointer */
int ICS_insert(int grid_list_index, PyHocObject* my_states, long num_nodes, long* neighbors,
                long* ordered_x_nodes, long* ordered_y_nodes, long* ordered_z_nodes,
                long* x_line_defs, long x_lines_length, long* y_line_defs, long y_lines_length, long* z_line_defs,
                long z_lines_length, double d, double dx) {

    //TODO change ICS_make_Grid into a constructor
    Grid_node *new_Grid = ICS_make_Grid(my_states, num_nodes, neighbors, ordered_x_nodes,
            ordered_y_nodes, ordered_z_nodes, x_line_defs, x_lines_length, y_line_defs,
            y_lines_length, z_line_defs, z_lines_length, d, dx);
    return new_Grid->insert(grid_list_index);;
}

/*Set the diffusion coefficients*/
int set_diffusion(int grid_list_index, int grid_id, double dc_x, double dc_y, double dc_z)
{
    int id = 0;
    Grid_node* node = Parallel_grids[grid_list_index];
    while(id < grid_id)
    {
        node = node->next;
        id++;
        if(node == NULL)
            return -1;
    }
    node->dc_x = dc_x;
    node->dc_y = dc_y;
    node->dc_z = dc_z;
    return 0;
}

extern "C" void ics_set_grid_concentrations(int grid_list_index, int index_in_list, int64_t* nodes_per_seg, int64_t* nodes_per_seg_start_indices, PyObject* neuron_pointers) {
    Grid_node* g;
    ssize_t i;
    ssize_t j;
    ssize_t num_nodes;
    ssize_t n = (ssize_t)PyList_Size(neuron_pointers);  //number of segments. nodes_per_seg should have the same length as neuron_pointers
                                                        
    int total_nodes = nodes_per_seg_start_indices[n];   //nodes_per_seg_lengths has length n + 1 since it has a 0 as the first start index

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    g->ics_nodes_per_seg = (int64_t*)malloc(total_nodes*sizeof(int64_t));
    g->ics_nodes_per_seg = nodes_per_seg;

    g->ics_nodes_per_seg_start_indices = (int64_t*)malloc(n*sizeof(int64_t));
    g->ics_nodes_per_seg_start_indices = nodes_per_seg_start_indices;

    g->ics_seg_ptrs = (double**)malloc(n*sizeof(double*));
    for(i = 0; i < n; i++){
        g->ics_seg_ptrs[i] = ((PyHocObject*) PyList_GET_ITEM(neuron_pointers, i)) -> u.px_;
    }

    g->ics_num_segs = n;

}



/* TODO: make this work with Grid_node ptrs instead of pairs of list indices */
extern "C" void set_grid_concentrations(int grid_list_index, int index_in_list, PyObject* grid_indices, PyObject* neuron_pointers) {
    /*
    Preconditions:

    Assumes the specified grid has been created.
    Assumes len(grid_indices) = len(neuron_pointers) and that both are proper python lists
    */
    /* TODO: note that these will need updating anytime the structure of the model changes... look at the structure change count at each advance and trigger a callback to regenerate if necessary */
    Grid_node* g;
    ssize_t i;
    ssize_t n = (ssize_t)PyList_Size(grid_indices);

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    /* free the old concentration list */
    free(g->concentration_list);

    /* allocate space for the new list */
    g->concentration_list = (Concentration_Pair*)malloc(sizeof(Concentration_Pair) * n);
    g->num_concentrations = n;

    /* populate the list */
    /* TODO: it would be more general to iterate instead of assuming lists and using list lookups */
    for (i = 0; i < n; i++) {
        /* printf("set_grid_concentrations %ld\n", i); */
        g->concentration_list[i].source = PyInt_AS_LONG(PyList_GET_ITEM(grid_indices, i));
        g->concentration_list[i].destination = ((PyHocObject*) PyList_GET_ITEM(neuron_pointers, i)) -> u.px_;
    }
}

/* TODO: make this work with Grid_node ptrs instead of pairs of list indices */
extern "C" void set_grid_currents(int grid_list_index, int index_in_list, PyObject* grid_indices, PyObject* neuron_pointers, PyObject* scale_factors) {
    /*
    Preconditions:

    Assumes the specified grid has been created.
    Assumes len(grid_indices) = len(neuron_pointers) = len(scale_factors) and that all are proper python lists
    */
    /* TODO: note that these will need updating anytime the structure of the model changes... look at the structure change count at each advance and trigger a callback to regenerate if necessary */
    Grid_node* g;
    ssize_t i;
    ssize_t n = (ssize_t)PyList_Size(grid_indices);
    long* dests;

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    /* free the old current list */
    free(g->current_list);

    /* allocate space for the new list */
    g->current_list = (Current_Triple*)malloc(sizeof(Current_Triple) * n);
    g->num_currents = n;

    /* populate the list */
    /* TODO: it would be more general to iterate instead of assuming lists and using list lookups */
    for (i = 0; i < n; i++) {
        g->current_list[i].destination = PyInt_AS_LONG(PyList_GET_ITEM(grid_indices, i));
        g->current_list[i].scale_factor = PyFloat_AS_DOUBLE(PyList_GET_ITEM(scale_factors, i));
        g->current_list[i].source = ((PyHocObject*) PyList_GET_ITEM(neuron_pointers, i)) -> u.px_;
        /* printf("set_grid_currents %ld out of %ld, %ld, %ld\n", i, n, PyList_Size(neuron_pointers), PyList_Size(scale_factors)); */
    }/*
    if (PyErr_Occurred()) {
        printf("uh oh\n");
        PyErr_PrintEx(0);
    } else {
        printf("no problems\n");
    }
    PyErr_Clear(); */

#if NRNMPI
    if(nrnmpi_use)
    {
        /*Gather an array of the number of currents for each process*/
        g->proc_num_currents[nrnmpi_myid] = n;
        nrnmpi_int_allgather_inplace(g->proc_num_currents, 1);
        
        g->proc_offsets[0] = 0; 
        for(i = 1; i < nrnmpi_numprocs; i++)
            g->proc_offsets[i] =  g->proc_offsets[i-1] + g->proc_num_currents[i-1];
         g->num_all_currents = g->proc_offsets[i-1] + g->proc_num_currents[i-1];
        
        /*Copy array of current destinations (index to states) across all processes*/
        free(g->current_dest);
        free(g->all_currents);
        g->current_dest = (long*)malloc(g->num_all_currents*sizeof(long));
        g->all_currents = (double*)malloc(g->num_all_currents*sizeof(double));
        dests = g->current_dest + g->proc_offsets[nrnmpi_myid];
        /*TODO: Get rid of duplication with current_list*/
        for(i = 0; i < n; i++)
        {
            dests[i] = g->current_list[i].destination;
        }
        nrnmpi_long_allgatherv_inplace(g->current_dest, g->proc_num_currents, g->proc_offsets);
    }
    else
    {
        free(g->all_currents);
        g->all_currents = (double*)malloc(sizeof(double) * g->num_currents);
        g->num_all_currents = g->num_currents;
    }
#else
    free(g->all_currents);
    g->all_currents = (double*)malloc(sizeof(double) * g->num_currents);
    g->num_all_currents = g->num_currents;
#endif
}

// Delete a specific Grid_node from the list
int remove(Grid_node **head, Grid_node *find) {
    if(*head == find) {
        Grid_node *temp = *head;
        *head = (*head)->next;
        temp->free_Grid();
        return 1;
    }
    Grid_node *temp = *head;
    while(temp->next != find) {
        temp = temp->next;
    }
    if(!temp) return 0;
    Grid_node *delete_me = temp->next;
    temp->next = delete_me->next;
    delete_me->free_Grid();
    return 1;
}

extern "C" void delete_by_id(int id) {
    Grid_node *grid;
    int k;
    for(k = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid -> next, k++)
    {
        if(k == id)
        {
            remove(Parallel_grids, grid);
            break;
        }
    }
}


// Destroy the list located at list_index and free all memory
void empty_list(int list_index) {
    Grid_node **head = &(Parallel_grids[list_index]);
    while(*head != NULL) {
        Grid_node *old_head = *head;
        *head = (*head)->next;
        old_head->free_Grid();
    }
}

static double get_alpha_scalar(double* alpha, int idx)
{
	return alpha[0];
}
static double get_alpha_array(double* alpha, int idx)
{
	return alpha[idx];
}


static double get_lambda_scalar(double* lambda, int idx)
{
	return 1.; /*already rescale the diffusion coefficients*/
}
static double get_lambda_array(double* lambda, int idx)
{
	return lambda[idx];
}

int Grid_node::insert(int grid_list_index){

    int i = 0;

    Grid_node **head = &(Parallel_grids[grid_list_index]);
    Grid_node *save;

    if(!(*head)) {
        *head = this;
        save = *head;
    }
    else {
		i++;	/*count the head as the first grid*/
        save = *head;
        Grid_node *end = *head;
        while(end->next != NULL) {
            i++;
            end = end->next;
        }
        end->next = this;
    }
    return i;
}

/*****************************************************************************
*
* Begin ECS_Grid_node Functions
*
*****************************************************************************/

void ECS_Grid_node::set_num_threads(const int n)
{
    int i;
    if(ecs_tasks != NULL)
    {
        for(i = 0; i<NUM_THREADS; i++)
        {
            free(ecs_tasks[i].scratchpad);
        }
    }
    free(ecs_tasks);
    ecs_tasks = (ECSAdiGridData*)malloc(NUM_THREADS*sizeof(ECSAdiGridData));
    for(i=0; i<n; i++)
    {
        ecs_tasks[i].scratchpad = (double*)malloc(sizeof(double) * MAX(size_x,MAX(size_y, size_z)));
        ecs_tasks[i].g = this;
    }
}

void ECS_Grid_node::do_grid_currents(double dt, int grid_id)
{
    //TODO: Avoid this call and put the code from do_currents in this method
    do_currents(this, states_cur, dt, grid_id);
}

void ECS_Grid_node::volume_setup()
{
    switch(VARIABLE_ECS_VOLUME)
    {
        case VOLUME_FRACTION:
            ecs_set_adi_vol(this);
            break;
        case TORTUOSITY:
            ecs_set_adi_tort(this);
            break;
        default:
            ecs_set_adi_homogeneous(this);
    }    
}

int ECS_Grid_node::dg_adi()
{
    //double* tmp;
    /* first step: advance the x direction */
    ecs_run_threaded_dg_adi(size_y, size_z, this, ecs_adi_dir_x, size_x);

    /* second step: advance the y direction */
    ecs_run_threaded_dg_adi(size_x, size_z, this, ecs_adi_dir_y, size_y);

    /* third step: advance the z direction */
    ecs_run_threaded_dg_adi(size_x, size_y, this, ecs_adi_dir_z, size_z);

    /* transfer data */
    /*TODO: Avoid copy by switching pointers and updating Python copy
    tmp = g->states;
    g->states = g->adi_dir_z->states_out;
    g->adi_dir_z->states_out = tmp;
    */
    memcpy(states, ecs_adi_dir_z->states_out, sizeof(double)*size_x*size_y*size_z);
    return 0;
}

void ECS_Grid_node::scatter_grid_concentrations()
{
    ssize_t i, n;
    Concentration_Pair* cp;
    double* my_states;  

    my_states = states;
    n = num_concentrations;
    cp = concentration_list;
    for (i = 0; i < n; i++) {
        (*cp[i].destination) = states[cp[i].source];
    }  
}

// Free a single Grid_node
void ECS_Grid_node::free_Grid(){
    int i;
    free(states_x);
    free(states_y);
    free(states_cur);
    free(concentration_list);
    free(current_list);
	free(alpha);
	free(lambda);
    free(bc);
    free(current_dest);
#if NRNMPI
    if(nrnmpi_use)
    {
        free(proc_offsets);
        free(proc_num_currents);
    }
#endif
    free(all_currents);
    free(ecs_adi_dir_x);
    free(ecs_adi_dir_y);
    free(ecs_adi_dir_z);


    if(ecs_tasks != NULL)
    {   
        for(i=0; i<NUM_THREADS; i++)
        {
            free(ecs_tasks[i].scratchpad);
        }
    }
    free(ecs_tasks);
    free(this);
}

/*****************************************************************************
*
* Begin ICS_Grid_node Functions
*
*****************************************************************************/
static int find_min_element_index(const int thread_sizes[]){
    int new_min_index = 0;
    int min_element = thread_sizes[0];
    for(int i = 0; i < NUM_THREADS; i++){
        if(min_element > thread_sizes[i]){
            min_element = thread_sizes[i];
            new_min_index = i;
        }
    }
    return new_min_index;
}

void ICS_Grid_node::divide_x_work(){
    int i, j, k;
    //nodes in each thread. (work each thread has to do)
    int* nodes_per_thread = (int*)calloc(NUM_THREADS, sizeof(int));
    //number of lines in each thread
    int* lines_per_thread = (int*)calloc(NUM_THREADS, sizeof(int));
    //To determine which index to put the start node and line length in thread_line_defs
    int* thread_idx_counter = (int*)calloc(NUM_THREADS, sizeof(int));
    //To determine which thread array to put the start node and line length in thread_line_defs
    int line_thread_id[_x_lines_length / 2];
    //Array of NUM_THREADS arrays that hold the line defs for each thread
    int** thread_line_defs = (int**)malloc(NUM_THREADS*sizeof(int*));

    int min_index = 0;

    //Find the total line length for each thread
    for(i = 0; i < _x_lines_length; i+=2){
        min_index = find_min_element_index(nodes_per_thread); 
        nodes_per_thread[min_index] += _sorted_x_lines[i+1];
        line_thread_id[i/2] = min_index;
        lines_per_thread[min_index] += 1;
    } 

    //Allocate memory for each array in thread_line_defs
    for(i = 0; i < NUM_THREADS; i++){
        thread_line_defs[i] = (int*)malloc(lines_per_thread[i]*2*sizeof(int));
    }
    
    int thread_idx;
    int line_idx;

    //Populate each array of thread_line_defs
    for(i = 0; i < _x_lines_length; i+=2){
        thread_idx = line_thread_id[i/2];
        line_idx = thread_idx_counter[thread_idx];
        thread_line_defs[thread_idx][line_idx] = _sorted_x_lines[i];
        thread_line_defs[thread_idx][line_idx+1] = _sorted_x_lines[i+1];
        thread_idx_counter[thread_idx] += 2; 
    }

    //Populate ordered_line_def
    int ordered_line_def_counter = 0;
    for(i = 0; i< NUM_THREADS; i++){
        for(j=0; j<lines_per_thread[i]*2;j++){
            ics_adi_dir_x->ordered_line_defs[ordered_line_def_counter] = thread_line_defs[i][j];
            ordered_line_def_counter++;
        }
    }
    //Set direction node and line start/stop indices
    //thread 0 start and stop
    ics_adi_dir_x->ordered_start_stop_indices[0] = 0; 
    ics_adi_dir_x->ordered_start_stop_indices[1] = nodes_per_thread[0];
    ics_adi_dir_x->line_start_stop_indices[0] = 0; 
    ics_adi_dir_x->line_start_stop_indices[1] = lines_per_thread[0]*2;
    long start_node;
    long line_start_node;
    for(i = 2; i < NUM_THREADS * 2; i+=2){
        start_node = ics_adi_dir_x->ordered_start_stop_indices[i-1];
        ics_adi_dir_x->ordered_start_stop_indices[i] = start_node;
        ics_adi_dir_x->ordered_start_stop_indices[i+1] = start_node + nodes_per_thread[i/2];

        line_start_node = ics_adi_dir_x->line_start_stop_indices[i-1];
        ics_adi_dir_x->line_start_stop_indices[i] = line_start_node;
        ics_adi_dir_x->line_start_stop_indices[i+1] = line_start_node + lines_per_thread[i/2]*2;
        
    }

    //Put the Nodes in order in the ordered_nodes array
    int ordered_node_idx_counter = 0;
    int current_node;
    for(i = 0; i < NUM_THREADS; i++){
        for(j = 0; j < lines_per_thread[i] * 2; j+=2){
            current_node = thread_line_defs[i][j];
            ics_adi_dir_x->ordered_nodes[ordered_node_idx_counter] = current_node;
            ics_adi_dir_x->states_in[ordered_node_idx_counter] = states[current_node];
            ordered_node_idx_counter++;
            for(k = 1; k < thread_line_defs[i][j+1]; k++){
                  current_node = _neighbors[current_node * 3];
                  ics_adi_dir_x->ordered_nodes[ordered_node_idx_counter] = current_node;
                  ics_adi_dir_x->states_in[ordered_node_idx_counter] = states[current_node];
                  ordered_node_idx_counter++;
            }
        }
    }

    //Delete thread_line_defs array
    for(i = 0; i < NUM_THREADS; i++){
        delete thread_line_defs[i];
    }
    delete thread_line_defs;
}

void ICS_Grid_node::divide_y_work(){
    int i, j, k;
    //nodes in each thread. (work each thread has to do)
    int* nodes_per_thread = (int*)calloc(NUM_THREADS, sizeof(int));
    //number of lines in each thread
    int* lines_per_thread = (int*)calloc(NUM_THREADS, sizeof(int));
    //To determine which index to put the start node and line length in thread_line_defs
    int* thread_idx_counter = (int*)calloc(NUM_THREADS, sizeof(int));
    //To determine which thread array to put the start node and line length in thread_line_defs
    int line_thread_id[_y_lines_length / 2];
    //Array of NUM_THREADS arrays that hold the line defs for each thread
    int** thread_line_defs = (int**)malloc(NUM_THREADS*sizeof(int*));

    int min_index = 0;

    //Find the total line length for each thread
    for(i = 0; i < _y_lines_length; i+=2){
        min_index = find_min_element_index(nodes_per_thread); 
        nodes_per_thread[min_index] += _sorted_y_lines[i+1];
        line_thread_id[i/2] = min_index;
        lines_per_thread[min_index] += 1;
    } 

    //Allocate memory for each array in thread_line_defs
    for(i = 0; i < NUM_THREADS; i++){
        thread_line_defs[i] = (int*)malloc(lines_per_thread[i]*2*sizeof(int));
    }
    
    int thread_idx;
    int line_idx;

    //Populate each array of thread_line_defs
    for(i = 0; i < _y_lines_length; i+=2){
        thread_idx = line_thread_id[i/2];
        line_idx = thread_idx_counter[thread_idx];
        thread_line_defs[thread_idx][line_idx] = _sorted_y_lines[i];
        thread_line_defs[thread_idx][line_idx+1] = _sorted_y_lines[i+1];
        thread_idx_counter[thread_idx] += 2; 
    }

    //Populate ordered_line_def
    int ordered_line_def_counter = 0;
    for(i = 0; i< NUM_THREADS; i++){
        for(j=0; j<lines_per_thread[i]*2;j++){
            ics_adi_dir_y->ordered_line_defs[ordered_line_def_counter] = thread_line_defs[i][j];
            ordered_line_def_counter++;
        }
    }

    //Set direction start/stop indices
    //thread 0 start and stop
    ics_adi_dir_y->ordered_start_stop_indices[0] = 0; 
    ics_adi_dir_y->ordered_start_stop_indices[1] = nodes_per_thread[0];
    ics_adi_dir_y->line_start_stop_indices[0] = 0; 
    ics_adi_dir_y->line_start_stop_indices[1] = lines_per_thread[0]*2;

    long start_node;
    long line_start_node;
    for(i = 2; i < NUM_THREADS * 2; i+=2){
        start_node = ics_adi_dir_y->ordered_start_stop_indices[i-1];
        ics_adi_dir_y->ordered_start_stop_indices[i] = start_node;
        ics_adi_dir_y->ordered_start_stop_indices[i+1] = start_node + nodes_per_thread[i/2];

        line_start_node = ics_adi_dir_y->line_start_stop_indices[i-1];
        ics_adi_dir_y->line_start_stop_indices[i] = line_start_node;
        ics_adi_dir_y->line_start_stop_indices[i+1] = line_start_node + (lines_per_thread[i/2]*2);
    }

    //Put the Nodes in order in the ordered_nodes array
    int ordered_node_idx_counter = 0;
    int current_node;
    for(i = 0; i < NUM_THREADS; i++){
        for(j = 0; j < lines_per_thread[i] * 2; j+=2){
            current_node = thread_line_defs[i][j];
            ics_adi_dir_y->ordered_nodes[ordered_node_idx_counter] = current_node;
            ics_adi_dir_y->states_in[ordered_node_idx_counter] = states[current_node];
            ordered_node_idx_counter++;
            for(k = 1; k < thread_line_defs[i][j+1]; k++){
                  current_node = _neighbors[(current_node * 3) + 1];
                  ics_adi_dir_y->ordered_nodes[ordered_node_idx_counter] = current_node;
                  ics_adi_dir_y->states_in[ordered_node_idx_counter] = states[current_node];
                  ordered_node_idx_counter++;
            }
        }
    }

    //Delete thread_line_defs array
    for(i = 0; i < NUM_THREADS; i++){
        delete thread_line_defs[i];
    }
    delete thread_line_defs;
}

void ICS_Grid_node::divide_z_work(){
    int i, j, k;
    //nodes in each thread. (work each thread has to do)
    int* nodes_per_thread = (int*)calloc(NUM_THREADS, sizeof(int));
    //number of lines in each thread
    int* lines_per_thread = (int*)calloc(NUM_THREADS, sizeof(int));
    //To determine which index to put the start node and line length in thread_line_defs
    int* thread_idx_counter = (int*)calloc(NUM_THREADS, sizeof(int));
    //To determine which thread array to put the start node and line length in thread_line_defs
    int line_thread_id[_z_lines_length / 2];
    //Array of NUM_THREADS arrays that hold the line defs for each thread
    int** thread_line_defs = (int**)malloc(NUM_THREADS*sizeof(int*));

    int min_index = 0;

    //Find the total line length for each thread
    for(i = 0; i < _z_lines_length; i+=2){
        min_index = find_min_element_index(nodes_per_thread); 
        nodes_per_thread[min_index] += _sorted_z_lines[i+1];
        line_thread_id[i/2] = min_index;
        lines_per_thread[min_index] += 1;
    } 

    //Allocate memory for each array in thread_line_defs
    //Add indices to Grid_data
    for(i = 0; i < NUM_THREADS; i++){
        thread_line_defs[i] = (int*)malloc(lines_per_thread[i]*2*sizeof(int));
    }
    
    int thread_idx;
    int line_idx;

    //Populate each array of thread_line_defs
    for(i = 0; i < _z_lines_length; i+=2){
        thread_idx = line_thread_id[i/2];
        line_idx = thread_idx_counter[thread_idx];
        thread_line_defs[thread_idx][line_idx] = _sorted_z_lines[i];
        thread_line_defs[thread_idx][line_idx+1] = _sorted_z_lines[i+1];
        thread_idx_counter[thread_idx] += 2; 
    }

    //Populate ordered_line_def
    int ordered_line_def_counter = 0;
    for(i = 0; i< NUM_THREADS; i++){
        for(j=0; j<lines_per_thread[i]*2;j++){
            ics_adi_dir_z->ordered_line_defs[ordered_line_def_counter] = thread_line_defs[i][j];
            ordered_line_def_counter++;
        }
    }

    //Set direction start/stop indices
    //thread 0 start and stop
    ics_adi_dir_z->ordered_start_stop_indices[0] = 0; 
    ics_adi_dir_z->ordered_start_stop_indices[1] = nodes_per_thread[0];
    ics_adi_dir_z->line_start_stop_indices[0] = 0; 
    ics_adi_dir_z->line_start_stop_indices[1] = lines_per_thread[0]*2;

    long start_node;
    long line_start_node;
    for(i = 2; i < NUM_THREADS * 2; i+=2){
        start_node = ics_adi_dir_z->ordered_start_stop_indices[i-1];
        ics_adi_dir_z->ordered_start_stop_indices[i] = start_node;
        ics_adi_dir_z->ordered_start_stop_indices[i+1] = start_node + nodes_per_thread[i/2];

        line_start_node = ics_adi_dir_z->line_start_stop_indices[i-1];
        ics_adi_dir_z->line_start_stop_indices[i] = line_start_node;
        ics_adi_dir_z->line_start_stop_indices[i+1] = line_start_node + lines_per_thread[i/2]*2;
    }

    //Put the Nodes in order in the ordered_nodes array
    int ordered_node_idx_counter = 0;
    int current_node;
    for(i = 0; i < NUM_THREADS; i++){
        for(j = 0; j < lines_per_thread[i] * 2; j+=2){
            current_node = thread_line_defs[i][j];
            ics_adi_dir_z->ordered_nodes[ordered_node_idx_counter] = current_node;
            ics_adi_dir_z->states_in[ordered_node_idx_counter] = states[current_node];
            ordered_node_idx_counter++;
            for(k = 1; k < thread_line_defs[i][j+1]; k++){
                  current_node = _neighbors[(current_node * 3) + 2];
                  ics_adi_dir_z->ordered_nodes[ordered_node_idx_counter] = current_node;
                  ics_adi_dir_z->states_in[ordered_node_idx_counter] = states[current_node];
                  ordered_node_idx_counter++;
            }
        }
    }

    //Delete thread_line_defs array
    for(i = 0; i < NUM_THREADS; i++){
        delete thread_line_defs[i];
    }
    delete thread_line_defs;
}


void ICS_Grid_node::set_num_threads(const int n)
{
    int i;
    if(ics_tasks != NULL)
    {
        for(i = 0; i<NUM_THREADS; i++)
        {
            free(ics_tasks[i].scratchpad);
        }
    }
    free(ics_tasks);
    ics_tasks = (ICSAdiGridData*)malloc(NUM_THREADS*sizeof(ICSAdiGridData));
    for(i=0; i<n; i++)
    {
        ics_tasks[i].scratchpad = (double*)malloc(sizeof(double) * _line_length_max);
        ics_tasks[i].g = this;
    }
}

void ICS_Grid_node::do_grid_currents(double dt, int grid_id)
{

}

void ICS_Grid_node::volume_setup()
{
    ics_adi_dir_x->ics_dg_adi_dir = ics_dg_adi_x;
    ics_adi_dir_y->ics_dg_adi_dir = ics_dg_adi_y;
    ics_adi_dir_z->ics_dg_adi_dir = ics_dg_adi_z;
}

int ICS_Grid_node::dg_adi()
{
    run_threaded_deltas(this, ics_adi_dir_x);
    run_threaded_deltas(this, ics_adi_dir_y);
    run_threaded_deltas(this, ics_adi_dir_z);
    run_threaded_ics_dg_adi(this, ics_adi_dir_x);
    run_threaded_ics_dg_adi(this, ics_adi_dir_y);
    run_threaded_ics_dg_adi(this, ics_adi_dir_z);

    //memcpy(states, ics_adi_dir_z->states_out, sizeof(double)*_num_nodes)*/
    return 0;
}

void ICS_Grid_node::scatter_grid_concentrations()
{
    ssize_t i, j, n;
    double* my_states;
    double total_seg_concentration;  
    double average_seg_concentration;
    int seg_start_index, seg_stop_index;

    my_states = states;
    n = ics_num_segs;

    for (i = 0; i < n; i++) {
        total_seg_concentration = 0.0;
        seg_start_index = ics_nodes_per_seg_start_indices[i];
        seg_stop_index = ics_nodes_per_seg_start_indices[i+1];
        for(j = seg_start_index; j < seg_stop_index; j++){
            total_seg_concentration += states[ics_nodes_per_seg[j]];
        }
        average_seg_concentration = total_seg_concentration / (seg_stop_index - seg_start_index);

        *ics_seg_ptrs[i] = average_seg_concentration;
    } 
}

// Free a single Grid_node
void ICS_Grid_node::free_Grid(){
    int i;
    free(states_x);
    free(states_y);
    free(states_cur);
    free(concentration_list);
    free(current_list);
	free(alpha);
	free(lambda);
    free(bc);
    free(current_dest);
#if NRNMPI
    if(nrnmpi_use)
    {
        free(proc_offsets);
        free(proc_num_currents);
    }
#endif
    free(all_currents);
    free(ics_adi_dir_x);
    free(ics_adi_dir_y);
    free(ics_adi_dir_z);


    if(ics_tasks != NULL)
    {   
        for(i=0; i<NUM_THREADS; i++)
        {
            free(ics_tasks[i].scratchpad);
        }
    }
    free(ics_tasks);
    free(this);
}