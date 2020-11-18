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

// Want to have the classical NEURON PatternStim functionality available
// in coreneuron to allow debugging and trajectory verification on
// desktop single process tests.  Since pattern.mod provides most of what
// we need even in the coreneuron context, we placed a minimally modified
// version of that in coreneuron/mechanism/mech/modfile/pattern.mod and this file
// provides an interface that creates an instance of the
// PatternStim ARTIFICIAL_CELL in thread 0 and attaches the spike raster
// data to it.

#include <algorithm>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/io/output_spikes.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/coreneuron.hpp"

namespace coreneuron {
void _pattern_reg(void);
extern void pattern_stim_setup_helper(int size,
                                      double* tvec,
                                      int* gidvec,
                                      int icnt,
                                      int cnt,
                                      double* _p,
                                      Datum* _ppvar,
                                      ThreadDatum* _thread,
                                      NrnThread* _nt,
                                      double v);

static size_t read_raster_file(const char* fname, double** tvec, int** gidvec, double tstop);

int nrn_extra_thread0_vdata;

void nrn_set_extra_thread0_vdata() {
    // limited to PatternStim for now.
    // if called, must be called before nrn_setup and after mk_mech.
    int type = nrn_get_mechtype("PatternStim");
    if (!corenrn.get_memb_func(type).initialize) {
        // the NEURON mod file version is not vectorized so the param size
        // differs by 1 from the coreneuron version.
        corenrn.get_prop_param_size()[type] += 1;
        _pattern_reg();
    }
    nrn_extra_thread0_vdata = corenrn.get_prop_dparam_size()[type];
}

// fname is the filename of an output_spikes.h format raster file.
// todo : add function for memory cleanup (to be called at the end of simulation)
void nrn_mkPatternStim(const char* fname, double tstop) {
    int type = nrn_get_mechtype("PatternStim");
    if (!corenrn.get_memb_func(type).sym) {
        printf("nrn_set_extra_thread_vdata must be called (after mk_mech, and before nrn_setup\n");
        assert(0);
    }

    // if there is empty thread then return, don't need patternstim
    if (nrn_threads == nullptr || nrn_threads->ncell == 0) {
        return;
    }

    double* tvec;
    int* gidvec;

    // todo : handle when spike raster will be very large (int < size_t)
    size_t size = read_raster_file(fname, &tvec, &gidvec, tstop);

    Point_process* pnt = nrn_artcell_instantiate("PatternStim");
    NrnThread* nt = nrn_threads + pnt->_tid;

    Memb_list* ml = nt->_ml_list[type];
    int layout = corenrn.get_mech_data_layout()[type];
    int sz = corenrn.get_prop_param_size()[type];
    int psz = corenrn.get_prop_dparam_size()[type];
    int _cntml = ml->nodecount;
    int _iml = pnt->_i_instance;
    double* _p = ml->data;
    Datum* _ppvar = ml->pdata;
    if (layout == 1) {
        _p += _iml * sz;
        _ppvar += _iml * psz;
    } else if (layout == 0) {
        ;
    } else {
        assert(0);
    }
    pattern_stim_setup_helper(size, tvec, gidvec, _iml, _cntml, _p, _ppvar, nullptr, nt, 0.0);
}

// comparator to sort spikes based on time
using spike_type = std::pair<double, int>;
static bool spike_comparator(const spike_type& l, const spike_type& r) {
    return l.first < r.first;
}


size_t read_raster_file(const char* fname, double** tvec, int** gidvec, double tstop) {
    FILE* f = fopen(fname, "r");
    nrn_assert(f);

    // skip first line containing "scatter" string
    char dummy[100];
    nrn_assert(fgets(dummy, 100, f));

    std::vector<spike_type> spikes;
    spikes.reserve(10000);

    double stime;
    int gid;

    while (fscanf(f, "%lf %d\n", &stime, &gid) == 2) {
        if ( stime >= t && stime <= tstop) {
            spikes.push_back(std::make_pair(stime, gid));
        }
    }

    fclose(f);

    // pattern.mod expects sorted spike raster (this is to avoid
    // injecting all events at the begining of the simulation).
    // sort spikes according to time
    std::sort(spikes.begin(), spikes.end(), spike_comparator);

    // fill gid and time vectors
    *tvec = (double*)emalloc(spikes.size() * sizeof(double));
    *gidvec = (int*)emalloc(spikes.size() * sizeof(int));

    for (size_t i = 0; i < spikes.size(); i++) {
        (*tvec)[i] = spikes[i].first;
        (*gidvec)[i] = spikes[i].second;
    }

    return spikes.size();
}

// see nrn_setup.cpp:read_phase2 for how it creates NrnThreadMembList instances.
static NrnThreadMembList* alloc_nrn_thread_memb(int type) {
    NrnThreadMembList* tml = (NrnThreadMembList*)emalloc(sizeof(NrnThreadMembList));
    tml->dependencies = nullptr;
    tml->ndependencies = 0;
    tml->index = type;
    tml->next = nullptr;

    // fill in tml->ml info. The data is not in the cache efficient
    // NrnThread arrays but there should not be many of these instances.
    int psize = corenrn.get_prop_param_size()[type];
    int dsize = corenrn.get_prop_dparam_size()[type];

    tml->ml = (Memb_list*)emalloc(sizeof(Memb_list));
    tml->ml->nodecount = 1;
    tml->ml->_nodecount_padded = tml->ml->nodecount;
    tml->ml->nodeindices = nullptr;
    tml->ml->data = (double*)ecalloc(tml->ml->nodecount * psize, sizeof(double));
    tml->ml->pdata = (Datum*)ecalloc(tml->ml->nodecount * dsize, sizeof(Datum));
    tml->ml->_thread = nullptr;
    tml->ml->_net_receive_buffer = nullptr;
    tml->ml->_net_send_buffer = nullptr;
    tml->ml->_permute = nullptr;

    return tml;
}

// Opportunistically implemented to create a single PatternStim.
// So only does enough to get that functionally incorporated into the model
// and other types may require additional work. In particular, we
// append a new NrnThreadMembList with one item to the thread 0 tml list
// in order for the artificial cell to get its INITIAL block called but
// we do not modify any of the other thread 0 data arrays or counts.

Point_process* nrn_artcell_instantiate(const char* mechname) {
    int type = nrn_get_mechtype(mechname);
    NrnThread* nt = nrn_threads + 0;

    // printf("nrn_artcell_instantiate %s type=%d\n", mechname, type);

    // create and append to nt.tml
    auto tml = alloc_nrn_thread_memb(type);

    assert(nt->_ml_list[type] == nullptr);  // FIXME
    nt->_ml_list[type] = tml->ml;

    if (!nt->tml) {
        nt->tml = tml;
    } else {
        for (NrnThreadMembList* i = nt->tml; i; i = i->next) {
            if (!i->next) {
                i->next = tml;
                break;
            }
        }
    }

    // Here we have a problem with no easy general solution. ml->pdata are
    // integer indexes into the nt->_data nt->_idata and nt->_vdata array
    // depending on context,
    // but nrn_setup.cpp allocated these to exactly have the size needed by
    // the file defined model (at least for _vdata) and so there are no slots
    // for pdata to index into for this new instance.
    // So nrn_setup.cpp:phase2 needs to
    // be notified that some extra space will be required. For now, defer
    // the general situation of several instances for several types and
    // demand that this method is never called more than once. We introduce
    // a int nrn_extra_thread0_vdata (only that is needed by PatternStim)
    //  which will be used by
    // nrn_setup.cpp:phase2 to allocate the appropriately larger
    // _vdata arrays for thread 0 (without changing _nvdata so
    // that we can fill in the indices here)
    static int cnt = 0;
    if (++cnt > 1) {
        printf("nrn_artcell_instantiate cannot be called more than once\n");
        assert(0);
    }
    // note that PatternStim internal usage for the 4 ppvar values  is:
    // #define _nd_area  _nt->_data[_ppvar[0]]  (not used since ARTIFICIAL_CELL)
    // #define _p_ptr  _nt->_vdata[_ppvar[2]] (the BBCORE_POINTER)
    // #define _tqitem &(_nt->_vdata[_ppvar[3]]) (for net_send)
    // and general external usage is:
    // _nt->_vdata[_ppvar[1]] = Point_process*
    //

    Point_process* pnt = new Point_process;
    pnt->_type = type;
    pnt->_tid = nt->id;
    pnt->_i_instance = 0;
    // as though all dparam index into _vdata
    int dsize = corenrn.get_prop_dparam_size()[type];
    assert(dsize <= nrn_extra_thread0_vdata);
    for (int i = 0; i < dsize; ++i) {
        tml->ml->pdata[i] = nt->_nvdata + i;
    }
    nt->_vdata[nt->_nvdata + 1] = (void*)pnt;

    return pnt;
}
}  // namespace coreneuron
