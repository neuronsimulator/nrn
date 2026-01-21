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

#include <algorithm>
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

/// @brief Print to ostream the SummationReport
inline std::ostream& operator<<(std::ostream& os, const SummationReport& report) {
    os << "SummationReport:\n";

    os << "summation_ (" << report.summation_.size() << "): [";
    for (size_t i = 0; i < report.summation_.size(); ++i) {
        os << report.summation_[i];
        if (i + 1 < report.summation_.size())
            os << ", ";
    }
    os << "]\n";

    os << "currents_ (" << report.currents_.size() << "):\n";
    for (const auto& [seg_id, vec]: report.currents_) {
        os << "  " << seg_id << ": [";
        for (size_t i = 0; i < vec.size(); ++i) {
            const auto& [ptr, scale] = vec[i];
            os << "(ptr=" << ptr << ", val=" << (ptr ? *ptr : 0.0) << ", scale=" << scale << ")";
            if (i + 1 < vec.size())
                os << ", ";
        }
        os << "]\n";
    }

    os << "gid_segments_ (" << report.gid_segments_.size() << "):\n";
    for (const auto& [gid, segs]: report.gid_segments_) {
        os << "  " << gid << ": [";
        for (size_t i = 0; i < segs.size(); ++i) {
            os << segs[i];
            if (i + 1 < segs.size())
                os << ", ";
        }
        os << "]\n";
    }

    return os;
}

struct SummationReportMapping {
    // Map containing a SummationReport object per report
    std::unordered_map<std::string, SummationReport> summation_reports_;
};

struct SpikesInfo {
    std::string file_name = "out";
    std::vector<std::pair<std::string, int>> population_info;
};

/**
 * @brief Converts an enum value to its corresponding string representation.
 *
 * @tparam EnumT Enum type.
 * @tparam N Size of the mapping array.
 * @param e Enum value to convert.
 * @param mapping A fixed-size array mapping enum values to string views.
 * @param enum_name Name of the enum type, used for error reporting.
 * @return The corresponding string representation of the enum value.
 *
 * @note Aborts the program if the enum value is not found in the mapping.
 */
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

/**
 * @brief Compares two strings for equality, ignoring case.
 *
 * @param a First string.
 * @param b Second string.
 * @return true if both strings are equal ignoring case, false otherwise.
 */
inline bool equals_case_insensitive(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    return std::equal(a.begin(), a.end(), b.begin(), [](unsigned char c1, unsigned char c2) {
        return std::tolower(c1) == std::tolower(c2);
    });
}

/**
 * @brief Converts a string to its corresponding enum value, case-insensitively.
 *
 * @tparam EnumT Enum type.
 * @tparam N Size of the mapping array.
 * @param str Input string to convert.
 * @param mapping A fixed-size array mapping enum values to string views.
 * @param enum_name Name of the enum type, used for error reporting.
 * @param file_path Optional path to the source file providing the string value.
 *        Used to give additional context when reporting errors.
 * @return The corresponding enum value for the input string.
 *
 * @note Aborts the program if the string does not match any entry in the mapping.
 */
template <typename EnumT, std::size_t N>
EnumT from_string(const std::string_view str,
                  const std::array<std::pair<EnumT, std::string_view>, N>& mapping,
                  const std::string_view enum_name,
                  const std::string_view file_path) {
    auto it = std::find_if(mapping.begin(), mapping.end(), [&str](const auto& pair) {
        return equals_case_insensitive(str, pair.second);
    });
    if (it != mapping.end()) {
        return it->first;
    }

    std::cerr << "Unknown string for " << enum_name << ": \"" << str << "\".\n";
    std::cerr << "Valid options are: ";
    for (std::size_t i = 0; i < mapping.size(); ++i) {
        std::cerr << "\"" << mapping[i].second << "\"";
        if (i + 1 != mapping.size())
            std::cerr << ", ";
    }
    std::cerr << ".\n";
    if (file_path.size()) {
        std::cerr << "Probably neuron is not compatible with \"" << file_path << "\".\n";
    }
    nrn_abort(1);
}

