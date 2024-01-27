/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <numeric>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <iostream>
#include <unordered_map>

namespace coreneuron {

/** type to store every section and associated segments */
using segvec_type = std::vector<int>;
using secseg_map_type = std::map<int, segvec_type>;
using secseg_it_type = secseg_map_type::iterator;

/** @brief Section to segment mapping
 *
 *  For a section list (of a particulat type), store mapping
 *  of section to segments
 *  a section is a arbitrary user classification to recognize some segments (ex: api, soma, dend,
 * axon)
 *
 */
struct SecMapping {
    /** name of section list */
    std::string name;

    /** map of section and associated segments */
    secseg_map_type secmap;

    SecMapping() = default;

    explicit SecMapping(std::string s)
        : name(std::move(s)) {}

    /** @brief return total number of sections in section list */
    size_t num_sections() const noexcept {
        return secmap.size();
    }

    /** @brief return number of segments in section list */
    size_t num_segments() const {
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
    std::vector<SecMapping*> secmapvec;

    /** map containing segment ids an its respective lfp factors */
    std::unordered_map<int, std::vector<double>> lfp_factors;

    CellMapping(int g)
        : gid(g) {}

    /** @brief total number of sections in a cell */
    int num_sections() const {
        return std::accumulate(secmapvec.begin(),
                               secmapvec.end(),
                               0,
                               [](int psum, const auto& secmap) {
                                   return psum + secmap->num_sections();
                               });
    }

    /** @brief return number of segments in a cell */
    int num_segments() const {
        return std::accumulate(secmapvec.begin(),
                               secmapvec.end(),
                               0,
                               [](int psum, const auto& secmap) {
                                   return psum + secmap->num_segments();
                               });
    }

    /** @brief return the number of electrodes in the lfp_factors map **/
    int num_electrodes() const {
        int num_electrodes = 0;
        if (!lfp_factors.empty()) {
            num_electrodes = lfp_factors.begin()->second.size();
        }
        return num_electrodes;
    }

    /** @brief number of section lists */
    size_t size() const noexcept {
        return secmapvec.size();
    }

    /** @brief add new SecMapping */
    void add_sec_map(SecMapping* s) {
        secmapvec.push_back(s);
    }

    /** @brief return section list mapping with given name */
    SecMapping* get_seclist_mapping(const std::string& name) const {
        for (auto& secmap: secmapvec) {
            if (name == secmap->name) {
                return secmap;
            }
        }

        std::cout << "Warning: Section mapping list " << name << " doesn't exist! \n";
        return nullptr;
    }

    /** @brief return segment count for specific section list with given name */
    size_t get_seclist_segment_count(const std::string& name) const {
        SecMapping* s = get_seclist_mapping(name);
        size_t count = 0;
        if (s) {
            count = s->num_segments();
        }
        return count;
    }
    /** @brief return segment count for specific section list with given name */
    size_t get_seclist_section_count(const std::string& name) const {
        SecMapping* s = get_seclist_mapping(name);
        size_t count = 0;
        if (s) {
            count = s->num_sections();
        }
        return count;
    }

    /** @brief add the lfp electrode factors of a segment_id */
    void add_segment_lfp_factor(const int segment_id, std::vector<double>& factors) {
        lfp_factors.insert({segment_id, factors});
    }

    ~CellMapping() {
        for (size_t i = 0; i < secmapvec.size(); i++) {
            delete secmapvec[i];
        }
    }
};

/** @brief Compartment mapping information for NrnThread
 *
 * NrnThread could have more than one cell in cellgroup
 * and we store this in vector.
 */
struct NrnThreadMappingInfo {
    /** list of cells mapping */
    std::vector<CellMapping*> mappingvec;

    /** list of segment ids */
    std::vector<int> segment_ids;

    std::vector<double> _lfp;

    /** @brief number of cells */
    size_t size() const {
        return mappingvec.size();
    }

    /** @brief memory cleanup */
    ~NrnThreadMappingInfo() {
        for (size_t i = 0; i < mappingvec.size(); i++) {
            delete mappingvec[i];
        }
    }

    /** @brief get cell mapping information for given gid
     *	if exist otherwise return nullptr.
     */
    CellMapping* get_cell_mapping(int gid) const {
        for (const auto& mapping: mappingvec) {
            if (mapping->gid == gid) {
                return mapping;
            }
        }
        return nullptr;
    }

    /** @brief add mapping information of new cell */
    void add_cell_mapping(CellMapping* c) {
        mappingvec.push_back(c);
    }

    /** @brief add a new segment */
    void add_segment_id(const int segment_id) {
        segment_ids.push_back(segment_id);
    }

    /** @brief Resize the lfp vector */
    void prepare_lfp() {
        size_t lfp_size = std::accumulate(mappingvec.begin(),
                                          mappingvec.end(),
                                          0,
                                          [](size_t total, const auto& mapping) {
                                              return total + mapping->num_electrodes();
                                          });
        _lfp.resize(lfp_size);
    }
};
}  // namespace coreneuron
