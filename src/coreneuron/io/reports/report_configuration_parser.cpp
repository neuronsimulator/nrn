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

std::vector<ReportConfiguration> create_report_configurations(const std::string& conf_file,
                                                              const std::string& output_dir,
                                                              std::string& spikes_population_name) {
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
        if (report.type_str == "compartment") {
            if (report_on == "i_membrane") {
                nrn_use_fast_imem = true;
                report.type = IMembraneReport;
            } else {
                switch (report.target_type) {
                    case TargetType::Soma:
                        report.type = SomaReport;
                        break;
                    case TargetType::Compartment:
                        report.type = CompartmentReport;
                        break;
                    case TargetType::Axon:
                        report.type = SectionReport;
                        report.section_type = Axon;
                        report.section_all_compartments = false;
                        break;
                    case TargetType::AxonComp:
                        report.type = SectionReport;
                        report.section_type = Axon;
                        report.section_all_compartments = true;
                        break;
                    case TargetType::Dendrite:
                        report.type = SectionReport;
                        report.section_type = Dendrite;
                        report.section_all_compartments = false;
                        break;
                    case TargetType::DendriteComp:
                        report.type = SectionReport;
                        report.section_type = Dendrite;
                        report.section_all_compartments = true;
                        break;
                    case TargetType::Apical:
                        report.type = SectionReport;
                        report.section_type = Apical;
                        report.section_all_compartments = false;
                        break;
                    case TargetType::ApicalComp:
                        report.type = SectionReport;
                        report.section_type = Apical;
                        report.section_all_compartments = true;
                        break;
                    default:
                        std::cerr << "Report error: unsupported target type" << std::endl;
                        nrn_abort(1);
                }
            }
        } else if (report.type_str == "synapse") {
            report.type = SynapseReport;
        } else if (report.type_str == "summation") {
            report.type = SummationReport;
        } else {
            std::cerr << "Report error: unsupported type " << report.type_str << std::endl;
            nrn_abort(1);
        }
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

    report_conf >> spikes_population_name;
    return reports;
}
}  // namespace coreneuron
