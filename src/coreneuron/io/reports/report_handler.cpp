/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <sstream>
#include <unordered_set>
#include "report_handler.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/utils/utils.hpp"

namespace coreneuron {
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

/**
 * @brief Map each segment_id to a single node index in the given Memb_list.
 *
 * Lazily populates the map only if empty. Aborts if a segment_id appears more than once,
 * which is invalid for compartment reports expecting exactly one variable per segment.
 * Differs from the vector variant by enforcing uniqueness and aborting on duplicates.
 */
static void fill_segment_id_2_node_id(Memb_list* ml,
                                      std::unordered_map<int, int>& segment_id_2_node_id) {
    nrn_assert(ml && "Memb_list is a nullptr!");

    // do not fill if not empty. We already did this. Be lazy
    if (!segment_id_2_node_id.empty()) {
        return;
    }

    for (int j = 0; j < ml->nodecount; j++) {
        const int segment_id = ml->nodeindices[j];
        auto result = segment_id_2_node_id.insert({segment_id, j});
        if (!result.second) {
            std::cerr << "Found duplicate in nodeindices: " << segment_id << " index: " << j
                      << std::endl;
            std::cerr << "The nodeindices (size: " << ml->nodecount << ") are:\n[";
            for (int j = 0; j < ml->nodecount; j++) {
                std::cerr << ml->nodeindices[j] << ", ";
            }
            std::cerr << "]\n";
            std::cerr << "Probably you asked for a compartment report for a synapse variable and "
                         "there are many synapses attached to the soma.\nThis is not allowed since "
                         "compartment reports require 1 and only 1 variable on every segment.\n";
            nrn_abort(1);
        }
    }
}

/**
 * @brief Map each segment_id to all corresponding node indices in the given Memb_list.
 *
 * Lazily populates the map only if empty. Allows multiple node indices per segment_id
 * and stores them in a vector. Differs from the single-index variant by permitting
 * duplicates and not performing any error checks.
 */
static void fill_segment_id_2_node_id(
    Memb_list* ml,
    std::unordered_map<int, std::vector<int>>& segment_id_2_node_id) {
    nrn_assert(ml && "Memb_list is a nullptr!");

    // do not fill if not empty. We already did this. Be lazy
    if (!segment_id_2_node_id.empty()) {
        return;
    }

    for (int j = 0; j < ml->nodecount; j++) {
        const int segment_id = ml->nodeindices[j];
        segment_id_2_node_id[segment_id].push_back(j);
    }
}

/**
 * @brief Retrieve a pointer to a single mechanism/state variable for a specific segment.
 *
 * For "i_membrane" and "v", returns a direct pointer to the relevant thread array.
 * Otherwise, looks up the mechanism in the thread, maps the segment to its node,
 * and retrieves the variable pointer. Aborts if the mechanism or variable is missing.
 * Differs from get_vars() in that it returns a single variable pointer and performs
 * strict error handling with aborts on failure.
 */
static double* get_one_var(const NrnThread& nt,
                           const int mech_id,
                           const std::string& var_name,
                           const std::string& mech_name,
                           const int segment_id,
                           std::unordered_map<int, int> segment_id_2_node_id,
                           int gid) {
    if (mech_name == "i_membrane") {
        return nt.nrn_fast_imem->nrn_sav_rhs + segment_id;
    }
    if (mech_name == "v") {
        return nt._actual_v + segment_id;
    }

    if (mech_id < 0) {
        std::cerr << "Mechanism (" << mech_id << ", " << var_name << ", " << mech_name
                  << ") not found\n";
        std::cerr << "Negative mech_id\n";
        nrn_abort(1);
    }

    Memb_list* ml = nt._ml_list[mech_id];

    if (!ml) {
        std::cerr << "Mechanism (" << mech_id << ", " << var_name << ", " << mech_name
                  << ") not found\n";
        std::cerr << "NULL Memb_list\n";
        nrn_abort(1);
    }

    // lazy cache
    fill_segment_id_2_node_id(ml, segment_id_2_node_id);

    const auto it = segment_id_2_node_id.find(segment_id);
    // there is no mechanism at this segment. Notice that segment_id_2_node_id
    // is a segment_id -> node_id that changes with mech_id and gid
    if (it == segment_id_2_node_id.end()) {
        std::cerr << "Mechanism (" << mech_id << ", " << var_name << ", " << mech_name
                  << ") not found at gid: " << gid << " segment: " << segment_id << std::endl;
        std::cerr << "The nodeindices (size: " << ml->nodecount << ") are:\n[";
        for (int j = 0; j < ml->nodecount; j++) {
            std::cerr << ml->nodeindices[j] << ", ";
        }
        std::cerr << "]\n";
        nrn_abort(1);
    }

    const int node_id = it->second;


    const auto ans = get_var_location_from_var_name(mech_id, mech_name, var_name, ml, node_id);
    if (ans == nullptr) {
        std::cerr
            << "get_var_location_from_var_name failed to retrieve a valid pointer for mechanism ("
            << mech_id << ", " << var_name << ", " << mech_name << ") at gid: " << gid
            << " segment: " << segment_id << std::endl;
        nrn_abort(1);
    }
    return ans;
}

/**
 * @brief Retrieve pointers to a mechanism/state variable for all nodes in a segment.
 *
 * For "i_membrane" and "v", returns a vector containing a single direct pointer.
 * Otherwise, looks up the mechanism in the thread, maps the segment to its nodes,
 * and retrieves pointers for each node. Returns an empty vector on any failure.
 * Differs from get_one_var() in that it returns multiple pointers (vector) and
 * fails gracefully without aborting.
 */
static std::vector<double*> get_vars(
    const NrnThread& nt,
    const int mech_id,
    const std::string& var_name,
    const std::string& mech_name,
    const int segment_id,
    std::unordered_map<int, std::vector<int>>& segment_id_2_node_ids) {
    if (mech_name == "i_membrane") {
        return {nt.nrn_fast_imem->nrn_sav_rhs + segment_id};
    }
    if (mech_name == "v") {
        return {nt._actual_v + segment_id};
    }

    if (mech_id < 0) {
        return {};
    }

    Memb_list* ml = nt._ml_list[mech_id];

    if (!ml) {
        return {};
    }

    // lazy cache
    fill_segment_id_2_node_id(ml, segment_id_2_node_ids);

    const auto it = segment_id_2_node_ids.find(segment_id);
    // there is no mechanism at this segment. Notice that segment_id_2_node_id
    // is a segment_id -> node_id that changes with mech_id and gid
    if (it == segment_id_2_node_ids.end()) {
        return {};
    }

    const std::vector<int>& node_ids = it->second;

    std::vector<double*> ans;
    for (auto node_id: node_ids) {
        const auto var = get_var_location_from_var_name(mech_id, mech_name, var_name, ml, node_id);
        if (var) {
            ans.push_back(var);
        }
    }
    return ans;
}

/**
 * @brief Compute a scaling factor for a mechanism variable.
 *
 * Inverts current for "IClamp" and "SEClamp". For area scaling (except "i_membrane"),
 * uses the segment area in µm² divided by 100. Returns 1.0 otherwise.
 */
static double get_scaling_factor(const std::string& mech_name,
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

static void append_sections_to_to_report_auto_variable(
    const std::shared_ptr<SecMapping>& sections,
    std::vector<VarWithMapping>& to_report,
    const ReportConfiguration& report_conf,
    const NrnThread& nt,
    std::unordered_map<int, int>& segment_id_2_node_id,
    int gid) {
    nrn_assert(report_conf.mech_ids.size() == 1);
    nrn_assert(report_conf.var_names.size() == 1);
    nrn_assert(report_conf.mech_names.size() == 1);

    const auto& mech_id = report_conf.mech_ids[0];
    const auto& var_name = report_conf.var_names[0];
    const auto& mech_name = report_conf.mech_names[0];
    for (const auto& section: sections->secmap) {
        // compartment_id
        int section_id = section.first;
        const auto& segment_ids = section.second;

        // get all compartment values (otherwise, just middle point)
        if (report_conf.compartments == Compartments::All) {
            for (const auto& segment_id: segment_ids) {
                double* variable = get_one_var(
                    nt, mech_id, var_name, mech_name, segment_id, segment_id_2_node_id, gid);
                to_report.emplace_back(VarWithMapping(section_id, variable));
            }
        } else if (report_conf.compartments == Compartments::Center) {
            nrn_assert(segment_ids.size() % 2 &&
                       "Section with an even number of compartments. I cannot pick the middle one. "
                       "This was not expected");
            // corresponding voltage in coreneuron voltage array
            const auto segment_id = segment_ids[segment_ids.size() / 2];
            double* variable = get_one_var(
                nt, mech_id, var_name, mech_name, segment_id, segment_id_2_node_id, gid);
            to_report.emplace_back(VarWithMapping(section_id, variable));
        } else {
            std::cerr << "[ERROR] Invalid compartments type: "
                      << to_string(report_conf.compartments) << " in report: " << report_conf.name
                      << std::endl;
            nrn_abort(1);
        }
    }
}


// fill to_report with (int section_id, double* variable)
static void append_sections_to_to_report(const std::shared_ptr<SecMapping>& sections,
                                         std::vector<VarWithMapping>& to_report,
                                         double* report_variable,
                                         const ReportConfiguration& report_conf) {
    for (const auto& section: sections->secmap) {
        // compartment_id
        int section_id = section.first;
        const auto& segment_ids = section.second;

        // get all compartment values (otherwise, just middle point)
        if (report_conf.compartments == Compartments::All) {
            for (const auto& segment_id: segment_ids) {
                // corresponding voltage in coreneuron voltage array
                double* variable = report_variable + segment_id;
                to_report.emplace_back(VarWithMapping(section_id, variable));
            }
        } else if (report_conf.compartments == Compartments::Center) {
            nrn_assert(segment_ids.size() % 2 &&
                       "Section with an even number of compartments. I cannot pick the middle one. "
                       "This was not expected");
            // corresponding voltage in coreneuron voltage array
            const auto segment_id = segment_ids[segment_ids.size() / 2];
            double* variable = report_variable + segment_id;
            to_report.emplace_back(VarWithMapping(section_id, variable));
        } else {
            std::cerr << "[ERROR] Invalid compartments type: "
                      << to_string(report_conf.compartments) << " in report: " << report_conf.name
                      << std::endl;
            nrn_abort(1);
        }
    }
}


/// @brief create an vector of ids in target_gids that are on this thread
static std::vector<int> get_intersection_ids(const NrnThread& nt, std::vector<int>& target_gids) {
    std::unordered_set<int> thread_gids;
    for (int i = 0; i < nt.ncell; i++) {
        thread_gids.insert(nt.presyns[i].gid_);
    }
    std::vector<int> intersection_ids;
    intersection_ids.reserve(target_gids.size());

    for (auto i = 0; i < target_gids.size(); ++i) {
        const auto gid = target_gids[i];
        if (thread_gids.find(gid) != thread_gids.end()) {
            intersection_ids.push_back(i);
        }
    }

    intersection_ids.shrink_to_fit();
    return intersection_ids;
}

/// loop over the points of a compartment set and to return VarsToReport
static VarsToReport get_compartment_set_vars_to_report(const NrnThread& nt,
                                                       const std::vector<int>& intersection_ids,
                                                       const ReportConfiguration& report) {
    nrn_assert(report.mech_ids.size() == 1);
    nrn_assert(report.var_names.size() == 1);
    nrn_assert(report.mech_names.size() == 1);
    nrn_assert(report.sections == SectionType::Invalid &&
               "[ERROR] : Compartment report `sections` should be Invalid");
    nrn_assert(report.compartments == Compartments::Invalid &&
               "[ERROR] : Compartment report `compartments` should be Invalid");

    nrn_assert(report.target.size() == report.point_compartment_ids.size());
    nrn_assert(report.target.size() == report.point_section_ids.size());

    const auto& mech_id = report.mech_ids[0];
    const auto& var_name = report.var_names[0];
    const auto& mech_name = report.mech_names[0];

    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[ERROR] : Compartment report mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    int old_gid = -1;
    std::unordered_map<int, int> segment_id_2_node_id;
    for (const auto& intersection_id: intersection_ids) {
        const auto gid = report.target[intersection_id];
        const auto section_id = report.point_section_ids[intersection_id];
        const auto compartment_id = report.point_compartment_ids[intersection_id];
        if (old_gid != gid) {
            // clear cache for new gid. We know they are strictly ordered
            segment_id_2_node_id.clear();
        }

        double* variable = get_one_var(
            nt, mech_id, var_name, mech_name, compartment_id, segment_id_2_node_id, gid);
        // automatically calls an empty constructor if key is not present
        vars_to_report[gid].emplace_back(section_id, variable);
        old_gid = gid;
    }
    return vars_to_report;
}


static VarsToReport get_section_vars_to_report(const NrnThread& nt,
                                               const std::vector<int>& intersection_ids,
                                               const ReportConfiguration& report) {
    nrn_assert(report.sections != SectionType::Invalid &&
               "[ERROR] : Compartment report `sections` should not be Invalid");
    nrn_assert(report.compartments != Compartments::Invalid &&
               "[ERROR] : Compartment report `compartments` should not be Invalid");
    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[ERROR] : Compartment report mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (const auto& intersection_id: intersection_ids) {
        const auto gid = report.target[intersection_id];
        std::unordered_map<int, int> segment_id_2_node_id;
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr << "[ERROR] : Compartment report mapping information is missing for gid "
                      << gid << '\n';
            nrn_abort(1);
        }
        std::vector<VarWithMapping> to_report;
        to_report.reserve(cell_mapping->size());

        if (report.sections == SectionType::All) {
            const auto& section_mapping = cell_mapping->sec_mappings;
            for (const auto& sections: section_mapping) {
                append_sections_to_to_report_auto_variable(
                    sections, to_report, report, nt, segment_id_2_node_id, gid);
            }
        } else {
            /** get section list mapping for the type, if available */
            if (cell_mapping->get_seclist_section_count(report.sections) > 0) {
                const auto& sections = cell_mapping->get_seclist_mapping(report.sections);
                append_sections_to_to_report_auto_variable(
                    sections, to_report, report, nt, segment_id_2_node_id, gid);
            }
        }
        to_report.shrink_to_fit();
        vars_to_report[gid] = to_report;
    }
    return vars_to_report;
}


static VarsToReport get_summation_vars_to_report(const NrnThread& nt,
                                                 const std::vector<int>& intersection_ids,
                                                 const ReportConfiguration& report) {
    VarsToReport vars_to_report;
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    auto& summation_report = nt.summation_report_handler_->summation_reports_[report.output_path];

    if (!mapinfo) {
        std::cerr << "[ERROR] : Compartment report mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }

    for (const auto& intersection_id: intersection_ids) {
        const auto gid = report.target[intersection_id];

        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr << "[ERROR] : Summation report mapping information is missing for gid " << gid
                      << '\n';
            nrn_abort(1);
        }

        // In case we need convertion of units
        for (auto i = 0; i < report.mech_ids.size(); ++i) {
            const auto& mech_id = report.mech_ids[i];
            const auto& var_name = report.var_names[i];
            const auto& mech_name = report.mech_names[i];
            std::unordered_map<int, std::vector<int>> segment_id_2_node_ids;

            const auto& section_mapping = cell_mapping->sec_mappings;
            for (const auto& sections: section_mapping) {
                for (auto& section: sections->secmap) {
                    int section_id = section.first;
                    auto& segment_ids = section.second;

                    for (const auto& segment_id: segment_ids) {
                        double scaling_factor =
                            get_scaling_factor(mech_name, report.scaling, nt, segment_id);
                        // I know that comparing a double to 0.0 is not the best practice,
                        // but they told me to never round this number and to follow how
                        // it is done in neurodamus + neuron. In any case it is innoquous,
                        // it is just to save some computational time. (Katta)
                        if (scaling_factor == 0.0) {
                            continue;
                        }

                        const std::vector<double*> var_ptrs = get_vars(
                            nt, mech_id, var_name, mech_name, segment_id, segment_id_2_node_ids);
                        for (auto var_ptr: var_ptrs) {
                            summation_report.currents_[segment_id].push_back(
                                std::make_pair(var_ptr, scaling_factor));
                        }
                    }
                }
            }
        }

        std::vector<VarWithMapping> to_report;
        to_report.reserve(cell_mapping->size());
        summation_report.summation_.resize(nt.end);
        double* report_variable = summation_report.summation_.data();
        nrn_assert(report.sections != SectionType::Invalid &&
                   "ReportHandler::get_summation_vars_to_report: sections should not be Invalid");

        if (report.sections != SectionType::All) {
            if (cell_mapping->get_seclist_section_count(report.sections) > 0) {
                const auto& sections = cell_mapping->get_seclist_mapping(report.sections);
                append_sections_to_to_report(sections, to_report, report_variable, report);
            }
        }

        const auto& section_mapping = cell_mapping->sec_mappings;
        for (const auto& sections: section_mapping) {
            for (auto& section: sections->secmap) {
                // compartment_id
                int section_id = section.first;
                auto& segment_ids = section.second;
                for (const auto& segment_id: segment_ids) {
                    if (report.sections == SectionType::All) {
                        double* variable = report_variable + segment_id;
                        to_report.emplace_back(VarWithMapping(section_id, variable));
                    } else if (report.sections == SectionType::Soma) {
                        summation_report.gid_segments_[gid].push_back(segment_id);
                    }
                }
            }
        }
        vars_to_report[gid] = to_report;
    }

    return vars_to_report;
}

static VarsToReport get_synapse_vars_to_report(const NrnThread& nt,
                                               const std::vector<int>& intersection_ids,
                                               const ReportConfiguration& report) {
    nrn_assert(report.mech_ids.size() == 1);
    nrn_assert(report.var_names.size() == 1);
    nrn_assert(report.mech_names.size() == 1);
    const auto& mech_id = report.mech_ids[0];
    const auto& mech_name = report.mech_names[0];
    const auto& var_name = report.var_names[0];
    VarsToReport vars_to_report;

    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    for (const auto& intersection_id: intersection_ids) {
        const auto gid = report.target[intersection_id];

        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr << "[ERROR] : Synapse report mapping information is missing for gid " << gid
                      << '\n';
            nrn_abort(1);
        }


        for (auto i = 0; i < report.mech_ids.size(); ++i) {
            std::unordered_map<int, std::vector<int>> segment_id_2_node_ids;

            const auto& section_mapping = cell_mapping->sec_mappings;
            for (const auto& sections: section_mapping) {
                for (auto& section: sections->secmap) {
                    int section_id = section.first;
                    auto& segment_ids = section.second;
                    for (const auto& segment_id: segment_ids) {
                        const std::vector<double*> selected_vars = get_vars(nt,
                                                                            mech_id,
                                                                            "selected_for_report",
                                                                            mech_name,
                                                                            segment_id,
                                                                            segment_id_2_node_ids);
                        const std::vector<double*> var_ptrs = get_vars(
                            nt, mech_id, var_name, mech_name, segment_id, segment_id_2_node_ids);
                        const std::vector<double*> synapseIDs = get_vars(
                            nt, mech_id, "synapseID", mech_name, segment_id, segment_id_2_node_ids);

                        nrn_assert(var_ptrs.size() == synapseIDs.size());
                        nrn_assert(selected_vars.empty() ||
                                   selected_vars.size() == var_ptrs.size());

                        for (auto i = 0; i < var_ptrs.size(); ++i) {
                            if (selected_vars.empty() || static_cast<bool>(selected_vars[i])) {
                                vars_to_report[gid].push_back(
                                    {static_cast<int>(*synapseIDs[i]), var_ptrs[i]});
                            }
                        }
                    }
                }
            }
        }
    }
    return vars_to_report;
}

