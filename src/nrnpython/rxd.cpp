#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "grids.h"
#include <cfloat>
#include "rxd.h"
#include <../nrnoc/section.h>
#include <../nrnoc/nrn_ansi.h>
#include <../nrnoc/multicore.h>
#include <nrnwrap_Python.h>
#include <nrnpython.h>

#include <thread>
#include <vector>
#include "ocmatrix.h"
#include "ivocvect.h"

static void ode_solve(double, double*, double*);
extern PyTypeObject* hocobject_type;
extern int structure_change_cnt;
extern int states_cvode_offset;
int prev_structure_change_cnt = 0;
unsigned char initialized = FALSE;

/*
    Globals
*/
extern NrnThread* nrn_threads;
int NUM_THREADS = 1;
namespace nrn {
namespace rxd {
std::vector<std::thread> Threads;
TaskQueue task_queue;
}  // namespace rxd
}  // namespace nrn
TaskQueue* AllTasks{&nrn::rxd::task_queue};
using namespace nrn::rxd;

extern double* dt_ptr;
extern double* t_ptr;


fptr _setup, _initialize, _setup_matrices, _setup_units;
extern NrnThread* nrn_threads;

/*intracellular diffusion*/
unsigned char diffusion = FALSE;
int _rxd_euler_nrow = 0, _rxd_euler_nnonzero = 0, _rxd_num_zvi = 0;
long* _rxd_euler_nonzero_i = NULL;
long* _rxd_euler_nonzero_j = NULL;
double* _rxd_euler_nonzero_values = NULL;
long* _rxd_zero_volume_indices = NULL;
double* _rxd_a = NULL;
double* _rxd_b = NULL;
double* _rxd_c = NULL;
double* _rxd_d = NULL;
long* _rxd_p = NULL;
unsigned int* _rxd_zvi_child_count = NULL;
long** _rxd_zvi_child = NULL;
static int _cvode_offset;
static int _ecs_count;

/*intracellular reactions*/
ICSReactions* _reactions = NULL;

/*Indices used to set atol scale*/
SpeciesIndexList* species_indices = NULL;

/*intracellular reactions*/
double* states;
unsigned int num_states = 0;
int _num_reactions = 0;
int _curr_count;
int* _curr_indices = NULL;
double* _curr_scales = NULL;
std::vector<neuron::container::data_handle<double>> _conc_ptrs, _curr_ptrs;
int _conc_count;
int* _conc_indices = NULL;

/*membrane fluxes*/
int _memb_curr_total = 0;            /*number of membrane currents (sum of
                                       _memb_species_count)*/
int _memb_curr_nodes = 0;            /*corresponding number of nodes
                                       equal to _memb_curr_total if Extracellular is not used*/
int _memb_count = 0;                 /*number of membrane currents*/
double* _rxd_flux_scale;             /*array length _memb_count to scale fluxes*/
double* _rxd_induced_currents_scale; /*array of Extracellular current scales*/
int* _cur_node_indices;              /*array length _memb_count into nodes index*/


int* _memb_species_count; /*array of length _memb_count
                           number of species involved in each membrane
                           current*/

/*arrays of size _memb_count by _memb_species_count*/
std::vector<std::vector<neuron::container::data_handle<double>>> _memb_cur_ptrs;
int** _memb_cur_charges;
int*** _memb_cur_mapped;     /*array of pairs of indices*/
int*** _memb_cur_mapped_ecs; /*array of pointer into ECS grids*/

double* _rxd_induced_currents = NULL; /*set when calculating reactions*/
ECS_Grid_node** _rxd_induced_currents_grid = NULL;

unsigned char _membrane_flux = FALSE; /*TRUE if any membrane fluxes are in the model*/
int* _membrane_lookup;                /*states index -> position in _rxd_induced_currents*/

int _node_flux_count = 0;
long* _node_flux_idx = NULL;
double* _node_flux_scale = NULL;
PyObject** _node_flux_src = NULL;

static void free_zvi_child() {
    int i;
    /*Clear previous _rxd_zvi_child*/
    if (_rxd_zvi_child != NULL && _rxd_zvi_child_count != NULL) {
        for (i = 0; i < _rxd_num_zvi; i++)
            if (_rxd_zvi_child_count[i] > 0)
                free(_rxd_zvi_child[i]);
        free(_rxd_zvi_child);
        free(_rxd_zvi_child_count);
        _rxd_zvi_child_count = NULL;
        _rxd_zvi_child = NULL;
    }
}

static void transfer_to_legacy() {
    /*TODO: support 3D*/
    int i;
    for (i = 0; i < _conc_count; i++) {
        *_conc_ptrs[i] = states[_conc_indices[i]];
    }
}

static inline void* allocopy(void* src, size_t size) {
    void* dst = malloc(size);
    memcpy(dst, src, size);
    return dst;
}

extern "C" void rxd_set_no_diffusion() {
    int i;
    diffusion = FALSE;
    if (_rxd_a != NULL) {
        free(_rxd_a);
        free(_rxd_b);
        free(_rxd_c);
        free(_rxd_d);
        free(_rxd_p);
        free(_rxd_euler_nonzero_i);
        free(_rxd_euler_nonzero_j);
        free(_rxd_euler_nonzero_values);
        _rxd_a = NULL;
    }
}

extern "C" void free_curr_ptrs() {
    _curr_count = 0;
    if (_curr_indices != NULL)
        free(_curr_indices);
    _curr_indices = NULL;
    if (_curr_scales != NULL)
        free(_curr_scales);
    _curr_scales = NULL;
    _curr_ptrs.clear();
}

extern "C" void free_conc_ptrs() {
    _conc_count = 0;
    if (_conc_indices != NULL)
        free(_conc_indices);
    _conc_indices = NULL;
    _conc_ptrs.clear();
}


extern "C" void rxd_setup_curr_ptrs(int num_currents,
                                    int* curr_index,
                                    double* curr_scale,
                                    PyHocObject** curr_ptrs) {
    free_curr_ptrs();
    /* info for NEURON currents - to update states */
    _curr_count = num_currents;
    _curr_indices = (int*) malloc(sizeof(int) * num_currents);
    memcpy(_curr_indices, curr_index, sizeof(int) * num_currents);

    _curr_scales = (double*) malloc(sizeof(double) * num_currents);
    memcpy(_curr_scales, curr_scale, sizeof(double) * num_currents);

    _curr_ptrs.resize(num_currents);
    for (int i = 0; i < num_currents; i++)
        _curr_ptrs[i] = curr_ptrs[i]->u.px_;
}

extern "C" void rxd_setup_conc_ptrs(int conc_count, int* conc_index, PyHocObject** conc_ptrs) {
    /* info for NEURON concentration - to transfer to legacy */
    int i;
    free_conc_ptrs();
    _conc_count = conc_count;
    _conc_indices = (int*) malloc(sizeof(int) * conc_count);
    memcpy(_conc_indices, conc_index, sizeof(int) * conc_count);
    _conc_ptrs.resize(conc_count);
    for (i = 0; i < conc_count; i++)
        _conc_ptrs[i] = conc_ptrs[i]->u.px_;
}

