/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/**
 * @file nrn_stats.h
 * @date 25th Dec 2014
 * @brief Function declarations for the cell statistics
 *
 */

#pragma once
namespace coreneuron {
/** @brief Reports global cell statistics of the simulation
 *
 *  This routine prints the global number of cells, synapses of the simulation
 *  @param void
 *  @return void
 */
void report_cell_stats();

}  // namespace coreneuron
