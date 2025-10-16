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

void check_version(const std::string& raw, const int expected_major, const int expected_minor, const std::string& file_name) {
    // Must start with 'v'
    if (raw.empty() || raw[0] != 'v') {
        throw std::runtime_error(
            "Invalid major version in: \"" + raw + "\". \"" + file_name + "\" may be missing a version line. "
            "Expected format: v<major>[.<minor>], e.g., v1.3 or v2"
        );
    }

    std::string s = raw.substr(1); // strip 'v'
    int major = 0;
    int minor = 0;

    size_t dot = s.find('.');
    std::string major_str = (dot == std::string::npos) ? s : s.substr(0, dot);
    std::string minor_str = (dot == std::string::npos) ? "" : s.substr(dot + 1);

    // Parse major
    try {
        major = std::stoi(major_str);
    } catch (...) {
        throw std::runtime_error(
            "Invalid major version in: \"" + raw + "\". \"" + file_name + "\" may be missing a version line. "
            "Expected format: v<major>[.<minor>], e.g., v1.3 or v2"
        );
    }

    // Major check
    if (major != expected_major) {
        throw std::runtime_error(
            "Expected version \"v" + std::to_string(expected_major) +
            "\", got \"" + raw + "\". Probably a version mismatch between neurodamus and neuron."
        );
    }

    // Parse minor if present
    if (!minor_str.empty()) {
        try {
            minor = std::stoi(minor_str);
        } catch (...) {
            throw std::runtime_error("Invalid minor version in: " + raw);
        }

        if (minor > expected_minor) {
            std::cerr << "Warning: version minor " << minor
                      << " is above expected " << expected_minor
                      << ". Continuing.\n";
        } else if (minor < expected_minor) {
            throw std::runtime_error(
                "Minor version below expected: probably a version mismatch between neurodamus and neuron. Got \"" +
                raw + "\""
            );
        }
    }
}

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

void fill_vec_int(std::ifstream& ss, std::vector<int>& v, const int n) {
    v.resize(n);
    ss.read(reinterpret_cast<char*>(v.data()), n * sizeof(int));
    // extra new line: skip
    ss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::vector<ReportConfiguration> create_report_configurations(const std::string& conf_file,
                                                              const std::string& output_dir,
                                                              SpikesInfo& spikes_info) {
    std::string token;
    int target;
    std::ifstream report_conf(conf_file);

    std::string version;
    report_conf >> version;

    // coreneuron is compatible with: report.conf v1.x or v1
    check_version(version, 1, 0, "report.conf"); 

    int num_reports = 0;
    report_conf >> num_reports;
    std::vector<ReportConfiguration> reports(num_reports);
    for (auto& report: reports) {
        report_conf >> report.name >> report.target_name;
        report.output_path = output_dir + "/" + report.name;
        // type
        report_conf >> token;
        report.type = report_type_from_string(token);
        // report_on
        report_conf >> token;
        if (report.type == ReportType::Synapse || report.type == ReportType::Summation ||
            report.type == ReportType::Compartment || report.type == ReportType::CompartmentSet) {
            parse_filter_string(token, report);
        }
        report_conf >> report.unit >> report.format;
        // sections
        report_conf >> token;
        report.sections = section_type_from_string(token);
        // compartments
        report_conf >> token;
        report.compartments = compartments_from_string(token);

        // doubles
        report_conf >> report.report_dt >> report.start >> report.stop >> report.num_gids >>
            report.buffer_size;
        // scaling
        report_conf >> token;
        report.scaling = scaling_from_string(token);

        if (report.type == ReportType::LFP) {
            nrn_use_fast_imem = true;
        }

        // checks
        if (report.type == ReportType::Compartment || report.type == ReportType::CompartmentSet) {
            nrn_assert(report.mech_ids.empty());
            nrn_assert(report.mech_names.size() == 1);
            nrn_assert(report.var_names.size() == 1);
        }

        report_conf.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        // gids
        if (report.num_gids) {
            fill_vec_int(report_conf, report.target, report.num_gids);
            if (report.type == ReportType::CompartmentSet) {
                fill_vec_int(report_conf, report.point_section_ids, report.num_gids);
                fill_vec_int(report_conf, report.point_compartment_ids, report.num_gids);
            }
        }
    }
    // read population information for spike report
    int num_populations;
    std::string spikes_population_name;
    int spikes_population_offset;
    if (report_conf.peek() == '\n') {
        // skip newline and move forward to spike reports
        report_conf.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
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
        spikes_info.population_info.emplace_back(
            std::make_pair(spikes_population_name, spikes_population_offset));
    }
    report_conf >> spikes_info.file_name;

    return reports;
}
}  // namespace coreneuron