extern "C" void rxd_include_node_flux3D(int grid_count,
                                        int* grid_counts,
                                        int* grids,
                                        long* index,
                                        double* scales,
                                        PyObject** sources) {
    Grid_node* g;
    int i = 0, j, k, n, grid_id;
    int offset = 0;

    for (g = Parallel_grids[0]; g != NULL; g = g->next) {
        if (g->node_flux_count > 0) {
            g->node_flux_count = 0;
            free(g->node_flux_scale);
            free(g->node_flux_idx);
            free(g->node_flux_src);
        }
    }
    if (grid_count == 0)
        return;
    for (grid_id = 0, g = Parallel_grids[0]; g != NULL; grid_id++, g = g->next) {
#if NRNMPI
        if (nrnmpi_use && dynamic_cast<ECS_Grid_node*>(g)) {
            if (grid_id == grids[i])
                n = grid_counts[i++];
            else
                n = 0;

            g->proc_num_fluxes[nrnmpi_myid] = n;
            nrnmpi_int_allgather_inplace(g->proc_num_fluxes, 1);

            g->proc_flux_offsets[0] = 0;
            for (j = 1; j < nrnmpi_numprocs; j++)
                g->proc_flux_offsets[j] = g->proc_flux_offsets[j - 1] + g->proc_num_fluxes[j - 1];
            g->node_flux_count = g->proc_flux_offsets[j - 1] + g->proc_num_fluxes[j - 1];
            /*Copy array of the indexes and scales -- sources are evaluated at runtime*/
            if (n > 0) {
                g->node_flux_idx = (long*) malloc(g->node_flux_count * sizeof(long));
                g->node_flux_scale = (double*) malloc(g->node_flux_count * sizeof(double));
                g->node_flux_src = (PyObject**) allocopy(&sources[offset], n * sizeof(PyObject*));
            }

            for (j = 0, k = g->proc_flux_offsets[nrnmpi_myid]; j < n; j++, k++) {
                g->node_flux_idx[k] = index[offset + j];
                g->node_flux_scale[k] = scales[offset + j];
            }
            nrnmpi_long_allgatherv_inplace(g->node_flux_idx,
                                           g->proc_num_fluxes,
                                           g->proc_flux_offsets);
            nrnmpi_dbl_allgatherv_inplace(g->node_flux_scale,
                                          g->proc_num_fluxes,
                                          g->proc_flux_offsets);

            offset += n;
        } else {
            if (grid_id == grids[i]) {
                g->node_flux_count = grid_counts[i];
                if (grid_counts[i] > 0) {
                    g->node_flux_idx = (long*) allocopy(&index[offset],
                                                        grid_counts[i] * sizeof(long));
                    g->node_flux_scale = (double*) allocopy(&scales[offset],
                                                            grid_counts[i] * sizeof(double));
                    g->node_flux_src = (PyObject**) allocopy(&sources[offset],
                                                             grid_counts[i] * sizeof(PyObject*));
                }
                offset += grid_counts[i++];
            }
        }
#else
        if (grid_id == grids[i]) {
            g->node_flux_count = grid_counts[i];
            if (grid_counts[i] > 0) {
                g->node_flux_idx = (long*) allocopy(&index[offset], grid_counts[i] * sizeof(long));
                g->node_flux_scale = (double*) allocopy(&scales[offset],
                                                        grid_counts[i] * sizeof(double));
                g->node_flux_src = (PyObject**) allocopy(&sources[offset],
                                                         grid_counts[i] * sizeof(PyObject*));
            }
            offset += grid_counts[i++];
        }
#endif
    }
}

extern "C" void rxd_include_node_flux1D(int n, long* index, double* scales, PyObject** sources) {
    if (_node_flux_count != 0) {
        free(_node_flux_idx);
        free(_node_flux_scale);
        free(_node_flux_src);
    }
    _node_flux_count = n;
    if (n > 0) {
        _node_flux_idx = (long*) allocopy(index, n * sizeof(long));
        _node_flux_scale = (double*) allocopy(scales, n * sizeof(double));
        _node_flux_src = (PyObject**) allocopy(sources, n * sizeof(PyObject*));
    }
}


void apply_node_flux(int n,
                     long* index,
                     double* scale,
                     PyObject** source,
                     double dt,
                     double* states) {
    for (size_t i = 0; i < n; i++) {
        size_t j = index == nullptr ? i : index[i];
        if (PyFloat_Check(source[i])) {
            states[j] += dt * PyFloat_AsDouble(source[i]) / scale[i];
        } else if (PyCallable_Check(source[i])) {
            /* It is a Python function or a PyHocObject*/
            if (PyObject_TypeCheck(source[i], hocobject_type)) {
                auto src = (PyHocObject*) source[i];
                /*TODO: check it is a reference */
                if (src->type_ == PyHoc::HocRefNum) {
                    states[j] += dt * (src->u.x_) / scale[i];
                } else {
                    states[j] += dt * *(src->u.px_) / scale[i];
                }
            } else {
                auto result = PyObject_CallObject(source[i], nullptr);
                if (PyFloat_Check(result)) {
                    states[j] += dt * PyFloat_AsDouble(result) / scale[i];
                } else if (PyLong_Check(result)) {
                    states[j] += dt * (double) PyLong_AsLong(result) / scale[i];
                } else if (PyInt_Check(result)) {
                    states[j] += dt * (double) PyInt_AsLong(result) / scale[i];
                } else {
                    PyErr_SetString(PyExc_Exception,
                                    "node._include_flux callback did not return a number.\n");
                }
                Py_DECREF(result);
            }
        } else {
            PyErr_SetString(PyExc_Exception, "node._include_flux unrecognised source term.\n");
        }
    }
}


static void apply_node_flux1D(double dt, double* states) {
    apply_node_flux(_node_flux_count, _node_flux_idx, _node_flux_scale, _node_flux_src, dt, states);
}

extern "C" void rxd_set_euler_matrix(int nrow,
                                     int nnonzero,
                                     long* nonzero_i,
                                     long* nonzero_j,
                                     double* nonzero_values,
                                     double* c_diagonal) {
    long i, j, idx;
    double val;
    unsigned int k, ps;
    unsigned int* parent_count;
    /*free old data*/
    if (_rxd_a != NULL) {
        free(_rxd_a);
        free(_rxd_b);
        free(_rxd_c);
        free(_rxd_d);
        free(_rxd_p);
        free(_rxd_euler_nonzero_i);
        free(_rxd_euler_nonzero_j);
        free(_rxd_euler_nonzero_values);
        _rxd_a = NULL;
    }
    diffusion = TRUE;

    _rxd_euler_nrow = nrow;
    _rxd_euler_nnonzero = nnonzero;
    _rxd_euler_nonzero_i = (long*) allocopy(nonzero_i, sizeof(long) * nnonzero);
    _rxd_euler_nonzero_j = (long*) allocopy(nonzero_j, sizeof(long) * nnonzero);
    _rxd_euler_nonzero_values = (double*) allocopy(nonzero_values, sizeof(double) * nnonzero);

    _rxd_a = (double*) calloc(nrow, sizeof(double));
    _rxd_b = (double*) calloc(nrow, sizeof(double));
    _rxd_c = (double*) calloc(nrow, sizeof(double));
    _rxd_d = (double*) calloc(nrow, sizeof(double));
    _rxd_p = (long*) malloc(nrow * sizeof(long));
    parent_count = (unsigned int*) calloc(nrow, sizeof(unsigned int));

    for (idx = 0; idx < nrow; idx++)
        _rxd_p[idx] = -1;

    for (idx = 0; idx < nnonzero; idx++) {
        i = nonzero_i[idx];
        j = nonzero_j[idx];
        val = nonzero_values[idx];
        if (i < j) {
            _rxd_p[j] = i;
            parent_count[i]++;
            _rxd_a[j] = val;
        } else if (i == j) {
            _rxd_d[i] = val;
        } else {
            _rxd_b[i] = val;
        }
    }

    for (idx = 0; idx < nrow; idx++) {
        _rxd_c[idx] = _rxd_d[idx] > 0 ? c_diagonal[idx] : 1.0;
    }

    if (_rxd_num_zvi > 0) {
        _rxd_zvi_child_count = (unsigned int*) malloc(_rxd_num_zvi * sizeof(unsigned int));
        _rxd_zvi_child = (long**) malloc(_rxd_num_zvi * sizeof(long*));

        /* find children of zero-volume-indices */
        for (i = 0; i < _rxd_num_zvi; i++) {
            ps = parent_count[_rxd_zero_volume_indices[i]];
            if (ps == 0) {
                _rxd_zvi_child_count[i] = 0;
                _rxd_zvi_child[i] = NULL;

                continue;
            }
            _rxd_zvi_child[i] = (long*) malloc(ps * sizeof(long));
            _rxd_zvi_child_count[i] = ps;
            for (j = 0, k = 0; k < ps; j++) {
                if (_rxd_zero_volume_indices[i] == _rxd_p[j]) {
                    _rxd_zvi_child[i][k] = j;
                    k++;
                }
            }
        }
    }
    free(parent_count);
}

