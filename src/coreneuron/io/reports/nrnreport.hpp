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

#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "coreneuron/utils/utils.hpp"

#define REPORT_MAX_NAME_LEN     256
#define REPORT_MAX_FILEPATH_LEN 4096

namespace coreneuron {

struct SummationReport {
    // Contains the values of the summation with index == segment_id
    std::vector<double> summation_ = {};
    // Map containing the pointers of the currents and its scaling factor for every segment_id
    std::unordered_map<size_t, std::vector<std::pair<double*, double>>> currents_;
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

template <typename EnumT, std::size_t N>
std::string to_string(EnumT e,
                      const std::array<std::pair<EnumT, std::string_view>, N>& mapping,
                      const std::string_view enum_name) {
    auto it = std::find_if(mapping.begin(), mapping.end(), [e](const auto& pair) {
        return pair.first == e;
    });
    if (it != mapping.end()) {
        return std::string(it->second);
    }

    std::cerr << "Unknown value for " << enum_name << ": " << static_cast<int>(e) << std::endl;
    nrn_abort(1);
}

inline bool equals_case_insensitive(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    return std::equal(a.begin(), a.end(), b.begin(), [](unsigned char c1, unsigned char c2) {
        return std::tolower(c1) == std::tolower(c2);
    });
}

template <typename EnumT, std::size_t N>
EnumT from_string(std::string_view str,
                  const std::array<std::pair<EnumT, std::string_view>, N>& mapping,
                  const std::string_view enum_name) {
    auto it = std::find_if(mapping.begin(), mapping.end(), [&str](const auto& pair) {
        return equals_case_insensitive(str, pair.second);
    });
    if (it != mapping.end()) {
        return it->first;
    }

    std::cerr << "Unknown string for " << enum_name << ": " << str << std::endl;
    nrn_abort(1);
}

// ReportType
enum class ReportType { Compartment, Summation, Synapse, LFP };
constexpr std::array<std::pair<ReportType, std::string_view>, 4> report_type_map{
    {{ReportType::Compartment, "Compartment"},
     {ReportType::Summation, "Summation"},
     {ReportType::Synapse, "Synapse"},
     {ReportType::LFP, "LFP"}}};
inline std::string to_string(ReportType t) {
    return to_string(t, report_type_map, "ReportType");
}
inline ReportType report_type_from_string(const std::string& s) {
    return from_string<ReportType>(s, report_type_map, "ReportType");
}

// TargetType
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

constexpr std::array<std::pair<TargetType, std::string_view>, 10> target_type_map{
    {{TargetType::Compartment, "Compartment"},
     {TargetType::Cell, "Cell"},
     {TargetType::SectionSoma, "SectionSoma"},
     {TargetType::SectionAxon, "SectionAxon"},
     {TargetType::SectionDendrite, "SectionDendrite"},
     {TargetType::SectionApical, "SectionApical"},
     {TargetType::SectionSomaAll, "SectionSomaAll"},
     {TargetType::SectionAxonAll, "SectionAxonAll"},
     {TargetType::SectionDendriteAll, "SectionDendriteAll"},
     {TargetType::SectionApicalAll, "SectionApicalAll"}}};

inline std::string to_string(TargetType t) {
    return to_string(t, target_type_map, "TargetType");
}
inline TargetType target_type_from_string(const std::string& s) {
    return from_string<TargetType>(s, target_type_map, "TargetType");
}

// SectionType
enum class SectionType { Cell, Soma, Axon, Dendrite, Apical, Custom, All };

constexpr std::array<std::pair<SectionType, std::string_view>, 7> section_type_map{{
    {SectionType::Cell, "Cell"},
    {SectionType::Soma, "Soma"},
    {SectionType::Axon, "Axon"},
    {SectionType::Dendrite, "Dendrite"},
    {SectionType::Apical, "Apical"},
    {SectionType::Custom, "Custom"},
    {SectionType::All, "All"},
}};

inline std::string to_string(SectionType t) {
    return to_string(t, section_type_map, "SectionType");
}
inline SectionType section_type_from_string(std::string_view str) {
    auto it =
        std::find_if(section_type_map.begin(), section_type_map.end(), [&str](const auto& pair) {
            return equals_case_insensitive(str, pair.second);
        });

    if (it != section_type_map.end()) {
        return it->first;
    }

    // Default to Custom instead of aborting
    return SectionType::Custom;
}

// Scaling
enum class Scaling { None, Area };

constexpr std::array<std::pair<Scaling, std::string_view>, 2> scaling_map{
    {{Scaling::None, "None"}, {Scaling::Area, "Area"}}};

inline std::string to_string(Scaling s) {
    return to_string(s, scaling_map, "Scaling");
}
inline Scaling scaling_from_string(const std::string& str) {
    return from_string<Scaling>(str, scaling_map, "Scaling");
}

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
    Scaling scaling;
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
