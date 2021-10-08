/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

#include "coreneuron/nrnconf.h"
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpidec.h"

#include "coreneuron/network/netcon.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/utils/ivocvect.hpp"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/utils/utils.hpp"

#if NRNMPI
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/core/nrnmpi.hpp"
int localgid_size_;
int ag_send_nspike;
int* nrnmpi_nin_;
int ovfl_capacity;
int icapacity;
unsigned char* spikeout_fixed;
unsigned char* spfixin_ovfl_;
unsigned char* spikein_fixed;
int ag_send_size;
int ovfl;
int nout;
coreneuron::NRNMPI_Spikebuf* spbufout;
coreneuron::NRNMPI_Spikebuf* spbufin;
#endif

namespace coreneuron {
class PreSyn;
class InputPreSyn;

static double t_exchange_;
static double dt1_;  // 1/dt

void nrn_spike_exchange_init();

#if NRNMPI
NRNMPI_Spike* spikeout;
NRNMPI_Spike* spikein;

void nrn_timeout(int);
void nrn_spike_exchange(NrnThread*);
void nrn2ncs_outputevent(int netcon_output_index, double firetime);

// for compressed gid info during spike exchange
bool nrn_use_localgid_;
void nrn_outputevent(unsigned char localgid, double firetime);
std::vector<std::map<int, InputPreSyn*>> localmaps;

static int ocapacity_;  // for spikeout
// require it to be smaller than  min_interprocessor_delay.
static double wt_;   // wait time for nrnmpi_spike_exchange
static double wt1_;  // time to find the PreSyns and send the spikes.
static bool use_compress_;
static int spfixout_capacity_;
static int idxout_;
static void nrn_spike_exchange_compressed(NrnThread*);

#endif  // NRNMPI

static bool active_ = false;
static double usable_mindelay_;
static double mindelay_;  // the one actually used. Some of our optional algorithms
static double last_maxstep_arg_;
static std::vector<NetParEvent> npe_;  // nrn_nthread of them

#if NRNMPI
// for combination of threads and mpi.
static OMP_Mutex mut;
#endif

/// Allocate space for spikes: 200 structs of {int gid; double time}
/// coming from nrnmpi.h and array of int of the global domain size
static void alloc_mpi_space() {
#if NRNMPI
    if (corenrn_param.mpi_enable && !spikeout) {
        ocapacity_ = 100;
        spikeout = (NRNMPI_Spike*) emalloc(ocapacity_ * sizeof(NRNMPI_Spike));
        icapacity = 100;
        spikein = (NRNMPI_Spike*) malloc(icapacity * sizeof(NRNMPI_Spike));
        nrnmpi_nin_ = (int*) emalloc(nrnmpi_numprocs * sizeof(int));
#if nrn_spikebuf_size > 0
        spbufout = (NRNMPI_Spikebuf*) emalloc(sizeof(NRNMPI_Spikebuf));
        spbufin = (NRNMPI_Spikebuf*) emalloc(nrnmpi_numprocs * sizeof(NRNMPI_Spikebuf));
#endif
    }
#endif
}

NetParEvent::NetParEvent()
    : ithread_(-1)
    , wx_(0.)
    , ws_(0.) {}

void NetParEvent::send(double tt, NetCvode* nc, NrnThread* nt) {
    nc->event(tt + usable_mindelay_, this, nt);
}

void NetParEvent::deliver(double tt, NetCvode* nc, NrnThread* nt) {
    net_cvode_instance->deliver_events(tt, nt);
    nt->_stop_stepping = 1;
    nt->_t = tt;
    send(tt, nc, nt);
}

void NetParEvent::pr(const char* m, double tt, NetCvode*) {
    printf("%s NetParEvent %d t=%.15g tt-t=%g\n", m, ithread_, tt, tt - nrn_threads[ithread_]._t);
}

#if NRNMPI
inline static void sppk(unsigned char* c, int gid) {
    for (int i = localgid_size_ - 1; i >= 0; --i) {
        c[i] = gid & 255;
        gid >>= 8;
    }
}
inline static int spupk(unsigned char* c) {
    int gid = *c++;
    for (int i = 1; i < localgid_size_; ++i) {
        gid <<= 8;
        gid += *c++;
    }
    return gid;
}

void nrn_outputevent(unsigned char localgid, double firetime) {
    if (!active_) {
        return;
    }
    std::lock_guard<OMP_Mutex> lock(mut);
    nout++;
    int i = idxout_;
    idxout_ += 2;
    if (idxout_ >= spfixout_capacity_) {
        spfixout_capacity_ *= 2;
        spikeout_fixed = (unsigned char*) erealloc(spikeout_fixed,
                                                   spfixout_capacity_ * sizeof(unsigned char));
    }
    spikeout_fixed[i++] = (unsigned char) ((firetime - t_exchange_) * dt1_ + .5);
    spikeout_fixed[i] = localgid;
    // printf("%d idx=%d lgid=%d firetime=%g t_exchange_=%g [0]=%d [1]=%d\n", nrnmpi_myid, i,
    // (int)localgid, firetime, t_exchange_, (int)spikeout_fixed[i-1], (int)spikeout_fixed[i]);
}

void nrn2ncs_outputevent(int gid, double firetime) {
    if (!active_) {
        return;
    }
    std::lock_guard<OMP_Mutex> lock(mut);
    if (use_compress_) {
        nout++;
        int i = idxout_;
        idxout_ += 1 + localgid_size_;
        if (idxout_ >= spfixout_capacity_) {
            spfixout_capacity_ *= 2;
            spikeout_fixed = (unsigned char*) erealloc(spikeout_fixed,
                                                       spfixout_capacity_ * sizeof(unsigned char));
        }
        // printf("%d nrnncs_outputevent %d %.20g %.20g %d\n", nrnmpi_myid, gid, firetime,
        // t_exchange_,
        //(int)((unsigned char)((firetime - t_exchange_)*dt1_ + .5)));
        spikeout_fixed[i++] = (unsigned char) ((firetime - t_exchange_) * dt1_ + .5);
        // printf("%d idx=%d firetime=%g t_exchange_=%g spfixout=%d\n", nrnmpi_myid, i, firetime,
        // t_exchange_, (int)spikeout_fixed[i-1]);
        sppk(spikeout_fixed + i, gid);
        // printf("%d idx=%d gid=%d spupk=%d\n", nrnmpi_myid, i, gid, spupk(spikeout_fixed+i));
    } else {
#if nrn_spikebuf_size == 0
        int i = nout++;
        if (i >= ocapacity_) {
            ocapacity_ *= 2;
            spikeout = (NRNMPI_Spike*) erealloc(spikeout, ocapacity_ * sizeof(NRNMPI_Spike));
        }
        // printf("%d cell %d in slot %d fired at %g\n", nrnmpi_myid, gid, i, firetime);
        spikeout[i].gid = gid;
        spikeout[i].spiketime = firetime;
#else
        int i = nout++;
        if (i >= nrn_spikebuf_size) {
            i -= nrn_spikebuf_size;
            if (i >= ocapacity_) {
                ocapacity_ *= 2;
                spikeout = (NRNMPI_Spike*) hoc_Erealloc(spikeout,
                                                        ocapacity_ * sizeof(NRNMPI_Spike));
                hoc_malchk();
            }
            spikeout[i].gid = gid;
            spikeout[i].spiketime = firetime;
        } else {
            spbufout->gid[i] = gid;
            spbufout->spiketime[i] = firetime;
        }
#endif
    }
    // printf("%d cell %d in slot %d fired at %g\n", nrnmpi_myid, gid, i, firetime);
}
#endif  // NRNMPI

static bool nrn_need_npe() {
    if (active_ || nrn_nthread > 1) {
        if (last_maxstep_arg_ == 0) {
            last_maxstep_arg_ = 100.;
        }
        return true;
    } else {
        if (!npe_.empty()) {
            npe_.clear();
            npe_.shrink_to_fit();
        }
        return false;
    }
}

#define TBUFSIZE 0

void nrn_spike_exchange_init() {
    // printf("nrn_spike_exchange_init\n");
    if (!nrn_need_npe()) {
        return;
    }
    alloc_mpi_space();
    usable_mindelay_ = mindelay_;
#if NRN_MULTISEND
    if (use_multisend_ && n_multisend_interval == 2) {
        usable_mindelay_ *= 0.5;
    }
#endif
    if (nrn_nthread > 1) {
        usable_mindelay_ -= dt;
    }
    if ((usable_mindelay_ < 1e-9) || (usable_mindelay_ < dt)) {
        if (nrnmpi_myid == 0) {
            hoc_execerror("usable mindelay is 0", "(or less than dt for fixed step method)");
        } else {
            return;
        }
    }

#if TBUFSIZE
    itbuf_ = 0;
#endif

#if NRN_MULTISEND
    if (use_multisend_) {
        nrn_multisend_init();
    }
#endif

    if (npe_.size() != nrn_nthread) {
        if (!npe_.empty()) {
            npe_.clear();
            npe_.shrink_to_fit();
        }
        npe_.resize(nrn_nthread);
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        npe_[i].ithread_ = i;
        npe_[i].wx_ = 0.;
        npe_[i].ws_ = 0.;
        npe_[i].send(t, net_cvode_instance, nrn_threads + i);
    }
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        if (use_compress_) {
            idxout_ = 2;
            t_exchange_ = t;
            dt1_ = rev_dt;
            usable_mindelay_ = floor(mindelay_ * dt1_ + 1e-9) * dt;
            assert(usable_mindelay_ >= dt && (usable_mindelay_ * dt1_) < 255);
        } else {
#if nrn_spikebuf_size > 0
            if (spbufout) {
                spbufout->nspike = 0;
            }
#endif
        }
        nout = 0;
    }
#endif  // NRNMPI
        // if (nrnmpi_myid == 0){printf("usable_mindelay_ = %g\n", usable_mindelay_);}
}

