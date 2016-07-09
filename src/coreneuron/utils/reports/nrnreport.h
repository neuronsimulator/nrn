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
#include "coreneuron/nrniv/netcon.h"

/** global instance */
extern NetCvode* net_cvode_instance;

/** To manage report events, subclass of DiscreteEvent */
class ReportEvent: public DiscreteEvent {

    private:
        /** every thread or event can have different dt */
        double dt;
        unsigned long step;

    public:
        ReportEvent(double t);

        /** on deliver, call ReportingLib and setup next event */
        virtual void deliver(double t, NetCvode *nc, NrnThread *nt);
};



/** class for managing report generation with ReportingLib */
class ReportGenerator {

    private:

        /** every thread should have an event */
        std::vector<ReportEvent *> events;
        double start;
        double stop;
        double dt;
        double dt_report;
        std::string report_filepath;

    public:

        ReportGenerator(double start, double stop, double dt, double dt_report, std::string path);

        double * get_soma_voltage_ptr(PreSyn &presyn, NrnThread& nt);

        #ifdef ENABLE_REPORTING
            void register_report();
        #endif

        ~ReportGenerator() {
            events.clear();
        }
};

#endif //_H_NRN_REPORT_

