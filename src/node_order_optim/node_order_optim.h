#pragma once

namespace neuron {

extern int interleave_permute_type;

/// @brief Select node ordering for optimum gaussian elimination
/// @param type
///    0  cell together (Section construction order)
///    1  Interleave, identical cells warp adjacent
///    2  Depth order, optimize adjacent nodes to have adjacent parents.
void nrn_optimize_node_order(int type);

/// @brief Compute and carry out the permutation for interleave_permute_type
void nrn_permute_node_order();

/**
 *
 * \brief Solve the Hines matrices based on the interleave_permute_type (1 or 2).
 *
 * For interleave_permute_type == 1 : Naive interleaving -> Each execution thread deals with one
 * Hines matrix (cell) For interleave_permute_type == 2 : Advanced interleaving -> Each Hines matrix
 * is solved by multiple execution threads (with coalesced memory access as well)
 */
extern void solve_interleaved(int ith);

}  // namespace neuron
