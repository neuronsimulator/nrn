/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/**
 * @file nrnreport.h
 * @brief interface with reportinglib for soma reports
 */

#ifndef _H_NRN_REPORT_
#define _H_NRN_REPORT_

#include <string>
#include <vector>
#include <set>

#define REPORT_MAX_NAME_LEN     256
#define REPORT_MAX_FILEPATH_LEN 4096

namespace coreneuron {
// name of the variable in mod file that is used to indicate which synapse
// is enabled or disable for reporting
#define SELECTED_VAR_MOD_NAME "selected_for_report"

/// name of the variable in mod file used for setting synapse id
#define SYNAPSE_ID_MOD_NAME "synapseID"

// enumerate that defines the type of target report requested
enum ReportType { SomaReport, CompartmentReport, SynapseReport, IMembraneReport, SectionReport };

// enumerate that defines the section type for a Section report
enum SectionType { Axon, Dendrite, Apical };

struct ReportConfiguration {
    std::string name;               // name of the report
    std::string output_path;        // full path of the report
    std::string target_name;        // target of the report
    std::string mech_name;          // mechanism name
    std::string var_name;           // variable name
    std::string unit;               // unit of the report
    std::string format;             // format of the report (Bin, hdf5, SONATA)
    std::string type_str;           // type of report string
    std::string population_name;    // population name of the report
    ReportType type;                // type of the report
    SectionType section_type;       // type of section report
    bool section_all_compartments;  // flag for section report (all values)
    int mech_id;                    // mechanism
    double report_dt;               // reporting timestep
    double start;                   // start time of report
    double stop;                    // stop time of report
    int num_gids;                   // total number of gids
    int buffer_size;                // hint on buffer size used for this report
    std::set<int> target;           // list of gids for this report
};

void setup_report_engine(double dt_report, double mindelay);
std::vector<ReportConfiguration> create_report_configurations(const std::string& filename,
                                                              const std::string& output_dir,
                                                              std::string& spikes_population_name);
void finalize_report();
void nrn_flush_reports(double t);
void set_report_buffer_size(int n);

}  // namespace coreneuron
#endif  //_H_NRN_REPORT_
