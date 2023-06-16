#pragma once
#include "multicore.h"
#include "neuron/container/generic_data_handle.hpp"
#include "nrnredef.h"
/**
 * @file section_fwd.hpp
 * @brief Forward declarations of Section, Node etc. to be included in translated MOD files.
 */
struct Node;
struct Prop;
struct Section;
using Datum = neuron::container::generic_data_handle;

extern int cvode_active_;
extern int secondorder;
extern int use_sparse13;

double nrn_ghk(double, double, double, double);
Datum* nrn_prop_datum_alloc(int type, int count, Prop* p);

#if EXTRACELLULAR
/* pruned to only work with sparse13 */
extern int nrn_nlayer_extracellular;
#define nlayer (nrn_nlayer_extracellular) /* first (0) layer is extracellular next to membrane */
struct Extnode {
    std::vector<neuron::container::data_handle<double>> param{};
    // double* param; /* points to extracellular parameter vector */
    /* v is membrane potential. so v internal = Node.v + Node.vext[0] */
    /* However, the Node equation is for v internal. */
    /* This is reconciled during update. */

    /* Following all have allocated size of nlayer */
    double* v; /* v external. */
    double* _a;
    double* _b;
    double** _d;
    double** _rhs; /* d, rhs, a, and b are analogous to those in node */
    double** _a_matelm;
    double** _b_matelm;
    double** _x12; /* effect of v[layer] on eqn layer-1 (or internal)*/
    double** _x21; /* effect of v[layer-1 or internal] on eqn layer*/
};
#endif

#define NODEA(n)   _nrn_mechanism_access_a(n)
#define NODEB(n)   _nrn_mechanism_access_b(n)
#define NODED(n)   _nrn_mechanism_access_d(n)
#define NODERHS(n) _nrn_mechanism_access_rhs(n)
#ifdef NODEV
// Defined on macOS ("no device") at least
#undef NODEV
#endif
#define NODEV(n) _nrn_mechanism_access_voltage(n)

/**
 * A point process is computed just like regular mechanisms. Ie it appears in the property list
 * whose type specifies which allocation, current, and state functions to call. This means some
 * nodes have more properties than other nodes even in the same section. The Point_process structure
 * allows the interface to hoc variable names. Each variable symbol u.rng->type refers to the point
 * process mechanism. The variable is treated as a vector variable whose first index specifies
 * "which one" of that mechanisms insertion points we are talking about. Finally the variable
 * u.rng->index tells us where in the p-array to look. The number of point_process vectors is the
 * number of different point process types. This is different from the mechanism type which
 * enumerates all mechanisms including the point_processes. It is the responsibility of
 * create_point_process to set up the vectors and fill in the symbol information. However only after
 * the process is given a location can the variables be set or accessed. This is because the
 * allocation function may have to connect to some ionic parameters and the process exists primarily
 * as a property of a node.
 */
struct Point_process {
    Section* sec{}; /* section and node location for the point mechanism*/
    Node* node{};
    Prop* prop{};    /* pointer to the actual property linked to the
                  node property list */
    Object* ob{};    /* object that owns this process */
    void* presyn_{}; /* non-threshold presynapse for NetCon */
    void* nvi_{};    /* NrnVarIntegrator (for local step method) */
    void* _vnt{};    /* NrnThread* (for NET_RECEIVE and multicore) */
};
