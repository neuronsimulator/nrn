/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <sstream>
#include "report_handler.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/utils/utils.hpp"

namespace coreneuron {

SectionType check_section_type(SectionType st) {
    if (st == SectionType::All)
        return SectionType::All;
    if (st == SectionType::Cell)
        return SectionType::Soma;
    if (st == SectionType::Soma)
        return SectionType::Soma;
    if (st == SectionType::All)
        return SectionType::Axon;
    if (st == SectionType::Dendrite)
        return SectionType::Dendrite;
    if (st == SectionType::Apical)
        return SectionType::Apical;

    std::cerr << "[Error] Invalid SectionType enum value: " << to_string(st) << "\n";
    nrn_abort(1);
}

template <typename T>
std::vector<T> intersection_gids(const NrnThread& nt, std::vector<T>& target_gids) {
    std::vector<int> thread_gids;
    for (int i = 0; i < nt.ncell; i++) {
        thread_gids.push_back(nt.presyns[i].gid_);
    }
    std::vector<T> intersection;

    std::sort(thread_gids.begin(), thread_gids.end());
    std::sort(target_gids.begin(), target_gids.end());

    std::set_intersection(thread_gids.begin(),
                          thread_gids.end(),
                          target_gids.begin(),
                          target_gids.end(),
                          back_inserter(intersection));

    return intersection;
}

void ReportHandler::create_report(ReportConfiguration& report_config,
                                  double dt,
                                  double tstop,
                                  double delay) {
#ifdef ENABLE_SONATA_REPORTS
    if (report_config.start < t) {
        report_config.start = t;
    }
    report_config.stop = std::min(report_config.stop, tstop);

    for (const auto& mech: report_config.mech_names) {
        report_config.mech_ids.emplace_back(nrn_get_mechtype(mech.data()));
    }
    if (report_config.type == ReportType::Synapse && report_config.mech_ids.empty()) {
        std::cerr << "[ERROR] mechanism to report: " << report_config.mech_names[0]
                  << " is not mapped in this simulation, cannot report on it \n";
        nrn_abort(1);
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];

        if (!nt.ncell) {
            continue;
        }
        auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
        const std::vector<int>& nodes_to_gid = map_gids(nt);


        const std::vector<int> gids_to_report = intersection_gids(nt, report_config.target);
        VarsToReport vars_to_report;
        const bool is_soma_target = report_config.section_type == SectionType::Soma ||
                                    report_config.section_type == SectionType::Cell;
        switch (report_config.type) {
        case ReportType::Compartment: {
            const auto& mech_name = report_config.mech_names[0];
            double* report_variable;
            if (mech_name == "v") {
                report_variable = nt._actual_v;
            } else if (mech_name == "i_membrane") {
                report_variable = nt.nrn_fast_imem->nrn_sav_rhs;
            } else {
                std::cerr << "The variable name '" << mech_name
                          << "' is not currently supported by compartment reports.\n";
                nrn_abort(1);
            }
            vars_to_report = get_section_vars_to_report(nt,
                                                        gids_to_report,
                                                        report_variable,
                                                        report_config.section_type,
                                                        report_config.section_all_compartments);
            register_section_report(nt, report_config, vars_to_report, is_soma_target);
            break;
        }
        case ReportType::Summation: {
            vars_to_report =
                get_summation_vars_to_report(nt, gids_to_report, report_config, nodes_to_gid);
            register_custom_report(nt, report_config, vars_to_report);
            break;
        }
        case ReportType::LFP: {
            mapinfo->prepare_lfp();
            vars_to_report = get_lfp_vars_to_report(
                nt, gids_to_report, report_config, mapinfo->_lfp.data(), nodes_to_gid);
            register_section_report(nt, report_config, vars_to_report, is_soma_target);
            break;
        }
        default: {
            vars_to_report =
                get_synapse_vars_to_report(nt, gids_to_report, report_config, nodes_to_gid);
            register_custom_report(nt, report_config, vars_to_report);
        }
        }
        if (!vars_to_report.empty()) {
            auto report_event = std::make_unique<ReportEvent>(dt,
                                                              t,
                                                              vars_to_report,
                                                              report_config.output_path.data(),
                                                              report_config.report_dt,
                                                              report_config.type);
            report_event->send(t, net_cvode_instance, &nt);
            m_report_events.push_back(std::move(report_event));
        }
    }
#else
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Reporting is disabled. Please recompile with libsonata.\n";
    }
#endif
}