static void add_currents(double* result) {
    long i, j, k, idx;
    short side;
    /*Add currents to the result*/
    for (k = 0; k < _curr_count; k++)
        result[_curr_indices[k]] += _curr_scales[k] * (*_curr_ptrs[k]);

    /*Subtract rxd induced currents*/
    if (_membrane_flux) {
        for (i = 0, k = 0; i < _memb_count; i++) {
            for (j = 0; j < _memb_species_count[i]; j++, k++) {
                for (side = 0; side < 2; side++) {
                    idx = _memb_cur_mapped[i][j][side];
                    if (idx != SPECIES_ABSENT) {
                        result[_curr_indices[idx]] -= _curr_scales[idx] * _rxd_induced_currents[k];
                    }
                }
            }
        }
    }
}
static void mul(int nnonzero,
                long* nonzero_i,
                long* nonzero_j,
                const double* nonzero_values,
                const double* v,
                double* result) {
    long i, j, k;
    /* now loop through all the nonzero locations */
    /* NOTE: this would be more efficient if not repeatedly doing the result[i] lookup */
    for (k = 0; k < nnonzero; k++) {
        i = *nonzero_i++;
        j = *nonzero_j++;
        result[i] -= (*nonzero_values++) * v[j];
    }
}

extern "C" void set_setup(const fptr setup_fn) {
    _setup = setup_fn;
}

extern "C" void set_initialize(const fptr initialize_fn) {
    _initialize = initialize_fn;
    set_num_threads(NUM_THREADS);
}

extern "C" void set_setup_matrices(fptr setup_matrices) {
    _setup_matrices = setup_matrices;
}

extern "C" void set_setup_units(fptr setup_units) {
    _setup_units = setup_units;
}

/* nrn_tree_solve modified from nrnoc/ldifus.c */
static void nrn_tree_solve(double* a,
                           double* b,
                           double* c,
                           double* dbase,
                           double* rhs,
                           long* pindex,
                           long n,
                           double dt) {
    /*
        treesolver

        a      - above the diagonal
        b      - below the diagonal
        c      - used to define diagonal: diag = c + dt * dbase
        dbase  - together with c, used to define diagonal
        rhs    - right hand side, which is changed to the result
        pindex - parent indices
        n      - number of states
    */
    long i, pin;
    double* d = (double*) malloc(sizeof(double) * n);
    double* myd;
    double* myc;
    double* mydbase;

    /* TODO: check that d non-null */

    /* construct diagonal */
    myd = d;
    mydbase = dbase;
    myc = c;
    for (i = 0; i < n; i++) {
        *myd++ = *myc++ + dt * (*mydbase++);
    }

    /* triang */
    for (i = n - 1; i > 0; --i) {
        pin = pindex[i];
        if (pin > -1) {
            double p;
            p = dt * a[i] / d[i];
            d[pin] -= dt * p * b[i];
            rhs[pin] -= p * rhs[i];
        }
    }
    /* bksub */
    for (i = 0; i < n; ++i) {
        pin = pindex[i];
        if (pin > -1) {
            rhs[i] -= dt * b[i] * rhs[pin];
        }
        rhs[i] /= d[i];
    }

    free(d);
}


static void ode_solve(double dt, double* p1, double* p2) {
    long i, j;
    double* b = p1 + _cvode_offset;
    double* y = p2 + _cvode_offset;
    double *full_b, *full_y;
    long* zvi = _rxd_zero_volume_indices;

    if (_rxd_num_zvi > 0) {
        full_b = (double*) calloc(sizeof(double), num_states);
        full_y = (double*) calloc(sizeof(double), num_states);
        for (i = 0, j = 0; i < num_states; i++) {
            if (i == zvi[j]) {
                j++;
            } else {
                full_b[i] = b[i - j];
                full_y[i] = y[i - j];
            }
        }
    } else {
        full_b = b;
        full_y = y;
    }
    if (diffusion)
        nrn_tree_solve(_rxd_a, _rxd_b, _rxd_c, _rxd_d, full_b, _rxd_p, _rxd_euler_nrow, dt);

    do_ics_reactions(full_y, full_b, y, b);

    if (_rxd_num_zvi > 0) {
        for (i = 0, j = 0; i < num_states; i++) {
            if (i == zvi[j])
                j++;
            else
                b[i - j] = full_b[i];
        }
        free(full_b);
        free(full_y);
    }
}

static void ode_abs_tol(double* p1) {
    int i, j;
    double* y = p1 + _cvode_offset;
    if (species_indices != NULL) {
        SpeciesIndexList* list;
        for (list = species_indices; list->next != NULL; list = list->next) {
            for (i = 0, j = 0; i < list->length; i++) {
                for (; j < _rxd_num_zvi && _rxd_zero_volume_indices[j] <= list->indices[i]; j++)
                    ;
                y[list->indices[i] - j] *= list->atolscale;
            }
        }
    }
}

static void free_currents() {
    int i, j;
    if (!_membrane_flux)
        return;
    for (i = 0; i < _memb_count; i++) {
        for (j = 0; j < _memb_species_count[i]; j++) {
            free(_memb_cur_mapped[i][j]);
        }
        free(_memb_cur_mapped[i]);
    }
    _memb_cur_ptrs.clear();
    free(_memb_cur_mapped);
    free(_memb_species_count);
    free(_cur_node_indices);
    free(_rxd_induced_currents);
    free(_rxd_flux_scale);
    free(_membrane_lookup);
    free(_memb_cur_mapped_ecs);
    free(_rxd_induced_currents_grid);
    free(_rxd_induced_currents_scale);
    _membrane_flux = FALSE;
}

extern "C" void setup_currents(int num_currents,
                               int num_fluxes,
                               int* num_species,
                               int* node_idxs,
                               double* scales,
                               PyHocObject** ptrs,
                               int* mapped,
                               int* mapped_ecs) {
    int i, j, k, id, side, count;
    int* induced_currents_ecs_idx;
    int* induced_currents_grid_id;
    int* ecs_indices;
    double* current_scales;
    PyHocObject** ecs_ptrs;

    Current_Triple* c;
    Grid_node* g;
    ECS_Grid_node* grid;


    free_currents();

    _memb_count = num_currents;
    _memb_curr_total = num_fluxes;
    _memb_species_count = (int*) malloc(sizeof(int) * num_currents);
    memcpy(_memb_species_count, num_species, sizeof(int) * num_currents);

    _rxd_flux_scale = (double*) calloc(sizeof(double), num_fluxes);
    // memcpy(_rxd_flux_scale,scales,sizeof(double)*num_currents);

    /*TODO: if memory is an issue - replace with a map*/
    _membrane_lookup = (int*) malloc(sizeof(int) * num_states);
    memset(_membrane_lookup, SPECIES_ABSENT, sizeof(int) * num_states);

    _memb_cur_ptrs.resize(num_currents);
    _memb_cur_mapped_ecs = (int***) malloc(sizeof(int*) * num_currents);
    _memb_cur_mapped = (int***) malloc(sizeof(int**) * num_currents);
    induced_currents_ecs_idx = (int*) malloc(sizeof(int) * _memb_curr_total);
    induced_currents_grid_id = (int*) malloc(sizeof(int) * _memb_curr_total);
    // initialize memory here to allow currents from an intracellular species
    // with no corresponding nrn_region='o' or Extracellular species
    memset(induced_currents_ecs_idx, SPECIES_ABSENT, sizeof(int) * _memb_curr_total);

    for (i = 0, k = 0; i < num_currents; i++) {
        _memb_cur_ptrs[i].resize(num_species[i]);
        _memb_cur_mapped_ecs[i] = (int**) malloc(sizeof(int*) * num_species[i]);
        _memb_cur_mapped[i] = (int**) malloc(sizeof(int*) * num_species[i]);


        for (j = 0; j < num_species[i]; j++, k++) {
            _memb_cur_ptrs[i][j] = ptrs[k]->u.px_;
            _memb_cur_mapped[i][j] = (int*) malloc(2 * sizeof(int));
            _memb_cur_mapped_ecs[i][j] = (int*) malloc(2 * sizeof(int));


            for (side = 0; side < 2; side++) {
                _memb_cur_mapped[i][j][side] = mapped[2 * k + side];
                _memb_cur_mapped_ecs[i][j][side] = mapped_ecs[2 * k + side];
            }
            for (side = 0; side < 2; side++) {
                if (_memb_cur_mapped[i][j][side] != SPECIES_ABSENT) {
                    _membrane_lookup[_curr_indices[_memb_cur_mapped[i][j][side]]] = k;
                    _rxd_flux_scale[k] = scales[i];
                    if (_memb_cur_mapped[i][j][(side + 1) % 2] == SPECIES_ABSENT) {
                        induced_currents_grid_id[k] = _memb_cur_mapped_ecs[i][j][0];
                        induced_currents_ecs_idx[k] = _memb_cur_mapped_ecs[i][j][1];
                    }
                }
            }
        }
    }
    _rxd_induced_currents_grid = (ECS_Grid_node**) calloc(_memb_curr_total, sizeof(ECS_Grid_node*));
    _rxd_induced_currents_scale = (double*) calloc(_memb_curr_total, sizeof(double));
    for (id = 0, g = Parallel_grids[0]; g != NULL; g = g->next, id++) {
        grid = dynamic_cast<ECS_Grid_node*>(g);
        if (grid == NULL)
            continue;  // ignore ICS grids

        for (count = 0, k = 0; k < _memb_curr_total; k++) {
            if (induced_currents_grid_id[k] == id) {
                _rxd_induced_currents_grid[k] = grid;
                count++;
            }
        }
        if (count > 0) {
            ecs_indices = (int*) malloc(count * sizeof(int));
            ecs_ptrs = (PyHocObject**) malloc(count * sizeof(PyHocObject*));
            for (i = 0, k = 0; k < _memb_curr_total; k++) {
                if (induced_currents_grid_id[k] == id) {
                    ecs_indices[i] = induced_currents_ecs_idx[k];
                    ecs_ptrs[i++] = ptrs[k];
                }
            }
            current_scales = grid->set_rxd_currents(count, ecs_indices, ecs_ptrs);
            free(ecs_ptrs);

            for (i = 0, k = 0; k < _memb_curr_total; k++) {
                if (induced_currents_grid_id[k] == id)
                    _rxd_induced_currents_scale[k] = current_scales[i];
            }
        }
    }
    /*index into arrays of nodes/states*/
    _cur_node_indices = (int*) malloc(sizeof(int) * num_currents);
    memcpy(_cur_node_indices, node_idxs, sizeof(int) * num_currents);
    _membrane_flux = TRUE;
    _rxd_induced_currents = (double*) malloc(sizeof(double) * _memb_curr_total);
    free(induced_currents_ecs_idx);
    free(induced_currents_grid_id);
}

