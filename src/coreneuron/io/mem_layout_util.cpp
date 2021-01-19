/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "mem_layout_util.hpp"

namespace coreneuron {

/// calculate size after padding for specific memory layout
// Warning: this function is declared extern in nrniv_decl.h
int nrn_soa_padded_size(int cnt, int layout) {
    return soa_padded_size<NRN_SOA_PAD>(cnt, layout);
}

/// return the new offset considering the byte aligment settings
size_t nrn_soa_byte_align(size_t size) {
    if (LAYOUT == Layout::SoA) {
        size_t dbl_align = NRN_SOA_BYTE_ALIGN / sizeof(double);
        size_t remainder = size % dbl_align;
        if (remainder) {
            size += dbl_align - remainder;
        }
        nrn_assert((size * sizeof(double)) % NRN_SOA_BYTE_ALIGN == 0);
    }
    return size;
}

int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout) {
    switch (layout) {
        case Layout::AoS:
            return icnt * sz + isz;
        case Layout::SoA:
            int padded_cnt = nrn_soa_padded_size(cnt,
                                                 layout);  // may want to factor out to save time
            return icnt + isz * padded_cnt;
    }

    nrn_assert(false);
    return 0;
}

// file data is AoS. ie.
// organized as cnt array instances of mtype each of size sz.
// So input index i refers to i_instance*sz + i_item offset
// Return the corresponding SoA index -- taking into account the
// alignment requirements. Ie. i_instance + i_item*align_cnt.

int nrn_param_layout(int i, int mtype, Memb_list* ml) {
    int layout = corenrn.get_mech_data_layout()[mtype];
    switch (layout) {
        case Layout::AoS:
            return i;
        case Layout::SoA:
            nrn_assert(layout == Layout::SoA);
            int sz = corenrn.get_prop_param_size()[mtype];
            return nrn_i_layout(i / sz, ml->nodecount, i % sz, sz, layout);
    }
    nrn_assert(false);
    return 0;
}
}  // namespace coreneuron