#ifdef ENABLE_SONATA_REPORTS
void ReportHandler::register_section_report(const NrnThread& nt,
                                            const ReportConfiguration& config,
                                            const VarsToReport& vars_to_report,
                                            bool is_soma_target) {
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Format '" << config.format << "' in report '"
                  << config.output_path << "' not supported.\n";
    }
}
void ReportHandler::register_custom_report(const NrnThread& nt,
                                           const ReportConfiguration& config,
                                           const VarsToReport& vars_to_report) {
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Format '" << config.format << "' in report '"
                  << config.output_path << "' not supported.\n";
    }
}

// fill to_report with (int section_id, double* variable)
void append_sections_to_to_report(const std::shared_ptr<SecMapping>& sections,
                                  std::vector<VarWithMapping>& to_report,
                                  double* report_variable,
                                  bool all_compartments) {
    for (const auto& section: sections->secmap) {
        // compartment_id
        int section_id = section.first;
        const auto& segment_ids = section.second;

        // get all compartment values (otherwise, just middle point)
        if (all_compartments) {
            for (const auto& segment_id: segment_ids) {
                // corresponding voltage in coreneuron voltage array
                double* variable = report_variable + segment_id;
                to_report.emplace_back(VarWithMapping(section_id, variable));
            }
        } else {
            nrn_assert(segment_ids.size() % 2);
            // corresponding voltage in coreneuron voltage array
            const auto segment_id = segment_ids[segment_ids.size() / 2];
            double* variable = report_variable + segment_id;
            to_report.emplace_back(VarWithMapping(section_id, variable));
        }
    }
}

VarsToReport ReportHandler::get_section_vars_to_report(const NrnThread& nt,
                                                       const std::vector<int>& gids_to_report,
                                                       double* report_variable,
                                                       SectionType section_type,
                                                       bool all_compartments) const {
    VarsToReport vars_to_report;
    section_type = check_section_type(section_type);
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[COMPARTMENTS] Error : mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (const auto& gid: gids_to_report) {
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr
                << "[COMPARTMENTS] Error : Compartment mapping information is missing for gid "
                << gid << '\n';
            nrn_abort(1);
        }
        std::vector<VarWithMapping> to_report;
        to_report.reserve(cell_mapping->size());

        if (section_type == SectionType::All) {
            const auto& section_mapping = cell_mapping->sec_mappings;
            for (const auto& sections: section_mapping) {
                append_sections_to_to_report(sections,
                                             to_report,
                                             report_variable,
                                             all_compartments);
            }
        } else {
            /** get section list mapping for the type, if available */
            if (cell_mapping->get_seclist_section_count(section_type) > 0) {
                const auto& sections = cell_mapping->get_seclist_mapping(section_type);
                append_sections_to_to_report(sections,
                                             to_report,
                                             report_variable,
                                             all_compartments);
            }
        }
        to_report.shrink_to_fit();
        vars_to_report[gid] = to_report;
    }
    return vars_to_report;
}

/**
 * @brief Retrieves a pointer to a variable associated with a mechanism or state variable.
 *
 * This function returns a pointer to the memory location of a variable specified by
 * its mechanism name and variable name for a given segment in the neuron thread.
 *
 * Special cases:
 * - If `mech_name` is "i_membrane", returns the pointer to the membrane current (`nrn_sav_rhs`)
 *   at the specified `segment_id`.
 * - If `mech_name` is "v", returns the pointer to the membrane voltage (`_actual_v`)
 *   at the specified `segment_id`.
 *
 * For other mechanisms:
 * - Retrieves the `Memb_list` corresponding to `mech_id` from the neuron thread.
 * - Uses `segment_id_2_node_id` as a cache mapping segment IDs to node indices.
 *   If the cache is empty, it populates it by iterating over all nodes in `ml`.
 * - Returns the pointer to the variable obtained via `get_var_location_from_var_name`.
 *
 * @param nt Reference to the NrnThread containing mechanism data.
 * @param mech_id Mechanism type ID.
 * @param var_name Name of the variable to retrieve within the mechanism.
 * @param mech_name Name of the mechanism (e.g., "i_membrane", "v", or other mechanisms).
 * @param segment_id ID of the segment for which to retrieve the variable.
 * @param segment_id_2_node_id Cache mapping segment IDs to node indices; populated if empty.
 * @return Pointer to the requested variable's memory location, or nullptr if mechanism not found.
 */
