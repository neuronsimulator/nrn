/*
   Copyright (c) 2016, Blue Brain Project
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
   3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
   THE POSSIBILITY OF SUCH DAMAGE.
   */

/**
 * @file nrnreport.h
 * @date 25th April 2015
 *
 * @brief interface with reportinglib for soma reports
 */

#ifndef _H_NRN_REPORT_
#define _H_NRN_REPORT_

#include <string>
#include <vector>
#include <set>
#define MAX_REPORT_NAME_LEN 256
#define MAX_REPORT_PATH_LEN 512

namespace coreneuron {
// name of the variable in mod file that is used to indicate which synapse
// is enabled or disable for reporting
#define SELECTED_VAR_MOD_NAME "selected_for_report"

/// name of the variable in mod file used for setting synapse id
#define SYNAPSE_ID_MOD_NAME "synapseID"

enum ReportType { SomaReport, CompartmentReport, SynapseReport };

struct ReportConfiguration{
    char name[MAX_REPORT_NAME_LEN];         // name of the report
    char output_path[MAX_REPORT_PATH_LEN];  // full path of the report
    char target_name[MAX_REPORT_NAME_LEN];  // target of the report
    char mech_name[MAX_REPORT_NAME_LEN];    // mechanism name
    char var_name[MAX_REPORT_NAME_LEN];     // variable name
    char unit[MAX_REPORT_NAME_LEN];         // unit of the report
    char format[MAX_REPORT_NAME_LEN];       // format of the report (bin, hdf5)
    char type_str[MAX_REPORT_NAME_LEN];     // type of report string
    ReportType type;                        // type of the report
    int mech_id;                            // mechanism
    double report_dt;                       // reporting timestep
    double start;                           // start time of report
    double stop;                            // stop time of report
    int num_gids;                           // total number of gids
    std::set<int> target;                   // list of gids for this report
};

void setup_report_engine(double dt_report, double mindelay);
void register_report(double dt, double tstop, double delay, ReportConfiguration& config);
std::vector<ReportConfiguration> create_report_configurations(const char* filename,
                                                              const char* output_dir);
void setup_report_engine(double dt_report, double mindelay);
void finalize_report();
void nrn_flush_reports(double t);
} //namespace coreneuron
#endif  //_H_NRN_REPORT_
