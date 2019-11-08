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

#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/netcvode.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/utils/reports/nrnreport.h"
#include "coreneuron/utils/reports/nrnsection_mapping.h"
#include "coreneuron/nrnoc/mech_mapping.hpp"
#include "coreneuron/nrnoc/membfunc.h"
#ifdef ENABLE_REPORTING
#ifdef ENABLE_SONATA_REPORTS
#include "reportinglib/records.h"
#else
#include "reportinglib/Records.h"
#endif
#endif
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <cmath>

namespace coreneuron {

#ifdef ENABLE_REPORTING

class ReportEvent;

static std::vector<ReportEvent*> reports;
struct VarWithMapping {
    int id;
    double* var_value;
    VarWithMapping(int id_, double* v_) : id(id_), var_value(v_) {
    }
};

// mapping the set of variables pointers to report to its gid
typedef std::map<int, std::vector<VarWithMapping> > VarsToReport;

class ReportEvent : public DiscreteEvent {
  private:
    double dt;
    double step;
    char report_path[MAX_FILEPATH_LEN];
    std::vector<int> gids_to_report;
    double tstart;

  public:
    ReportEvent(double dt, double tstart, VarsToReport& filtered_gids, const char* name)
        : dt(dt), tstart(tstart) {
        strcpy(report_path, name);
        VarsToReport::iterator it;
        nrn_assert(filtered_gids.size());
        step = tstart / dt;
        gids_to_report.reserve(filtered_gids.size());
        for (it = filtered_gids.begin(); it != filtered_gids.end(); ++it) {
            gids_to_report.push_back(it->first);
        }
        std::sort(gids_to_report.begin(), gids_to_report.end());
    }

    /** on deliver, call ReportingLib and setup next event */
    virtual void deliver(double t, NetCvode* nc, NrnThread* nt) {
/** @todo: reportinglib is not thread safe */
#pragma omp critical
        {
            // each thread needs to know its own step
            records_nrec(step, gids_to_report.size(), &gids_to_report[0], report_path);
            send(t + dt, nc, nt);
            step++;
        }
    }

