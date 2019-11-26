/*
   Copyright (c) 2018, Blue Brain Project
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

#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/utils/reports/nrnreport.h"
#include "coreneuron/nrnoc/fast_imem.h"
#include "coreneuron/nrnoc/mech_mapping.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <string.h>

namespace coreneuron {

/*
 * Split filter string ("mech.var_name") into mech_id and var_name
 */
void parse_filter_string(char* filter, ReportConfiguration& config) {
    char* token = strtok(filter, ".");
    if (!token) {
        std::cerr << "Error : Invalid report variable, should be mch_name.var_name" << std::endl;
        abort();
    }
    strcpy(config.mech_name, token);
    token = strtok(nullptr, "\n");
    if (!token) {
        std::cerr << "Error : Invalid report variable, should be mch_name.var_name" << std::endl;
        abort();
    }
    strcpy(config.var_name, token);
}

std::vector<ReportConfiguration> create_report_configurations(const char* conf_file,
                                                              const char* output_dir) {
    std::vector<ReportConfiguration> reports;
    int num_reports = 0;
    char report_on[MAX_REPORT_NAME_LEN] = "";
    char raw_line[MAX_FILEPATH_LEN] = "";
    int is_soma;
    int* gids;

    FILE* fp = fopen(conf_file, "r");
    if (!fp) {
        std::cerr << "Cannot open configuration file: " << conf_file << ", aborting execution"
                  << std::endl;
        abort();
    }

    fgets(raw_line, MAX_FILEPATH_LEN, fp);
    sscanf(raw_line, "%d\n", &num_reports);
    for (int i = 0; i < num_reports; i++) {
        reports.push_back(ReportConfiguration());
        ReportConfiguration& report = reports[reports.size() - 1];
        // mechansim id registered in coreneuron
        report.mech_id = -1;
        report.buffer_size = 4;  // default size to 4 Mb
        fgets(raw_line, MAX_FILEPATH_LEN, fp);
        sscanf(raw_line, "\n%s %s %s %s %s %s %d %lf %lf %lf %d %d\n", report.name,
               report.target_name, report.type_str, report_on, report.unit, report.format, &is_soma,
               &report.report_dt, &report.start, &report.stop, &report.num_gids,
               &report.buffer_size);
        for (int i = 0; i < MAX_REPORT_NAME_LEN; i++) {
            report.type_str[i] = tolower(report.type_str[i]);
        }
        sprintf(report.output_path, "%s/%s", output_dir, report.name);
        if (strcmp(report.type_str, "compartment") == 0) {
            if (strcmp(report_on, "i_membrane") == 0) {
                nrn_use_fast_imem = true;
                report.type = IMembraneReport;
            } else if (is_soma)
                report.type = SomaReport;
            else
                report.type = CompartmentReport;
        } else if (strcmp(report.type_str, "synapse") == 0) {
            report.type = SynapseReport;
        } else {
            std::cerr << "Report error: unsupported type " << report.type_str << "\n";
            abort();
        }

        if (report.type == SynapseReport)
            parse_filter_string(report_on, report);

        if (report.num_gids) {
            gids = (int*)calloc(report.num_gids, sizeof(int));
            fread(gids, sizeof(int), report.num_gids, fp);
            // extra new line
            fgets(raw_line, MAX_FILEPATH_LEN, fp);
            for (int i = 0; i < report.num_gids; i++) {
                report.target.insert(gids[i]);
            }
            free(gids);
        }
    }
    fclose(fp);
    return reports;
}
}  // namespace coreneuron