static void _currents(double* rhs) {
    int i, j, k, idx, side;
    Grid_node* g;
    ECS_Grid_node* grid;
    if (!_membrane_flux)
        return;
    get_all_reaction_rates(states, NULL, NULL);
    for (g = Parallel_grids[0]; g != NULL; g = g->next) {
        grid = dynamic_cast<ECS_Grid_node*>(g);
        if (grid)
            grid->induced_idx = 0;
    }
    for (i = 0, k = 0; i < _memb_count; i++) {
        idx = _cur_node_indices[i];

        for (j = 0; j < _memb_species_count[i]; j++, k++) {
            rhs[idx] -= _rxd_induced_currents[k];
            *(_memb_cur_ptrs[i][j]) += _rxd_induced_currents[k];

            for (side = 0; side < 2; side++) {
                if (_memb_cur_mapped[i][j][side] == SPECIES_ABSENT) {
                    /*Extracellular region is within the ECS grid*/
                    grid = _rxd_induced_currents_grid[k];
                    if (grid != NULL && _memb_cur_mapped[i][j][(side + 1) % 2] != SPECIES_ABSENT)
                        grid->local_induced_currents[grid->induced_idx++] =
                            _rxd_induced_currents[k];
                }
            }
        }
    }
}

extern "C" int rxd_nonvint_block(int method, int size, double* p1, double* p2, int) {
    if (initialized) {
        if (structure_change_cnt != prev_structure_change_cnt) {
            /*TODO: Exclude irrelevant (non-rxd) structural changes*/
            /*Needed for node.include_flux*/
            _setup_matrices();
        }
    }
    switch (method) {
    case 0:
        _setup();
        break;
    case 1:
        _initialize();
        // TODO: is there a better place to initialize multicompartment reactions
        Grid_node* grid;
        ECS_Grid_node* g;
        for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
            g = dynamic_cast<ECS_Grid_node*>(grid);
            if (g)
                g->initialize_multicompartment_reaction();
        }
        break;
    case 2:
        /* compute outward current to be subtracted from rhs */
        _currents(p1);
        break;
    case 3:
        /* conductance to be added to d */
        break;
    case 4:
        /* fixed step solve */
        _fadvance();
        _fadvance_fixed_step_3D();
        break;
    case 5:
        /* ode_count */
        _cvode_offset = size;
        _ecs_count = ode_count(size + num_states - _rxd_num_zvi);
        return _ecs_count + num_states - _rxd_num_zvi;
    case 6:
        /* ode_reinit(y) */
        _ode_reinit(p1);
        _ecs_ode_reinit(p1);
        break;
    case 7:
        /* ode_fun(t, y, ydot); from t and y determine ydot */
        _rhs_variable_step(p1, p2);
        _rhs_variable_step_ecs(p1, p2);
        break;
    case 8:
        ode_solve(*dt_ptr, p1, p2); /*solve mx=b replace b with x */
        /* TODO: we can probably reuse the dgadi code here... for now, we do nothing, which
         * implicitly approximates the Jacobian as the identity matrix */
        // y= p1 = states and b = p2 = RHS for x direction
        ics_ode_solve(*dt_ptr, p1, p2);
        break;
    case 9:
        // ode_jacobian(*dt_ptr, p1, p2); /* optionally prepare jacobian for fast ode_solve */
        break;
    case 10:
        ode_abs_tol(p1);
        ecs_atolscale(p1);
        /* ode_abs_tol(y_abs_tolerance); fill with cvode.atol() * scalefactor */
        break;
    default:
        printf("Unknown rxd_nonvint_block call: %d\n", method);
        break;
    }
    return 0;
}


/*****************************************************************************
 *
 * Begin intracellular code
 *
 *****************************************************************************/


