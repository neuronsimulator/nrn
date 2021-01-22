/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "report_handler.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"

namespace coreneuron {

void ReportHandler::create_report(double dt, double tstop, double delay) {
#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
    if (m_report_config.start < t) {
        m_report_config.start = t;
    }
    m_report_config.stop = std::min(m_report_config.stop, tstop);

    m_report_config.mech_id = nrn_get_mechtype(m_report_config.mech_name.data());
    if (m_report_config.type == SynapseReport && m_report_config.mech_id == -1) {
        std::cerr << "[ERROR] mechanism to report: " << m_report_config.mech_name
                  << " is not mapped in this simulation, cannot report on it \n";
        nrn_abort(1);
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        if (!nt.ncell) {
            continue;
        }
        const std::vector<int>& nodes_to_gid = map_gids(nt);
        VarsToReport vars_to_report;
        switch (m_report_config.type) {
            case SomaReport:
                vars_to_report = get_soma_vars_to_report(nt, m_report_config.target, nt._actual_v);
                register_soma_report(nt, m_report_config, vars_to_report);
                break;
            case CompartmentReport:
                vars_to_report =
                    get_compartment_vars_to_report(nt, m_report_config.target, nt._actual_v);
                register_compartment_report(nt, m_report_config, vars_to_report);
                break;
            case IMembraneReport:
                vars_to_report = get_compartment_vars_to_report(nt,
                                                                m_report_config.target,
                                                                nt.nrn_fast_imem->nrn_sav_rhs);
                register_compartment_report(nt, m_report_config, vars_to_report);
                break;
            case SectionReport:
                vars_to_report =
                    get_section_vars_to_report(nt,
                                               m_report_config.target,
                                               nt._actual_v,
                                               m_report_config.section_type,
                                               m_report_config.section_all_compartments);
                register_compartment_report(nt, m_report_config, vars_to_report);
                break;
            default:
                vars_to_report = get_custom_vars_to_report(nt, m_report_config, nodes_to_gid);
                register_custom_report(nt, m_report_config, vars_to_report);
        }
        if (!vars_to_report.empty()) {
            auto report_event = std::make_unique<ReportEvent>(dt,
                                                              t,
                                                              vars_to_report,
                                                              m_report_config.output_path.data());
            report_event->send(t, net_cvode_instance, &nt);
            m_report_events.push_back(std::move(report_event));
        }
    }
#else
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Reporting is disabled. Please recompile with either libsonata or "
                     "reportinglib. \n";
    }
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
}

