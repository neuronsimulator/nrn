/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "binary_report_handler.hpp"
#ifdef ENABLE_BIN_REPORTS
#include "reportinglib/Records.h"
#endif  // ENABLE_BIN_REPORTS

namespace coreneuron {

void BinaryReportHandler::create_report(double dt, double tstop, double delay) {
#ifdef ENABLE_BIN_REPORTS
    records_set_atomic_step(dt);
#endif  // ENABLE_BIN_REPORTS
    ReportHandler::create_report(dt, tstop, delay);
}

#ifdef ENABLE_BIN_REPORTS
static void create_soma_extra(const CellMapping& mapping, std::array<int, 5>& extra) {
    extra = {1, 0, 0, 0, 0};
    /* report extra "mask" all infos not written in report: here only soma count is reported */
    extra[1] = mapping.get_seclist_segment_count("soma");
}

static void create_compartment_extra(const CellMapping& mapping, std::array<int, 5>& extra) {
    extra[1] = mapping.get_seclist_section_count("soma");
    extra[2] = mapping.get_seclist_section_count("axon");
    extra[3] = mapping.get_seclist_section_count("dend");
    extra[4] = mapping.get_seclist_section_count("apic");
    extra[0] = std::accumulate(extra.begin() + 1, extra.end(), 0);
}

static void create_custom_extra(const CellMapping& mapping, std::array<int, 5>& extra) {
    extra = {1, 0, 0, 0, 1};
    extra[1] = mapping.get_seclist_section_count("soma");
    // extra[2] and extra[3]
    extra[4] = mapping.get_seclist_section_count("apic");
    extra[0] = std::accumulate(extra.begin() + 1, extra.end(), 0);
}

void BinaryReportHandler::register_soma_report(const NrnThread& nt,
                                               ReportConfiguration& config,
                                               const VarsToReport& vars_to_report) {
    create_extra_func create_extra = create_soma_extra;
    register_report(nt, config, vars_to_report, create_extra);
}

void BinaryReportHandler::register_compartment_report(const NrnThread& nt,
                                                      ReportConfiguration& config,
                                                      const VarsToReport& vars_to_report) {
    create_extra_func create_extra = create_compartment_extra;
    register_report(nt, config, vars_to_report, create_extra);
}

void BinaryReportHandler::register_custom_report(const NrnThread& nt,
                                                 ReportConfiguration& config,
                                                 const VarsToReport& vars_to_report) {
    create_extra_func create_extra = create_custom_extra;
    register_report(nt, config, vars_to_report, create_extra);
}

void BinaryReportHandler::register_report(const NrnThread& nt,
                                          ReportConfiguration& config,
                                          const VarsToReport& vars_to_report,
                                          create_extra_func& create_extra) {
    int sizemapping = 1;
    int extramapping = 5;
    std::array<int, 1> mapping = {0};
    std::array<int, 5> extra;
    for (const auto& var: vars_to_report) {
        int gid = var.first;
        auto& vars = var.second;
        if (vars.empty()) {
            continue;
        }
        const auto* mapinfo = static_cast<NrnThreadMappingInfo*>(nt.mapping);
        const CellMapping* m = mapinfo->get_cell_mapping(gid);
        extra[0] = vars.size();
        create_extra(*m, extra);
        records_add_report(config.output_path.data(),
                           gid,
                           gid,
                           gid,
                           config.start,
                           config.stop,
                           config.report_dt,
                           sizemapping,
                           config.type_str.data(),
                           extramapping,
                           config.unit.data());

        records_set_report_max_buffer_size_hint(config.output_path.data(), config.buffer_size);
        records_extra_mapping(config.output_path.data(), gid, 5, extra.data());
        for (const auto& var: vars) {
            mapping[0] = var.id;
            records_add_var_with_mapping(
                config.output_path.data(), gid, var.var_value, sizemapping, mapping.data());
        }
    }
}
#endif  // ENABLE_BIN_REPORTS

}  // Namespace coreneuron
