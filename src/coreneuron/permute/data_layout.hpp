/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

namespace coreneuron {
struct Memb_list;
int get_data_index(int node_index, int variable_index, int mtype, Memb_list* ml);
}  // namespace coreneuron
