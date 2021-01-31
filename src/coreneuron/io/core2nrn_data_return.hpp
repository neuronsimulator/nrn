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

/** @brief Copies back to NEURON the voltage, i_membrane_, and mechanism data.
 */
extern void core2nrn_data_return();

}  // namespace coreneuron
#endif  // _H_CORENRNDATARETURN_
