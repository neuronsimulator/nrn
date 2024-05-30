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
double* dt_ptr;
double* t_ptr;
Grid_node* Parallel_grids[100] = {NULL};

/* globals used by ECS do_currents */
extern TaskQueue* AllTasks;
/*Current from multicompartment reations*/
extern int _memb_curr_total;
extern int* _rxd_induced_currents_grid;
extern int* _rxd_induced_currents_ecs_idx;
extern double* _rxd_induced_currents_ecs;
extern double* _rxd_induced_currents_scale;

// Set dt, t pointers
extern "C" void make_time_ptr(PyHocObject* my_dt_ptr, PyHocObject* my_t_ptr) {
    dt_ptr = static_cast<double*>(my_dt_ptr->u.px_);
    t_ptr = static_cast<double*>(my_t_ptr->u.px_);
}

static double get_alpha_scalar(double* alpha, int) {
    return alpha[0];
}
static double get_alpha_array(double* alpha, int idx) {
    return alpha[idx];
}


static double get_permeability_scalar(double*, int) {
    return 1.; /*already rescale the diffusion coefficients*/
}
static double get_permeability_array(double* permeability, int idx) {
    return permeability[idx];
}

// Make a new Grid_node given required Grid_node parameters
ECS_Grid_node::ECS_Grid_node(){};
ECS_Grid_node::ECS_Grid_node(PyHocObject* my_states,
                             int my_num_states_x,
                             int my_num_states_y,
                             int my_num_states_z,
                             double my_dc_x,
                             double my_dc_y,
                             double my_dc_z,
                             double my_dx,
                             double my_dy,
                             double my_dz,
                             PyHocObject* my_alpha,
                             PyHocObject* my_permeability,
                             int bc_type,
                             double bc_value,
                             double atolscale) {
    int k;
    states = static_cast<double*>(my_states->u.px_);

    /*TODO: When there are multiple grids share the largest intermediate arrays to save memory*/
    /*intermediate states for DG-ADI*/
    states_x = (double*) malloc(sizeof(double) * my_num_states_x * my_num_states_y *
                                my_num_states_z);
    states_y = (double*) malloc(sizeof(double) * my_num_states_x * my_num_states_y *
                                my_num_states_z);
    states_cur = (double*) malloc(sizeof(double) * my_num_states_x * my_num_states_y *
                                  my_num_states_z);

    size_x = my_num_states_x;
    size_y = my_num_states_y;
    size_z = my_num_states_z;

    dc_x = my_dc_x;
    dc_y = my_dc_y;
    dc_z = my_dc_z;
    diffusable = (dc_x > 0) || (dc_y > 0) || (dc_z > 0);

    dx = my_dx;
    dy = my_dy;
    dz = my_dz;

    concentration_list = NULL;
    num_concentrations = 0;
    current_list = NULL;
    num_currents = 0;

    next = NULL;
    VARIABLE_ECS_VOLUME = FALSE;

    /*Check to see if variable tortuosity/volume fraction is used*/
    if (PyFloat_Check(my_permeability)) {
        /*note permeability is the tortuosity squared*/
        permeability = (double*) malloc(sizeof(double));
        permeability[0] = PyFloat_AsDouble((PyObject*) my_permeability);
        get_permeability = &get_permeability_scalar;

        /*apply the tortuosity*/
        dc_x = my_dc_x * permeability[0];
        dc_y = my_dc_y * permeability[0];
        dc_z = my_dc_z * permeability[0];
    } else {
        permeability = static_cast<double*>(my_permeability->u.px_);
        VARIABLE_ECS_VOLUME = TORTUOSITY;
        get_permeability = &get_permeability_array;
    }

    if (PyFloat_Check(my_alpha)) {
        alpha = (double*) malloc(sizeof(double));
        alpha[0] = PyFloat_AsDouble((PyObject*) my_alpha);
        get_alpha = &get_alpha_scalar;

    } else {
        alpha = static_cast<double*>(my_alpha->u.px_);
        VARIABLE_ECS_VOLUME = VOLUME_FRACTION;
        get_alpha = &get_alpha_array;
    }
#if NRNMPI
    if (nrnmpi_use) {
        proc_offsets = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_num_currents = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_flux_offsets = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_num_fluxes = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_num_reactions = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_num_reaction_states = (int*) calloc(nrnmpi_numprocs, sizeof(int*));
        proc_induced_current_count = (int*) calloc(nrnmpi_numprocs, sizeof(int*));
        proc_induced_current_offset = (int*) calloc(nrnmpi_numprocs, sizeof(int*));
    }
#endif
    all_reaction_indices = NULL;
    reaction_indices = NULL;
    all_reaction_states = NULL;
    react_offsets = (int*) calloc(1, sizeof(int));
    multicompartment_inititalized = TRUE;
    total_reaction_states = 0;
    react_offset_count = 1;
    num_all_currents = 0;
    current_dest = NULL;
    all_currents = NULL;
    induced_currents_scale = NULL;
    induced_currents_index = NULL;
    induced_currents = NULL;
    local_induced_currents = NULL;
    induced_current_count = 0;

    bc = (BoundaryConditions*) malloc(sizeof(BoundaryConditions));
    bc->type = bc_type;
    bc->value = bc_value;

    ecs_tasks = NULL;
    ecs_tasks = (ECSAdiGridData*) malloc(NUM_THREADS * sizeof(ECSAdiGridData));
    for (k = 0; k < NUM_THREADS; k++) {
        ecs_tasks[k].scratchpad = (double*) malloc(
            sizeof(double) * MAX(my_num_states_x, MAX(my_num_states_y, my_num_states_z)));
        ecs_tasks[k].g = this;
    }


    ecs_adi_dir_x = (ECSAdiDirection*) malloc(sizeof(ECSAdiDirection));
    ecs_adi_dir_x->states_in = states;
    ecs_adi_dir_x->states_out = states_x;
    ecs_adi_dir_x->line_size = my_num_states_x;


    ecs_adi_dir_y = (ECSAdiDirection*) malloc(sizeof(ECSAdiDirection));
    ecs_adi_dir_y->states_in = states_x;
    ecs_adi_dir_y->states_out = states_y;
    ecs_adi_dir_y->line_size = my_num_states_y;


    ecs_adi_dir_z = (ECSAdiDirection*) malloc(sizeof(ECSAdiDirection));
    ecs_adi_dir_z->states_in = states_y;
    ecs_adi_dir_z->states_out = states_x;
    ecs_adi_dir_z->line_size = my_num_states_z;

    this->atolscale = atolscale;

    node_flux_count = 0;
    node_flux_idx = NULL;
    node_flux_scale = NULL;
    node_flux_src = NULL;
    hybrid = false;
    volume_setup();
}


// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
/* returns the grid number
   TODO: change this to returning the pointer */
extern "C" int ECS_insert(int grid_list_index,
                          PyHocObject* my_states,
                          int my_num_states_x,
                          int my_num_states_y,
                          int my_num_states_z,
                          double my_dc_x,
                          double my_dc_y,
                          double my_dc_z,
                          double my_dx,
                          double my_dy,
                          double my_dz,
                          PyHocObject* my_alpha,
                          PyHocObject* my_permeability,
                          int bc,
                          double bc_value,
                          double atolscale) {
    ECS_Grid_node* new_Grid = new ECS_Grid_node(my_states,
                                                my_num_states_x,
                                                my_num_states_y,
                                                my_num_states_z,
                                                my_dc_x,
                                                my_dc_y,
                                                my_dc_z,
                                                my_dx,
                                                my_dy,
                                                my_dz,
                                                my_alpha,
                                                my_permeability,
                                                bc,
                                                bc_value,
                                                atolscale);

    return new_Grid->insert(grid_list_index);
}

