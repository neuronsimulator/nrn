/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/utils/utils.hpp"

namespace coreneuron {


/*
 * Split filter comma separated strings ("mech.var_name") into mech_name and var_name
 */
void parse_filter_string(const std::string& filter, ReportConfiguration& config) {
    std::vector<std::string> mechanisms;
    std::stringstream ss(filter);
    std::string mechanism;
    // Multiple report variables are separated by `,`
    while (getline(ss, mechanism, ',')) {
        mechanisms.push_back(mechanism);

        // Split mechanism name and corresponding reporting variable
        std::string mech_name;
        std::string var_name;
        std::istringstream iss(mechanism);
        std::getline(iss, mech_name, '.');
        std::getline(iss, var_name, '.');
        if (var_name.empty()) {
            var_name = "i";
        }
        config.mech_names.emplace_back(mech_name);
        config.var_names.emplace_back(var_name);
        if (mech_name == "i_membrane") {
            nrn_use_fast_imem = true;
        }
    }
}

void register_target_type(ReportConfiguration& report, ReportType report_type) {
    report.type = report_type;
    switch (report.target_type) {
        case TargetType::Compartment:
            report.section_type = All;
            report.section_all_compartments = true;
            break;
        case TargetType::Cell:
            report.section_type = Cell;
            report.section_all_compartments = false;
            break;
        case TargetType::SectionSoma:
            report.section_type = Soma;
            report.section_all_compartments = false;
            break;
        case TargetType::SectionSomaAll:
            report.section_type = Soma;
            report.section_all_compartments = true;
            break;
        case TargetType::SectionAxon:
            report.section_type = Axon;
            report.section_all_compartments = false;
            break;
        case TargetType::SectionAxonAll:
            report.section_type = Axon;
            report.section_all_compartments = true;
            break;
        case TargetType::SectionDendrite:
            report.section_type = Dendrite;
            report.section_all_compartments = false;
            break;
        case TargetType::SectionDendriteAll:
            report.section_type = Dendrite;
            report.section_all_compartments = true;
            break;
        case TargetType::SectionApical:
            report.section_type = Apical;
            report.section_all_compartments = false;
            break;
        case TargetType::SectionApicalAll:
            report.section_type = Apical;
            report.section_all_compartments = true;
            break;
        default:
            std::cerr << "Report error: unsupported target type" << std::endl;
            nrn_abort(1);
    }
}

std::vector<ReportConfiguration> create_report_configurations(
    const std::string& conf_file,
    const std::string& output_dir,
    std::vector<std::pair<std::string, int>>& spikes_population_name_offset) {
    std::vector<ReportConfiguration> reports;
    std::string report_on;
    int target;
    std::ifstream report_conf(conf_file);

    int num_reports = 0;
    report_conf >> num_reports;
    for (int i = 0; i < num_reports; i++) {
        ReportConfiguration report;
        report.buffer_size = 4;  // default size to 4 Mb

        report_conf >> report.name >> report.target_name >> report.type_str >> report_on >>
            report.unit >> report.format >> target >> report.report_dt >> report.start >>
            report.stop >> report.num_gids >> report.buffer_size >> report.population_name >>
            report.population_offset;

        report.target_type = static_cast<TargetType>(target);
        std::transform(report.type_str.begin(),
                       report.type_str.end(),
                       report.type_str.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        report.output_path = output_dir + "/" + report.name;
        ReportType report_type;
        if (report.type_str == "compartment") {
            report_type = SectionReport;
            if (report_on == "i_membrane") {
                nrn_use_fast_imem = true;
                report_type = IMembraneReport;
            }
        } else if (report.type_str == "synapse") {
            report_type = SynapseReport;
        } else if (report.type_str == "summation") {
            report_type = SummationReport;
        } else {
            std::cerr << "Report error: unsupported type " << report.type_str << std::endl;
            nrn_abort(1);
        }
        register_target_type(report, report_type);
        if (report.type == SynapseReport || report.type == SummationReport) {
            parse_filter_string(report_on, report);
        }
        if (report.num_gids) {
            std::vector<int> new_gids(report.num_gids);
            report_conf.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            report_conf.read(reinterpret_cast<char*>(new_gids.data()),
                             report.num_gids * sizeof(int));
            report.target = std::set<int>(new_gids.begin(), new_gids.end());
            // extra new line: skip
            report_conf.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        reports.push_back(report);
    }
    // read population information for spike report
    int num_populations;
    std::string spikes_population_name;
    int spikes_population_offset;
    if (isdigit(report_conf.peek())) {
        report_conf >> num_populations;
    } else {
        // support old format: one single line "All"
        num_populations = 1;
    }
    for (int i = 0; i < num_populations; i++) {
        if (!(report_conf >> spikes_population_name >> spikes_population_offset)) {
            // support old format: one single line "All"
            report_conf >> spikes_population_name;
            spikes_population_offset = 0;
        }
        spikes_population_name_offset.emplace_back(
            std::make_pair(spikes_population_name, spikes_population_offset));
    }

    return reports;
}
}  // namespace coreneuron
