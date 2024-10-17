/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <array>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"

namespace coreneuron {

#if !defined(NRN_SOA_PAD)
// for layout 0, every range variable array must have a size which
// is a multiple of NRN_SOA_PAD doubles
#define NRN_SOA_PAD 8
#endif

/// return the new offset considering the byte aligment settings
size_t nrn_soa_byte_align(size_t i);

/// This function return the index in a flat array of a matrix coordinate (icnt, isz).
/// The matrix size is (cnt, sz)
/// Depending of the layout some padding can be calculated
int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout);

/// \brief Split a legacy index into the three SoAoS indices.
///
/// The legacy index is the AoS of the data, i.e. all values are arranged in a
/// table. Different variables and elements of those variables for array
/// variables occupy different columns, while different instances occupy
/// different rows.
///
/// The legacy index is then simply the elements flat (one-dimensional) index
/// in that (unpadded) row-major table.
///
/// Given this `legacy_index` and `array_dims`, i.e. the number of array
/// elements for each variable (1 for regular variables and >= 1 for array
/// variables), compute the triplet `(i, j, k)` where `i` is the index of the
/// instance, `j` of the variable, and `k` the of array element.
std::array<int, 3> legacy2soaos_index(int legacy_index, const std::vector<int>& array_dims);

/// \brief Compute the CoreNEURON index given an SoAoS index.
///
/// The CoreNEURON index is the index in a flat array starting from a pointer
/// pointing to the beginning of the data of that mechanism. Note, that
/// CoreNEURON pads to the number of instances.
///
/// If `permute` is not null, then the instance index `i` is permuted. Before
/// computing before computing the flat index.
///
/// Consecutive array elements are always stored consecutively, next fastest
/// index is over instances, and the slowest index runs over variables. Note,
/// up to padding and permutation of the instances this is the same SoAoS order
/// as in NEURON.
int soaos2cnrn_index(const std::array<int, 3>& soaos_indices,
                     const std::vector<int>& array_dims,
                     int padded_node_count,
                     int* permute);

}  // namespace coreneuron
