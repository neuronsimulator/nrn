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

void ReportEvent::deliver(double t, NetCvode *nc, NrnThread *nt) {
//avoid pgc++-Fatal-/opt/pgi/linux86-64/16.5/bin/pggpp2ex TERMINATED by signal 11
#ifdef ENABLE_REPORTING

    /** @todo: reportinglib is not thread safe, fix this */
    #pragma omp critical
    {
        // each thread needs to know its own step
        #ifdef ENABLE_REPORTING
            // TODO: do not rebuild this cellid vector each reporting dt!
            std::vector<int> temp;
            for( int i=0; i<nt->ncell; i++ ) {
                PreSyn& presyn = nt->presyns[i];
                temp.push_back(presyn.gid_);
            }
            std::sort( temp.begin(), temp.end() );
            records_nrec(step, nt->ncell, &temp[0] );
        #endif
        send(t+dt, nc, nt);
        step++;
    }
#endif //ENABLE_REPORTING
}

/** for given PreSyn, return the pointer to the soma voltage */
double * ReportGenerator::get_soma_voltage_ptr(PreSyn &presyn, NrnThread& nt) {

    double *actual_v = nt._actual_v;
    int *parent_index = nt._v_parent_index;
    int node_count = nt.end;
    int ncells = nt.ncell;
    int presyn_voltage_index = presyn.thvar_index_;
    bool found = false;

    /** make sure index is in the range */
    if(! ( presyn_voltage_index > 0 && presyn_voltage_index < node_count) ) {
        std::cout << "\n Error! : presyn index not in the range!";
        abort();
    }

    /** these will be the maximum number of searches */
    int search_count = presyn_voltage_index;

    /** we could break if presyn_voltage_index < ncells but just to avoid deadlock in case of bugs */
    for(int i = 0; i <= search_count; i++) {
        if(presyn_voltage_index < ncells) {
            /** sanity check */
            if(parent_index[presyn_voltage_index] == 0) {
                found = true;
            }
            break;
        } else {
            presyn_voltage_index = parent_index[presyn_voltage_index];
        }
    }

    /** just sanity check for now */
    if(!found) {
        std::cout << "\n Error: Soma location cant found!, Report this error!";
        fflush(stdout);
        abort();
    }

    /** return pointer to soma voltage */
    return actual_v + presyn_voltage_index;
}

/** based on command line arguments, setup reportinglib interface */
ReportGenerator::ReportGenerator(double start_, double stop_, double dt_, double dt_report_, std::string path) {
    start = start_;
    stop = stop_;
    dt = dt_;
    dt_report = dt_report_;
    report_filepath = path + std::string("/soma");
}

#ifdef ENABLE_REPORTING

void ReportGenerator::register_report() {

    /** @todo: hard coded parameters for ReportingLib from Jim*/
    int sizemapping = 1;
    int extramapping = 5;
    int mapping[] = {0};
    int extra[] = {1, 0, 0, 0, 1};

    /** @todo: change reportingLib to accept const char * */
    char *report_name = new char[report_filepath.size() + 1];
    std::copy(report_filepath.begin(), report_filepath.end(), report_name);
    report_name[report_filepath.size()] = '\0';

    /* simulation dt */
    records_set_atomic_step(dt);

    for (int ith = 0; ith < nrn_nthread; ++ith) {

        NrnThread& nt = nrn_threads[ith];

        /** avoid empty NrnThread */
        if(nt.ncell) {

            /** new event for every thread */
            events.push_back(new ReportEvent(dt));
            events[ith]->send(dt, net_cvode_instance, &nt);

            /** register every gid to reportinglib. note that we
             *  have to just use first ncell presyns, rest are
             *  artificial cells */
            for (int i = 0; i < nt.ncell; ++i) {
                PreSyn& presyn = nt.presyns[i];
                int gid = presyn.gid_;
// put gid into array later
                double *v = get_soma_voltage_ptr(presyn, nt);
                records_add_report(report_name, gid, gid, gid, start, stop, dt_report, sizemapping, "compartment", extramapping, "mV");
                records_add_var_with_mapping(report_name, gid, v, sizemapping, mapping);
                records_extra_mapping(report_name, gid, 5, extra);
            }
        }
    }

    /** reportinglib setup */
    //records_set_steps_to_buffer(100); //specify how many reporting steps to buffer
    records_setup_communicator();
    records_finish_and_share();

    if(nrnmpi_myid == 0) {
        std::cout << " Report registration finished!\n";
    }

    delete [] report_name;
}

extern "C" void nrn_flush_reports(double t) {
    records_flush(t);
}

#endif //ENABLE_REPORTING