ICS_Grid_node::ICS_Grid_node(){};
ICS_Grid_node::ICS_Grid_node(PyHocObject* my_states,
                             long num_nodes,
                             long* neighbors,
                             long* x_line_defs,
                             long x_lines_length,
                             long* y_line_defs,
                             long y_lines_length,
                             long* z_line_defs,
                             long z_lines_length,
                             double* dc,
                             double* dcgrid,
                             double dx,
                             bool is_diffusable,
                             double atolscale,
                             double* ics_alphas) {
    int k;
    _num_nodes = num_nodes;
    diffusable = is_diffusable;
    this->atolscale = atolscale;

    states = static_cast<double*>(my_states->u.px_);
    states_x = (double*) malloc(sizeof(double) * _num_nodes);
    states_y = (double*) malloc(sizeof(double) * _num_nodes);
    states_z = (double*) malloc(sizeof(double) * _num_nodes);
    states_cur = (double*) malloc(sizeof(double) * _num_nodes);
    next = NULL;

    size_x = _num_nodes;
    size_y = 1;
    size_z = 1;


    concentration_list = NULL;
    num_concentrations = 0;
    current_list = NULL;
    num_currents = 0;
    node_flux_count = 0;

    ics_surface_nodes_per_seg = NULL;
    ics_surface_nodes_per_seg_start_indices = NULL;
    ics_scale_factors = NULL;
    ics_current_seg_ptrs = NULL;

#if NRNMPI
    if (nrnmpi_use) {
        proc_offsets = (int*) malloc(nrnmpi_numprocs * sizeof(int));
        proc_num_currents = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_num_fluxes = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_flux_offsets = (int*) malloc(nrnmpi_numprocs * sizeof(int));
    }
#endif
    num_all_currents = 0;
    current_dest = NULL;
    all_currents = NULL;

    _ics_alphas = ics_alphas;
    VARIABLE_ECS_VOLUME = ICS_ALPHA;

    // stores the positive x,y, and z neighbors for each node. [node0_x, node0_y, node0_z, node1_x
    // ...]
    _neighbors = neighbors;

    /*Line definitions from Python. In pattern of [line_start_node, line_length, ...]
      Array is sorted from longest to shortest line */
    _sorted_x_lines = x_line_defs;
    _sorted_y_lines = y_line_defs;
    _sorted_z_lines = z_line_defs;

    // Lengths of _sorted_lines arrays. Used to find thread start and stop indices
    _x_lines_length = x_lines_length;
    _y_lines_length = y_lines_length;
    _z_lines_length = z_lines_length;

    // Find the maximum line length for scratchpad memory allocation
    long x_max = x_line_defs[1];
    long y_max = y_line_defs[1];
    long z_max = z_line_defs[1];
    long xy_max = (x_max > y_max) ? x_max : y_max;
    _line_length_max = (z_max > xy_max) ? z_max : xy_max;

    ics_tasks = (ICSAdiGridData*) malloc(NUM_THREADS * sizeof(ICSAdiGridData));

    for (k = 0; k < NUM_THREADS; k++) {
        ics_tasks[k].RHS = (double*) malloc(sizeof(double) * (_line_length_max));
        ics_tasks[k].scratchpad = (double*) malloc(sizeof(double) * (_line_length_max - 1));
        ics_tasks[k].g = this;
        ics_tasks[k].u_diag = (double*) malloc(sizeof(double) * _line_length_max - 1);
        ics_tasks[k].diag = (double*) malloc(sizeof(double) * _line_length_max);
        ics_tasks[k].l_diag = (double*) malloc(sizeof(double) * _line_length_max - 1);
    }

    hybrid = false;
    hybrid_data = (Hybrid_data*) malloc(sizeof(Hybrid_data));

    ics_adi_dir_x = (ICSAdiDirection*) malloc(sizeof(ICSAdiDirection));
    ics_adi_dir_x->states_in = states_x;
    ics_adi_dir_x->states_out = states;
    ics_adi_dir_x->ordered_start_stop_indices = (long*) malloc(sizeof(long) * NUM_THREADS * 2);
    ics_adi_dir_x->line_start_stop_indices = (long*) malloc(sizeof(long) * NUM_THREADS * 2);
    ics_adi_dir_x->ordered_nodes = (long*) malloc(sizeof(long) * _num_nodes);
    ics_adi_dir_x->ordered_line_defs = (long*) malloc(sizeof(long) * _x_lines_length);
    ics_adi_dir_x->deltas = (double*) malloc(sizeof(double) * _num_nodes);
    ics_adi_dir_x->d = dx;

    ics_adi_dir_y = (ICSAdiDirection*) malloc(sizeof(ICSAdiDirection));
    ics_adi_dir_y->states_in = states_y;
    ics_adi_dir_y->states_out = states;
    ics_adi_dir_y->ordered_start_stop_indices = (long*) malloc(sizeof(long) * NUM_THREADS * 2);
    ics_adi_dir_y->line_start_stop_indices = (long*) malloc(sizeof(long) * NUM_THREADS * 2);
    ics_adi_dir_y->ordered_nodes = (long*) malloc(sizeof(long) * _num_nodes);
    ics_adi_dir_y->ordered_line_defs = (long*) malloc(sizeof(long) * _y_lines_length);
    ics_adi_dir_y->deltas = (double*) malloc(sizeof(double) * _num_nodes);
    ics_adi_dir_y->d = dx;

    ics_adi_dir_z = (ICSAdiDirection*) malloc(sizeof(ICSAdiDirection));
    ics_adi_dir_z->states_in = states_z;
    ics_adi_dir_z->states_out = states;
    ics_adi_dir_z->ordered_start_stop_indices = (long*) malloc(sizeof(long) * NUM_THREADS * 2);
    ics_adi_dir_z->line_start_stop_indices = (long*) malloc(sizeof(long) * NUM_THREADS * 2);
    ics_adi_dir_z->ordered_nodes = (long*) malloc(sizeof(long) * _num_nodes);
    ics_adi_dir_z->ordered_line_defs = (long*) malloc(sizeof(long) * _z_lines_length);
    ics_adi_dir_z->deltas = (double*) malloc(sizeof(double) * _num_nodes);
    ics_adi_dir_z->d = dx;

    if (dcgrid == NULL) {
        ics_adi_dir_x->dc = dc[0];
        ics_adi_dir_y->dc = dc[1];
        ics_adi_dir_z->dc = dc[2];
        ics_adi_dir_x->dcgrid = NULL;
        ics_adi_dir_y->dcgrid = NULL;
        ics_adi_dir_z->dcgrid = NULL;
    } else {
        ics_adi_dir_x->dcgrid = dcgrid;
        ics_adi_dir_y->dcgrid = &dcgrid[_num_nodes];
        ics_adi_dir_z->dcgrid = &dcgrid[2 * _num_nodes];
    }
    volume_setup();
    divide_x_work(NUM_THREADS);
    divide_y_work(NUM_THREADS);
    divide_z_work(NUM_THREADS);

    node_flux_count = 0;
    node_flux_idx = NULL;
    node_flux_scale = NULL;
    node_flux_src = NULL;
}


// Insert a Grid_node "new_Grid" into the list located at grid_list_index in Parallel_grids
/* returns the grid number
   TODO: change this to returning the pointer */
extern "C" int ICS_insert(int grid_list_index,
                          PyHocObject* my_states,
                          long num_nodes,
                          long* neighbors,
                          long* x_line_defs,
                          long x_lines_length,
                          long* y_line_defs,
                          long y_lines_length,
                          long* z_line_defs,
                          long z_lines_length,
                          double* dcs,
                          double dx,
                          bool is_diffusable,
                          double atolscale,
                          double* ics_alphas) {
    ICS_Grid_node* new_Grid = new ICS_Grid_node(my_states,
                                                num_nodes,
                                                neighbors,
                                                x_line_defs,
                                                x_lines_length,
                                                y_line_defs,
                                                y_lines_length,
                                                z_line_defs,
                                                z_lines_length,
                                                dcs,
                                                NULL,
                                                dx,
                                                is_diffusable,
                                                atolscale,
                                                ics_alphas);

    return new_Grid->insert(grid_list_index);
}

int ICS_insert_inhom(int grid_list_index,
                     PyHocObject* my_states,
                     long num_nodes,
                     long* neighbors,
                     long* x_line_defs,
                     long x_lines_length,
                     long* y_line_defs,
                     long y_lines_length,
                     long* z_line_defs,
                     long z_lines_length,
                     double* dcs,
                     double dx,
                     bool is_diffusable,
                     double atolscale,
                     double* ics_alphas) {
    ICS_Grid_node* new_Grid = new ICS_Grid_node(my_states,
                                                num_nodes,
                                                neighbors,
                                                x_line_defs,
                                                x_lines_length,
                                                y_line_defs,
                                                y_lines_length,
                                                z_line_defs,
                                                z_lines_length,
                                                NULL,
                                                dcs,
                                                dx,
                                                is_diffusable,
                                                atolscale,
                                                ics_alphas);
    return new_Grid->insert(grid_list_index);
}


extern "C" int set_diffusion(int grid_list_index, int grid_id, double* dc, int length) {
    int id = 0;
    Grid_node* node = Parallel_grids[grid_list_index];
    while (id < grid_id) {
        node = node->next;
        id++;
        if (node == NULL)
            return -1;
    }
    node->set_diffusion(dc, length);
    return 0;
}