    virtual bool require_checkpoint() {
        return false;
    }
};

VarsToReport get_soma_vars_to_report(NrnThread& nt, std::set<int>& target, double* report_variable) {
    VarsToReport vars_to_report;
    NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;

    if (!mapinfo) {
        std::cout << "[SOMA] Error : mapping information is missing for a Cell group " << std::endl;
        nrn_abort(1);
    }

    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        // only one element for each gid in this case
        std::vector<VarWithMapping> to_report;
        if (target.find(gid) != target.end()) {
            CellMapping* m = mapinfo->get_cell_mapping(gid);
            if (m == NULL) {
                std::cout << "[SOMA] Error : mapping information is missing for Soma Report! \n";
                nrn_abort(1);
            }
            /** get  section list mapping for soma */
            SecMapping* s = m->get_seclist_mapping("soma");
            /** 1st key is section-id and 1st value is segment of soma */
            int section_id = s->secmap.begin()->first;
            int idx = s->secmap.begin()->second.front();
            double* variable = report_variable + idx;
            to_report.push_back(VarWithMapping(section_id, variable));
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

VarsToReport get_compartment_vars_to_report(NrnThread& nt, std::set<int>& target, double* report_variable) {
    VarsToReport vars_to_report;
    NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;
    if (!mapinfo) {
        std::cout << "[COMPARTMENTS] Error : mapping information is missing for a Cell group "
                  << nt.ncell << std::endl;
        nrn_abort(1);
    }

    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (target.find(gid) != target.end()) {
            CellMapping* m = mapinfo->get_cell_mapping(gid);
            if (m == NULL) {
                std::cout
                    << "[COMPARTMENTS] Error : Compartment mapping information is missing! \n";
                nrn_abort(1);
            }
            std::vector<VarWithMapping> to_report;
            to_report.reserve(m->size());
            for (int j = 0; j < m->size(); j++) {
                SecMapping* s = m->secmapvec[j];
                for (secseg_it_type iterator = s->secmap.begin(); iterator != s->secmap.end();
                     iterator++) {
                    int compartment_id = iterator->first;
                    segvec_type& vec = iterator->second;
                    for (size_t k = 0; k < vec.size(); k++) {
                        int idx = vec[k];
                        /** corresponding voltage in coreneuron voltage array */
                        double* variable = report_variable + idx;
                        to_report.push_back(VarWithMapping(compartment_id, variable));
                    }
                }
            }
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

VarsToReport get_custom_vars_to_report(NrnThread& nt,
                                       ReportConfiguration& report,
                                       std::vector<int>& nodes_to_gids) {
    VarsToReport vars_to_report;
    for (int i = 0; i < nt.ncell; i++) {
        int gid = nt.presyns[i].gid_;
        if (report.target.find(gid) == report.target.end())
            continue;
        Memb_list* ml = nt._ml_list[report.mech_id];
        if (!ml)
            continue;
        std::vector<VarWithMapping> to_report;
        to_report.reserve(ml->nodecount);

        for (int j = 0; j < ml->nodecount; j++) {
            double* is_selected =
                get_var_location_from_var_name(report.mech_id, SELECTED_VAR_MOD_NAME, ml, j);
            bool report_variable = false;

            /// if there is no variable in mod file then report on every compartment
            /// otherwise check the flag set in mod file
            if (is_selected == NULL)
                report_variable = true;
            else
                report_variable = *is_selected;

            if ((nodes_to_gids[ml->nodeindices[j]] == gid) && report_variable) {
                double* var_value =
                    get_var_location_from_var_name(report.mech_id, report.var_name, ml, j);
                double* synapse_id =
                    get_var_location_from_var_name(report.mech_id, SYNAPSE_ID_MOD_NAME, ml, j);
                assert(synapse_id && var_value);
                to_report.push_back(VarWithMapping((int)*synapse_id, var_value));
            }
        }
        if (to_report.size()) {
            vars_to_report[gid] = to_report;
        }
    }
    return vars_to_report;
}

void register_soma_report(NrnThread& nt,
                          ReportConfiguration& config,
                          VarsToReport& vars_to_report) {
    int sizemapping = 1;
    int extramapping = 5;
    // first column i.e. section numbers
    int mapping[] = {0};
    // first row, from 2nd value (skip gid)
    int extra[] = {1, 0, 0, 0, 0};
    VarsToReport::iterator it;

    for (it = vars_to_report.begin(); it != vars_to_report.end(); ++it) {
        int gid = it->first;
        std::vector<VarWithMapping>& vars = it->second;
        if (!vars.size())
            continue;
        NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;
        CellMapping* m = mapinfo->get_cell_mapping(gid);

        /* report extra "mask" all infos not written in report: here only soma count is reported */
        extra[0] = vars.size();
        extra[1] = m->get_seclist_segment_count("soma");

        /** for this gid, get mapping information */
        records_add_report((char*)config.output_path, gid, gid, gid, config.start, config.stop,
                           config.report_dt, sizemapping, (char*)config.type_str, extramapping,
                           (char*)config.unit);

        records_set_report_max_buffer_size_hint((char*)config.output_path, config.buffer_size);
        /** add extra mapping */
        records_extra_mapping(config.output_path, gid, 5, extra);
        for (int var_idx = 0; var_idx < vars.size(); ++var_idx) {
            /** 1st key is section-id and 1st value is segment of soma */
            mapping[0] = vars[var_idx].id;
            records_add_var_with_mapping(config.output_path, gid, vars[var_idx].var_value,
                                         sizemapping, mapping);
        }
    }
}

void register_compartment_report(NrnThread& nt,
                                 ReportConfiguration& config,
                                 VarsToReport& vars_to_report) {
    int sizemapping = 1;
    int extramapping = 5;
    int mapping[] = {0};
    int extra[] = {1, 0, 0, 0, 1};

    VarsToReport::iterator it;
    for (it = vars_to_report.begin(); it != vars_to_report.end(); ++it) {
        int gid = it->first;
        std::vector<VarWithMapping>& vars = it->second;
        if (!vars.size())
            continue;
        NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;
        CellMapping* m = mapinfo->get_cell_mapping(gid);
        extra[1] = m->get_seclist_section_count("soma");
        extra[2] = m->get_seclist_section_count("axon");
        extra[3] = m->get_seclist_section_count("dend");
        extra[4] = m->get_seclist_section_count("apic");
        extra[0] = extra[1] + extra[2] + extra[3] + extra[4];
        records_add_report((char*)config.output_path, gid, gid, gid, config.start, config.stop,
                           config.report_dt, sizemapping, (char*)config.type_str, extramapping,
                           (char*)config.unit);

        records_set_report_max_buffer_size_hint((char*)config.output_path, config.buffer_size);
        /** add extra mapping */
        records_extra_mapping(config.output_path, gid, 5, extra);
        for (int var_idx = 0; var_idx < vars.size(); ++var_idx) {
            mapping[0] = vars[var_idx].id;
            records_add_var_with_mapping(config.output_path, gid, vars[var_idx].var_value,
                                         sizemapping, mapping);
        }
    }
}

void register_custom_report(NrnThread& nt,
                            ReportConfiguration& config,
                            VarsToReport& vars_to_report) {
    int sizemapping = 1;
    int extramapping = 5;
    int mapping[] = {0};
    int extra[] = {1, 0, 0, 0, 1};
    int segment_count = 0;
    int section_count = 0;

    VarsToReport::iterator it;
    for (it = vars_to_report.begin(); it != vars_to_report.end(); ++it) {
        int gid = it->first;
        std::vector<VarWithMapping>& vars = it->second;
        if (!vars.size())
            continue;
        NrnThreadMappingInfo* mapinfo = (NrnThreadMappingInfo*)nt.mapping;
        CellMapping* m = mapinfo->get_cell_mapping(gid);
        extra[1] = m->get_seclist_section_count("soma");
        // todo: axon seems masked on neurodamus side, need to check
        // extra[2] and extra[3]
        extra[4] = m->get_seclist_section_count("apic");
        extra[0] = extra[1] + extra[2] + extra[3] + extra[4];
        records_add_report((char*)config.output_path, gid, gid, gid, config.start, config.stop,
                           config.report_dt, sizemapping, (char*)config.type_str, extramapping,
                           (char*)config.unit);

        records_set_report_max_buffer_size_hint((char*)config.output_path, config.buffer_size);
        /** add extra mapping : @todo api changes in reportinglib*/
        records_extra_mapping((char*)config.output_path, gid, 5, extra);
        for (int var_idx = 0; var_idx < vars.size(); ++var_idx) {
            mapping[0] = vars[var_idx].id;
            records_add_var_with_mapping(config.output_path, gid, vars[var_idx].var_value,
                                         sizemapping, mapping);
        }
    }
}

// map GIDs of every compartment, it consist in a backward sweep then forward sweep algorithm
std::vector<int> map_gids(NrnThread& nt) {
    std::vector<int> nodes_gid(nt.end, -1);
    // backward sweep: from presyn compartment propagate back GID to parent
    for (int i = 0; i < nt.n_presyn; i++) {
        int gid = nt.presyns[i].gid_;
        int thvar_index = nt.presyns[i].thvar_index_;
        // only for non artificial cells
        if (thvar_index >= 0) {
            // setting all roots gids of the presyns nodes,
            // index 0 have parent set to 0, so we must stop at j > 0
            // also 0 is the parent of all, so it is an error to attribute a GID to it.
            nodes_gid[thvar_index] = gid;
            for (int j = thvar_index; j > 0; j = nt._v_parent_index[j]) {
                nodes_gid[nt._v_parent_index[j]] = gid;
            }
        }
    }
    // forward sweep: setting all compartements nodes to the GID of its root
    //  already sets on above loop. This is working only because compartments are stored in order
    //  parents follow by childrens
    for (int i = nt.ncell + 1; i < nt.end; i++) {
        nodes_gid[i] = nodes_gid[nt._v_parent_index[i]];
    }
    return nodes_gid;
}
#endif  // ENABLE_REPORTING

// Size in MB of the report buffer
static int size_report_buffer = 4;

void nrn_flush_reports(double t) {
#ifdef ENABLE_REPORTING
    // flush before buffer is full
    records_end_iteration(t);
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
#ifdef ENABLE_REPORTING
    /** reportinglib setup */
    int min_steps_to_record = static_cast<int>(std::round(mindelay / dt_report));
    records_set_min_steps_to_record(min_steps_to_record);
    records_setup_communicator();
    records_finish_and_share();
#endif  // ENABLE_REPORTING
}

// Size in MB of the report buffers
void set_report_buffer_size(int n) {
    size_report_buffer = n;
#ifdef ENABLE_REPORTING
    records_set_max_buffer_size_hint(size_report_buffer);
#endif
}

// TODO: we can have one ReportEvent per register_report_call generated by MPI rank
// instead of one per cell group
void register_report(double dt, double tstop, double delay, ReportConfiguration& report) {
#ifdef ENABLE_REPORTING

    if (report.start < t) {
        report.start = t;
    }
    report.stop = std::min(report.stop, tstop);

    records_set_atomic_step(dt);
    report.mech_id = nrn_get_mechtype(report.mech_name);
    if (report.type == SynapseReport && report.mech_id == -1) {
        std::cerr << "[ERROR] mechanism to report: " << report.mech_name
                  << " is not mapped in this simulation, cannot report on it" << std::endl;
        nrn_abort(1);
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        if (!nt.ncell)
            continue;
        std::vector<int> nodes_to_gid = map_gids(nt);
        VarsToReport vars_to_report;
        switch (report.type) {
            case SomaReport:
                vars_to_report = get_soma_vars_to_report(nt, report.target, nt._actual_v);
                register_soma_report(nt, report, vars_to_report);
                break;
            case CompartmentReport:
                vars_to_report = get_compartment_vars_to_report(nt, report.target, nt._actual_v);
                register_compartment_report(nt, report, vars_to_report);
                break;
            case IMembraneReport:
                vars_to_report = get_compartment_vars_to_report(nt, report.target, nt.nrn_fast_imem->nrn_sav_rhs);
                register_compartment_report(nt, report, vars_to_report);
                break;
            default:
                vars_to_report = get_custom_vars_to_report(nt, report, nodes_to_gid);
                register_custom_report(nt, report, vars_to_report);
        }
        if (vars_to_report.size()) {
            reports.push_back(new ReportEvent(dt, t, vars_to_report, report.output_path));
            reports[reports.size() - 1]->send(t, net_cvode_instance, &nt);
        }
    }
#else
    if (nrnmpi_myid == 0)
        printf("\n WARNING! : Can't enable reports, recompile with ReportingLib! \n");
#endif  // ENABLE_REPORTING
}

void finalize_report() {
#ifdef ENABLE_REPORTING
    records_flush(nrn_threads[0]._t);
    for (int i = 0; i < reports.size(); i++) {
        delete reports[i];
    }
    reports.clear();
#endif
}
}