extern "C" void register_rate(int nspecies,
                              int nparam,
                              int nregions,
                              int nseg,
                              int* sidx,
                              int necs,
                              int necsparam,
                              int* ecs_ids,
                              int* ecsidx,
                              int nmult,
                              double* mult,
                              PyHocObject** vptrs,
                              ReactionRate f) {
    int i, j, k, idx, ecs_id, ecs_index, ecs_offset;
    unsigned char counted;
    Grid_node* g;
    ECS_Grid_node* grid;
    ICSReactions* react = (ICSReactions*) malloc(sizeof(ICSReactions));
    react->reaction = f;
    react->num_species = nspecies;
    react->num_regions = nregions;
    react->num_params = nparam;
    react->num_segments = nseg;
    react->num_ecs_species = necs;
    react->num_ecs_params = necsparam;
    react->num_mult = nmult;
    react->icsN = 0;
    react->ecsN = 0;
    if (vptrs != NULL) {
        react->vptrs = (double**) malloc(nseg * sizeof(double*));
        for (i = 0; i < nseg; i++)
            react->vptrs[i] = static_cast<double*>(vptrs[i]->u.px_);
    } else {
        react->vptrs = NULL;
    }
    react->state_idx = (int***) malloc(nseg * sizeof(int**));
    for (i = 0, idx = 0; i < nseg; i++) {
        react->state_idx[i] = (int**) malloc((nspecies + nparam) * sizeof(int*));
        for (j = 0; j < nspecies + nparam; j++) {
            react->state_idx[i][j] = (int*) malloc(nregions * sizeof(int));
            for (k = 0; k < nregions; k++, idx++) {
                if (sidx[idx] < 0) {
                    react->state_idx[i][j][k] = SPECIES_ABSENT;
                } else {
                    react->state_idx[i][j][k] = sidx[idx];
                    if (i == 0 && j < nspecies)
                        react->icsN++;
                }
            }
        }
    }
    if (nmult > 0) {
        react->mc_multiplier = (double**) malloc(nmult * sizeof(double*));
        for (i = 0; i < nmult; i++) {
            react->mc_multiplier[i] = (double*) malloc(nseg * sizeof(double));
            memcpy(react->mc_multiplier[i], (mult + i * nseg), nseg * sizeof(double));
        }
    }

    if (react->num_ecs_species + react->num_ecs_params > 0) {
        react->ecs_grid = (ECS_Grid_node**) malloc(react->num_ecs_species * sizeof(Grid_node*));
        react->ecs_state = (double***) malloc(nseg * sizeof(double**));
        react->ecs_index = (int**) malloc(nseg * sizeof(int*));
        react->ecs_offset_index = (int*) malloc(react->num_ecs_species * sizeof(int));
        for (i = 0; i < nseg; i++) {
            react->ecs_state[i] = (double**) malloc((necs + necsparam) * sizeof(double*));
            react->ecs_index[i] = (int*) malloc((necs + necsparam) * sizeof(int));
        }
        for (j = 0; j < necs + necsparam; j++) {
            ecs_offset = num_states - _rxd_num_zvi;

            for (ecs_id = 0, g = Parallel_grids[0]; g != NULL; g = g->next, ecs_id++) {
                if (ecs_id == ecs_ids[j]) {
                    grid = dynamic_cast<ECS_Grid_node*>(g);
                    assert(grid != NULL);
                    if (j < necs) {
                        react->ecs_grid[j] = grid;
                        react->ecs_offset_index[j] =
                            grid->add_multicompartment_reaction(nseg, &ecsidx[j], necs + necsparam);
                    }

                    for (i = 0, counted = FALSE; i < nseg; i++) {
                        // nseg x nregion x nspecies
                        ecs_index = ecsidx[i * (necs + necsparam) + j];

                        if (ecs_index >= 0) {
                            react->ecs_state[i][j] = &(grid->states[ecs_index]);
                            react->ecs_index[i][j] = ecs_offset + ecs_index;
                            if (j < necs && !counted) {
                                react->ecsN++;
                                counted = TRUE;
                            }
                        } else {
                            react->ecs_state[i][j] = NULL;
                        }
                    }
                    ecs_offset += grid->size_x * grid->size_y * grid->size_z;
                }
            }
        }
    } else {
        react->ecs_state = NULL;
    }
    if (_reactions == NULL) {
        _reactions = react;
        react->next = NULL;

    } else {
        react->next = _reactions;
        _reactions = react;
    }

    for (g = Parallel_grids[0]; g != NULL; g = g->next) {
        grid = dynamic_cast<ECS_Grid_node*>(g);
        if (grid)
            grid->initialize_multicompartment_reaction();
    }
}

extern "C" void clear_rates() {
    ICSReactions *react, *prev;
    int i, j;
    for (react = _reactions; react != NULL;) {
        if (react->vptrs != NULL)
            free(react->vptrs);
        for (i = 0; i < react->num_segments; i++) {
            for (j = 0; j < react->num_species; j++) {
                free(react->state_idx[i][j]);
            }
            free(react->state_idx[i]);

            if (react->num_ecs_species + react->num_ecs_params > 0) {
                free(react->ecs_state[i]);
            }
        }
        if (react->num_mult > 0) {
            for (i = 0; i < react->num_mult; i++)
                free(react->mc_multiplier[i]);
            free(react->mc_multiplier);
        }

        free(react->state_idx);
        SAFE_FREE(react->ecs_state);
        prev = react;
        react = react->next;
        SAFE_FREE(prev);
    }
    _reactions = NULL;
    /*clear extracellular reactions*/
    clear_rates_ecs();
    // There are NUM_THREADS-1 std::thread objects alive, and these need to be
    // cleaned up before exit (otherwise the std::thread destructor will call
    // std::terminate.).
    set_num_threads(1);
}


extern "C" void species_atolscale(int id, double scale, int len, int* idx) {
    SpeciesIndexList* list;
    SpeciesIndexList* prev;
    if (species_indices != NULL) {
        for (list = species_indices, prev = NULL; list != NULL; list = list->next) {
            if (list->id == id) {
                list->atolscale = scale;
                return;
            }
            prev = list;
        }
        prev->next = (SpeciesIndexList*) malloc(sizeof(SpeciesIndexList));
        list = prev->next;
    } else {
        species_indices = (SpeciesIndexList*) malloc(sizeof(SpeciesIndexList));
        list = species_indices;
    }
    list->id = id;
    list->indices = (int*) malloc(sizeof(int) * len);
    memcpy(list->indices, idx, sizeof(int) * len);
    list->length = len;
    list->atolscale = scale;
    list->next = NULL;
}

extern "C" void remove_species_atolscale(int id) {
    SpeciesIndexList* list;
    SpeciesIndexList* prev;
    for (list = species_indices, prev = NULL; list != NULL; prev = list, list = list->next) {
        if (list->id == id) {
            if (prev == NULL)
                species_indices = list->next;
            else
                prev->next = list->next;
            free(list->indices);
            free(list);
            break;
        }
    }
}

extern "C" void setup_solver(double* my_states, int my_num_states, long* zvi, int num_zvi) {
    free_currents();
    states = my_states;
    num_states = my_num_states;
    free_zvi_child();
    _rxd_num_zvi = num_zvi;
    if (_rxd_zero_volume_indices != NULL)
        free(_rxd_zero_volume_indices);
    if (num_zvi == 0)
        _rxd_zero_volume_indices = NULL;
    else
        _rxd_zero_volume_indices = (long*) allocopy(zvi, num_zvi * sizeof(long));
    dt_ptr = &nrn_threads->_dt;
    t_ptr = &nrn_threads->_t;
    set_num_threads(NUM_THREADS);
    initialized = TRUE;
    prev_structure_change_cnt = structure_change_cnt;
}

void TaskQueue_add_task(TaskQueue* q, void* (*task)(void*), void* args, void* result) {
    auto* t = new TaskList{};
    t->task = task;
    t->args = args;
    t->result = result;
    t->next = nullptr;

    // Add task to the queue
    {
        std::lock_guard<std::mutex> _{q->task_mutex};
        if (!q->first) {
            // empty queue
            q->first = t;
            q->last = t;
        } else {
            // queue not empty
            q->last->next = t;
            q->last = t;
        }
        {
            std::lock_guard<std::mutex> _{q->waiting_mutex};
            ++q->length;
        }
    }

    // signal a waiting thread that there is a new task to pick up
    q->task_cond.notify_one();
}

void TaskQueue_exe_tasks(std::size_t thread_index, TaskQueue* q) {
    for (;;) {
        // Wait for a task to be available in the queue, then execute it.
        {
            TaskList* job{};
            {
                std::unique_lock<std::mutex> lock{q->task_mutex};
                // Wait until either for a new task to be received or for this
                // thread to be told to exit.
                q->task_cond.wait(lock,
                                  [q, thread_index] { return q->first || q->exit[thread_index]; });
                if (q->exit[thread_index]) {
                    return;
                }
                job = q->first;
                q->first = job->next;
            }
            // Execute the task
            job->result = job->task(job->args);
            delete job;
        }
        // Decrement the list length, if it's now empty then broadcast that to
        // the master thread, which may be waiting for the queue to be empty.
        auto const new_length = [q] {
            std::lock_guard<std::mutex> _{q->waiting_mutex};
            return --(q->length);
        }();
        // The immediately-executed lambda means we release the mutex before
        // calling notify_one()
        if (new_length == 0) {
            // Queue is empty. Notify the main thread, which may be blocking on
            // this condition.
            q->waiting_cond.notify_one();
        }
    }
}