extern "C" int set_tortuosity(int grid_list_index, int grid_id, PyHocObject* my_permeability) {
    int id = 0;
    Grid_node* node = Parallel_grids[grid_list_index];
    while (id < grid_id) {
        node = node->next;
        id++;
        if (node == NULL)
            return -1;
    }
    static_cast<ECS_Grid_node*>(node)->set_tortuosity(my_permeability);
    return 0;
}


void ECS_Grid_node::set_tortuosity(PyHocObject* my_permeability) {
    /*Check to see if variable tortuosity/volume fraction is used*/
    /*note permeability is the 1/tortuosity^2*/
    if (PyFloat_Check(my_permeability)) {
        if (get_permeability == &get_permeability_scalar) {
            double new_permeability = PyFloat_AsDouble((PyObject*) my_permeability);
            get_permeability = &get_permeability_scalar;
            dc_x *= new_permeability / permeability[0];
            dc_y *= new_permeability / permeability[0];
            dc_z *= new_permeability / permeability[0];
            permeability[0] = new_permeability;
        } else {
            permeability = (double*) malloc(sizeof(double));
            permeability[0] = PyFloat_AsDouble((PyObject*) my_permeability);
            /* don't free the old permeability -- let python do it */
            dc_x *= permeability[0];
            dc_y *= permeability[0];
            dc_z *= permeability[0];
            get_permeability = &get_permeability_scalar;
            VARIABLE_ECS_VOLUME = (VARIABLE_ECS_VOLUME == TORTUOSITY) ? FALSE : VARIABLE_ECS_VOLUME;
        }
    } else {
        if (get_permeability == &get_permeability_scalar) {
            /* rescale to remove old permeability*/
            dc_x /= permeability[0];
            dc_y /= permeability[0];
            dc_z /= permeability[0];
            free(permeability);
            permeability = static_cast<double*>(my_permeability->u.px_);
            VARIABLE_ECS_VOLUME = (VARIABLE_ECS_VOLUME == FALSE) ? TORTUOSITY : VARIABLE_ECS_VOLUME;
            get_permeability = &get_permeability_array;
        } else {
            permeability = static_cast<double*>(my_permeability->u.px_);
        }
    }
}

extern "C" int set_volume_fraction(int grid_list_index, int grid_id, PyHocObject* my_alpha) {
    int id = 0;
    Grid_node* node = Parallel_grids[grid_list_index];
    while (id < grid_id) {
        node = node->next;
        id++;
        if (node == NULL)
            return -1;
    }
    static_cast<ECS_Grid_node*>(node)->set_volume_fraction(my_alpha);
    return 0;
}

void ECS_Grid_node::set_volume_fraction(PyHocObject* my_alpha) {
    if (PyFloat_Check(my_alpha)) {
        if (get_alpha == &get_alpha_scalar) {
            alpha[0] = PyFloat_AsDouble((PyObject*) my_alpha);
        } else {
            alpha = (double*) malloc(sizeof(double));
            alpha[0] = PyFloat_AsDouble((PyObject*) my_alpha);
            get_alpha = &get_alpha_scalar;
            VARIABLE_ECS_VOLUME = (get_permeability == &get_permeability_scalar) ? TORTUOSITY
                                                                                 : FALSE;
        }
    } else {
        if (get_alpha == &get_alpha_scalar)
            free(alpha);
        alpha = static_cast<double*>(my_alpha->u.px_);
        VARIABLE_ECS_VOLUME = VOLUME_FRACTION;
        get_alpha = &get_alpha_array;
    }
}


/*Set the diffusion coefficients*/
void ICS_Grid_node::set_diffusion(double* dc, int length) {
    if (length == 1) {
        ics_adi_dir_x->dc = dc[0];
        ics_adi_dir_y->dc = dc[1];
        ics_adi_dir_z->dc = dc[2];
        // NOTE: dcgrid is owned by _IntracellularSpecies in python
        if (ics_adi_dir_x->dcgrid != NULL) {
            ics_adi_dir_x->dcgrid = NULL;
            ics_adi_dir_y->dcgrid = NULL;
            ics_adi_dir_z->dcgrid = NULL;
        }
    } else {
        assert(length == _num_nodes);
        ics_adi_dir_x->dcgrid = dc;
        ics_adi_dir_y->dcgrid = &dc[_num_nodes];
        ics_adi_dir_z->dcgrid = &dc[2 * _num_nodes];
    }
    volume_setup();
}


/*Set the diffusion coefficients*/
void ECS_Grid_node::set_diffusion(double* dc, int) {
    if (get_permeability == &get_permeability_scalar) {
        /* note permeability[0] is the tortuosity squared*/
        dc_x = dc[0] * permeability[0];
        dc_y = dc[1] * permeability[0];
        dc_z = dc[2] * permeability[0];
    } else {
        dc_x = dc[0];
        dc_y = dc[1];
        dc_z = dc[2];
    }
    diffusable = (dc_x > 0) || (dc_y > 0) || (dc_z > 0);
}


extern "C" void ics_set_grid_concentrations(int grid_list_index,
                                            int index_in_list,
                                            int64_t* nodes_per_seg,
                                            int64_t* nodes_per_seg_start_indices,
                                            PyObject* neuron_pointers) {
    Grid_node* g;
    ssize_t i;
    ssize_t n = (ssize_t) PyList_Size(neuron_pointers);  // number of segments.

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    g->ics_surface_nodes_per_seg = nodes_per_seg;

    g->ics_surface_nodes_per_seg_start_indices = nodes_per_seg_start_indices;
    g->ics_concentration_seg_handles.reserve(n);
    for (i = 0; i < n; i++) {
        g->ics_concentration_seg_handles.push_back(
            reinterpret_cast<PyHocObject*>(PyList_GET_ITEM(neuron_pointers, i))->u.px_);
    }
}

extern "C" void ics_set_grid_currents(int grid_list_index,
                                      int index_in_list,
                                      PyObject* neuron_pointers,
                                      double* scale_factors) {
    Grid_node* g;
    ssize_t i;
    ssize_t n = (ssize_t) PyList_Size(neuron_pointers);
    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }
    g->ics_scale_factors = scale_factors;

    g->ics_current_seg_ptrs = (double**) malloc(n * sizeof(double*));

    for (i = 0; i < n; i++) {
        g->ics_current_seg_ptrs[i] = static_cast<double*>(
            ((PyHocObject*) PyList_GET_ITEM(neuron_pointers, i))->u.px_);
    }
}


/* TODO: make this work with Grid_node ptrs instead of pairs of list indices */
extern "C" void set_grid_concentrations(int grid_list_index,
                                        int index_in_list,
                                        PyObject* grid_indices,
                                        PyObject* neuron_pointers) {
    /*
    Preconditions:

    Assumes the specified grid has been created.
    Assumes len(grid_indices) = len(neuron_pointers) and that both are proper python lists
    */
    /* TODO: note that these will need updating anytime the structure of the model changes... look
     * at the structure change count at each advance and trigger a callback to regenerate if
     * necessary */
    Grid_node* g;
    ssize_t i;
    ssize_t n = (ssize_t) PyList_Size(grid_indices);

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    /* free the old concentration list */
    delete[] g->concentration_list;

    /* allocate space for the new list */
    g->concentration_list = new Concentration_Pair[n];
    g->num_concentrations = n;

    /* populate the list */
    /* TODO: it would be more general to iterate instead of assuming lists and using list lookups */
    for (i = 0; i < n; i++) {
        /* printf("set_grid_concentrations %ld\n", i); */
        g->concentration_list[i].source = PyInt_AS_LONG(PyList_GET_ITEM(grid_indices, i));
        g->concentration_list[i].destination =
            reinterpret_cast<PyHocObject*>(PyList_GET_ITEM(neuron_pointers, i))->u.px_;
    }
}

