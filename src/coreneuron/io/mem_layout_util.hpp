/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"

namespace coreneuron {

#if !defined(NRN_SOA_PAD)
// for layout 0, every range variable array must have a size which
// is a multiple of NRN_SOA_PAD doubles
#define NRN_SOA_PAD 8
#endif

#if !defined(LAYOUT)
#define LAYOUT        Layout::AoS
#define MATRIX_LAYOUT Layout::AoS
#else
#define MATRIX_LAYOUT LAYOUT
#endif

/// return the new offset considering the byte aligment settings
size_t nrn_soa_byte_align(size_t i);

/// This function return the index in a flat array of a matrix coordinate (icnt, isz).
/// The matrix size is (cnt, sz)
/// Depending of the layout some padding can be calculated
int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout);

// file data is AoS. ie.
// organized as cnt array instances of mtype each of size sz.
// So input index i refers to i_instance*sz + i_item offset
// Return the corresponding SoA index -- taking into account the
// alignment requirements. Ie. i_instance + i_item*align_cnt.

int nrn_param_layout(int i, int mtype, Memb_list* ml);
}  // namespace coreneuron