void set_num_threads(const int n) {
    assert(n > 0);
    assert(NUM_THREADS > 0);
    // n and NUM_THREADS include the main thread, old_num and new_num refer to
    // the number of std::thread workers
    std::size_t const old_num = NUM_THREADS - 1;
    std::size_t const new_num = n - 1;
    assert(old_num == Threads.size());
    assert(old_num == task_queue.exit.size());
    if (new_num < old_num) {
        // Kill some threads. First, wait until the queue is empty.
        TaskQueue_sync(&task_queue);
        // Now signal to the threads that are to be killed that they need to
        // exit.
        {
            std::lock_guard<std::mutex> _{task_queue.task_mutex};
            for (auto k = new_num; k < old_num; ++k) {
                task_queue.exit[k] = true;
            }
        }
        task_queue.task_cond.notify_all();
        // Finally, join those threads and destroy the std::thread objects.
        for (auto k = new_num; k < old_num; ++k) {
            Threads[k].join();
        }
        // And update the structures
        {
            std::lock_guard<std::mutex> _{task_queue.task_mutex};
            Threads.resize(new_num);
            task_queue.exit.resize(new_num);
        }
    } else if (new_num > old_num) {
        // Create some threads
        std::lock_guard<std::mutex> _{task_queue.task_mutex};
        task_queue.exit.reserve(new_num);
        Threads.reserve(new_num);
        for (auto k = old_num; k < new_num; ++k) {
            assert(k == Threads.size());
            Threads.emplace_back(TaskQueue_exe_tasks, k, &task_queue);
            task_queue.exit.emplace_back(false);
        }
    }
    assert(new_num == Threads.size());
    assert(new_num == task_queue.exit.size());
    set_num_threads_3D(n);
    NUM_THREADS = n;
}

void TaskQueue_sync(TaskQueue* q) {
    // Wait till the queue is empty
    std::unique_lock<std::mutex> lock{q->waiting_mutex};
    q->waiting_cond.wait(lock, [q] { return q->length == 0; });
}

int get_num_threads(void) {
    return NUM_THREADS;
}


void _fadvance(void) {
    double dt = *dt_ptr;
    long i;
    /*variables for diffusion*/
    double* rhs;
    long* zvi = _rxd_zero_volume_indices;

    rhs = (double*) calloc(num_states, sizeof(double));
    /*diffusion*/
    if (diffusion)
        mul(_rxd_euler_nnonzero,
            _rxd_euler_nonzero_i,
            _rxd_euler_nonzero_j,
            _rxd_euler_nonzero_values,
            states,
            rhs);

    add_currents(rhs);

    /* multiply rhs vector by dt */
    for (i = 0; i < num_states; i++) {
        rhs[i] *= dt;
    }

    if (diffusion)
        nrn_tree_solve(_rxd_a, _rxd_b, _rxd_c, _rxd_d, rhs, _rxd_p, _rxd_euler_nrow, dt);

    /* increment states by rhs which is now really deltas */
    for (i = 0; i < num_states; i++) {
        states[i] += rhs[i];
    }

    /* clear zero volume indices (conservation nodes) */
    for (i = 0; i < _rxd_num_zvi; i++) {
        states[zvi[i]] = 0;
    }

    free(rhs);

    /*reactions*/
    do_ics_reactions(states, NULL, NULL, NULL);

    /*node fluxes*/
    apply_node_flux1D(dt, states);

    transfer_to_legacy();
}


void _ode_reinit(double* y) {
    long i, j;
    long* zvi = _rxd_zero_volume_indices;
    y += _cvode_offset;
    /*Copy states to CVode*/
    if (_rxd_num_zvi > 0) {
        for (i = 0, j = 0; i < num_states; i++) {
            if (zvi[j] == i)
                j++;
            else {
                y[i - j] = states[i];
            }
        }
    } else {
        memcpy(y, states, sizeof(double) * num_states);
    }
}


void _rhs_variable_step(const double* p1, double* p2) {
    Grid_node* grid;
    long i, j, p, c;
    unsigned int k;
    const unsigned char calculate_rhs = p2 == NULL ? 0 : 1;
    const double* my_states = p1 + _cvode_offset;
    double* ydot = p2 + _cvode_offset;
    /*variables for diffusion*/
    double* rhs;
    long* zvi = _rxd_zero_volume_indices;

    double const* const orig_states3d = p1 + states_cvode_offset;
    double* const orig_ydot3d = p2 + states_cvode_offset;

    /*Copy states from CVode*/
    if (_rxd_num_zvi > 0) {
        for (i = 0, j = 0; i < num_states; i++) {
            if (zvi[j] == i)
                j++;
            else {
                states[i] = my_states[i - j];
            }
        }
    } else {
        memcpy(states, my_states, sizeof(double) * num_states);
    }

    if (diffusion) {
        for (i = 0; i < _rxd_num_zvi; i++) {
            j = zvi[i];
            p = _rxd_p[j];
            states[j] = (p > 0 ? -(_rxd_b[j] / _rxd_d[j]) * states[p] : 0);
            for (k = 0; k < _rxd_zvi_child_count[i]; k++) {
                c = _rxd_zvi_child[i][k];
                states[j] -= (_rxd_a[c] / _rxd_d[j]) * states[c];
            }
        }
    }

    transfer_to_legacy();

    if (!calculate_rhs) {
        for (i = 0; i < _rxd_num_zvi; i++)
            states[zvi[i]] = 0;
        return;
    }

    /*diffusion*/
    rhs = (double*) calloc(num_states, sizeof(double));

    if (diffusion)
        mul(_rxd_euler_nnonzero,
            _rxd_euler_nonzero_i,
            _rxd_euler_nonzero_j,
            _rxd_euler_nonzero_values,
            states,
            rhs);

    /*reactions*/
    memset(&ydot[num_states - _rxd_num_zvi], 0, sizeof(double) * _ecs_count);
    get_all_reaction_rates(states, rhs, ydot);


    const double* states3d = orig_states3d;
    double* ydot3d = orig_ydot3d;
    int grid_size;
    for (grid = Parallel_grids[0]; grid != NULL; grid = grid->next) {
        grid_size = grid->size_x * grid->size_y * grid->size_z;
        if (grid->hybrid) {
            grid->variable_step_hybrid_connections(states3d, ydot3d, states, rhs);
        }
        ydot3d += grid_size;
        states3d += grid_size;
    }

    /*Add currents to the result*/
    add_currents(rhs);

    /*Add node fluxes to the result*/
    apply_node_flux1D(1.0, rhs);

    /* increment states by rhs which is now really deltas */
    if (_rxd_num_zvi > 0) {
        for (i = 0, j = 0; i < num_states; i++) {
            if (zvi[j] == i) {
                states[i] = 0;
                j++;
            } else {
                ydot[i - j] = rhs[i];
            }
        }
    } else {
        memcpy(ydot, rhs, sizeof(double) * num_states);
    }
    free(rhs);
}