/* TODO: make this work with Grid_node ptrs instead of pairs of list indices */
extern "C" void set_grid_currents(int grid_list_index,
                                  int index_in_list,
                                  PyObject* grid_indices,
                                  PyObject* neuron_pointers,
                                  PyObject* scale_factors) {
    /*
    Preconditions:

    Assumes the specified grid has been created.
    Assumes len(grid_indices) = len(neuron_pointers) = len(scale_factors) and that all are proper
    python lists
    */
    /* TODO: note that these will need updating anytime the structure of the model changes... look
     * at the structure change count at each advance and trigger a callback to regenerate if
     * necessary */
    Grid_node* g;
    ssize_t i;
    ssize_t n = (ssize_t) PyList_Size(grid_indices);
    long* dests;

    /* Find the Grid Object */
    g = Parallel_grids[grid_list_index];
    for (i = 0; i < index_in_list; i++) {
        g = g->next;
    }

    /* free the old current list */
    delete[] g->current_list;

    /* allocate space for the new list */
    g->current_list = new Current_Triple[n];
    g->num_currents = n;

    /* populate the list */
    /* TODO: it would be more general to iterate instead of assuming lists and using list lookups */
    for (i = 0; i < n; i++) {
        g->current_list[i].destination = PyInt_AS_LONG(PyList_GET_ITEM(grid_indices, i));
        g->current_list[i].scale_factor = PyFloat_AS_DOUBLE(PyList_GET_ITEM(scale_factors, i));
        g->current_list[i].source =
            reinterpret_cast<PyHocObject*>(PyList_GET_ITEM(neuron_pointers, i))->u.px_;
        /* printf("set_grid_currents %ld out of %ld, %ld, %ld\n", i, n,
         * PyList_Size(neuron_pointers), PyList_Size(scale_factors)); */
    } /*
     if (PyErr_Occurred()) {
         printf("uh oh\n");
         PyErr_PrintEx(0);
     } else {
         printf("no problems\n");
     }
     PyErr_Clear(); */

#if NRNMPI
    if (nrnmpi_use) {
        /*Gather an array of the number of currents for each process*/
        g->proc_num_currents[nrnmpi_myid] = n;
        nrnmpi_int_allgather_inplace(g->proc_num_currents, 1);

        g->proc_offsets[0] = 0;
        for (i = 1; i < nrnmpi_numprocs; i++)
            g->proc_offsets[i] = g->proc_offsets[i - 1] + g->proc_num_currents[i - 1];
        g->num_all_currents = g->proc_offsets[i - 1] + g->proc_num_currents[i - 1];

        /*Copy array of current destinations (index to states) across all processes*/
        free(g->current_dest);
        free(g->all_currents);
        g->current_dest = (long*) malloc(g->num_all_currents * sizeof(long));
        g->all_currents = (double*) malloc(g->num_all_currents * sizeof(double));
        dests = g->current_dest + g->proc_offsets[nrnmpi_myid];
        /*TODO: Get rid of duplication with current_list*/
        for (i = 0; i < n; i++) {
            dests[i] = g->current_list[i].destination;
        }
        nrnmpi_long_allgatherv_inplace(g->current_dest, g->proc_num_currents, g->proc_offsets);
    } else {
        free(g->all_currents);
        g->all_currents = (double*) malloc(sizeof(double) * g->num_currents);
        g->num_all_currents = g->num_currents;
    }
#else
    free(g->all_currents);
    g->all_currents = (double*) malloc(sizeof(double) * g->num_currents);
    g->num_all_currents = g->num_currents;
#endif
}

// Delete a specific Grid_node from the list
int remove(Grid_node** head, Grid_node* find) {
    if (*head == find) {
        Grid_node* temp = *head;
        *head = (*head)->next;
        delete temp;
        return 1;
    }
    Grid_node* temp = *head;
    while (temp->next != find) {
        temp = temp->next;
    }
    if (!temp)
        return 0;
    Grid_node* delete_me = temp->next;
    temp->next = delete_me->next;
    delete delete_me;
    return 1;
}

extern "C" void delete_by_id(int id) {
    Grid_node* grid;
    int k;
    for (k = 0, grid = Parallel_grids[0]; grid != NULL; grid = grid->next, k++) {
        if (k == id) {
            remove(Parallel_grids, grid);
            break;
        }
    }
}


// Destroy the list located at list_index and free all memory
void empty_list(int list_index) {
    Grid_node** head = &(Parallel_grids[list_index]);
    while (*head != NULL) {
        Grid_node* old_head = *head;
        *head = (*head)->next;
        delete old_head;
    }
}