#if NRNMPI
void nrn_spike_exchange(NrnThread* nt) {
    if (!active_) {
        return;
    }
#if NRN_MULTISEND
    if (use_multisend_) {
        nrn_multisend_receive(nt);
        return;
    }
#endif
    if (use_compress_) {
        nrn_spike_exchange_compressed(nt);
        return;
    }
#if TBUFSIZE
    nrnmpi_barrier();
#endif

#if nrn_spikebuf_size > 0
    spbufout->nspike = nout;
#endif
    double wt = nrn_wtime();

    int n = nrnmpi_spike_exchange(
        nrnmpi_nin_, spikeout, icapacity, spikein, ovfl, nout, spbufout, spbufin);

    wt_ = nrn_wtime() - wt;
    wt = nrn_wtime();
#if TBUFSIZE
    tbuf_[itbuf_++] = (unsigned long) nout;
    tbuf_[itbuf_++] = (unsigned long) n;
#endif

    errno = 0;
    // if (n > 0) {
    // printf("%d nrn_spike_exchange sent %d received %d\n", nrnmpi_myid, nout, n);
    //}
    nout = 0;
    if (n == 0) {
        return;
    }
#if nrn_spikebuf_size > 0
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        int nn = spbufin[i].nspike;
        if (nn > nrn_spikebuf_size) {
            nn = nrn_spikebuf_size;
        }
        for (int j = 0; j < nn; ++j) {
            auto gid2in_it = gid2in.find(spbufin[i].gid[j]);
            if (gid2in_it != gid2in.end()) {
                InputPreSyn* ps = gid2in_it->second;
                ps->send(spbufin[i].spiketime[j], net_cvode_instance, nt);
            }
        }
    }
    n = ovfl;
