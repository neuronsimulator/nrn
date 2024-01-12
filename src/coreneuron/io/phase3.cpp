/*
# =============================================================================
# Copyright (c) 2016 - 2024 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#include <algorithm>

#include "coreneuron/io/phase3.hpp"

void (*nrn2core_get_dat3_cell_count)(int& cell_count);
void (*nrn2core_get_dat3_cellmapping)(int i, int& gid, int& nsec, int& nseg, int& n_seclist);
void (*nrn2core_get_dat3_secmapping)(int i_c,
                                     int i_sec,
                                     std::string& segname,
                                     int& nsec,
                                     int& nseg,
                                     size_t& total_lfp_factors,
                                     int& n_electrode,
                                     std::vector<int>& data_sec,
                                     std::vector<int>& data_seg,
                                     std::vector<double>& data_lfp);

namespace coreneuron {
void Phase3::read_file(FileHandler& F, NrnThreadMappingInfo* ntmapping) {
    int count = 0;
    F.read_mapping_cell_count(&count);
    /** for every neuron */
    for (int i = 0; i < count; i++) {
        int gid, nsec, nseg, nseclist;
        // read counts
        F.read_mapping_count(&gid, &nsec, &nseg, &nseclist);
        CellMapping* cmap = new CellMapping(gid);
        // read section-segment mapping for every section list
        for (int j = 0; j < nseclist; j++) {
            SecMapping* smap = new SecMapping();
            F.read_mapping_info(smap, ntmapping, cmap);
            cmap->add_sec_map(smap);
        }
        ntmapping->add_cell_mapping(cmap);
    }
}

void Phase3::read_direct(NrnThreadMappingInfo* ntmapping) {
    int count;
    nrn2core_get_dat3_cell_count(count);
    /** for every neuron */
    for (int i = 0; i < count; i++) {
        int gid;
        int t_sec;
        int t_seg;
        int nseclist;
        nrn2core_get_dat3_cellmapping(i, gid, t_sec, t_seg, nseclist);
        auto cmap = new CellMapping(gid);
        for (int j = 0; j < nseclist; j++) {
            std::string sclname;
            int n_sec;
            int n_seg;
            int n_electrodes;
            size_t total_lfp_factors;
            std::vector<int> data_sec;
            std::vector<int> data_seg;
            std::vector<double> data_lfp;
            nrn2core_get_dat3_secmapping(i,
                                         j,
                                         sclname,
                                         n_sec,
                                         n_seg,
                                         total_lfp_factors,
                                         n_electrodes,
                                         data_sec,
                                         data_seg,
                                         data_lfp);
            auto smap = new SecMapping();
            smap->name = sclname;
            for (int i_seg = 0; i_seg < n_seg; i_seg++) {
                smap->add_segment(data_sec[i_seg], data_seg[i_seg]);
                ntmapping->add_segment_id(data_seg[i_seg]);
                int factor_offset = i_seg * n_electrodes;
                if (total_lfp_factors > 0) {
                    // Abort if the factors contains a NaN
                    nrn_assert(count_if(data_lfp.begin(), data_lfp.end(), [](double d) {
                                   return std::isnan(d);
                               }) == 0);
                    std::vector<double> segment_factors(data_lfp.begin() + factor_offset,
                                                        data_lfp.begin() + factor_offset +
                                                            n_electrodes);
                    cmap->add_segment_lfp_factor(data_seg[i], segment_factors);
                }
            }
            cmap->add_sec_map(smap);
        }
        ntmapping->add_cell_mapping(cmap);
    }
}

}  // namespace coreneuron