int Grid_node::insert(int grid_list_index) {
    int i = 0;
    Grid_node** head = &(Parallel_grids[grid_list_index]);

    if (!(*head)) {
        *head = this;
    } else {
        i++; /*count the head as the first grid*/
        Grid_node* end = *head;
        while (end->next != NULL) {
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

void ECS_Grid_node::set_num_threads(const int n) {
    int i;
    if (ecs_tasks != NULL) {
        for (i = 0; i < NUM_THREADS; i++) {
            free(ecs_tasks[i].scratchpad);
        }
    }
    free(ecs_tasks);
    ecs_tasks = (ECSAdiGridData*) malloc(n * sizeof(ECSAdiGridData));
    for (i = 0; i < n; i++) {
        ecs_tasks[i].scratchpad = (double*) malloc(sizeof(double) *
                                                   MAX(size_x, MAX(size_y, size_z)));
        ecs_tasks[i].g = this;
    }
}

static void* gather_currents(void* dataptr) {
    CurrentData* d = (CurrentData*) dataptr;
    Grid_node* grid = d->g;
    double* val = d->val;
    int i, start = d->onset, stop = d->offset;
    Current_Triple* c = grid->current_list;
    if (grid->VARIABLE_ECS_VOLUME == VOLUME_FRACTION) {
        for (i = start; i < stop; i++)
            val[i] = c[i].scale_factor * (*c[i].source) / grid->alpha[c[i].destination];
    } else if (grid->VARIABLE_ECS_VOLUME == ICS_ALPHA) {
        // TODO: Check if this is used.
        for (i = start; i < stop; i++)
            val[i] = c[i].scale_factor * (*c[i].source) /
                     ((ICS_Grid_node*) grid)->_ics_alphas[c[i].destination];
    } else {
        for (i = start; i < stop; i++)
            val[i] = c[i].scale_factor * (*c[i].source) / grid->alpha[0];
    }
    return NULL;
}

/*
 * do_current - process the current for a given grid
 * Grid_node* grid - the grid used
 * double* output - for fixed step this is the states for variable ydot
 * double dt - for fixed step the step size, for variable step 1
 */
void ECS_Grid_node::do_grid_currents(double* output, double dt, int grid_id) {
    ssize_t m, n, i;
    /*Currents to broadcast via MPI*/
    /*TODO: Handle multiple grids with one pass*/
    /*Maybe TODO: Should check #currents << #voxels and not the other way round*/
    double* val;
    // memset(output, 0, sizeof(double)*grid->size_x*grid->size_y*grid->size_z);
    /* currents, via explicit Euler */
    n = num_all_currents;
    m = num_currents;
    CurrentData* tasks = (CurrentData*) malloc(NUM_THREADS * sizeof(CurrentData));
#if NRNMPI
    val = all_currents + (nrnmpi_use ? proc_offsets[nrnmpi_myid] : 0);
#else
    val = all_currents;
#endif
    int tasks_per_thread = (m + NUM_THREADS - 1) / NUM_THREADS;

    for (i = 0; i < NUM_THREADS; i++) {
        tasks[i].g = this;
        tasks[i].onset = i * tasks_per_thread;
        tasks[i].offset = MIN((i + 1) * tasks_per_thread, m);
        tasks[i].val = val;
    }
    for (i = 0; i < NUM_THREADS - 1; i++) {
        TaskQueue_add_task(AllTasks, &gather_currents, &tasks[i], NULL);
    }
    /* run one task in the main thread */
    gather_currents(&tasks[NUM_THREADS - 1]);

    /* wait for them to finish */
    TaskQueue_sync(AllTasks);
    free(tasks);
#if NRNMPI
    if (nrnmpi_use) {
        nrnmpi_dbl_allgatherv_inplace(all_currents, proc_num_currents, proc_offsets);
        nrnmpi_dbl_allgatherv_inplace(induced_currents,
                                      proc_induced_current_count,
                                      proc_induced_current_offset);
        for (i = 0; i < n; i++)
            output[current_dest[i]] += dt * all_currents[i];
    } else {
        for (i = 0; i < n; i++) {
            output[current_list[i].destination] += dt * all_currents[i];
        }
    }

#else
    for (i = 0; i < n; i++)
        output[current_list[i].destination] += dt * all_currents[i];
#endif

    /*Remove the contribution from membrane currents*/
    for (i = 0; i < induced_current_count; i++)
        output[induced_currents_index[i]] -= dt * (induced_currents[i] * induced_currents_scale[i]);
    memset(induced_currents, 0, induced_current_count * sizeof(double));
}

double* ECS_Grid_node::set_rxd_currents(int current_count,
                                        int* current_indices,
                                        PyHocObject** ptrs) {
    int i, j;
    double volume_fraction;

    free(induced_currents_scale);
    free(induced_currents_index);
    induced_currents_scale = (double*) calloc(current_count, sizeof(double));
    multicompartment_inititalized = FALSE;
    induced_current_count = current_count;
    induced_currents_index = current_indices;
    for (i = 0; i < current_count; i++) {
        for (j = 0; j < num_all_currents; j++) {
            if (ptrs[i]->u.px_ == current_list[j].source) {
                volume_fraction = (VARIABLE_ECS_VOLUME == VOLUME_FRACTION
                                       ? alpha[current_list[j].destination]
                                       : alpha[0]);
                induced_currents_scale[i] = current_list[j].scale_factor / volume_fraction;
                assert(current_list[j].destination == current_indices[i]);
                break;
            }
        }
    }
    return induced_currents_scale;
}

void ECS_Grid_node::apply_node_flux3D(double dt, double* ydot) {
    double* dest;
    if (ydot == NULL)
        dest = states_cur;
    else
        dest = ydot;
#if NRNMPI
    double* sources;
    int i;
    int offset;

    if (nrnmpi_use) {
        sources = (double*) calloc(node_flux_count, sizeof(double));
        offset = proc_flux_offsets[nrnmpi_myid];
        apply_node_flux(proc_num_fluxes[nrnmpi_myid],
                        NULL,
                        &node_flux_scale[offset],
                        node_flux_src,
                        dt,
                        &sources[offset]);

        nrnmpi_dbl_allgatherv_inplace(sources, proc_num_fluxes, proc_flux_offsets);

        for (i = 0; i < node_flux_count; i++)
            dest[node_flux_idx[i]] += sources[i];
        free(sources);
    } else {
        apply_node_flux(node_flux_count, node_flux_idx, node_flux_scale, node_flux_src, dt, dest);
    }
#else
    apply_node_flux(node_flux_count, node_flux_idx, node_flux_scale, node_flux_src, dt, dest);
#endif
}


void ECS_Grid_node::volume_setup() {
    switch (VARIABLE_ECS_VOLUME) {
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

int ECS_Grid_node::dg_adi() {
    unsigned long i;
    if (diffusable) {
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
        memcpy(states, ecs_adi_dir_z->states_out, sizeof(double) * size_x * size_y * size_z);
    } else {
        for (i = 0; i < size_x * size_y * size_z; i++)
            states[i] += states_cur[i];
    }
    // TODO: Should this return 0?
    return 0;
}

void ECS_Grid_node::variable_step_diffusion(const double* states, double* ydot) {
    switch (VARIABLE_ECS_VOLUME) {
    case VOLUME_FRACTION:
        _rhs_variable_step_helper_vol(this, states, ydot);
        break;
    case TORTUOSITY:
        _rhs_variable_step_helper_tort(this, states, ydot);
        break;
    default:
        _rhs_variable_step_helper(this, states, ydot);
    }
}

void ECS_Grid_node::scatter_grid_concentrations() {
    ssize_t i, n;
    Concentration_Pair* cp;

    n = num_concentrations;
    cp = concentration_list;
    for (i = 0; i < n; i++) {
        (*cp[i].destination) = states[cp[i].source];
    }
}

void ECS_Grid_node::hybrid_connections() {}

void ECS_Grid_node::variable_step_hybrid_connections(const double*,
                                                     double* const,
                                                     const double*,
                                                     double* const) {}

int ECS_Grid_node::add_multicompartment_reaction(int nstates, int* indices, int step) {
    // Set the current reaction offsets and increment the local offset count
    // This is stored as an array/pointer so it can be updated if the grid is
    // on multiple MPI nodes.
    int i = 0, j = 0;
    int offset = react_offsets[react_offset_count - 1];
    // Increase the size of the reaction indices to accomidate the maximum
    // number of potential new indicies
    reaction_indices = (int*) realloc(reaction_indices, (offset + nstates) * sizeof(int));
    // TODO: Aggregate the common indicies on the local node, so the state
    // changes can be added together before they are broadcast.
    for (i = 0, j = 0; i < nstates; i++, j += step) {
        if (indices[j] != SPECIES_ABSENT)
            reaction_indices[offset++] = indices[j];
    }
    // Resize to remove ununused indices
    if (offset < react_offsets[react_offset_count - 1] + nstates)
        reaction_indices = (int*) realloc((void*) reaction_indices, offset * sizeof(int));

    // Store the new offset and return the start offset to the Reaction
    react_offsets = (int*) realloc((void*) react_offsets, (++react_offset_count) * sizeof(int));
    react_offsets[react_offset_count - 1] = offset;
    multicompartment_inititalized = FALSE;
    return react_offset_count - 2;
}

void ECS_Grid_node::clear_multicompartment_reaction() {
    free(all_reaction_states);
    free(react_offsets);
    if (multicompartment_inititalized)
        free(all_reaction_indices);
    else
        free(reaction_indices);
    all_reaction_indices = NULL;
    all_reaction_states = NULL;
    reaction_indices = NULL;
    react_offsets = (int*) calloc(1, sizeof(int));
    react_offset_count = 1;
    multicompartment_inititalized = induced_current_count == 0;
    total_reaction_states = 0;
}


void ECS_Grid_node::initialize_multicompartment_reaction() {
#if NRNMPI
    int i, j;
    int total_react = 0;
    int start_state;
    int* proc_num_init;
    int* all_indices;
    double* all_scales;
    // Copy the reaction indices across all nodes and update the local
    // react_offsets used by Reaction to access the indicies.
    if (nrnmpi_use) {
        proc_num_init = (int*) calloc(nrnmpi_numprocs, sizeof(int));
        proc_num_init[nrnmpi_myid] = (int) multicompartment_inititalized;
        nrnmpi_int_allgather_inplace(proc_num_init, 1);
        for (i = 0; i < nrnmpi_numprocs; i++)
            if (!proc_num_init[i])
                break;

        if (i != nrnmpi_numprocs) {
            // number of offsets (Reaction) stored in each process
            proc_num_reactions = (int*) calloc(nrnmpi_numprocs, sizeof(int));
            proc_num_reactions[nrnmpi_myid] = react_offset_count;

            // number of states/indices stored in each process
            proc_num_reaction_states = (int*) calloc(nrnmpi_numprocs, sizeof(int));
            proc_num_reaction_states[nrnmpi_myid] = react_offsets[react_offset_count - 1];
            nrnmpi_int_allgather_inplace(proc_num_reactions, 1);
            nrnmpi_int_allgather_inplace(proc_num_reaction_states, 1);

            for (i = 0; i < nrnmpi_numprocs; i++) {
                if (i == nrnmpi_myid)
                    start_state = total_reaction_states;
                proc_num_reactions[i] = total_reaction_states;
                total_react += proc_num_reactions[i];
                total_reaction_states += proc_num_reaction_states[i];
            }

            // Move the offsets for each reaction so they reference the
            // corresponding indices in the all_reaction_indices array
            for (j = 0; j < react_offset_count; j++)
                react_offsets[j] += start_state;

            all_reaction_indices = (int*) malloc(total_reaction_states * sizeof(int));
            all_reaction_states = (double*) calloc(total_reaction_states, sizeof(double));

            memcpy(&all_reaction_indices[start_state],
                   reaction_indices,
                   proc_num_reaction_states[nrnmpi_myid] * sizeof(int));
            nrnmpi_int_allgatherv_inplace(all_reaction_indices,
                                          proc_num_reaction_states,
                                          proc_num_reactions);
            free(reaction_indices);
            reaction_indices = NULL;
            multicompartment_inititalized = TRUE;

            // Handle currents induced by multicompartment reactions.
            proc_induced_current_count[nrnmpi_myid] = induced_current_count;
            nrnmpi_int_allgather_inplace(proc_induced_current_count, 1);
            proc_induced_current_offset[0] = 0;
            for (i = 1; i < nrnmpi_numprocs; i++)
                proc_induced_current_offset[i] = proc_induced_current_offset[i - 1] +
                                                 proc_induced_current_count[i - 1];
            induced_current_count = proc_induced_current_offset[nrnmpi_numprocs - 1] +
                                    proc_induced_current_count[nrnmpi_numprocs - 1];

            all_scales = (double*) malloc(induced_current_count * sizeof(double));
            all_indices = (int*) malloc(induced_current_count * sizeof(int));
            memcpy(&all_scales[proc_induced_current_offset[nrnmpi_myid]],
                   induced_currents_scale,
                   sizeof(double) * proc_induced_current_count[nrnmpi_myid]);

            memcpy(&all_indices[proc_induced_current_offset[nrnmpi_myid]],
                   induced_currents_index,
                   sizeof(int) * proc_induced_current_count[nrnmpi_myid]);

            nrnmpi_dbl_allgatherv_inplace(all_scales,
                                          proc_induced_current_count,
                                          proc_induced_current_offset);

            nrnmpi_int_allgatherv_inplace(all_indices,
                                          proc_induced_current_count,
                                          proc_induced_current_offset);
            free(induced_currents_scale);
            free(induced_currents_index);
            free(induced_currents);
            induced_currents_scale = all_scales;
            induced_currents_index = all_indices;
            induced_currents = (double*) malloc(induced_current_count * sizeof(double));
            local_induced_currents = &induced_currents[proc_induced_current_offset[nrnmpi_myid]];
        }
    } else {
        if (!multicompartment_inititalized) {
            total_reaction_states = react_offsets[react_offset_count - 1];
            all_reaction_indices = reaction_indices;
            all_reaction_states = (double*) calloc(total_reaction_states, sizeof(double));
            multicompartment_inititalized = TRUE;
            induced_currents = (double*) malloc(induced_current_count * sizeof(double));
            local_induced_currents = induced_currents;
        }
    }
#else
    if (!multicompartment_inititalized) {
        total_reaction_states = react_offsets[react_offset_count - 1];
        all_reaction_indices = reaction_indices;
        all_reaction_states = (double*) calloc(total_reaction_states, sizeof(double));
        multicompartment_inititalized = TRUE;
        induced_currents = (double*) malloc(induced_current_count * sizeof(double));
        local_induced_currents = induced_currents;
    }
#endif
}

void ECS_Grid_node::do_multicompartment_reactions(double* result) {
    int i;
#if NRNMPI
    if (nrnmpi_use) {
        // Copy the states between all of the nodes
        nrnmpi_dbl_allgatherv_inplace(all_reaction_states,
                                      proc_num_reaction_states,
                                      proc_num_reactions);
    }
#endif
    if (result == NULL)  // fixed step
    {
        for (i = 0; i < total_reaction_states; i++)
            states[all_reaction_indices[i]] += all_reaction_states[i];
    } else  // variable step
    {
        for (i = 0; i < total_reaction_states; i++)
            result[all_reaction_indices[i]] += all_reaction_states[i];
    }
    memset(all_reaction_states, 0, total_reaction_states * sizeof(int));
}

// TODO: Implement this
void ECS_Grid_node::variable_step_ode_solve(double*, double) {}

// Free a single Grid_node
ECS_Grid_node::~ECS_Grid_node() {
    int i;
    free(states_x);
    free(states_y);
    free(states_cur);
    delete[] concentration_list;
    delete[] current_list;
    free(bc);
    free(current_dest);
#if NRNMPI
    if (nrnmpi_use) {
        free(proc_offsets);
        free(proc_num_currents);
        free(proc_flux_offsets);
        free(proc_num_fluxes);
        free(proc_num_reaction_states);
        free(proc_num_reactions);
    }
#endif
    free(all_currents);
    free(ecs_adi_dir_x);
    free(ecs_adi_dir_y);
    free(ecs_adi_dir_z);
    if (node_flux_count > 0) {
        free(node_flux_idx);
        free(node_flux_scale);
        free(node_flux_src);
    }


    if (ecs_tasks != NULL) {
        for (i = 0; i < NUM_THREADS; i++) {
            free(ecs_tasks[i].scratchpad);
        }
    }
    free(ecs_tasks);
}

/*****************************************************************************
 *
 * Begin ICS_Grid_node Functions
 *
 *****************************************************************************/
static int find_min_element_index(const int thread_sizes[], const int nthreads) {
    int new_min_index = 0;
    int min_element = thread_sizes[0];
    for (int i = 0; i < nthreads; i++) {
        if (min_element > thread_sizes[i]) {
            min_element = thread_sizes[i];
            new_min_index = i;
        }
    }
    return new_min_index;
}

void ICS_Grid_node::divide_x_work(const int nthreads) {
    int i, j, k;
    // nodes in each thread. (work each thread has to do)
    int* nodes_per_thread = (int*) calloc(nthreads, sizeof(int));
    // number of lines in each thread
    int* lines_per_thread = (int*) calloc(nthreads, sizeof(int));
    // To determine which index to put the start node and line length in thread_line_defs
    int* thread_idx_counter = (int*) calloc(nthreads, sizeof(int));
    // To determine which thread array to put the start node and line length in thread_line_defs
    int line_thread_id[_x_lines_length / 2];
    // Array of nthreads arrays that hold the line defs for each thread
    int** thread_line_defs = (int**) malloc(nthreads * sizeof(int*));

    int min_index = 0;

    // Find the total line length for each thread
    for (i = 0; i < _x_lines_length; i += 2) {
        min_index = find_min_element_index(nodes_per_thread, nthreads);
        nodes_per_thread[min_index] += _sorted_x_lines[i + 1];
        line_thread_id[i / 2] = min_index;
        lines_per_thread[min_index] += 1;
    }

    // Allocate memory for each array in thread_line_defs
    for (i = 0; i < nthreads; i++) {
        thread_line_defs[i] = (int*) malloc(lines_per_thread[i] * 2 * sizeof(int));
    }

    int thread_idx;
    int line_idx;

    // Populate each array of thread_line_defs
    for (i = 0; i < _x_lines_length; i += 2) {
        thread_idx = line_thread_id[i / 2];
        line_idx = thread_idx_counter[thread_idx];
        thread_line_defs[thread_idx][line_idx] = _sorted_x_lines[i];
        thread_line_defs[thread_idx][line_idx + 1] = _sorted_x_lines[i + 1];
        thread_idx_counter[thread_idx] += 2;
    }

    // Populate ordered_line_def
    int ordered_line_def_counter = 0;
    for (i = 0; i < nthreads; i++) {
        for (j = 0; j < lines_per_thread[i] * 2; j++) {
            ics_adi_dir_x->ordered_line_defs[ordered_line_def_counter] = thread_line_defs[i][j];
            ordered_line_def_counter++;
        }
    }
    // Set direction node and line start/stop indices
    // thread 0 start and stop
    ics_adi_dir_x->ordered_start_stop_indices[0] = 0;
    ics_adi_dir_x->ordered_start_stop_indices[1] = nodes_per_thread[0];
    ics_adi_dir_x->line_start_stop_indices[0] = 0;
    ics_adi_dir_x->line_start_stop_indices[1] = lines_per_thread[0] * 2;
    long start_node;
    long line_start_node;
    for (i = 2; i < nthreads * 2; i += 2) {
        start_node = ics_adi_dir_x->ordered_start_stop_indices[i - 1];
        ics_adi_dir_x->ordered_start_stop_indices[i] = start_node;
        ics_adi_dir_x->ordered_start_stop_indices[i + 1] = start_node + nodes_per_thread[i / 2];

        line_start_node = ics_adi_dir_x->line_start_stop_indices[i - 1];
        ics_adi_dir_x->line_start_stop_indices[i] = line_start_node;
        ics_adi_dir_x->line_start_stop_indices[i + 1] = line_start_node +
                                                        lines_per_thread[i / 2] * 2;
    }

    // Put the Nodes in order in the ordered_nodes array
    int ordered_node_idx_counter = 0;
    int current_node;
    for (i = 0; i < nthreads; i++) {
        for (j = 0; j < lines_per_thread[i] * 2; j += 2) {
            current_node = thread_line_defs[i][j];
            ics_adi_dir_x->ordered_nodes[ordered_node_idx_counter] = current_node;
            ics_adi_dir_x->states_in[ordered_node_idx_counter] = states[current_node];
            ordered_node_idx_counter++;
            for (k = 1; k < thread_line_defs[i][j + 1]; k++) {
                current_node = _neighbors[current_node * 3];
                ics_adi_dir_x->ordered_nodes[ordered_node_idx_counter] = current_node;
                ics_adi_dir_x->states_in[ordered_node_idx_counter] = states[current_node];
                ordered_node_idx_counter++;
            }
        }
    }

    // Delete thread_line_defs array
    for (i = 0; i < nthreads; i++) {
        free(thread_line_defs[i]);
    }
    free(thread_line_defs);
    free(nodes_per_thread);
    free(lines_per_thread);
    free(thread_idx_counter);
}

void ICS_Grid_node::divide_y_work(const int nthreads) {
    int i, j, k;
    // nodes in each thread. (work each thread has to do)
    int* nodes_per_thread = (int*) calloc(nthreads, sizeof(int));
    // number of lines in each thread
    int* lines_per_thread = (int*) calloc(nthreads, sizeof(int));
    // To determine which index to put the start node and line length in thread_line_defs
    int* thread_idx_counter = (int*) calloc(nthreads, sizeof(int));
    // To determine which thread array to put the start node and line length in thread_line_defs
    int line_thread_id[_y_lines_length / 2];
    // Array of nthreads arrays that hold the line defs for each thread
    int** thread_line_defs = (int**) malloc(nthreads * sizeof(int*));

    int min_index = 0;

    // Find the total line length for each thread
    for (i = 0; i < _y_lines_length; i += 2) {
        min_index = find_min_element_index(nodes_per_thread, nthreads);
        nodes_per_thread[min_index] += _sorted_y_lines[i + 1];
        line_thread_id[i / 2] = min_index;
        lines_per_thread[min_index] += 1;
    }

    // Allocate memory for each array in thread_line_defs
    for (i = 0; i < nthreads; i++) {
        thread_line_defs[i] = (int*) malloc(lines_per_thread[i] * 2 * sizeof(int));
    }

    int thread_idx;
    int line_idx;

    // Populate each array of thread_line_defs
    for (i = 0; i < _y_lines_length; i += 2) {
        thread_idx = line_thread_id[i / 2];
        line_idx = thread_idx_counter[thread_idx];
        thread_line_defs[thread_idx][line_idx] = _sorted_y_lines[i];
        thread_line_defs[thread_idx][line_idx + 1] = _sorted_y_lines[i + 1];
        thread_idx_counter[thread_idx] += 2;
    }

    // Populate ordered_line_def
    int ordered_line_def_counter = 0;
    for (i = 0; i < nthreads; i++) {
        for (j = 0; j < lines_per_thread[i] * 2; j++) {
            ics_adi_dir_y->ordered_line_defs[ordered_line_def_counter] = thread_line_defs[i][j];
            ordered_line_def_counter++;
        }
    }

    // Set direction start/stop indices
    // thread 0 start and stop
    ics_adi_dir_y->ordered_start_stop_indices[0] = 0;
    ics_adi_dir_y->ordered_start_stop_indices[1] = nodes_per_thread[0];
    ics_adi_dir_y->line_start_stop_indices[0] = 0;
    ics_adi_dir_y->line_start_stop_indices[1] = lines_per_thread[0] * 2;

    long start_node;
    long line_start_node;
    for (i = 2; i < nthreads * 2; i += 2) {
        start_node = ics_adi_dir_y->ordered_start_stop_indices[i - 1];
        ics_adi_dir_y->ordered_start_stop_indices[i] = start_node;
        ics_adi_dir_y->ordered_start_stop_indices[i + 1] = start_node + nodes_per_thread[i / 2];

        line_start_node = ics_adi_dir_y->line_start_stop_indices[i - 1];
        ics_adi_dir_y->line_start_stop_indices[i] = line_start_node;
        ics_adi_dir_y->line_start_stop_indices[i + 1] = line_start_node +
                                                        (lines_per_thread[i / 2] * 2);
    }

    // Put the Nodes in order in the ordered_nodes array
    int ordered_node_idx_counter = 0;
    int current_node;
    for (i = 0; i < nthreads; i++) {
        for (j = 0; j < lines_per_thread[i] * 2; j += 2) {
            current_node = thread_line_defs[i][j];
            ics_adi_dir_y->ordered_nodes[ordered_node_idx_counter] = current_node;
            ics_adi_dir_y->states_in[ordered_node_idx_counter] = states[current_node];
            ordered_node_idx_counter++;
            for (k = 1; k < thread_line_defs[i][j + 1]; k++) {
                current_node = _neighbors[(current_node * 3) + 1];
                ics_adi_dir_y->ordered_nodes[ordered_node_idx_counter] = current_node;
                ics_adi_dir_y->states_in[ordered_node_idx_counter] = states[current_node];
                ordered_node_idx_counter++;
            }
        }
    }

    // Delete thread_line_defs array
    for (i = 0; i < nthreads; i++) {
        free(thread_line_defs[i]);
    }
    free(thread_line_defs);
    free(nodes_per_thread);
    free(lines_per_thread);
    free(thread_idx_counter);
}

void ICS_Grid_node::divide_z_work(const int nthreads) {
    int i, j, k;
    // nodes in each thread. (work each thread has to do)
    int* nodes_per_thread = (int*) calloc(nthreads, sizeof(int));
    // number of lines in each thread
    int* lines_per_thread = (int*) calloc(nthreads, sizeof(int));
    // To determine which index to put the start node and line length in thread_line_defs
    int* thread_idx_counter = (int*) calloc(nthreads, sizeof(int));
    // To determine which thread array to put the start node and line length in thread_line_defs
    int line_thread_id[_z_lines_length / 2];
    // Array of nthreads arrays that hold the line defs for each thread
    int** thread_line_defs = (int**) malloc(nthreads * sizeof(int*));

    int min_index = 0;

    // Find the total line length for each thread
    for (i = 0; i < _z_lines_length; i += 2) {
        min_index = find_min_element_index(nodes_per_thread, nthreads);
        nodes_per_thread[min_index] += _sorted_z_lines[i + 1];
        line_thread_id[i / 2] = min_index;
        lines_per_thread[min_index] += 1;
    }

    // Allocate memory for each array in thread_line_defs
    // Add indices to Grid_data
    for (i = 0; i < nthreads; i++) {
        thread_line_defs[i] = (int*) malloc(lines_per_thread[i] * 2 * sizeof(int));
    }

    int thread_idx;
    int line_idx;

    // Populate each array of thread_line_defs
    for (i = 0; i < _z_lines_length; i += 2) {
        thread_idx = line_thread_id[i / 2];
        line_idx = thread_idx_counter[thread_idx];
        thread_line_defs[thread_idx][line_idx] = _sorted_z_lines[i];
        thread_line_defs[thread_idx][line_idx + 1] = _sorted_z_lines[i + 1];
        thread_idx_counter[thread_idx] += 2;
    }

    // Populate ordered_line_def
    int ordered_line_def_counter = 0;
    for (i = 0; i < nthreads; i++) {
        for (j = 0; j < lines_per_thread[i] * 2; j++) {
            ics_adi_dir_z->ordered_line_defs[ordered_line_def_counter] = thread_line_defs[i][j];
            ordered_line_def_counter++;
        }
    }

    // Set direction start/stop indices
    // thread 0 start and stop
    ics_adi_dir_z->ordered_start_stop_indices[0] = 0;
    ics_adi_dir_z->ordered_start_stop_indices[1] = nodes_per_thread[0];
    ics_adi_dir_z->line_start_stop_indices[0] = 0;
    ics_adi_dir_z->line_start_stop_indices[1] = lines_per_thread[0] * 2;

    long start_node;
    long line_start_node;
    for (i = 2; i < nthreads * 2; i += 2) {
        start_node = ics_adi_dir_z->ordered_start_stop_indices[i - 1];
        ics_adi_dir_z->ordered_start_stop_indices[i] = start_node;
        ics_adi_dir_z->ordered_start_stop_indices[i + 1] = start_node + nodes_per_thread[i / 2];

        line_start_node = ics_adi_dir_z->line_start_stop_indices[i - 1];
        ics_adi_dir_z->line_start_stop_indices[i] = line_start_node;
        ics_adi_dir_z->line_start_stop_indices[i + 1] = line_start_node +
                                                        lines_per_thread[i / 2] * 2;
    }

    // Put the Nodes in order in the ordered_nodes array
    int ordered_node_idx_counter = 0;
    int current_node;
    for (i = 0; i < nthreads; i++) {
        for (j = 0; j < lines_per_thread[i] * 2; j += 2) {
            current_node = thread_line_defs[i][j];
            ics_adi_dir_z->ordered_nodes[ordered_node_idx_counter] = current_node;
            ics_adi_dir_z->states_in[ordered_node_idx_counter] = states[current_node];
            ordered_node_idx_counter++;
            for (k = 1; k < thread_line_defs[i][j + 1]; k++) {
                current_node = _neighbors[(current_node * 3) + 2];
                ics_adi_dir_z->ordered_nodes[ordered_node_idx_counter] = current_node;
                ics_adi_dir_z->states_in[ordered_node_idx_counter] = states[current_node];
                ordered_node_idx_counter++;
            }
        }
    }

    // Delete thread_line_defs array
    for (i = 0; i < nthreads; i++) {
        free(thread_line_defs[i]);
    }
    free(thread_line_defs);
    free(nodes_per_thread);
    free(lines_per_thread);
    free(thread_idx_counter);
}


void ICS_Grid_node::set_num_threads(const int n) {
    int i;
    if (ics_tasks != NULL) {
        for (i = 0; i < NUM_THREADS; i++) {
            free(ics_tasks[i].scratchpad);
            free(ics_tasks[i].RHS);
        }
    }
    free(ics_tasks);
    ics_tasks = (ICSAdiGridData*) malloc(n * sizeof(ICSAdiGridData));
    for (i = 0; i < n; i++) {
        ics_tasks[i].RHS = (double*) malloc(sizeof(double) * _line_length_max);
        ics_tasks[i].scratchpad = (double*) malloc(sizeof(double) * _line_length_max - 1);
        ics_tasks[i].g = this;
        ics_tasks[i].u_diag = (double*) malloc(sizeof(double) * _line_length_max - 1);
        ics_tasks[i].diag = (double*) malloc(sizeof(double) * _line_length_max);
        ics_tasks[i].l_diag = (double*) malloc(sizeof(double) * _line_length_max - 1);
    }

    free(ics_adi_dir_x->ordered_start_stop_indices);
    free(ics_adi_dir_x->line_start_stop_indices);

    free(ics_adi_dir_y->ordered_start_stop_indices);
    free(ics_adi_dir_y->line_start_stop_indices);

    free(ics_adi_dir_z->ordered_start_stop_indices);
    free(ics_adi_dir_z->line_start_stop_indices);

    ics_adi_dir_x->ordered_start_stop_indices = (long*) malloc(sizeof(long) * n * 2);
    ics_adi_dir_x->line_start_stop_indices = (long*) malloc(sizeof(long) * n * 2);

    ics_adi_dir_y->ordered_start_stop_indices = (long*) malloc(sizeof(long) * n * 2);
    ics_adi_dir_y->line_start_stop_indices = (long*) malloc(sizeof(long) * n * 2);

    ics_adi_dir_z->ordered_start_stop_indices = (long*) malloc(sizeof(long) * n * 2);
    ics_adi_dir_z->line_start_stop_indices = (long*) malloc(sizeof(long) * n * 2);

    divide_x_work(n);
    divide_y_work(n);
    divide_z_work(n);
}


void ICS_Grid_node::apply_node_flux3D(double dt, double* ydot) {
    double* dest;
    if (ydot == NULL)
        dest = states_cur;
    else
        dest = ydot;
    apply_node_flux(node_flux_count, node_flux_idx, node_flux_scale, node_flux_src, dt, dest);
}

void ICS_Grid_node::do_grid_currents(double* output, double dt, int) {
    memset(states_cur, 0, sizeof(double) * _num_nodes);
    if (ics_current_seg_ptrs != NULL) {
        ssize_t i, j;
        int seg_start_index, seg_stop_index;
        int state_index;
        double seg_cur;
        auto const n = ics_concentration_seg_handles.size();
        for (i = 0; i < n; i++) {
            seg_start_index = ics_surface_nodes_per_seg_start_indices[i];
            seg_stop_index = ics_surface_nodes_per_seg_start_indices[i + 1];
            seg_cur = *ics_current_seg_ptrs[i];
            for (j = seg_start_index; j < seg_stop_index; j++) {
                state_index = ics_surface_nodes_per_seg[j];
                output[state_index] += seg_cur * ics_scale_factors[state_index] * dt;
            }
        }
    }
}

void ICS_Grid_node::volume_setup() {
    if (ics_adi_dir_x->dcgrid == NULL) {
        ics_adi_dir_x->ics_dg_adi_dir = ics_dg_adi_x;
        ics_adi_dir_y->ics_dg_adi_dir = ics_dg_adi_y;
        ics_adi_dir_z->ics_dg_adi_dir = ics_dg_adi_z;
    } else {
        ics_adi_dir_x->ics_dg_adi_dir = ics_dg_adi_x_inhom;
        ics_adi_dir_y->ics_dg_adi_dir = ics_dg_adi_y_inhom;
        ics_adi_dir_z->ics_dg_adi_dir = ics_dg_adi_z_inhom;
    }
}

int ICS_Grid_node::dg_adi() {
    if (diffusable) {
        run_threaded_deltas(this, ics_adi_dir_x);
        run_threaded_deltas(this, ics_adi_dir_y);
        run_threaded_deltas(this, ics_adi_dir_z);
        run_threaded_ics_dg_adi(ics_adi_dir_x);
        run_threaded_ics_dg_adi(ics_adi_dir_y);
        run_threaded_ics_dg_adi(ics_adi_dir_z);
    }
    return 0;
}


void ICS_Grid_node::variable_step_diffusion(const double* states, double* ydot) {
    if (diffusable) {
        _ics_rhs_variable_step_helper(this, states, ydot);
    }

    // TODO: Get volume fraction/tortuosity working as well as take care of this in this file
    /*switch(VARIABLE_ECS_VOLUME)
    {
        case VOLUME_FRACTION:
            _ics_rhs_variable_step_helper_vol(this, states, ydot);
            break;
        case TORTUOSITY:
            _ics_rhs_variable_step_helper_tort(this, states, ydot);
            break;
        default:
            _ics_rhs_variable_step_helper(this, states, ydot);
    }*/
}

void ICS_Grid_node::variable_step_ode_solve(double* RHS, double dt) {
    if (diffusable) {
        ics_ode_solve_helper(this, dt, RHS);
    }
}

void ICS_Grid_node::hybrid_connections() {
    _ics_hybrid_helper(this);
}

void ICS_Grid_node::variable_step_hybrid_connections(const double* cvode_states_3d,
                                                     double* const ydot_3d,
                                                     const double* cvode_states_1d,
                                                     double* const ydot_1d) {
    _ics_variable_hybrid_helper(this, cvode_states_3d, ydot_3d, cvode_states_1d, ydot_1d);
}

void ICS_Grid_node::scatter_grid_concentrations() {
    auto const n = ics_concentration_seg_handles.size();
    for (auto i = 0ul; i < n; ++i) {
        double total_seg_concentration{};
        auto const seg_start_index = ics_surface_nodes_per_seg_start_indices[i];
        auto const seg_stop_index = ics_surface_nodes_per_seg_start_indices[i + 1];
        for (auto j = seg_start_index; j < seg_stop_index; j++) {
            total_seg_concentration += states[ics_surface_nodes_per_seg[j]];
        }
        auto const average_seg_concentration = total_seg_concentration /
                                               (seg_stop_index - seg_start_index);
        *ics_concentration_seg_handles[i] = average_seg_concentration;
    }
}

// Free a single Grid_node
ICS_Grid_node::~ICS_Grid_node() {
    int i;
    free(states_x);
    free(states_y);
    free(states_z);
    free(states_cur);
    delete[] concentration_list;
    delete[] current_list;
    free(current_dest);
#if NRNMPI
    if (nrnmpi_use) {
        free(proc_offsets);
        free(proc_num_currents);
        free(proc_num_fluxes);
    }
#endif
    free(ics_adi_dir_x->ordered_start_stop_indices);
    free(ics_adi_dir_x->line_start_stop_indices);
    free(ics_adi_dir_x->ordered_nodes);
    free(ics_adi_dir_x->deltas);
    free(ics_adi_dir_x);

    free(ics_adi_dir_y->ordered_start_stop_indices);
    free(ics_adi_dir_y->line_start_stop_indices);
    free(ics_adi_dir_y->ordered_nodes);
    free(ics_adi_dir_y->deltas);
    free(ics_adi_dir_y);

    free(ics_adi_dir_z->ordered_start_stop_indices);
    free(ics_adi_dir_z->line_start_stop_indices);
    free(ics_adi_dir_z->ordered_nodes);
    free(ics_adi_dir_z->deltas);
    free(ics_adi_dir_z);

    free(hybrid_data);
    if (node_flux_count > 0) {
        free(node_flux_idx);
        free(node_flux_scale);
        free(node_flux_src);
    }

    if (ics_tasks != NULL) {
        for (i = 0; i < NUM_THREADS; i++) {
            free(ics_tasks[i].scratchpad);
            free(ics_tasks[i].RHS);
            free(ics_tasks[i].u_diag);
            free(ics_tasks[i].l_diag);
        }
    }
    free(ics_tasks);
}
