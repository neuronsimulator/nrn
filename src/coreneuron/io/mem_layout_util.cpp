/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "mem_layout_util.hpp"

#include <numeric>

namespace coreneuron {

/// calculate size after padding for specific memory layout
// Warning: this function is declared extern in nrniv_decl.h
int nrn_soa_padded_size(int cnt, int layout) {
    return soa_padded_size<NRN_SOA_PAD>(cnt, layout);
}

/// return the new offset considering the byte aligment settings
size_t nrn_soa_byte_align(size_t size) {
    static_assert(NRN_SOA_BYTE_ALIGN % sizeof(double) == 0,
                  "NRN_SOA_BYTE_ALIGN should be a multiple of sizeof(double)");
    constexpr size_t dbl_align = NRN_SOA_BYTE_ALIGN / sizeof(double);
    size_t remainder = size % dbl_align;
    if (remainder) {
        size += dbl_align - remainder;
    }
    nrn_assert((size * sizeof(double)) % NRN_SOA_BYTE_ALIGN == 0);
    return size;
}

int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout) {
    switch (layout) {
    case Layout::AoS:
        return icnt * sz + isz;
    case Layout::SoA:
        int padded_cnt = nrn_soa_padded_size(cnt, layout);
        return icnt + isz * padded_cnt;
    }

    nrn_assert(false);
    return 0;
}

std::array<int, 3> legacy2soaos_index(int legacy_index, const std::vector<int>& array_dims) {
    int variable_count = static_cast<int>(array_dims.size());
    int row_width = std::accumulate(array_dims.begin(), array_dims.end(), 0);

    int column_index = legacy_index % row_width;

    int instance_index = legacy_index / row_width;
    int variable_index = 0;
    int prefix_sum = 0;
    for (size_t k = 0; k < variable_count - 1; ++k) {
        if (column_index >= prefix_sum + array_dims[k]) {
            prefix_sum += array_dims[k];
            variable_index = k + 1;
        } else {
            break;
        }
    }
    int array_index = column_index - prefix_sum;

    return {instance_index, variable_index, array_index};
}

int soaos2cnrn_index(const std::array<int, 3>& soaos_indices,
                     const std::vector<int>& array_dims,
                     int padded_node_count,
                     int* permute) {
    auto [i, j, k] = soaos_indices;
    if (permute) {
        i = permute[i];
    }

    int offset = 0;
    for (int ij = 0; ij < j; ++ij) {
        offset += padded_node_count * array_dims[ij];
    }

    int K = array_dims[j];
    return offset + i * K + k;
}


}  // namespace coreneuron
