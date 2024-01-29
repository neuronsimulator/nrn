/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#define SOA_LAYOUT 0
#define AOS_LAYOUT 1
namespace coreneuron {
struct Memb_list;
int get_data_index(int node_index, int variable_index, int mtype, Memb_list* ml);
}  // namespace coreneuron
