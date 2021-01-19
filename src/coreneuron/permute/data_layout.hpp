/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#ifndef NRN_DATA_LAYOUT_HPP
#define NRN_DATA_LAYOUT_HPP

#define SOA_LAYOUT 0
#define AOS_LAYOUT 1
namespace coreneuron {
struct Memb_list;
int get_data_index(int node_index, int variable_index, int mtype, Memb_list* ml);
}  // namespace coreneuron
#endif