#endif  // nrn_spikebuf_size > 0
    for (int i = 0; i < n; ++i) {
        auto gid2in_it = gid2in.find(spikein[i].gid);
        if (gid2in_it != gid2in.end()) {
            InputPreSyn* ps = gid2in_it->second;
            ps->send(spikein[i].spiketime, net_cvode_instance, nt);
        }
    }
    wt1_ = nrn_wtime() - wt;
}

void nrn_spike_exchange_compressed(NrnThread* nt) {
    if (!active_) {
        return;
    }
#if TBUFSIZE
    nrnmpi_barrier();
#endif

    assert(nout < 0x10000);
    spikeout_fixed[1] = (unsigned char) (nout & 0xff);
    spikeout_fixed[0] = (unsigned char) (nout >> 8);

    double wt = nrn_wtime();

    int n = nrnmpi_spike_exchange_compressed(localgid_size_,
                                             spfixin_ovfl_,
                                             ag_send_nspike,
                                             nrnmpi_nin_,
                                             ovfl_capacity,
                                             spikeout_fixed,
                                             ag_send_size,
                                             spikein_fixed,
                                             ovfl);
    wt_ = nrn_wtime() - wt;
    wt = nrn_wtime();
#if TBUFSIZE
    tbuf_[itbuf_++] = (unsigned long) nout;
    tbuf_[itbuf_++] = (unsigned long) n;
#endif
    errno = 0;
    // if (n > 0) {
    // printf("%d nrn_spike_exchange sent %d received %d\n", nrnmpi_myid, nout, n);
    //}
    nout = 0;
    idxout_ = 2;
    if (n == 0) {
        t_exchange_ = nrn_threads->_t;
        return;
    }
    if (nrn_use_localgid_) {
        int idxov = 0;
        for (int i = 0; i < nrnmpi_numprocs; ++i) {
            int j, nnn;
            int nn = nrnmpi_nin_[i];
            if (nn) {
                if (i == nrnmpi_myid) {  // skip but may need to increment idxov.
                    if (nn > ag_send_nspike) {
                        idxov += (nn - ag_send_nspike) * (1 + localgid_size_);
                    }
                    continue;
                }
                std::map<int, InputPreSyn*> gps = localmaps[i];
                if (nn > ag_send_nspike) {
                    nnn = ag_send_nspike;
                } else {
                    nnn = nn;
                }
                int idx = 2 + i * ag_send_size;
                for (j = 0; j < nnn; ++j) {
                    // order is (firetime,gid) pairs.
                    double firetime = spikein_fixed[idx++] * dt + t_exchange_;
                    int lgid = (int) spikein_fixed[idx];
                    idx += localgid_size_;
                    auto gid2in_it = gps.find(lgid);
                    if (gid2in_it != gps.end()) {
                        InputPreSyn* ps = gid2in_it->second;
                        ps->send(firetime + 1e-10, net_cvode_instance, nt);
                    }
                }
                for (; j < nn; ++j) {
                    double firetime = spfixin_ovfl_[idxov++] * dt + t_exchange_;
                    int lgid = (int) spfixin_ovfl_[idxov];
                    idxov += localgid_size_;
                    auto gid2in_it = gps.find(lgid);
                    if (gid2in_it != gps.end()) {
                        InputPreSyn* ps = gid2in_it->second;
                        ps->send(firetime + 1e-10, net_cvode_instance, nt);
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < nrnmpi_numprocs; ++i) {
            int nn = nrnmpi_nin_[i];
            if (nn > ag_send_nspike) {
                nn = ag_send_nspike;
            }
            int idx = 2 + i * ag_send_size;
            for (int j = 0; j < nn; ++j) {
                // order is (firetime,gid) pairs.
                double firetime = spikein_fixed[idx++] * dt + t_exchange_;
                int gid = spupk(spikein_fixed + idx);
                idx += localgid_size_;
                auto gid2in_it = gid2in.find(gid);
                if (gid2in_it != gid2in.end()) {
                    InputPreSyn* ps = gid2in_it->second;
                    ps->send(firetime + 1e-10, net_cvode_instance, nt);
                }
            }
        }
        n = ovfl;
        int idx = 0;
        for (int i = 0; i < n; ++i) {
            double firetime = spfixin_ovfl_[idx++] * dt + t_exchange_;
            int gid = spupk(spfixin_ovfl_ + idx);
            idx += localgid_size_;
            auto gid2in_it = gid2in.find(gid);
            if (gid2in_it != gid2in.end()) {
                InputPreSyn* ps = gid2in_it->second;
                ps->send(firetime + 1e-10, net_cvode_instance, nt);
            }
        }
    }
    t_exchange_ = nrn_threads->_t;
    wt1_ = nrn_wtime() - wt;
}

static void mk_localgid_rep() {
    // how many gids are there on this machine
    // and can they be compressed into one byte
    int ngid = 0;
    for (const auto& gid2out_elem: gid2out) {
        if (gid2out_elem.second->output_index_ >= 0) {
            ++ngid;
        }
    }

    int ngidmax = nrnmpi_int_allmax(ngid);
    if (ngidmax > 256) {
        // do not compress
        return;
    }
    localgid_size_ = sizeof(unsigned char);
    nrn_use_localgid_ = true;

    // allocate Allgather receive buffer (send is the nrnmpi_myid one)
    int* rbuf = new int[nrnmpi_numprocs * (ngidmax + 1)];
    int* sbuf = new int[ngidmax + 1];

    sbuf[0] = ngid;
    ++sbuf;
    ngid = 0;
    // define the local gid and fill with the gids on this machine
    for (const auto& gid2out_elem: gid2out) {
        if (gid2out_elem.second->output_index_ >= 0) {
            gid2out_elem.second->localgid_ = (unsigned char) ngid;
            sbuf[ngid] = gid2out_elem.second->output_index_;
            ++ngid;
        }
    }
    --sbuf;

    // exchange everything
    nrnmpi_int_allgather(sbuf, rbuf, ngidmax + 1);
    delete[] sbuf;
    errno = 0;

    // create the maps
    // there is a lot of potential for efficiency here. i.e. use of
    // perfect hash functions, or even simple Vectors.
    localmaps.clear();
    localmaps.resize(nrnmpi_numprocs);

    // fill in the maps
    for (int i = 0; i < nrnmpi_numprocs; ++i)
        if (i != nrnmpi_myid) {
            sbuf = rbuf + i * (ngidmax + 1);
            ngid = *(sbuf++);
            for (int k = 0; k < ngid; ++k) {
                auto gid2in_it = gid2in.find(int(sbuf[k]));
                if (gid2in_it != gid2in.end()) {
                    localmaps[i][k] = gid2in_it->second;
                }
            }
        }

    // cleanup
    delete[] rbuf;
}

#endif  // NRNMPI

// may stimulate a gid for a cell not owned by this cpu. This allows
// us to run single cells or subnets and stimulate exactly according to
// their input in a full parallel net simulation.
// For some purposes, it may be useful to simulate a spike from a
// cell that does exist and would normally send its own spike, eg.
// recurrent stimulation. This can be useful in debugging where the
// spike raster comes from another implementation and one wants to
// get complete control of all input spikes without the confounding
// effects of output spikes from the simulated cells. In this case
// set the third arg to 1 and set the output cell thresholds very
// high so that they do not themselves generate spikes.
// Can only be called by thread 0 because of the ps->send.
void nrn_fake_fire(int gid, double spiketime, int fake_out) {
    auto gid2in_it = gid2in.find(gid);
    if (gid2in_it != gid2in.end()) {
        InputPreSyn* psi = gid2in_it->second;
        assert(psi);
        // printf("nrn_fake_fire %d %g\n", gid, spiketime);
        psi->send(spiketime, net_cvode_instance, nrn_threads);
    } else if (fake_out) {
        std::map<int, PreSyn*>::iterator gid2out_it;
        gid2out_it = gid2out.find(gid);
        if (gid2out_it != gid2out.end()) {
            PreSyn* ps = gid2out_it->second;
            assert(ps);
            // printf("nrn_fake_fire fake_out %d %g\n", gid, spiketime);
            ps->send(spiketime, net_cvode_instance, nrn_threads);
        }
    }
}

static int timeout_ = 0;
int nrn_set_timeout(int timeout) {
    int tt = timeout_;
    timeout_ = timeout;
    return tt;
}

void BBS_netpar_solve(double tstop) {
    double time = nrn_wtime();

#if NRNMPI
    if (corenrn_param.mpi_enable) {
        tstopunset;
        double mt = dt;
        double md = mindelay_ - 1e-10;
        if (md < mt) {
            if (nrnmpi_myid == 0) {
                hoc_execerror("mindelay is 0", "(or less than dt for fixed step method)");
            } else {
                return;
            }
        }

        nrn_timeout(timeout_);
        ncs2nrn_integrate(tstop * (1. + 1e-11));
        nrn_spike_exchange(nrn_threads);
        nrn_timeout(0);
        if (!npe_.empty()) {
            npe_[0].wx_ = npe_[0].ws_ = 0.;
        };
        // printf("%d netpar_solve exit t=%g tstop=%g mindelay_=%g\n",nrnmpi_myid, t, tstop,
        // mindelay_);
        nrnmpi_barrier();
    } else
#endif
    {
        ncs2nrn_integrate(tstop);
    }
    tstopunset;

    if (nrnmpi_myid == 0 && !corenrn_param.is_quiet()) {
        printf("\nSolver Time : %g\n", nrn_wtime() - time);
    }
}

double set_mindelay(double maxdelay) {
    double mindelay = maxdelay;
    last_maxstep_arg_ = maxdelay;

    // if all==1 then minimum delay of all NetCon no matter the source.
    // except if src in same thread as NetCon
    int all = (nrn_nthread > 1);
    // minumum delay of all NetCon having an InputPreSyn source

    /** we have removed nt_ from PreSyn. Build local map of PreSyn
     *  and NrnThread which will be used to find out if src in same thread as NetCon */
    std::map<PreSyn*, NrnThread*> presynmap;

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        for (int i = 0; i < nt.n_presyn; ++i) {
            presynmap[nt.presyns + i] = nrn_threads + ith;
        }
    }

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        // if single thread or file transfer then definitely empty.
        std::vector<int>& negsrcgid_tid = nrnthreads_netcon_negsrcgid_tid[ith];
        size_t i_tid = 0;
        for (int i = 0; i < nt.n_netcon; ++i) {
            NetCon* nc = nt.netcons + i;
            bool chk = false;  // ignore nc.delay_
            int gid = nrnthreads_netcon_srcgid[ith][i];
            int tid = ith;
            if (!negsrcgid_tid.empty() && gid < -1) {
                tid = negsrcgid_tid[i_tid++];
            }
            PreSyn* ps;
            InputPreSyn* psi;
            netpar_tid_gid2ps(tid, gid, &ps, &psi);
            if (psi) {
                chk = true;
            } else if (all) {
                chk = true;
                // but ignore if src in same thread as NetCon
                if (ps && presynmap[ps] == &nt) {
                    chk = false;
                }
            }
            if (chk && nc->delay_ < mindelay) {
                mindelay = nc->delay_;
            }
        }
    }

#if NRNMPI
    if (corenrn_param.mpi_enable) {
        active_ = true;
        if (use_compress_) {
            if (mindelay / dt > 255) {
                mindelay = 255 * dt;
            }
        }

        // printf("%d netpar_mindelay local %g now calling nrnmpi_mindelay\n", nrnmpi_myid,
        // mindelay);
        //	double st = time();
        mindelay_ = nrnmpi_dbl_allmin(mindelay);
        //	add_wait_time(st);
        // printf("%d local min=%g  global min=%g\n", nrnmpi_myid, mindelay, mindelay_);
        errno = 0;
    } else
#endif  // NRNMPI
    {
        mindelay_ = mindelay;
    }
    return mindelay_;
}

/*  08-Nov-2010
The workhorse for spike exchange on up to 10K machines is MPI_Allgather
but as the number of machines becomes far greater than the fanout per
cell we have been exploring a class of exchange methods called multisend
where the spikes only go to those machines that need them and there is
overlap between communication and computation.  The numer of variants of
multisend has grown so that some method selection function is needed
that makes sense.

The situation that needs to be captured by xchng_meth is

Allgather
multisend implemented as MPI_ISend
multisend DCMF (only for Blue Gene/P)
multisend record_replay (only for Blue Gene/P with recordreplay_v1r4m2.patch)

Note that Allgather allows spike compression and an allgather spike buffer
 with size chosen at setup time.  All methods allow bin queueing.

All the multisend methods should allow two phase multisend.

Note that, in principle, MPI_ISend allows the source to send the index
 of the target PreSyn to avoid a hash table lookup (even with a two phase
 variant)

RecordReplay should be best on the BG/P. The whole point is to make the
spike transfer initiation as lowcost as possible since that is what causes
most load imbalance. I.e. since 10K more spikes arrive than are sent, spikes
received per processor per interval are much more statistically
balanced than spikes sent per processor per interval. And presently
DCMF multisend injects 10000 messages per spike into the network which
is quite expensive. record replay avoids this overhead and the idea of
two phase multisend distributes the injection.
*/

int nrnmpi_spike_compress(int nspike, bool gid_compress, int xchng_meth) {
#if NRNMPI
    if (corenrn_param.mpi_enable) {
#if NRN_MULTISEND
        if (xchng_meth > 0) {
            use_multisend_ = 1;
            return 0;
        }
#endif
        nrn_assert(xchng_meth == 0);
        if (nspike >= 0) {
            ag_send_nspike = 0;
            if (spikeout_fixed) {
                free(spikeout_fixed);
                spikeout_fixed = nullptr;
            }
            if (spikein_fixed) {
                free(spikein_fixed);
                spikein_fixed = nullptr;
            }
            if (spfixin_ovfl_) {
                free(spfixin_ovfl_);
                spfixin_ovfl_ = nullptr;
            }
            localmaps.clear();
        }
        if (nspike == 0) {  // turn off
            use_compress_ = false;
            nrn_use_localgid_ = false;
        } else if (nspike > 0) {  // turn on
            use_compress_ = true;
            ag_send_nspike = nspike;
            nrn_use_localgid_ = false;
            if (gid_compress) {
                // we can only do this after everything is set up
                mk_localgid_rep();
                if (!nrn_use_localgid_ && nrnmpi_myid == 0) {
                    printf(
                        "Notice: gid compression did not succeed. Probably more than 255 cells on "
                        "one "
                        "cpu.\n");
                }
            }
            if (!nrn_use_localgid_) {
                localgid_size_ = sizeof(unsigned int);
            }
            ag_send_size = 2 + ag_send_nspike * (1 + localgid_size_);
            spfixout_capacity_ = ag_send_size + 50 * (1 + localgid_size_);
            spikeout_fixed = (unsigned char*) emalloc(spfixout_capacity_);
            spikein_fixed = (unsigned char*) emalloc(nrnmpi_numprocs * ag_send_size);
            ovfl_capacity = 100;
            spfixin_ovfl_ = (unsigned char*) emalloc(ovfl_capacity * (1 + localgid_size_));
        }
        return ag_send_nspike;
    } else
#endif
    {
        return 0;
    }
}
}  // namespace coreneuron