void get_reaction_rates(ICSReactions* react, double* states, double* rates, double* ydot) {
    int segment;
    int i, j, k, idx;
    double** states_for_reaction = (double**) malloc(react->num_species * sizeof(double*));
    double** params_for_reaction = (double**) malloc(react->num_params * sizeof(double*));
    double** result_array = (double**) malloc(react->num_species * sizeof(double*));
    double* mc_mult = NULL;
    double** flux = NULL;
    if (react->num_mult > 0)
        mc_mult = (double*) malloc(react->num_mult * sizeof(double));

    double* ecs_states_for_reaction = NULL;
    double* ecs_params_for_reaction = NULL;
    double* ecs_result = NULL;
    int* ecsindex = NULL;
    double v = 0;
    if (react->num_ecs_species > 0) {
        ecs_states_for_reaction = (double*) malloc(react->num_ecs_species * sizeof(double));
        ecs_result = (double*) malloc(react->num_ecs_species * sizeof(double));
    }
    if (react->num_ecs_params > 0)
        ecs_params_for_reaction = (double*) calloc(react->num_ecs_params, sizeof(double));

    if (_membrane_flux) {
        flux = (double**) malloc(react->icsN * sizeof(double*));
        for (i = 0; i < react->icsN; i++)
            flux[i] = (double*) calloc(react->num_regions, sizeof(double));
    }

    for (i = 0; i < react->num_species; i++) {
        states_for_reaction[i] = (double*) calloc(react->num_regions, sizeof(double));
        result_array[i] = (double*) malloc(react->num_regions * sizeof(double));
    }
    for (i = 0; i < react->num_params; i++)
        params_for_reaction[i] = (double*) calloc(react->num_regions, sizeof(double));
    ecsindex = (int*) malloc(react->num_ecs_species * sizeof(int));
    for (i = 0; i < react->num_ecs_species; i++)
        ecsindex[i] = react->ecs_grid[i]->react_offsets[react->ecs_offset_index[i]];

    for (segment = 0; segment < react->num_segments; segment++) {
        for (i = 0; i < react->num_species; i++) {
            for (j = 0; j < react->num_regions; j++) {
                if (react->state_idx[segment][i][j] != SPECIES_ABSENT) {
                    states_for_reaction[i][j] = states[react->state_idx[segment][i][j]];
                } else {
                    states_for_reaction[i][j] = NAN;
                }
            }
            memset(result_array[i], 0, react->num_regions * sizeof(double));
        }
        for (k = 0; i < react->num_species + react->num_params; i++, k++) {
            for (j = 0; j < react->num_regions; j++) {
                if (react->state_idx[segment][i][j] != SPECIES_ABSENT) {
                    params_for_reaction[k][j] = states[react->state_idx[segment][i][j]];
                } else {
                    params_for_reaction[k][j] = NAN;
                }
            }
        }

        for (i = 0; i < react->num_ecs_species; i++) {
            if (react->ecs_state[segment][i] != NULL) {
                ecs_states_for_reaction[i] = *(react->ecs_state[segment][i]);
            } else {
                ecs_states_for_reaction[i] = NAN;
            }
        }
        for (k = 0; i < react->num_ecs_species + react->num_ecs_params; i++, k++) {
            if (react->ecs_state[segment][i] != NULL) {
                ecs_params_for_reaction[k] = *(react->ecs_state[segment][i]);
            } else {
                ecs_params_for_reaction[k] = NAN;
            }
        }
        memset(ecs_result, 0, react->num_ecs_species * sizeof(double));

        for (i = 0; i < react->num_mult; i++) {
            mc_mult[i] = react->mc_multiplier[i][segment];
        }

        if (react->vptrs != NULL) {
            v = *(react->vptrs[segment]);
        }

        react->reaction(states_for_reaction,
                        params_for_reaction,
                        result_array,
                        mc_mult,
                        ecs_states_for_reaction,
                        ecs_params_for_reaction,
                        ecs_result,
                        flux,
                        v);

        for (i = 0; i < react->num_species; i++) {
            for (j = 0; j < react->num_regions; j++) {
                idx = react->state_idx[segment][i][j];
                if (idx != SPECIES_ABSENT) {
                    if (_membrane_flux && _membrane_lookup[idx] != SPECIES_ABSENT) {
                        _rxd_induced_currents[_membrane_lookup[idx]] -=
                            _rxd_flux_scale[_membrane_lookup[idx]] * flux[i][j];
                    }
                    if (rates != NULL) {
                        rates[idx] += result_array[i][j];
                    }
                }
            }
        }
        if (ydot != NULL) {
            for (i = 0; i < react->num_ecs_species; i++) {
                if (react->ecs_state[segment][i] != NULL)
                    react->ecs_grid[i]->all_reaction_states[ecsindex[i]++] = ecs_result[i];
                // ydot[react->ecs_index[segment][i]] += ecs_result[i];
            }
        }
    }
    /* free allocated memory */
    if (react->num_mult > 0)
        free(mc_mult);
    if (_membrane_flux) {
        for (i = 0; i < react->icsN; i++)
            free(flux[i]);
        free(flux);
    }
    if (react->num_ecs_species > 0) {
        free(ecs_states_for_reaction);
        free(ecs_result);
    }
    for (i = 0; i < react->num_species; i++) {
        free(states_for_reaction[i]);
        free(result_array[i]);
    }
    free(states_for_reaction);
    free(result_array);
    for (i = 0; i < react->num_params; i++) {
        free(params_for_reaction[i]);
    }
    free(params_for_reaction);
    if (react->num_ecs_params > 0) {
        free(ecs_params_for_reaction);
    }
}

