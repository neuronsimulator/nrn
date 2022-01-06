/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

/* Bool global variable to define if the fast_imem
 * calculations should be enabled.
 */
extern bool nrn_use_fast_imem;

/* Free memory allocated for the fast current membrane calculation.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
void fast_imem_free();

/* fast_imem_alloc() wrapper.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
void nrn_fast_imem_alloc();

/* Calculate the new values of rhs array at every timestep.
 * Found in src/nrnoc/fadvance.cpp in NEURON.
 */

void nrn_calc_fast_imem(NrnThread* _nt);
/* Initialization used only in offline (file) mode.
 * See NEURON nrn_calc_fast_imem_fixedstep_init in src/nrnoc/fadvance.cpp
 */
void nrn_calc_fast_imem_init(NrnThread* _nt);

}  // namespace coreneuron
