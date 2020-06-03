#include "report_event.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/nrn_assert.h"
#ifdef ENABLE_BIN_REPORTS
#include "reportinglib/Records.h"
#endif  // ENABLE_BIN_REPORTS
#ifdef ENABLE_SONATA_REPORTS
#include "bbp/sonata/reports.h"
#endif  // ENABLE_SONATA_REPORTS

namespace coreneuron {

#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
ReportEvent::ReportEvent(double dt,
                         double tstart,
                         const VarsToReport& filtered_gids,
                         const char* name)
    : dt(dt), tstart(tstart), report_path(name) {
    VarsToReport::iterator it;
    nrn_assert(filtered_gids.size());
    step = tstart / dt;
    gids_to_report.reserve(filtered_gids.size());
    for (const auto& gid : filtered_gids) {
        gids_to_report.push_back(gid.first);
    }
    std::sort(gids_to_report.begin(), gids_to_report.end());
}

/** on deliver, call ReportingLib and setup next event */
void ReportEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
/* reportinglib is not thread safe */
#pragma omp critical
    {
        // each thread needs to know its own step
#ifdef ENABLE_BIN_REPORTS
        records_nrec(step, gids_to_report.size(), gids_to_report.data(), report_path.data());
#endif
#ifdef ENABLE_SONATA_REPORTS
        sonata_record_node_data(step, gids_to_report.size(), gids_to_report.data(), report_path.data());
#endif
        send(t + dt, nc, nt);
        step++;
    }
}

bool ReportEvent::require_checkpoint() {
    return false;
}
#endif  // defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)

}  // Namespace coreneuron