void solve_reaction(ICSReactions* react,
                    double* states,
                    double* bval,
                    double* cvode_states,
                    double* cvode_b) {
    int segment;
    int i, j, k, idx, jac_i, jac_j, jac_idx;
    int N = react->icsN + react->ecsN; /*size of Jacobian (number species*regions for a segments)*/
    double pd;
    double dt = *dt_ptr;
    double dx = FLT_EPSILON;
    auto jacobian = std::make_unique<OcFullMatrix>(N, N);
    auto b = std::make_unique<IvocVect>(N);
    auto x = std::make_unique<IvocVect>(N);

    double** states_for_reaction = (double**) malloc(react->num_species * sizeof(double*));
    double** states_for_reaction_dx = (double**) malloc(react->num_species * sizeof(double*));
    double** params_for_reaction = (double**) malloc(react->num_params * sizeof(double*));
    double** result_array = (double**) malloc(react->num_species * sizeof(double*));
    double** result_array_dx = (double**) malloc(react->num_species * sizeof(double*));
    double* mc_mult = NULL;
    if (react->num_mult > 0)
        mc_mult = (double*) malloc(react->num_mult * sizeof(double));

    double* ecs_states_for_reaction = NULL;
    double* ecs_states_for_reaction_dx = NULL;
    double* ecs_params_for_reaction = NULL;
    double* ecs_result = NULL;
    double* ecs_result_dx = NULL;
    double v = 0;
    int* ecsindex = NULL;

    if (react->num_ecs_species > 0) {
        ecsindex = (int*) malloc(react->num_ecs_species * sizeof(int));
        for (i = 0; i < react->num_ecs_species; i++)
            ecsindex[i] = react->ecs_grid[i]->react_offsets[react->ecs_offset_index[i]];
        ecs_states_for_reaction = (double*) malloc(react->num_ecs_species * sizeof(double));
        ecs_states_for_reaction_dx = (double*) malloc(react->num_ecs_species * sizeof(double));
        ecs_result = (double*) malloc(react->num_ecs_species * sizeof(double));
        ecs_result_dx = (double*) malloc(react->num_ecs_species * sizeof(double));
    }

    if (react->num_ecs_params > 0)
        ecs_params_for_reaction = (double*) malloc(react->num_ecs_params * sizeof(double));


    for (i = 0; i < react->num_species; i++) {
        states_for_reaction[i] = (double*) malloc(react->num_regions * sizeof(double));
        states_for_reaction_dx[i] = (double*) malloc(react->num_regions * sizeof(double));
        result_array[i] = (double*) malloc(react->num_regions * sizeof(double));
        result_array_dx[i] = (double*) malloc(react->num_regions * sizeof(double));
    }
    for (i = 0; i < react->num_params; i++)
        params_for_reaction[i] = (double*) malloc(react->num_regions * sizeof(double));

    for (segment = 0; segment < react->num_segments; segment++) {
        if (react->vptrs != NULL)
            v = *(react->vptrs[segment]);

        for (i = 0; i < react->num_species; i++) {
            for (j = 0; j < react->num_regions; j++) {
                if (react->state_idx[segment][i][j] != SPECIES_ABSENT) {
                    states_for_reaction[i][j] = states[react->state_idx[segment][i][j]];
                    states_for_reaction_dx[i][j] = states_for_reaction[i][j];
                } else {
                    states_for_reaction[i][j] = SPECIES_ABSENT;
                    states_for_reaction_dx[i][j] = states_for_reaction[i][j];
                }
            }
            memset(result_array[i], 0, react->num_regions * sizeof(double));
            memset(result_array_dx[i], 0, react->num_regions * sizeof(double));
        }
        for (k = 0; i < react->num_species + react->num_params; i++, k++) {
            for (j = 0; j < react->num_regions; j++) {
                if (react->state_idx[segment][i][j] != SPECIES_ABSENT) {
                    params_for_reaction[k][j] = states[react->state_idx[segment][i][j]];
                } else {
                    params_for_reaction[k][j] = SPECIES_ABSENT;
                }
            }
        }


        for (i = 0; i < react->num_ecs_species; i++) {
            if (react->ecs_state[segment][i] != NULL) {
                if (cvode_states != NULL) {
                    ecs_states_for_reaction[i] = cvode_states[react->ecs_index[segment][i]];
                } else {
                    ecs_states_for_reaction[i] = *(react->ecs_state[segment][i]);
                }
                ecs_states_for_reaction_dx[i] = ecs_states_for_reaction[i];
            }
        }
        for (k = 0; i < react->num_ecs_species + react->num_ecs_params; i++, k++) {
            if (react->ecs_state[segment][i] != NULL) {
                ecs_params_for_reaction[k] = *(react->ecs_state[segment][i]);
            }
        }

        if (react->num_ecs_species > 0) {
            memset(ecs_result, 0, react->num_ecs_species * sizeof(double));
            memset(ecs_result_dx, 0, react->num_ecs_species * sizeof(double));
        }

        for (i = 0; i < react->num_mult; i++) {
            mc_mult[i] = react->mc_multiplier[i][segment];
        }

        react->reaction(states_for_reaction,
                        params_for_reaction,
                        result_array,
                        mc_mult,
                        ecs_states_for_reaction,
                        ecs_params_for_reaction,
                        ecs_result,
                        NULL,
                        v);

        /*Calculate I - Jacobian for ICS reactions*/
        for (i = 0, idx = 0; i < react->num_species; i++) {
            for (j = 0; j < react->num_regions; j++) {
                if (react->state_idx[segment][i][j] != SPECIES_ABSENT) {
                    if (bval == NULL)
                        b->elem(idx) = dt * result_array[i][j];
                    else
                        b->elem(idx) = bval[react->state_idx[segment][i][j]];


                    // set up the changed states array
                    states_for_reaction_dx[i][j] += dx;

                    /* TODO: Handle approximating the Jacobian at a function upper
                     * limit, e.g. acos(1)
                     */
                    react->reaction(states_for_reaction_dx,
                                    params_for_reaction,
                                    result_array_dx,
                                    mc_mult,
                                    ecs_states_for_reaction,
                                    ecs_params_for_reaction,
                                    ecs_result_dx,
                                    NULL,
                                    v);

                    for (jac_i = 0, jac_idx = 0; jac_i < react->num_species; jac_i++) {
                        for (jac_j = 0; jac_j < react->num_regions; jac_j++) {
                            // pd is our Jacobian approximated
                            if (react->state_idx[segment][jac_i][jac_j] != SPECIES_ABSENT) {
                                pd = (result_array_dx[jac_i][jac_j] - result_array[jac_i][jac_j]) /
                                     dx;
                                *jacobian->mep(jac_idx, idx) = (idx == jac_idx) - dt * pd;
                                jac_idx += 1;
                            }
                            result_array_dx[jac_i][jac_j] = 0;
                        }
                    }
                    for (jac_i = 0; jac_i < react->num_ecs_species; jac_i++) {
                        // pd is our Jacobian approximated
                        if (react->ecs_state[segment][jac_i] != NULL) {
                            pd = (ecs_result_dx[jac_i] - ecs_result[jac_i]) / dx;
                            *jacobian->mep(jac_idx, idx) = -dt * pd;
                            jac_idx += 1;
                        }
                        ecs_result_dx[jac_i] = 0;
                    }
                    // reset dx array
                    states_for_reaction_dx[i][j] -= dx;
                    idx++;
                }
            }
        }

        /*Calculate I - Jacobian for MultiCompartment ECS reactions*/
        for (i = 0; i < react->num_ecs_species; i++) {
            if (react->ecs_state[segment][i] != NULL) {
                if (bval == NULL)
                    b->elem(idx) = dt * ecs_result[i];
                else
                    b->elem(idx) = cvode_b[react->ecs_index[segment][i]];


                // set up the changed states array
                ecs_states_for_reaction_dx[i] += dx;

                /* TODO: Handle approximating the Jacobian at a function upper
                 * limit, e.g. acos(1)
                 */
                react->reaction(states_for_reaction,
                                params_for_reaction,
                                result_array_dx,
                                mc_mult,
                                ecs_states_for_reaction_dx,
                                ecs_params_for_reaction,
                                ecs_result_dx,
                                NULL,
                                v);

                for (jac_i = 0, jac_idx = 0; jac_i < react->num_species; jac_i++) {
                    for (jac_j = 0; jac_j < react->num_regions; jac_j++) {
                        // pd is our Jacobian approximated
                        if (react->state_idx[segment][jac_i][jac_j] != SPECIES_ABSENT) {
                            pd = (result_array_dx[jac_i][jac_j] - result_array[jac_i][jac_j]) / dx;
                            *jacobian->mep(jac_idx, idx) = -dt * pd;
                            jac_idx += 1;
                        }
                    }
                }
                for (jac_i = 0; jac_i < react->num_ecs_species; jac_i++) {
                    // pd is our Jacobian approximated
                    if (react->ecs_state[segment][jac_i] != NULL) {
                        pd = (ecs_result_dx[jac_i] - ecs_result[jac_i]) / dx;
                        *jacobian->mep(jac_idx, idx) = (idx == jac_idx) - dt * pd;
                        jac_idx += 1;
                    } else {
                        *jacobian->mep(idx, idx) = 1.0;
                    }
                    // reset dx array
                    ecs_states_for_reaction_dx[i] -= dx;
                }
                idx++;
            }
        }
        // solve for x, destructively
        jacobian->solv(b.get(), x.get(), false);

        if (bval != NULL)  // variable-step
        {
            for (i = 0, jac_idx = 0; i < react->num_species; i++) {
                for (j = 0; j < react->num_regions; j++) {
                    idx = react->state_idx[segment][i][j];
                    if (idx != SPECIES_ABSENT) {
                        bval[idx] = x->elem(jac_idx++);
                    }
                }
            }
            for (i = 0; i < react->num_ecs_species; i++) {
                if (react->ecs_state[segment][i] != NULL)
                    react->ecs_grid[i]->all_reaction_states[ecsindex[i]++] = x->elem(jac_idx++);
                // cvode_b[react->ecs_index[segment][i]] = x->elem(jac_idx++);
            }
        } else  // fixed-step
        {
            for (i = 0, jac_idx = 0; i < react->num_species; i++) {
                for (j = 0; j < react->num_regions; j++) {
                    idx = react->state_idx[segment][i][j];
                    if (idx != SPECIES_ABSENT)
                        states[idx] += x->elem(jac_idx++);
                }
            }
            for (i = 0; i < react->num_ecs_species; i++) {
                if (react->ecs_state[segment][i] != NULL)
                    react->ecs_grid[i]->all_reaction_states[ecsindex[i]++] = x->elem(jac_idx++);
            }
        }
    }
    free(ecsindex);
    for (i = 0; i < react->num_species; i++) {
        free(states_for_reaction[i]);
        free(states_for_reaction_dx[i]);
        free(result_array[i]);
        free(result_array_dx[i]);
    }
    for (i = 0; i < react->num_params; i++)
        free(params_for_reaction[i]);
    if (react->num_mult > 0)
        free(mc_mult);
    free(states_for_reaction_dx);
    free(states_for_reaction);
    free(params_for_reaction);
    free(result_array);
    free(result_array_dx);
    if (react->num_ecs_species > 0) {
        free(ecs_states_for_reaction);
        free(ecs_states_for_reaction_dx);
        free(ecs_result);
        free(ecs_result_dx);
    }
    if (react->num_ecs_params > 0)
        free(ecs_params_for_reaction);
}

void do_ics_reactions(double* states, double* b, double* cvode_states, double* cvode_b) {
    ICSReactions* react;
    for (react = _reactions; react != NULL; react = react->next) {
        if (react->icsN + react->ecsN > 0)
            solve_reaction(react, states, b, cvode_states, cvode_b);
    }
}

void get_all_reaction_rates(double* states, double* rates, double* ydot) {
    ICSReactions* react;
    if (_membrane_flux)
        memset(_rxd_induced_currents, 0, sizeof(double) * _memb_curr_total);
    for (react = _reactions; react != NULL; react = react->next) {
        if (react->icsN + react->ecsN > 0)
            get_reaction_rates(react, states, rates, ydot);
    }
}

/*****************************************************************************
 *
 * End intracellular code
 *
 *****************************************************************************/
