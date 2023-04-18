/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/**
 * @file nrnreport.h
 * @brief interface with libsonata for soma reports
 */

#ifndef _H_NRN_REPORT_
#define _H_NRN_REPORT_

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cstdint>

#define REPORT_MAX_NAME_LEN     256
#define REPORT_MAX_FILEPATH_LEN 4096

namespace coreneuron {

struct SummationReport {
    // Contains the values of the summation with index == segment_id
    std::vector<double> summation_ = {};
    // Map containing the pointers of the currents and its scaling factor for every segment_id
    std::unordered_map<size_t, std::vector<std::pair<double*, int>>> currents_;
    // Map containing the list of segment_ids per gid
    std::unordered_map<int, std::vector<size_t>> gid_segments_;
};

struct SummationReportMapping {
    // Map containing a SummationReport object per report
    std::unordered_map<std::string, SummationReport> summation_reports_;
};

struct SpikesInfo {
    std::string file_name = "out";
    std::vector<std::pair<std::string, int>> population_info;
};

// name of the variable in mod file that is used to indicate which synapse
// is enabled or disable for reporting
#define SELECTED_VAR_MOD_NAME "selected_for_report"

/// name of the variable in mod file used for setting synapse id
#define SYNAPSE_ID_MOD_NAME "synapseID"

/*
 * Defines the type of target, as per the following syntax:
 *   0=Compartment, 1=Cell/Soma, Section { 2=Axon, 3=Dendrite, 4=Apical }
 * The "Comp" variations are compartment-based (all segments, not middle only)
 */
enum class TargetType {
    Compartment = 0,
    Cell = 1,
    SectionSoma = 2,
    SectionAxon = 3,
    SectionDendrite = 4,
    SectionApical = 5,
    SectionSomaAll = 6,
    SectionAxonAll = 7,
    SectionDendriteAll = 8,
    SectionApicalAll = 9,
};

// enumerate that defines the type of target report requested
enum ReportType {
    SomaReport,
    CompartmentReport,
    SynapseReport,
    IMembraneReport,
    SectionReport,
    SummationReport,
    LFPReport
};

// enumerate that defines the section type for a Section report
enum SectionType { Cell, Soma, Axon, Dendrite, Apical, All };

struct ReportConfiguration {
    std::string name;                     // name of the report
    std::string output_path;              // full path of the report
    std::string target_name;              // target of the report
    std::vector<std::string> mech_names;  // mechanism names
    std::vector<std::string> var_names;   // variable names
    std::vector<int> mech_ids;            // mechanisms
    std::string unit;                     // unit of the report
    std::string format;                   // format of the report (SONATA)
    std::string type_str;                 // type of report string
    TargetType target_type;               // type of the target
    ReportType type;                      // type of the report
    SectionType section_type;             // type of section report
    bool section_all_compartments;        // flag for section report (all values)
    double report_dt;                     // reporting timestep
    double start;                         // start time of report
    double stop;                          // stop time of report
    int num_gids;                         // total number of gids
    int buffer_size;                      // hint on buffer size used for this report
    std::vector<int> target;              // list of gids for this report
};

void setup_report_engine(double dt_report, double mindelay);
std::vector<ReportConfiguration> create_report_configurations(const std::string& filename,
                                                              const std::string& output_dir,
                                                              SpikesInfo& spikes_info);
void finalize_report();
void nrn_flush_reports(double t);
void set_report_buffer_size(int n);

}  // namespace coreneuron

#endif  //_H_NRN_REPORT_
