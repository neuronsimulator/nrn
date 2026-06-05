/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/utils/utils.hpp"

namespace coreneuron {

/** @brief Section to segment mapping
 *
 *  For a section list (of a particulat type), store mapping
 *  of section to compartments
 *  a section is a arbitrary user classification to recognize some compartments (ex: api, soma,
 * dend, axon)
 *
 */
struct SecMapping {
    /** name of section list */
    SectionType type;

    /** map of section and associated compartments */
    std::unordered_map<int, std::vector<int>> secmap;

    SecMapping() = default;

    explicit SecMapping(SectionType t)
        : type(t) {}

    /** @brief return total number of sections in section list */
    size_t num_sections() const noexcept {
        return secmap.size();
    }

    /** @brief return number of compartments in section list */
    size_t num_compartments() const {
        return std::accumulate(secmap.begin(), secmap.end(), 0, [](int psum, const auto& item) {
            return psum + item.second.size();
        });
    }

    /** @brief add section to associated segment */
    void add_segment(int sec, int seg) {
        secmap[sec].push_back(seg);
    }
};

/** @brief Compartment mapping information for a cell
 *
 * A cell can have multiple section list types like
 * soma, axon, apic, dend etc. User will add these
 * section lists using HOC interface.
 */
struct CellMapping {
    /** gid of a cell */
    int gid;

    /** list of section lists (like soma, axon, apic) */
    std::vector<std::shared_ptr<SecMapping>> sec_mappings;

    /** segment ids for lfp factors (one per compartment with LFP data) */
    std::vector<int> lfp_segment_ids;

    /** flat array of lfp factors: stride = num_electrodes, indexed as [i * stride + e] */
    std::vector<double> lfp_factors_flat;

    CellMapping(int g)
        : gid(g) {}

    /** @brief total number of sections in a cell */
    int num_sections() const {
        return std::accumulate(sec_mappings.begin(),
                               sec_mappings.end(),
                               0,
                               [](int psum, const auto& secmap) {
                                   return psum + secmap->num_sections();
                               });
    }

    /** @brief return number of compartments in a cell */
    int num_compartments() const {
        return std::accumulate(sec_mappings.begin(),
                               sec_mappings.end(),
                               0,
                               [](int psum, const auto& secmap) {
                                   return psum + secmap->num_compartments();
                               });
    }

    /** @brief return the number of electrodes per segment **/
    size_t num_electrodes() const {
        return lfp_segment_ids.empty() ? 0 : lfp_factors_flat.size() / lfp_segment_ids.size();
    }

    /** @brief number of section lists */
    size_t size() const noexcept {
        return sec_mappings.size();
    }

    /** @brief add new SecMapping */
    void add_sec_map(std::shared_ptr<SecMapping> s) {
        sec_mappings.push_back(s);
    }

    /** @brief return section list mapping with given type */
    std::shared_ptr<SecMapping> get_seclist_mapping(const SectionType type) const {
        for (auto& secmap: sec_mappings) {
            if (type == secmap->type) {
                return secmap;
            }
        }

        std::cout << "Warning: Section mapping list " << to_string(type) << " doesn't exist! \n";
        return nullptr;
    }

    /** @brief return compartment count for specific section list with given type */
    size_t get_seclist_compartment_count(const SectionType type) const {
        auto s = get_seclist_mapping(type);
        if (!s) {
            return 0;
        }
        return s->num_compartments();
    }
    /** @brief return segment count for specific section list with given type */
    size_t get_seclist_section_count(const SectionType type) const {
        auto s = get_seclist_mapping(type);
        if (!s) {
            return 0;
        }
        return s->num_sections();
    }

    /** @brief add the lfp electrode factors of a segment_id */
    void add_segment_lfp_factor(const int segment_id, std::vector<double>& factors) {
        const size_t n = factors.size();
        if (n == 0) {
            return;
        }
        const auto curr_n_electrodes = num_electrodes();
        if (curr_n_electrodes > 0 && n != curr_n_electrodes) {
            std::cerr << "[ERROR] LFP factor count mismatch for gid " << gid << ": expected "
                      << curr_n_electrodes << ", got " << n << '\n';
            nrn_abort(1);
        }
        lfp_segment_ids.push_back(segment_id);
        lfp_factors_flat.insert(lfp_factors_flat.end(), factors.begin(), factors.end());
    }
};

/** @brief Compartment mapping information for NrnThread
 *
 * NrnThread could have more than one cell in cellgroup
 * and we store this in vector.
 */
struct NrnThreadMappingInfo {
    /** list of cells mapping */
    std::unordered_map<int, std::shared_ptr<CellMapping>> cell_mappings;

    /** list of segment ids */
    std::vector<int> segment_ids;

    std::vector<double> _lfp;

    /** @brief number of cells */
    size_t size() const {
        return cell_mappings.size();
    }

    /** @brief get cell mapping information for given gid
     *	if exist otherwise return nullptr.
     */
    std::shared_ptr<CellMapping> get_cell_mapping(int gid) const {
        auto it = cell_mappings.find(gid);
        if (it != cell_mappings.end()) {
            return it->second;
        }
        return nullptr;
    }

    /** @brief add mapping information of new cell */
    void add_cell_mapping(std::shared_ptr<CellMapping> c) {
        auto [it, inserted] = cell_mappings.insert({c->gid, c});
        if (!inserted) {
            std::cerr << "CellMapping for gid " << std::to_string(c->gid) << " already exists!\n";
            nrn_abort(1);
        }
    }

    /** @brief add a new segment */
    void add_segment_id(const int segment_id) {
        segment_ids.push_back(segment_id);
    }

    /** @brief Resize the lfp vector */
    void prepare_lfp() {
        size_t lfp_size = std::accumulate(cell_mappings.begin(),
                                          cell_mappings.end(),
                                          0,
                                          [](size_t total, const auto& mapping) {
                                              return total + mapping.second->num_electrodes();
                                          });
        _lfp.resize(lfp_size);
    }
};
}  // namespace coreneuron
