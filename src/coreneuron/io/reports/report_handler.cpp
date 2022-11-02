/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "report_handler.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/utils/utils.hpp"

namespace coreneuron {

void ReportHandler::create_report(double dt, double tstop, double delay) {
#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
    if (m_report_config.start < t) {
        m_report_config.start = t;
    }
    m_report_config.stop = std::min(m_report_config.stop, tstop);

    for (const auto& mech: m_report_config.mech_names) {
        m_report_config.mech_ids.emplace_back(nrn_get_mechtype(mech.data()));
    }
    if (m_report_config.type == SynapseReport && m_report_config.mech_ids.empty()) {
        std::cerr << "[ERROR] mechanism to report: " << m_report_config.mech_names[0]
                  << " is not mapped in this simulation, cannot report on it \n";
        nrn_abort(1);
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        double* report_variable = nt._actual_v;
        if (!nt.ncell) {
            continue;
        }
        const std::vector<int>& nodes_to_gid = map_gids(nt);
        VarsToReport vars_to_report;
        bool is_soma_target;
        switch (m_report_config.type) {
            case IMembraneReport:
                report_variable = nt.nrn_fast_imem->nrn_sav_rhs;
            case SectionReport:
                vars_to_report =
                    get_section_vars_to_report(nt,
                                               m_report_config.target,
                                               report_variable,
                                               m_report_config.section_type,
                                               m_report_config.section_all_compartments);
                is_soma_target = m_report_config.section_type == SectionType::Soma ||
                                 m_report_config.section_type == SectionType::Cell;
                register_section_report(nt, m_report_config, vars_to_report, is_soma_target);
                break;
            case SummationReport:
                vars_to_report = get_summation_vars_to_report(nt,
                                                              m_report_config.target,
                                                              m_report_config,
                                                              nodes_to_gid);
                register_custom_report(nt, m_report_config, vars_to_report);
                break;
            default:
                vars_to_report = get_synapse_vars_to_report(nt, m_report_config, nodes_to_gid);
                register_custom_report(nt, m_report_config, vars_to_report);
        }
        if (!vars_to_report.empty()) {
            auto report_event = std::make_unique<ReportEvent>(dt,
                                                              t,
                                                              vars_to_report,
                                                              m_report_config.output_path.data(),
                                                              m_report_config.report_dt);
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
void ReportHandler::register_section_report(const NrnThread& nt,
                                            ReportConfiguration& config,
                                            const VarsToReport& vars_to_report,
                                            bool is_soma_target) {
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

std::string getSectionTypeStr(SectionType type) {
    switch (type) {
        case All:
            return "All";
        case Cell:
        case Soma:
            return "soma";
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

void register_sections_to_report(const SecMapping* sections,
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

            if (section_type_str == "All") {
                const auto& section_mapping = cell_mapping->secmapvec;
                for (const auto& sections: section_mapping) {
                    register_sections_to_report(sections,
                                                to_report,
                                                report_variable,
                                                all_compartments);
                }
            } else {
                /** get section list mapping for the type, if available */
                if (cell_mapping->get_seclist_section_count(section_type_str) > 0) {
                    const auto& sections = cell_mapping->get_seclist_mapping(section_type_str);
                    register_sections_to_report(sections,
                                                to_report,
                                                report_variable,
                                                all_compartments);
                }
            }
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

VarsToReport ReportHandler::get_summation_vars_to_report(
    const NrnThread& nt,
    const std::set<int>& target,
    ReportConfiguration& report,
    const std::vector<int>& nodes_to_gids) const {
    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    auto& summation_report = nt.summation_report_handler_->summation_reports_[report.output_path];
    if (!mapinfo) {
        std::cerr << "[COMPARTMENTS] Error : mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (report.target.find(gid) == report.target.end()) {
            continue;
        }
        bool has_imembrane = false;
        // In case we need convertion of units
        int scale = 1;
        for (auto i = 0; i < report.mech_ids.size(); ++i) {
            auto mech_id = report.mech_ids[i];
            auto var_name = report.var_names[i];
            auto mech_name = report.mech_names[i];
            if (mech_name != "i_membrane") {
                // need special handling for Clamp processes to flip the current value
                if (mech_name == "IClamp" || mech_name == "SEClamp") {
                    scale = -1;
                }
                Memb_list* ml = nt._ml_list[mech_id];
                if (!ml) {
                    continue;
                }

                for (int j = 0; j < ml->nodecount; j++) {
                    auto segment_id = ml->nodeindices[j];
                    if ((nodes_to_gids[ml->nodeindices[j]] == gid)) {
                        double* var_value =
                            get_var_location_from_var_name(mech_id, var_name.data(), ml, j);
                        summation_report.currents_[segment_id].push_back(
                            std::make_pair(var_value, scale));
                    }
                }
            } else {
                has_imembrane = true;
            }
        }
        if (target.find(gid) != target.end()) {
            const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
            if (cell_mapping == nullptr) {
                std::cerr
                    << "[SUMMATION] Error : Compartment mapping information is missing for gid "
                    << gid << '\n';
                nrn_abort(1);
            }
            std::vector<VarWithMapping> to_report;
            to_report.reserve(cell_mapping->size());
            summation_report.summation_.resize(nt.end);
            double* report_variable = summation_report.summation_.data();
            const auto& section_type_str = getSectionTypeStr(report.section_type);
            if (report.section_type != SectionType::All) {
                if (cell_mapping->get_seclist_section_count(section_type_str) > 0) {
                    const auto& sections = cell_mapping->get_seclist_mapping(section_type_str);
                    register_sections_to_report(sections,
                                                to_report,
                                                report_variable,
                                                report.section_all_compartments);
                }
            }
            const auto& section_mapping = cell_mapping->secmapvec;
            for (const auto& sections: section_mapping) {
                for (auto& section: sections->secmap) {
                    // compartment_id
                    int section_id = section.first;
                    auto& segment_ids = section.second;
                    for (const auto& segment_id: segment_ids) {
                        // corresponding voltage in coreneuron voltage array
                        if (has_imembrane) {
                            summation_report.currents_[segment_id].push_back(
                                std::make_pair(nt.nrn_fast_imem->nrn_sav_rhs + segment_id, 1));
                        }
                        if (report.section_type == SectionType::All) {
                            double* variable = report_variable + segment_id;
                            to_report.emplace_back(VarWithMapping(section_id, variable));
                        } else if (report.section_type == SectionType::Cell) {
                            summation_report.gid_segments_[gid].push_back(segment_id);
                        }
                    }
                }
            }
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

VarsToReport ReportHandler::get_synapse_vars_to_report(
    const NrnThread& nt,
    ReportConfiguration& report,
    const std::vector<int>& nodes_to_gids) const {
    VarsToReport vars_to_report;
    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (report.target.find(gid) == report.target.end()) {
            continue;
        }
        // There can only be 1 mechanism
        nrn_assert(report.mech_ids.size() == 1);
        auto mech_id = report.mech_ids[0];
        auto var_name = report.var_names[0];
        Memb_list* ml = nt._ml_list[mech_id];
        if (!ml) {
            continue;
        }
        std::vector<VarWithMapping> to_report;
        to_report.reserve(ml->nodecount);

        for (int j = 0; j < ml->nodecount; j++) {
            double* is_selected =
                get_var_location_from_var_name(mech_id, SELECTED_VAR_MOD_NAME, ml, j);
            bool report_variable = false;

            /// if there is no variable in mod file then report on every compartment
            /// otherwise check the flag set in mod file
            if (is_selected == nullptr) {
                report_variable = true;
            } else {
                report_variable = *is_selected != 0.;
            }
            if ((nodes_to_gids[ml->nodeindices[j]] == gid) && report_variable) {
                double* var_value = get_var_location_from_var_name(mech_id, var_name.data(), ml, j);
                double* synapse_id =
                    get_var_location_from_var_name(mech_id, SYNAPSE_ID_MOD_NAME, ml, j);
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
