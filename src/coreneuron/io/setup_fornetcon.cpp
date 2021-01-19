/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/io/setup_fornetcon.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include <map>

namespace coreneuron {

/**
   If FOR_NETCON in use, setup NrnThread fornetcon related info.

   i.e NrnThread._fornetcon_perm_indices, NrnThread._fornetcon_weight_perm,
   and the relevant dparam element of each mechanism instance that uses
   a FOR_NETCONS statement.

   Makes use of nrn_fornetcon_cnt_, nrn_fornetcon_type_,
   and nrn_fornetcon_index_ that were specified during registration of
   mechanisms that use FOR_NETCONS.

   nrn_fornetcon_cnt_ is the number of mechanisms that use FOR_NETCONS,
   nrn_fornetcon_type_ is an int array of size nrn_fornetcon_cnt, that specifies
   the mechanism type.
   nrn_fornetcon_index_ is an int array of size nrn_fornetcon_cnt, that
   specifies the index into an instance's dparam int array having the
   fornetcon semantics.

   FOR_NETCONS (args) means to loop over all NetCon connecting to this
   target instance and args are the names of the items of each NetCon's
   weight vector (same as the enclosing NET_RECEIVE but possible different
   local names).

   NrnThread._weights is a vector of weight groups where the number of groups
   is the number of NetCon in this thread and each group has a size
   equal to the number of args in the target NET_RECEIVE block. The order
   of these groups is the NetCon Object order in HOC (the construction order).
   So the weight vector indices for the NetCons in the FOR_NETCONS loop
   are not adjacent.

   NrnThread._fornetcon_weight_perm is an index vector into the
   NrnThread._weight vector such that the list of indices that targets a
   mechanism instance are adjacent.
   NrnThread._fornetcon_perm_indices is an index vector into the
   NrnThread._fornetcon_weight_perm to the first of the list of NetCon weights
   that target the instance. The index of _fornetcon_perm_indices
   containing this first in the list is stored in the mechanism instances
   dparam at the dparam's semantic fornetcon slot. (Note that the next index
   points to the first index of the next target instance.)

**/

static int* fornetcon_slot(const int mtype,
                           const int instance,
                           const int fnslot,
                           const NrnThread& nt) {
    int layout = corenrn.get_mech_data_layout()[mtype];
    int sz = corenrn.get_prop_dparam_size()[mtype];
    Memb_list* ml = nt._ml_list[mtype];
    int* fn = nullptr;
    if (layout == 1) { /* AoS */
        fn = ml->pdata + (instance * sz + fnslot);
    } else if (layout == 0) { /* SoA */
        int padded_cnt = nrn_soa_padded_size(ml->nodecount, layout);
        fn = ml->pdata + (fnslot * padded_cnt + instance);
    }
    return fn;
}

void setup_fornetcon_info(NrnThread& nt) {
    if (nrn_fornetcon_cnt_ == 0) {
        return;
    }

    // Mechanism types in use that have FOR_NETCONS statements
    // Nice to have the dparam fornetcon slot as well so use map
    // instead of set
    std::map<int, int> type_to_slot;
    for (int i = 0; i < nrn_fornetcon_cnt_; ++i) {
        int type = nrn_fornetcon_type_[i];
        Memb_list* ml = nt._ml_list[type];
        if (ml && ml->nodecount) {
            type_to_slot[type] = nrn_fornetcon_index_[i];
        }
    }
    if (type_to_slot.empty()) {
        return;
    }

    // How many NetCons (weight groups) are involved.
    // Also count how many weight groups for each target instance.
    // For the latter we can count in the dparam fornetcon slot.

    // zero the dparam fornetcon slot for counting and count number of slots.
    size_t n_perm_indices = 0;
    for (const auto& kv: type_to_slot) {
        int mtype = kv.first;
        int fnslot = kv.second;
        int nodecount = nt._ml_list[mtype]->nodecount;
        for (int i = 0; i < nodecount; ++i) {
            int* fn = fornetcon_slot(mtype, i, fnslot, nt);
            *fn = 0;
            n_perm_indices += 1;
        }
    }

    // Count how many weight groups for each slot and total number of weight groups
    size_t n_weight_perm = 0;
    for (int i = 0; i < nt.n_netcon; ++i) {
        NetCon& nc = nt.netcons[i];
        int mtype = nc.target_->_type;
        auto search = type_to_slot.find(mtype);
        if (search != type_to_slot.end()) {
            int i_instance = nc.target_->_i_instance;
            int* fn = fornetcon_slot(mtype, i_instance, search->second, nt);
            *fn += 1;
            n_weight_perm += 1;
        }
    }

    // Displacement vector has an extra element since the number for last item
    // at n-1 is x[n] - x[n-1] and number for first is x[0] = 0.
    nt._fornetcon_perm_indices.resize(n_perm_indices + 1);
    nt._fornetcon_weight_perm.resize(n_weight_perm);

    // From dparam fornetcon slots, compute displacement vector, and
    // set the dparam fornetcon slot to the index of the displacement vector
    // to allow later filling the _fornetcon_weight_perm.
    size_t i_perm_indices = 0;
    nt._fornetcon_perm_indices[0] = 0;
    for (const auto& kv: type_to_slot) {
        int mtype = kv.first;
        int fnslot = kv.second;
        int nodecount = nt._ml_list[mtype]->nodecount;
        for (int i = 0; i < nodecount; ++i) {
            int* fn = fornetcon_slot(mtype, i, fnslot, nt);
            nt._fornetcon_perm_indices[i_perm_indices + 1] =
                nt._fornetcon_perm_indices[i_perm_indices] + size_t(*fn);
            *fn = int(nt._fornetcon_perm_indices[i_perm_indices]);
            i_perm_indices += 1;
        }
    }

    // One more iteration over NetCon to fill in weight index for
    // nt._fornetcon_weight_perm. To help with this we increment the
    // dparam fornetcon slot on each use.
    for (int i = 0; i < nt.n_netcon; ++i) {
        NetCon& nc = nt.netcons[i];
        int mtype = nc.target_->_type;
        auto search = type_to_slot.find(mtype);
        if (search != type_to_slot.end()) {
            int i_instance = nc.target_->_i_instance;
            int* fn = fornetcon_slot(mtype, i_instance, search->second, nt);
            size_t nc_w_index = size_t(nc.u.weight_index_);
            nt._fornetcon_weight_perm[size_t(*fn)] = nc_w_index;
            *fn += 1;  // next item conceptually adjacent
        }
    }

    // Put back the proper values into the dparam fornetcon slot
    i_perm_indices = 0;
    for (const auto& kv: type_to_slot) {
        int mtype = kv.first;
        int fnslot = kv.second;
        int nodecount = nt._ml_list[mtype]->nodecount;
        for (int i = 0; i < nodecount; ++i) {
            int* fn = fornetcon_slot(mtype, i, fnslot, nt);
            *fn = int(i_perm_indices);
            i_perm_indices += 1;
        }
    }
}

}  // namespace coreneuron