double* get_var(const NrnThread& nt,
                const int mech_id,
                const std::string& var_name,
                const std::string& mech_name,
                const int segment_id,
                std::unordered_map<int, int>& segment_id_2_node_id) {
    if (mech_name == "i_membrane") {
        return nt.nrn_fast_imem->nrn_sav_rhs + segment_id;
    }
    if (mech_name == "v") {
        return nt._actual_v + segment_id;
    }

    Memb_list* ml = nt._ml_list[mech_id];
    if (!ml) {
        return nullptr;
    }

    // lazy cache
    if (segment_id_2_node_id.empty()) {
        for (int j = 0; j < ml->nodecount; j++) {
            const int segment_id = ml->nodeindices[j];

            auto result = segment_id_2_node_id.insert({segment_id, j});
            nrn_assert(result.second && "Duplicate segment_id detected");
        }
    }

    const int node_id = segment_id_2_node_id[segment_id];
    return get_var_location_from_var_name(mech_id, mech_name, var_name, ml, node_id);
}

double get_scaling_factor(const std::string& mech_name,
                          Scaling scaling,
                          const NrnThread& nt,
                          const int segment_id) {
    // Scaling factors for specific mechanisms
    if (mech_name == "IClamp" || mech_name == "SEClamp") {
        return -1.0;  // Invert current for these mechanisms
    }

    if (mech_name != "i_membrane" && scaling == Scaling::Area) {
        return (*(nt._actual_area + segment_id)) / 100.0;
    }

    return 1.0;  // Default scaling factor
}

VarsToReport ReportHandler::get_summation_vars_to_report(
    const NrnThread& nt,
    const std::vector<int>& gids_to_report,
    const ReportConfiguration& report,
    const std::vector<int>& nodes_to_gids) const {
    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    auto& summation_report = nt.summation_report_handler_->summation_reports_[report.output_path];
    if (!mapinfo) {
        std::cerr << "[COMPARTMENTS] Error : mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (const auto& gid: gids_to_report) {
        {
            const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
            if (cell_mapping == nullptr) {
                std::cerr
                    << "[SUMMATION] Error : Compartment mapping information is missing for gid "
                    << gid << '\n';
                nrn_abort(1);
            }
            // In case we need convertion of units
            for (auto i = 0; i < report.mech_ids.size(); ++i) {
                const auto& mech_id = report.mech_ids[i];
                const auto& var_name = report.var_names[i];
                const auto& mech_name = report.mech_names[i];
                std::unordered_map<int, int> segment_id_2_node_id;

                const auto& section_mapping = cell_mapping->sec_mappings;
                for (const auto& sections: section_mapping) {
                    for (auto& section: sections->secmap) {
                        int section_id = section.first;
                        auto& segment_ids = section.second;
                        for (const auto& segment_id: segment_ids) {
                            double* var_ptr = get_var(
                                nt, mech_id, var_name, mech_name, segment_id, segment_id_2_node_id);
                            if (var_ptr != nullptr) {
                                double scaling_factor =
                                    get_scaling_factor(mech_name, report.scaling, nt, segment_id);

                                if (scaling_factor != 0.0) {
                                    summation_report.currents_[segment_id].push_back(
                                        std::make_pair(var_ptr, scaling_factor));
                                }
                            }
                        }
                    }
                }
            }
        }

        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr << "[SUMMATION] Error : Compartment mapping information is missing for gid "
                      << gid << '\n';
            nrn_abort(1);
        }

        std::vector<VarWithMapping> to_report;
        to_report.reserve(cell_mapping->size());
        summation_report.summation_.resize(nt.end);
        double* report_variable = summation_report.summation_.data();
        SectionType section_type = check_section_type(report.section_type);
        if (section_type != SectionType::All) {
            if (cell_mapping->get_seclist_section_count(section_type) > 0) {
                const auto& sections = cell_mapping->get_seclist_mapping(section_type);
                append_sections_to_to_report(sections,
                                             to_report,
                                             report_variable,
                                             report.section_all_compartments);
            }
        }

        const auto& section_mapping = cell_mapping->sec_mappings;
        for (const auto& sections: section_mapping) {
            for (auto& section: sections->secmap) {
                // compartment_id
                int section_id = section.first;
                auto& segment_ids = section.second;
                for (const auto& segment_id: segment_ids) {
                    if (report.section_type == SectionType::All) {
                        double* variable = report_variable + segment_id;
                        to_report.emplace_back(VarWithMapping(section_id, variable));
                    } else if (report.section_type == SectionType::Cell ||
                               report.section_type == SectionType::Soma) {
                        summation_report.gid_segments_[gid].push_back(segment_id);
                    }
                }
            }
        }
        vars_to_report[gid] = to_report;
    }
    return vars_to_report;
}