static VarsToReport get_lfp_vars_to_report(const NrnThread& nt,
                                           const std::vector<int>& intersection_ids,
                                           ReportConfiguration& report,
                                           double* report_variable) {
    const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
    if (!mapinfo) {
        std::cerr << "[ERROR] : LFP report mapping information is missing for a Cell group "
                  << nt.ncell << '\n';
        nrn_abort(1);
    }
    auto& summation_report = nt.summation_report_handler_->summation_reports_[report.output_path];
    VarsToReport vars_to_report;
    std::size_t offset_lfp = 0;
    for (const auto& intersection_id: intersection_ids) {
        const auto gid = report.target[intersection_id];
        const auto& cell_mapping = mapinfo->get_cell_mapping(gid);
        if (cell_mapping == nullptr) {
            std::cerr << "[ERROR] : LFP report mapping information is missing for gid " << gid
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


#endif


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
        std::cerr << "[ERROR] Synapse report mechanism to report: " << report_config.mech_names[0]
                  << " is not mapped in this simulation, cannot report on it \n";
        nrn_abort(1);
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];

        if (!nt.ncell) {
            continue;
        }
        auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);

        const std::vector<int> intersection_ids = get_intersection_ids(nt, report_config.target);
        VarsToReport vars_to_report;
        const bool is_soma_target = report_config.sections == SectionType::Soma;
        switch (report_config.type) {
        case ReportType::Compartment: {
            vars_to_report = get_section_vars_to_report(nt, intersection_ids, report_config);
            register_section_report(nt, report_config, vars_to_report, is_soma_target);
            break;
        }
        case ReportType::CompartmentSet: {
            vars_to_report =
                get_compartment_set_vars_to_report(nt, intersection_ids, report_config);
            register_section_report(nt, report_config, vars_to_report, is_soma_target);
            break;
        }
        case ReportType::Summation: {
            vars_to_report = get_summation_vars_to_report(nt, intersection_ids, report_config);
            register_custom_report(nt, report_config, vars_to_report);
            break;
        }
        case ReportType::LFP: {
            mapinfo->prepare_lfp();
            vars_to_report =
                get_lfp_vars_to_report(nt, intersection_ids, report_config, mapinfo->_lfp.data());
            register_section_report(nt, report_config, vars_to_report, is_soma_target);
            break;
        }
        case ReportType::Synapse: {
            vars_to_report = get_synapse_vars_to_report(nt, intersection_ids, report_config);
            register_custom_report(nt, report_config, vars_to_report);
            break;
        }
        default: {
            std::cerr << "[ERROR] Unknown report type: " << to_string(report_config.type) << '\n';
            nrn_abort(1);
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
}  // Namespace coreneuron
