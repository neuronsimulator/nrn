/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <cmath>

#include "coreneuron/network/netcon.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/mechanism/membfunc.hpp"
#ifdef ENABLE_BIN_REPORTS
#include "reportinglib/Records.h"
#endif
#ifdef ENABLE_SONATA_REPORTS
#include "bbp/sonata/reports.h"
#endif

namespace coreneuron {

// Size in MB of the report buffer
static int size_report_buffer = 4;

void nrn_flush_reports(double t) {
    // flush before buffer is full
#ifdef ENABLE_BIN_REPORTS
    records_end_iteration(t);
#endif
#ifdef ENABLE_SONATA_REPORTS
    sonata_check_and_flush(t);
#endif
}

/** in the current implementation, we call flush during every spike exchange
 *  interval. Hence there should be sufficient buffer to hold all reports
 *  for the duration of mindelay interval. In the below call we specify the
 *  number of timesteps that we have to buffer.
 *  TODO: revisit this because spike exchange can happen few steps before/after
 *  mindelay interval and hence adding two extra timesteps to buffer.
 */
void setup_report_engine(double dt_report, double mindelay) {
    int min_steps_to_record = static_cast<int>(std::round(mindelay / dt_report));
#ifdef ENABLE_BIN_REPORTS
    records_set_min_steps_to_record(min_steps_to_record);
    records_setup_communicator();
    records_finish_and_share();
#endif
#ifdef ENABLE_SONATA_REPORTS
    sonata_set_min_steps_to_record(min_steps_to_record);
    sonata_setup_communicators();
    sonata_prepare_datasets();
#endif
}

// Size in MB of the report buffers
void set_report_buffer_size(int n) {
    size_report_buffer = n;
#ifdef ENABLE_BIN_REPORTS
    records_set_max_buffer_size_hint(size_report_buffer);
#endif
#ifdef ENABLE_SONATA_REPORTS
    sonata_set_max_buffer_size_hint(size_report_buffer);
#endif
}

void finalize_report() {
#ifdef ENABLE_BIN_REPORTS
    records_flush(nrn_threads[0]._t);
#endif
#ifdef ENABLE_SONATA_REPORTS
    sonata_flush(nrn_threads[0]._t);
#endif
}
} // Namespace coreneuron