VarsToReport ReportHandler::get_synapse_vars_to_report(
    const NrnThread& nt,
    const std::vector<int>& gids_to_report,
    const ReportConfiguration& report,
    const std::vector<int>& nodes_to_gids) const {
    VarsToReport vars_to_report;
    for (const auto& gid: gids_to_report) {
        // There can only be 1 mechanism
        nrn_assert(report.mech_ids.size() == 1);
        const auto& mech_id = report.mech_ids[0];
        const auto& mech_name = report.mech_names[0];
        const auto& var_name = report.var_names[0];
        Memb_list* ml = nt._ml_list[mech_id];
        if (!ml) {
            continue;
        }
        std::vector<VarWithMapping> to_report;
        to_report.reserve(ml->nodecount);

        for (int j = 0; j < ml->nodecount; j++) {
            double* is_selected =
                get_var_location_from_var_name(mech_id, mech_name, SELECTED_VAR_MOD_NAME, ml, j);
            bool report_variable = false;

            /// if there is no variable in mod file then report on every compartment
            /// otherwise check the flag set in mod file
            if (is_selected == nullptr) {
                report_variable = true;
            } else {
                report_variable = *is_selected != 0.;
            }
            if ((nodes_to_gids[ml->nodeindices[j]] == gid) && report_variable) {
                double* var_value =
                    get_var_location_from_var_name(mech_id, mech_name, var_name, ml, j);
                double* synapse_id =
                    get_var_location_from_var_name(mech_id, mech_name, SYNAPSE_ID_MOD_NAME, ml, j);
                nrn_assert(synapse_id && var_value);
                to_report.emplace_back(static_cast<int>(*synapse_id), var_value);
            }
        }
        if (!to_report.empty()) {
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

VarsToReport ReportHandler::get_lfp_vars_to_report(const NrnThread& nt,
                                                   const std::vector<int>& gids_to_report,
                                                   ReportConfiguration& report,
                                                   double* report_variable,
                                                   const std::vector<int>& nodes_to_gids) const {
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[LFP] Error : mapping information is missing for a Cell group " << nt.ncell
                  << '\n';
        nrn_abort(1);
    }
    auto& summation_report = nt.summation_report_handler_->summation_reports_[report.output_path];
    VarsToReport vars_to_report;
    std::size_t offset_lfp = 0;
    for (const auto& gid: gids_to_report) {
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr << "[LFP] Error : Compartment mapping information is missing for gid " << gid
                      << '\n';
            nrn_abort(1);
        }
        std::vector<VarWithMapping> to_report;
        int num_electrodes = cell_mapping->num_electrodes();
        for (int electrode_id = 0; electrode_id < num_electrodes; electrode_id++) {
            to_report.emplace_back(VarWithMapping(electrode_id, report_variable + offset_lfp));
            offset_lfp++;
        }
        if (!to_report.empty()) {
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

// map GIDs of every compartment, it consist in a backward sweep then forward sweep algorithm
std::vector<int> ReportHandler::map_gids(const NrnThread& nt) const {
    std::vector<int> nodes_gid(nt.end, -1);
    // backward sweep: from presyn compartment propagate back GID to parent
    for (int i = 0; i < nt.n_presyn; i++) {
        const int gid = nt.presyns[i].gid_;
        const int thvar_index = nt.presyns[i].thvar_index_;
        // only for non artificial cells
        if (thvar_index >= 0) {
            // setting all roots gids of the presyns nodes,
            // index 0 have parent set to 0, so we must stop at j > 0
            // also 0 is the parent of all, so it is an error to attribute a GID to it.
            nodes_gid[thvar_index] = gid;
            for (int j = thvar_index; j > 0; j = nt._v_parent_index[j]) {
                nodes_gid[nt._v_parent_index[j]] = gid;
            }
        }
    }
    // forward sweep: setting all compartements nodes to the GID of its root
    //  already sets on above loop. This is working only because compartments are stored in order
    //  parents followed by children
    for (int i = nt.ncell + 1; i < nt.end; i++) {
        nodes_gid[i] = nodes_gid[nt._v_parent_index[i]];
    }
    return nodes_gid;
}
#endif

}  // Namespace coreneuron
