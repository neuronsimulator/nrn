/******************************************************************
Author: Austin Lachance
Date: 10/28/16
Description: Defines the functions for implementing and manipulating
a linked list of Grid_nodes
******************************************************************/
#include <stdio.h>
#include <assert.h>
#include "grids.h"
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif


double *dt_ptr;
double* t_ptr;
Grid_node *Parallel_grids[100] = {NULL};

// Set ∆t, t pointers
void make_time_ptr(PyHocObject* my_dt_ptr, PyHocObject* my_t_ptr) {
    dt_ptr = my_dt_ptr -> u.px_;
    t_ptr = my_t_ptr -> u.px_;
}

// Add Flux * "flux" to a given grid *grid
void add_flux(Grid_node **grid, Flux* flux) {
    (*grid)->flux_list = flux;
}

// Make a new Grid_node given required Grid_node parameters
Grid_node *make_Grid(PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz, PyHocObject* my_alpha,
	PyHocObject* my_lambda) {
    Grid_node *new_Grid = malloc(sizeof(Grid_node));
    assert(new_Grid);

    new_Grid->states = my_states->u.px_;

    new_Grid->size_x = my_num_states_x;
    new_Grid->size_y = my_num_states_y;
    new_Grid->size_z = my_num_states_z;

    new_Grid->flux_list = NULL;

    new_Grid->dc_x = my_dc_x;
    new_Grid->dc_y = my_dc_y;
    new_Grid->dc_z = my_dc_z;

    new_Grid->dx = my_dx;
    new_Grid->dy = my_dy;
    new_Grid->dz = my_dz;
    new_Grid->old_states = (double *) malloc(sizeof(double) * new_Grid->size_x * 
    new_Grid->size_y * new_Grid->size_z);

    assert(new_Grid->old_states);

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

    return new_Grid;
}


// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
/* returns the grid number
   TODO: change this to returning the pointer */
int insert(int grid_list_index, PyHocObject* my_states, int my_num_states_x, 
    int my_num_states_y, int my_num_states_z, double my_dc_x, double my_dc_y,
    double my_dc_z, double my_dx, double my_dy, double my_dz, 
	PyHocObject* my_alpha, PyHocObject* my_lambda) {
    int i = 0;


    Grid_node *new_Grid = make_Grid(my_states, my_num_states_x, my_num_states_y, 
            my_num_states_z, my_dc_x, my_dc_y, my_dc_z, my_dx, my_dy, my_dz, 
			my_alpha, my_lambda);
    Grid_node **head = &(Parallel_grids[grid_list_index]);
    Grid_node *save;

    if(!(*head)) {
        *head = new_Grid;
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
        end->next = new_Grid;
    }

	/*
    while(save != NULL) {
        printf("SIZE X: %d SIZE Y: %d SIZE Z: %d\n", save->size_x, save->size_y, save->size_z);
        save = save->next;
    }
	*/

    return i;
}

/* TODO: make this work with Grid_node ptrs instead of pairs of list indices */
void set_grid_concentrations(int grid_list_index, int index_in_list, PyObject* grid_indices, PyObject* neuron_pointers) {
    /*
    Preconditions:

    Assumes the specified grid has been created.
    Assumes len(grid_indices) = len(neuron_pointers) and that both are proper python lists
    */
    /* TODO: note that these will need updating anytime the structure of the model changes... look at the structure change count at each advance and trigger a callback to regenerate if necessary */
    Grid_node* g;
    Py_ssize_t i;
    Py_ssize_t n = PyList_Size(grid_indices);

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    /* free the old concentration list */
    free(g->concentration_list);

    /* allocate space for the new list */
    g->concentration_list = malloc(sizeof(Concentration_Pair) * n);
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
void set_grid_currents(int grid_list_index, int index_in_list, PyObject* grid_indices, PyObject* neuron_pointers, PyObject* scale_factors) {
    /*
    Preconditions:

    Assumes the specified grid has been created.
    Assumes len(grid_indices) = len(neuron_pointers) = len(scale_factors) and that all are proper python lists
    */
    /* TODO: note that these will need updating anytime the structure of the model changes... look at the structure change count at each advance and trigger a callback to regenerate if necessary */
    Grid_node* g;
    Py_ssize_t i;
    Py_ssize_t n = PyList_Size(grid_indices);

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    /* free the old current list */
    free(g->current_list);

    /* allocate space for the new list */
    g->current_list = malloc(sizeof(Current_Triple) * n);
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
}

// Free a single Grid_node
void free_Grid(Grid_node *grid) {
    free(grid->states);
    free(grid->old_states);
    free(grid->flux_list);
    free(grid->concentration_list);
    free(grid->current_list);
	free(grid->alpha);
	free(grid->lambda);
    free(grid);
}

// Insert a Grid_node into the linked list
/*void insert(Grid_node **head, Grid_node *new_Grid) {
    if(!(*head)) {
        *head = new_Grid;
        return;
    }

    Grid_node *end = *head;
    while(end->next != NULL) {
        end = end->next;
    }
    end->next = new_Grid;
}*/

// Delete a specific Grid_node from the list
int delete(Grid_node **head, Grid_node *find) {
    if(*head == find) {
        Grid_node *temp = *head;
        *head = (*head)->next;
        free_Grid(temp);
        return 1;
    }
    Grid_node *temp = *head;
    while(temp->next != find) {
        temp = temp->next;
    }
    if(!temp) return 0;
    Grid_node *delete_me = temp->next;
    temp->next = delete_me->next;
    free_Grid(delete_me);
    return 1;
}


// Destroy the list located at list_index and free all memory
void empty_list(int list_index) {
    Grid_node **head = &(Parallel_grids[list_index]);
    while(*head != NULL) {
        Grid_node *old_head = *head;
        *head = (*head)->next;
        free_Grid(old_head);
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

