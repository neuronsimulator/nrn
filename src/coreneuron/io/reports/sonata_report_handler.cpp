#include "sonata_report_handler.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#ifdef ENABLE_SONATA_REPORTS
#include "bbp/sonata/reports.h"
#endif  // ENABLE_SONATA_REPORTS

namespace coreneuron {

void SonataReportHandler::create_report(double dt, double tstop, double delay) {
#ifdef ENABLE_SONATA_REPORTS
    sonata_set_atomic_step(dt);
#endif  // ENABLE_SONATA_REPORTS
    ReportHandler::create_report(dt, tstop, delay);
}

#ifdef ENABLE_SONATA_REPORTS
void SonataReportHandler::register_soma_report(const NrnThread& nt,
                                               ReportConfiguration& config,
                                               const VarsToReport& vars_to_report) {
    register_report(nt, config, vars_to_report);
}

void SonataReportHandler::register_compartment_report(const NrnThread& nt,
                                                      ReportConfiguration& config,
                                                      const VarsToReport& vars_to_report) {
    register_report(nt, config, vars_to_report);
}

void SonataReportHandler::register_custom_report(const NrnThread& nt,
                                                 ReportConfiguration& config,
                                                 const VarsToReport& vars_to_report) {
    register_report(nt, config, vars_to_report);
}

void SonataReportHandler::register_report(const NrnThread& nt,
                                          ReportConfiguration& config,
                                          const VarsToReport& vars_to_report) {
    sonata_create_report(config.output_path.data(), config.start, config.stop, config.report_dt,
                         config.type_str.data());
    sonata_set_report_max_buffer_size_hint(config.output_path.data(), config.buffer_size);

    for (const auto& kv : vars_to_report) {
        int gid = kv.first;
        const std::vector<VarWithMapping>& vars = kv.second;
        if (!vars.size())
            continue;

        sonata_add_node(config.output_path.data(), config.population_name.data(), gid);
        sonata_set_report_max_buffer_size_hint(config.output_path.data(), config.buffer_size);
        for (const auto& variable : vars) {
            sonata_add_element(config.output_path.data(), config.population_name.data(), gid, variable.id, variable.var_value);
        }
    }
}
#endif  // ENABLE_SONATA_REPORTS
}  // Namespace coreneuron