#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
void ReportHandler::register_soma_report(const NrnThread& nt,
                                         ReportConfiguration& config,
                                         const VarsToReport& vars_to_report) {
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Format '" << config.format << "' in report '"
                  << config.output_path << "' not supported.\n";
    }
}
void ReportHandler::register_compartment_report(const NrnThread& nt,
                                                ReportConfiguration& config,
                                                const VarsToReport& vars_to_report) {
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Format '" << config.format << "' in report '"
                  << config.output_path << "' not supported.\n";
    }
}
void ReportHandler::register_custom_report(const NrnThread& nt,
                                           ReportConfiguration& config,
                                           const VarsToReport& vars_to_report) {
    if (nrnmpi_myid == 0) {
        std::cerr << "[WARNING] : Format '" << config.format << "' in report '"
                  << config.output_path << "' not supported.\n";
    }
}
VarsToReport ReportHandler::get_soma_vars_to_report(const NrnThread& nt,
                                                    const std::set<int>& target,
                                                    double* report_variable) const {
    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[SOMA] Error : mapping information is missing for a Cell group " << nt.ncell
                  << "\n";
        nrn_abort(1);
    }

    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        // only one element for each gid in this case
        std::vector<VarWithMapping> to_report;
        if (target.find(gid) != target.end()) {
            const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
            if (cell_mapping == nullptr) {
                std::cerr << "[SOMA] Error : Soma mapping information is missing for gid " << gid
                          << '\n';
                nrn_abort(1);
            }
            /** get  section list mapping for soma */
            const auto& section = cell_mapping->get_seclist_mapping("soma");
            /** 1st key is section-id and 1st value is segment of soma */
            int section_id = section->secmap.begin()->first;
            const auto& segment_ids = section->secmap.begin()->second;
            nrn_assert(segment_ids.size() % 2);
            const auto segment_id = segment_ids[segment_ids.size() / 2];
            double* variable = report_variable + segment_id;
            to_report.emplace_back(VarWithMapping(section_id, variable));
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

VarsToReport ReportHandler::get_compartment_vars_to_report(const NrnThread& nt,
                                                           const std::set<int>& target,
                                                           double* report_variable) const {
    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[COMPARTMENTS] Error : mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (target.find(gid) != target.end()) {
            const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
            if (cell_mapping == nullptr) {
                std::cerr
                    << "[COMPARTMENTS] Error : Compartment mapping information is missing for gid "
                    << gid << '\n';
                nrn_abort(1);
            }
            std::vector<VarWithMapping> to_report;
            to_report.reserve(cell_mapping->size());
            const auto& section_mapping = cell_mapping->secmapvec;
            for (const auto& sections: section_mapping) {
                for (auto& section: sections->secmap) {
                    // compartment_id
                    int section_id = section.first;
                    auto& segment_ids = section.second;
                    for (const auto& segment_id: segment_ids) {
                        /** corresponding voltage in coreneuron voltage array */
                        double* variable = report_variable + segment_id;
                        to_report.emplace_back(VarWithMapping(section_id, variable));
                    }
                }
            }
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

std::string getSectionTypeStr(SectionType type) {
    switch (type) {
        case Axon:
            return "axon";
        case Dendrite:
            return "dend";
        case Apical:
            return "apic";
        default:
            std::cerr << "SectionType not handled in getSectionTypeStr" << std::endl;
            nrn_abort(1);
    }
}

VarsToReport ReportHandler::get_section_vars_to_report(const NrnThread& nt,
                                                       const std::set<int>& target,
                                                       double* report_variable,
                                                       SectionType section_type,
                                                       bool all_compartments) const {
    VarsToReport vars_to_report;
    const auto& section_type_str = getSectionTypeStr(section_type);
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[COMPARTMENTS] Error : mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (target.find(gid) != target.end()) {
            const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
            if (cell_mapping == nullptr) {
                std::cerr
                    << "[COMPARTMENTS] Error : Compartment mapping information is missing for gid "
                    << gid << '\n';
                nrn_abort(1);
            }
            std::vector<VarWithMapping> to_report;
            to_report.reserve(cell_mapping->size());

            /** get section list mapping for the type, if available */
            if (cell_mapping->get_seclist_section_count(section_type_str) > 0) {
                const auto& sections = cell_mapping->get_seclist_mapping(section_type_str);
                for (const auto& section: sections->secmap) {
                    // compartment_id
                    int section_id = section.first;
                    const auto& segment_ids = section.second;

                    /** get all compartment values (otherwise, just middle point) */
                    if (all_compartments) {
                        for (const auto& segment_id: segment_ids) {
                            /** corresponding voltage in coreneuron voltage array */
                            double* variable = report_variable + segment_id;
                            to_report.emplace_back(VarWithMapping(section_id, variable));
                        }
                    } else {
                        nrn_assert(segment_ids.size() % 2);
                        /** corresponding voltage in coreneuron voltage array */
                        const auto segment_id = segment_ids[segment_ids.size() / 2];
                        double* variable = report_variable + segment_id;
                        to_report.emplace_back(VarWithMapping(section_id, variable));
                    }
                }
                vars_to_report[gid] = to_report;
            }
        }
    }
    return vars_to_report;
}

VarsToReport ReportHandler::get_custom_vars_to_report(const NrnThread& nt,
                                                      ReportConfiguration& report,
                                                      const std::vector<int>& nodes_to_gids) const {
    VarsToReport vars_to_report;
    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (report.target.find(gid) == report.target.end()) {
            continue;
        }
        Memb_list* ml = nt._ml_list[report.mech_id];
        if (!ml) {
            continue;
        }
        std::vector<VarWithMapping> to_report;
        to_report.reserve(ml->nodecount);

        for (int j = 0; j < ml->nodecount; j++) {
            double* is_selected =
                get_var_location_from_var_name(report.mech_id, SELECTED_VAR_MOD_NAME, ml, j);
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
                    get_var_location_from_var_name(report.mech_id, report.var_name.data(), ml, j);
                double* synapse_id =
                    get_var_location_from_var_name(report.mech_id, SYNAPSE_ID_MOD_NAME, ml, j);
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
    //  parents follow by childrens
    for (int i = nt.ncell + 1; i < nt.end; i++) {
        nodes_gid[i] = nodes_gid[nt._v_parent_index[i]];
    }
    return nodes_gid;
}
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)

}  // Namespace coreneuron
