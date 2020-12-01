/*
   Copyright (c) 2018, Blue Brain Project
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
   3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
   THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/utils/nrn_assert.h"

namespace coreneuron {

/*
 * Defines the type of target, as per the following syntax:
 *   0=Compartment, 1=Cell/Soma, Section { 2=Axon, 3=Dendrite, 4=Apical }
 * The "Comp" variations are compartment-based (all segments, not middle only)
 */
enum class TargetType {
    Compartment = 0,
    Soma = 1,
    Axon = 2,
    Dendrite = 3,
    Apical = 4,
    AxonComp = 5,
    DendriteComp = 6,
    ApicalComp = 7,
};

/*
 * Split filter string ("mech.var_name") into mech_id and var_name
 */
void parse_filter_string(const std::string &filter, ReportConfiguration &config) {
    std::istringstream iss(filter);
    std::string token;
    std::getline(iss, config.mech_name, '.');
    std::getline(iss, config.var_name, '.');
}

std::vector<ReportConfiguration> create_report_configurations(const std::string &conf_file,
                                                              const std::string &output_dir,
                                                              std::string &spikes_population_name) {
    std::vector<ReportConfiguration> reports;
    std::string report_on;
    int target_type;
    std::ifstream report_conf(conf_file);

    int num_reports = 0;
    report_conf >> num_reports;
    for (int i = 0; i < num_reports; i++) {
        ReportConfiguration report;
        // mechansim id registered in coreneuron
        report.mech_id = -1;
        report.buffer_size = 4; // default size to 4 Mb

        report_conf >> report.name >> report.target_name >> report.type_str >> report_on >>
               report.unit >> report.format >> target_type >> report.report_dt >>
               report.start >> report.stop >> report.num_gids >> report.buffer_size >>
               report.population_name;

        std::transform(report.type_str.begin(), report.type_str.end(),
                       report.type_str.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        report.output_path = output_dir + "/" + report.name;
        if (report.type_str == "compartment") {
            if (report_on == "i_membrane") {
                nrn_use_fast_imem = true;
                report.type = IMembraneReport;
            } else {
                switch (static_cast<TargetType>(target_type)) {
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
        } else {
            std::cerr << "Report error: unsupported type " << report.type_str << std::endl;
            nrn_abort(1);
        }
        if (report.type == SynapseReport) {
            parse_filter_string(report_on, report);
        }
        if (report.num_gids) {
            std::vector<int> new_gids(report.num_gids);
            report_conf.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            report_conf.read(reinterpret_cast<char *>(new_gids.data()), report.num_gids * sizeof(int));
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