// ReportType
enum class ReportType { Compartment, CompartmentSet, Summation, Synapse, LFP };
constexpr std::array<std::pair<ReportType, std::string_view>, 5> report_type_map{
    {{ReportType::Compartment, "compartment"},
     {ReportType::CompartmentSet, "compartment_set"},
     {ReportType::Summation, "summation"},
     {ReportType::Synapse, "synapse"},
     {ReportType::LFP, "lfp"}}};
inline std::string to_string(ReportType t) {
    return to_string(t, report_type_map, "ReportType");
}
inline ReportType report_type_from_string(const std::string_view str,
                                          const std::string_view file_path = "") {
    return from_string<ReportType>(str, report_type_map, "ReportType", file_path);
}

// SectionType
enum class SectionType { Cell, Soma, Axon, Dendrite, Apical, Ais, Node, Myelin, All, Invalid };

constexpr std::array<std::pair<SectionType, std::string_view>, 11> section_type_map{
    {{SectionType::Cell, "Cell"},
     {SectionType::Soma, "Soma"},
     {SectionType::Axon, "Axon"},
     {SectionType::Dendrite, "Dend"},
     {SectionType::Apical, "Apic"},
     {SectionType::Ais, "Ais"},
     {SectionType::Node, "Node"},
     {SectionType::Myelin, "Myelin"},
     {SectionType::All, "All"},
     {SectionType::Invalid, "Invalid"}}};

inline std::string to_string(SectionType t) {
    return to_string(t, section_type_map, "SectionType");
}
inline SectionType section_type_from_string(std::string_view str,
                                            const std::string_view file_path = "") {
    return from_string<SectionType>(str, section_type_map, "SectionType", file_path);
}

// Scaling
enum class Scaling { None, Area };

constexpr std::array<std::pair<Scaling, std::string_view>, 2> scaling_map{
    {{Scaling::None, "None"}, {Scaling::Area, "Area"}}};

inline std::string to_string(Scaling s) {
    return to_string(s, scaling_map, "Scaling");
}
inline Scaling scaling_from_string(const std::string_view str,
                                   const std::string_view file_path = "") {
    return from_string<Scaling>(str, scaling_map, "Scaling", file_path);
}

enum class Compartments { All, Center, Invalid };
constexpr std::array<std::pair<Compartments, std::string_view>, 3> compartments_map{
    {{Compartments::All, "All"},
     {Compartments::Center, "Center"},
     {Compartments::Invalid, "Invalid"}}};

inline std::string to_string(Compartments s) {
    return to_string(s, compartments_map, "Compartments");
}
inline Compartments compartments_from_string(const std::string_view str,
                                             const std::string_view file_path = "") {
    return from_string<Compartments>(str, compartments_map, "Compartments", file_path);
}

struct ReportConfiguration {
    std::string name;                        // name of the report
    std::string output_path;                 // full path of the report
    std::string target_name;                 // target of the report
    std::vector<std::string> mech_names;     // mechanism names
    std::vector<std::string> var_names;      // variable names
    std::vector<int> mech_ids;               // mechanisms
    std::string unit;                        // unit of the report
    std::string format;                      // format of the report (SONATA)
    ReportType type;                         // type of the report
    SectionType sections;                    // type of section report
    Compartments compartments;               // flag for section report (all values)
    double report_dt;                        // reporting timestep
    double start;                            // start time of report
    double stop;                             // stop time of report
    int num_gids;                            // total number of gids
    int buffer_size;                         // hint on buffer size used for this report
    std::vector<int> target;                 // list of gids for this report
    std::vector<int> point_section_ids;      // list of section_ids for this compartment set report
                                             // (empty otherwise)
    std::vector<int> point_compartment_ids;  // list of compartment_ids for this compartment set
                                             // report (empty otherwise)
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
