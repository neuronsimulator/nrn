/*
# =============================================================================
# Originally errcodes.h from SCoP library, Copyright (c) 1984-90 Duke University
# =============================================================================
# Subsequent extensive prototype and memory layout changes for CoreNEURON
#
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once
namespace coreneuron {
extern int abort_run(int);
namespace scopmath {
/** @brief Flag to disable some code sections at compile time.
 *
 *  Some methods, such as coreneuron::scopmath::sparse::getelm(...), decide at
 *  runtime whether they are simply accessors, or if they dynamically modify the
 *  matrix in question, possibly allocating new memory. Typically the second
 *  mode will be used during model initialisation, while the first will be used
 *  during computation/simulation. Compiling the more complicated code for the
 *  second mode can be problematic for targets such as GPU, where dynamic
 *  allocation and global state are complex. This enum is intended to be used as
 *  a template parameter to flag (at compile time) when this code can be
 *  omitted.
 */
enum struct enabled_code { all, compute_only };
}  // namespace scopmath
}  // namespace coreneuron
#define ROUNDOFF       1.e-20
#define ZERO           1.e-8
#define STEP           1.e-6
#define CONVERGE       1.e-6
#define MAXCHANGE      0.05
#define INITSIMPLEX    0.25
#define MAXITERS       50
#define MAXSMPLXITERS  100
#define MAXSTEPS       20
#define MAXHALVE       15
#define MAXORDER       6
#define MAXTERMS       3
#define MAXFAIL        10
#define MAX_JAC_ITERS  20
#define MAX_GOOD_ORDER 2
#define MAX_GOOD_STEPS 3

#define SUCCESS      0
#define EXCEED_ITERS 1
#define SINGULAR     2
#define PRECISION    3
#define CORR_FAIL    4
#define INCONSISTENT 5
#define BAD_START    6
#define NODATA       7
#define NO_SOLN      8
#define LOWMEM       9
#define DIVCHECK     10
#define NOFORCE      11
#define DIVERGED     12
#define NEG_ARG      13
#define RANGE        14
