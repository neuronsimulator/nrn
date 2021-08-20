/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef _H_CORENRNDATARETURN_
#define _H_CORENRNDATARETURN_

namespace coreneuron {

/** @brief Copies back to NEURON everything needed to analyze and continue simulation.
    I.e. voltage, i_membrane_, mechanism data, event queue, WATCH state,
    Play state, etc.
 */
extern void core2nrn_data_return();

/** @brief return first and last datum indices of WATCH statements
 */
extern void watch_datum_indices(int type, int& first, int& last);

}  // namespace coreneuron
#endif  // _H_CORENRNDATARETURN_
