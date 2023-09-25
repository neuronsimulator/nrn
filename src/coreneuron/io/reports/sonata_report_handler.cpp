/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "sonata_report_handler.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#ifdef ENABLE_SONATA_REPORTS
#include "bbp/sonata/reports.h"
#endif  // ENABLE_SONATA_REPORTS

namespace coreneuron {

void SonataReportHandler::create_report(ReportConfiguration& config,
                                        double dt,
                                        double tstop,
                                        double delay) {
#ifdef ENABLE_SONATA_REPORTS
    sonata_set_atomic_step(dt);
#endif  // ENABLE_SONATA_REPORTS
    ReportHandler::create_report(config, dt, tstop, delay);
}

#ifdef ENABLE_SONATA_REPORTS
void SonataReportHandler::register_section_report(const NrnThread& nt,
                                                  const ReportConfiguration& config,
                                                  const VarsToReport& vars_to_report,
                                                  bool is_soma_target) {
    register_report(nt, config, vars_to_report);
}

void SonataReportHandler::register_custom_report(const NrnThread& nt,
                                                 const ReportConfiguration& config,
                                                 const VarsToReport& vars_to_report) {
    register_report(nt, config, vars_to_report);
}

std::pair<std::string, int> SonataReportHandler::get_population_info(int gid) {
    if (m_spikes_info.population_info.empty()) {
        return std::make_pair("All", 0);
    }
    std::pair<std::string, int> prev = m_spikes_info.population_info.front();
    for (const auto& name_offset: m_spikes_info.population_info) {
        std::string pop_name = name_offset.first;
        int pop_offset = name_offset.second;
        if (pop_offset > gid) {
            break;
        }
        prev = name_offset;
    }
    return prev;
}

void SonataReportHandler::register_report(const NrnThread& nt,
                                          const ReportConfiguration& config,
                                          const VarsToReport& vars_to_report) {
    sonata_create_report(config.output_path.data(),
                         config.start,
                         config.stop,
                         config.report_dt,
                         config.unit.data(),
                         config.type_str.data());
    sonata_set_report_max_buffer_size_hint(config.output_path.data(), config.buffer_size);

    for (const auto& kv: vars_to_report) {
        uint64_t gid = kv.first;
        const std::vector<VarWithMapping>& vars = kv.second;
        if (!vars.size())
            continue;

        const auto& pop_info = get_population_info(gid);
        std::string population_name = pop_info.first;
        int population_offset = pop_info.second;
        sonata_add_node(config.output_path.data(), population_name.data(), population_offset, gid);
        sonata_set_report_max_buffer_size_hint(config.output_path.data(), config.buffer_size);
        for (const auto& variable: vars) {
            sonata_add_element(config.output_path.data(),
                               population_name.data(),
                               gid,
                               variable.id,
                               variable.var_value);
        }
    }
}
#endif  // ENABLE_SONATA_REPORTS
}  // Namespace coreneuron
