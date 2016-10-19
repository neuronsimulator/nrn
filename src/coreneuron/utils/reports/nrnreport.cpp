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

#include <iostream>
#include <vector>
#include <algorithm>
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/utils/reports/nrnreport.h"
#include "coreneuron/utils/reports/nrnsection_mapping.h"
#include "coreneuron/nrniv/nrn_assert.h"

#ifdef ENABLE_REPORTING
#include "reportinglib/Records.h"
#endif

int step = 0;

/** constructor */
ReportEvent::ReportEvent(double t) {
    dt = t;
    step = 0;
}

/** on deliver, call ReportingLib and setup next event */

void ReportEvent::deliver(double t, NetCvode* nc, NrnThread* nt) {
// avoid pgc++-Fatal-/opt/pgi/linux86-64/16.5/bin/pggpp2ex TERMINATED by signal 11
#ifdef ENABLE_REPORTING

    /** @todo: reportinglib is not thread safe, fix this */
    #pragma omp critical
    {
// each thread needs to know its own step
#ifdef ENABLE_REPORTING
        // TODO: do not rebuild this cellid vector each reporting dt!
        std::vector<int> temp;
        for (int i = 0; i < nt->ncell; i++) {
            PreSyn& presyn = nt->presyns[i];
            temp.push_back(presyn.gid_);
        }
        std::sort(temp.begin(), temp.end());
        records_nrec(step, nt->ncell, &temp[0]);
#endif
        send(t + dt, nc, nt);
        step++;
    }
#else
    (void)t;
    (void)nc;
    (void)nt;
#endif  // ENABLE_REPORTING
}

/** based on command line arguments, setup reportinglib interface */
ReportGenerator::ReportGenerator(int rtype,
                                 double start_,
                                 double stop_,
                                 double dt_,
                                 double delay,
                                 double dt_report_,
                                 std::string path) {
    start = start_;
    stop = stop_;
    dt = dt_;
    dt_report = dt_report_;
    mindelay = delay;

    switch (rtype) {
        case 1:
            type = SomaReport;
            report_filepath = path + std::string("/soma");
            break;

        case 2:
            type = CompartmentReport;
            report_filepath = path + std::string("/voltage");
            break;

        default:
            if (nrnmpi_myid == 0) {
                std::cout << " WARNING: Invalid report type, enabling Soma reports!\n";
            }
            type = SomaReport;
            report_filepath = path + std::string("/soma");
    }
}

#ifdef ENABLE_REPORTING

void ReportGenerator::register_report() {
    /* simulation dt */
    records_set_atomic_step(dt);

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;

        /** avoid empty NrnThread */
        if (nt.ncell) {
            /** new event for every thread */
            events.push_back(new ReportEvent(dt));
            events[ith]->send(dt, net_cvode_instance, &nt);

            /** @todo: hard coded parameters for ReportingLib from Jim*/
            int sizemapping = 1;
            int extramapping = 5;
            int mapping[] = {0};            // first column i.e. section numbers
            int extra[] = {1, 0, 0, 0, 1};  // first row, from 2nd value (skip gid)
            const char* unit = "mV";
            const char* kind = "compartment";
            const char* reportname = report_filepath.c_str();

            /** iterate over all neurons */
            for (int i = 0; i < nt.ncell; ++i) {
                /** for this gid, get mapping information */
                int gid = nt.presyns[i].gid_;
                CellMapping* m = mapinfo->get_cell_mapping(gid);

                if( m == NULL) {
                    std::cout << "Error : Compartment mapping information is missing! \n";
                    continue;
                }

                int nsections = m->num_segments();
                int nsegments = m->num_sections();

                // sum of sections and segments plus one should be equal to
                // number of nodes in coreneuron.
                  nrn_assert( (nsections+nsegments+1) == nt.end);

                /** for full compartment reports, set extra mapping */
                if (type == CompartmentReport) {
                    extra[0] = nsegments;
                    extra[1] = m->get_seclist_segment_count("soma");
                    extra[2] = m->get_seclist_segment_count("axon");
                    extra[3] = m->get_seclist_segment_count("dend");
                    extra[4] = m->get_seclist_segment_count("apic");
                }

                /** add report variable : @todo api changes in reportinglib*/
                records_add_report((char*)reportname, gid, gid, gid, start, stop, dt_report,
                                   sizemapping, (char*)kind, extramapping, (char*)unit);

                /** add extra mapping : @todo api changes in reportinglib*/
                records_extra_mapping((char*)reportname, gid, 5, extra);

                if( type == SomaReport ) {
                    /** get  section list mapping for soma */
                    SecMapping* s = m->get_seclist_mapping("soma");

                    /** 1st key is section-id and 1st value is segment of soma */
                    mapping[0] = s->secmap.begin()->first;
                    int idx = s->secmap.begin()->second.front();

                    /** corresponding voltage in coreneuron voltage array */
                    double* v = nt._actual_v + idx;

                    /** add segment for reporting */
                    records_add_var_with_mapping((char*)reportname, gid, v, sizemapping, mapping);
                } else {

                    for(size_t j = 0; j < m->size(); j++) {
                        SecMapping* s = m->secmapvec[j];

                        for(secseg_it_type iterator = s->secmap.begin(); iterator != s->secmap.end(); iterator++) {
                            mapping[0] = iterator->first;
                            segvec_type &vec = iterator->second;

                            for(size_t k = 0; k < vec.size(); k++) {
                                int idx = vec[k];

                                /** corresponding voltage in coreneuron voltage array */
                                double* v = nt._actual_v + idx;

                                /** add segment for reporting */
                                records_add_var_with_mapping((char*)reportname, gid, v, sizemapping, mapping);

                            }
                        }
                    }
                }
            }
        }
    }

    /** in the current implementation, we call flush during every spike exchange
     *  interval. Hence there should be sufficient buffer to hold all reports
     *  for the duration of mindelay interval. In the below call we specify the
     *  number of timesteps that we have to buffer.
     *  TODO: revisit this because spike exchnage can happen few steps before/after
     *  mindelay interval and hence adding two extra timesteps to buffer.
     */
    int timesteps_to_buffer = mindelay / dt_report + 2;
    records_set_steps_to_buffer(timesteps_to_buffer);

    /** reportinglib setup */
    records_setup_communicator();
    records_finish_and_share();

    if (nrnmpi_myid == 0) {
        if (type == SomaReport)
            std::cout << " Soma report registration finished!\n";
        else
            std::cout << " Full compartment report registration finished!\n";
    }
}

extern "C" void nrn_flush_reports(double t) {
    records_flush(t);
}

#endif  // ENABLE_REPORTING
