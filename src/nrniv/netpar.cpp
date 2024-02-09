#include "utils/profile/profiler_interface.h"
#include <../../nrnconf.h>
#include <InterViews/resource.h>
#include <math.h>
#include "nrncvode.h"
#include "nrniv_mf.h"
#include <nrnmpi.h>
#include <nrnoc2iv.h>
#include <stdio.h>
#include <stdlib.h>
#include <netpar.h>
#include <unordered_map>
#include <bbs.h>

#undef MD
#define MD 2147483648.

class PreSyn;

// hash table where buckets are binary search maps
using Gid2PreSyn = std::unordered_map<int, PreSyn*>;

#include <errno.h>
#include <netcon.h>
#include <cvodeobj.h>
#include <netcvode.h>
#include "ivocvect.h"

#include <atomic>
#include <vector>

static int n_multisend_interval;

#if NRN_MUSIC
#include "nrnmusicapi.h"
int nrnmusic;
#endif

static Symbol* netcon_sym_;
static Gid2PreSyn gid2out_;
static Gid2PreSyn gid2in_;
static IvocVect* all_spiketvec = NULL;
static IvocVect* all_spikegidvec = NULL;
static double t_exchange_;
static double dt1_;  // 1/dt
static int localgid_size_;
static int ag_send_size_;
static int ag_send_nspike_;
static int ovfl_capacity_;
static int ovfl_;
static unsigned char* spfixout_;
static unsigned char* spfixin_;
static unsigned char* spfixin_ovfl_;
static int nout_;
static int* nin_;
static NRNMPI_Spike* spikeout_;
static NRNMPI_Spike* spikein_;
static int icapacity_;
static void alloc_space();

extern NetCvode* net_cvode_instance;
extern double t, dt;
extern int nrn_use_selfqueue_;
extern void nrn_pending_selfqueue(double, NrnThread*);
extern Object* nrn_sec2cell(Section*);
extern void ncs2nrn_integrate(double tstop);
int nrnmpi_spike_compress(int nspike, bool gid_compress, int xchng_meth);
void nrn_cleanup_presyn(PreSyn*);
int nrn_set_timeout(int);
void nrnmpi_gid_clear(int);
extern void nrn_partrans_clear();
void nrn_spike_exchange_init();
extern double nrn_multisend_receive_time(int);
typedef void (*PFIO)(int, Object*);
extern void nrn_gidout_iter(PFIO);
extern Object* nrn_gid2obj(int);
extern PreSyn* nrn_gid2presyn(int);
extern int nrn_gid_exists(int);
extern double nrnmpi_step_wait_;  // barrier at beginning of spike exchange.

/**
 * @brief NEURON callback used from CORENEURON to transfer all spike vectors after simulation
 *
 * @param spiketvec - vector of spikes
 * @param spikegidvec - vector of gids
 * @return 1 if CORENEURON shall drop writing `out.dat`; 0 otherwise
 */
extern "C" int nrnthread_all_spike_vectors_return(std::vector<double>& spiketvec,
                                                  std::vector<int>& spikegidvec);

#if NRNMPI == 0
double nrn_multisend_receive_time(int) {
    return 0.;
}
#endif

#if NRNMPI
extern void nrnmpi_split_clear();
#endif
extern void nrnmpi_multisplit_clear();

static double set_mindelay(double maxdelay);

#if NRNMPI

#include "../nrnmpi/mpispike.h"

void nrn_timeout(int);
extern int nrnmpi_int_allmax(int);
extern void nrnmpi_int_allgather(int*, int*, int);
void nrn2ncs_outputevent(int netcon_output_index, double firetime);
bool nrn_use_compress_;  // global due to bbsavestate
#define use_compress_ nrn_use_compress_

#ifdef USENCS
extern int ncs_multisend_sending_info(int**);
extern int ncs_multisend_target_hosts(int, int**);
extern int ncs_multisend_target_info(int**);
extern int ncs_multisend_mindelays(int**, double**);

// get minimum delays for all presyn objects in gid2in_
int ncs_netcon_mindelays(int** hosts, double** delays) {
    return ncs_multisend_mindelays(hosts, delays);
}

double ncs_netcon_localmindelay(int srcgid) {
    PreSyn* ps = gid2out_.at(srcgid);
    return ps->mindelay();
}

// get the number of netcons for an object, if it sends here
int ncs_netcon_count(int srcgid, bool localNetCons) {
    const auto& map = localNetCons ? gid2out_ : gid2in_;
    const auto& iter = map.find(srcgid);
    PreSyn* ps{iter != map.end() ? iter->second : nullptr};

    if (!ps) {  // no cells on this cpu receive from the given gid
        fprintf(stderr, "should never happen!\n");
        return 0;
    }

    return ps->dil_.count();
}

// inject a spike into the appropriate netcon
void ncs_netcon_inject(int srcgid, int netconIndex, double spikeTime, bool localNetCons) {
    NetCvode* ns = net_cvode_instance;
    const auto& map = localNetCons ? gid2out_ : gid2in_;
    const auto& iter = map.find(srcgid);
    PreSyn* ps{iter != map.end() ? iter->second : nullptr};
    if (!ps) {  // no cells on this cpu receive from the given gid
        return;
    }

    // fprintf( stderr, "gid %d index %d!\n", srcgid, netconIndex );
    NetCon* d = ps->dil_.item(netconIndex);
    NrnThread* nt = nrn_threads;
    if (d->active_ && d->target_) {
#if BBTQ == 5
        ns->bin_event(spikeTime + d->delay_, d, nt);
#else
        ns->event(spikeTime + d->delay_, d, nt);
#endif
    }
}

int ncs_gid_receiving_info(int** presyngids) {
    return ncs_multisend_target_info(presyngids);
}

// given the gid of a cell, retrieve its target count
int ncs_gid_sending_count(int** sendlist2build) {
    return ncs_multisend_sending_info(sendlist2build);
}

int ncs_target_hosts(int gid, int** targetnodes) {
    return ncs_multisend_target_hosts(gid, targetnodes);
}

#endif

// for compressed gid info during spike exchange
bool nrn_use_localgid_;
void nrn_outputevent(unsigned char localgid, double firetime);
static std::vector<std::unique_ptr<Gid2PreSyn>> localmaps_;

static int nsend_, nsendmax_, nrecv_, nrecv_useful_;
static IvocVect* max_histogram_;

static int ocapacity_;  // for spikeout_
// require it to be smaller than  min_interprocessor_delay.
static double wt_;   // wait time for nrnmpi_spike_exchange
static double wt1_;  // time to find the PreSyns and send the spikes.
static int spfixout_capacity_;
static int idxout_;
static void nrn_spike_exchange_compressed(NrnThread*);
#endif  // NRNMPI

#if NRNMPI
bool use_multisend_;  // false: allgather, true: multisend (MPI_ISend)
static void nrn_multisend_setup();
static void nrn_multisend_init();
static void nrn_multisend_receive(NrnThread*);
extern void nrn_multisend_send(PreSyn*, double t);
static void nrn_multisend_cleanup_presyn(PreSyn*);
#endif

static int active_;
static double usable_mindelay_;
static double min_interprocessor_delay_;
static double mindelay_;  // the one actually used. Some of our optional algorithms
static double last_maxstep_arg_;
static NetParEvent* npe_;  // nrn_nthread of them
static int n_npe_;         // just to compare with nrn_nthread

double nrn_usable_mindelay() {
    return usable_mindelay_;
}
Symbol* nrn_netcon_sym() {
    return netcon_sym_;
}
Gid2PreSyn& nrn_gid2out() {
    return gid2out_;
}

#if NRNMPI
// for combination of threads and mpi.
#if NRN_ENABLE_THREADS
static MUTDEC
#endif
static std::atomic<int> seqcnt_;
static NrnThread* last_nt_;
#endif

NetParEvent::NetParEvent() {
    wx_ = ws_ = 0.;
    ithread_ = -1;
}
NetParEvent::~NetParEvent() {}
void NetParEvent::send(double tt, NetCvode* nc, NrnThread* nt) {
    nc->event(tt + usable_mindelay_, this, nt);
}


void NetParEvent::deliver(double tt, NetCvode* nc, NrnThread* nt) {
    int seq;
    if (nrn_use_selfqueue_) {  // first handle pending flag=1 self events
        nrn_pending_selfqueue(tt, nt);
    }
    // has to be the last event at this time in order to avoid a race
    // condition with HocEvent that may call things such as pc.barrier
    // actually allthread HocEvent (cvode.event(tev) and cvode.event(tev,stmt)
    // will be executed last after a thread join when nrn_allthread_handle
    // is called.
    net_cvode_instance->deliver_events(tt, nt);
    nt->_stop_stepping = 1;
    nt->_t = tt;
#if NRNMPI
    if (nrnmpi_numprocs > 0) {
        seq = ++seqcnt_;
        if (seq == nrn_nthread) {
            last_nt_ = nt;
#if NRNMPI
            if (use_multisend_) {
                nrn_multisend_receive(nt);
            } else {
                nrn_spike_exchange(nt);
            }
#else
            nrn_spike_exchange(nt);
#endif
            wx_ += wt_;
            ws_ += wt1_;
            seqcnt_ = 0;
        }
    }
#endif
    send(tt, nc, nt);
}
void NetParEvent::pgvts_deliver(double tt, NetCvode* nc) {
    assert(nrn_nthread == 1);
    deliver(tt, nc, nrn_threads);
}

void NetParEvent::pr(const char* m, double tt, NetCvode* nc) {
    Printf("%s NetParEvent %d t=%.15g tt-t=%g\n", m, ithread_, tt, tt - nrn_threads[ithread_]._t);
}

DiscreteEvent* NetParEvent::savestate_save() {
    // pr("savestate_save", 0, net_cvode_instance);
    NetParEvent* npe = new NetParEvent();
    npe->ithread_ = ithread_;
    return npe;
}

DiscreteEvent* NetParEvent::savestate_read(FILE* f) {
    int i;
    char buf[100];
    nrn_assert(fgets(buf, 100, f));
    nrn_assert(sscanf(buf, "%d\n", &i) == 1);
    // printf("NetParEvent::savestate_read %d\n", i);
    NetParEvent* npe = new NetParEvent();
    npe->ithread_ = i;
    return npe;
}

void NetParEvent::savestate_write(FILE* f) {
    // pr("savestate_write", 0, net_cvode_instance);
    fprintf(f, "%d\n%d\n", NetParEventType, ithread_);
}

void NetParEvent::savestate_restore(double tt, NetCvode* nc) {
#if NRNMPI
    if (use_compress_) {
        t_exchange_ = t;
    }
#endif
    if (ithread_ == 0) {
        // npe_->pr("savestate_restore", tt, nc);
        for (int i = 0; i < nrn_nthread; ++i)
            if (npe_ + i) {
                nc->event(tt, npe_ + i, nrn_threads + i);
            }
    }
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
    MUTLOCK
    nout_++;
    int i = idxout_;
    idxout_ += 2;
    if (idxout_ >= spfixout_capacity_) {
        spfixout_capacity_ *= 2;
        spfixout_ = (unsigned char*) hoc_Erealloc(spfixout_,
                                                  spfixout_capacity_ * sizeof(unsigned char));
        hoc_malchk();
    }
    spfixout_[i++] = (unsigned char) ((firetime - t_exchange_) * dt1_ + .5);
    spfixout_[i] = localgid;
    // printf("%d idx=%d lgid=%d firetime=%g t_exchange_=%g [0]=%d [1]=%d\n", nrnmpi_myid, i,
    // (int)localgid, firetime, t_exchange_, (int)spfixout_[i-1], (int)spfixout_[i]);
    MUTUNLOCK
}

#ifndef USENCS
void nrn2ncs_outputevent(int gid, double firetime) {
    if (!active_) {
        return;
    }
    MUTLOCK
    if (use_compress_) {
        nout_++;
        int i = idxout_;
        idxout_ += 1 + localgid_size_;
        if (idxout_ >= spfixout_capacity_) {
            spfixout_capacity_ *= 2;
            spfixout_ = (unsigned char*) hoc_Erealloc(spfixout_,
                                                      spfixout_capacity_ * sizeof(unsigned char));
            hoc_malchk();
        }
        // printf("%d nrnncs_outputevent %d %.20g %.20g %d\n", nrnmpi_myid, gid, firetime,
        // t_exchange_, (int)((unsigned char)((firetime - t_exchange_)*dt1_ + .5)));
        spfixout_[i++] = (unsigned char) ((firetime - t_exchange_) * dt1_ + .5);
        // printf("%d idx=%d firetime=%g t_exchange_=%g spfixout=%d\n", nrnmpi_myid, i, firetime,
        // t_exchange_, (int)spfixout_[i-1]);
        sppk(spfixout_ + i, gid);
        // printf("%d idx=%d gid=%d spupk=%d\n", nrnmpi_myid, i, gid, spupk(spfixout_+i));
    } else {
#if nrn_spikebuf_size == 0
        int i = nout_++;
        if (i >= ocapacity_) {
            ocapacity_ *= 2;
            spikeout_ = (NRNMPI_Spike*) hoc_Erealloc(spikeout_, ocapacity_ * sizeof(NRNMPI_Spike));
            hoc_malchk();
        }
        // printf("%d cell %d in slot %d fired at %g\n", nrnmpi_myid, gid, i, firetime);
        spikeout_[i].gid = gid;
        spikeout_[i].spiketime = firetime;
#else
        int i = nout_++;
        if (i >= nrn_spikebuf_size) {
            i -= nrn_spikebuf_size;
            if (i >= ocapacity_) {
                ocapacity_ *= 2;
                spikeout_ = (NRNMPI_Spike*) hoc_Erealloc(spikeout_,
                                                         ocapacity_ * sizeof(NRNMPI_Spike));
                hoc_malchk();
            }
            spikeout_[i].gid = gid;
            spikeout_[i].spiketime = firetime;
        } else {
            spbufout_->gid[i] = gid;
            spbufout_->spiketime[i] = firetime;
        }
#endif
    }
    MUTUNLOCK
    // printf("%d cell %d in slot %d fired at %g\n", nrnmpi_myid, gid, i, firetime);
}
#endif  // USENCS
#endif  // NRNMPI

static int nrn_need_npe() {
    int b = 0;
    if (active_) {
        b = 1;
    }
    if (nrn_use_selfqueue_) {
        b = 1;
    }
    if (nrn_nthread > 1) {
        b = 1;
    }
    if (b) {
        if (last_maxstep_arg_ == 0) {
            last_maxstep_arg_ = 100.;
        }
        set_mindelay(last_maxstep_arg_);
    } else {
        if (npe_) {
            delete[] npe_;
            npe_ = NULL;
            n_npe_ = 0;
        }
    }
    return b;
}

static void calc_actual_mindelay() {
    // reasons why mindelay_ can be smaller than min_interprocessor_delay
    // are use_multisend_
#if NRNMPI
    if (use_multisend_ && n_multisend_interval == 2) {
        mindelay_ = min_interprocessor_delay_ / 2.;
    } else {
        mindelay_ = min_interprocessor_delay_;
    }
#endif
}

#if NRNMPI
#include "multisend.cpp"
#else
#define TBUFSIZE 0
#define TBUF     /**/
#endif

void nrn_spike_exchange_init() {
    if (nrnmpi_step_wait_ >= 0.0) {
        nrnmpi_step_wait_ = 0.0;
    }
#ifdef USENCS
    nrn_multisend_setup();
    return;
#endif
    // printf("nrn_spike_exchange_init\n");
    if (!nrn_need_npe()) {
        return;
    }
    //	if (!active_ && !nrn_use_selfqueue_) { return; }
    alloc_space();
    // printf("nrnmpi_use=%d active=%d\n", nrnmpi_use, active_);
    calc_actual_mindelay();
    usable_mindelay_ = mindelay_;
    if (cvode_active_ == 0 && nrn_nthread > 1) {
        usable_mindelay_ -= dt;
    }
    if ((usable_mindelay_ < 1e-9) || (cvode_active_ == 0 && usable_mindelay_ < dt)) {
        if (nrnmpi_myid == 0) {
            hoc_execerror("usable mindelay is 0", "(or less than dt for fixed step method)");
        } else {
            return;
        }
    }

#if NRN_MUSIC
    if (nrnmusic) {
        nrnmusic_runtime_phase();
    }
#endif

#if TBUFSIZE
    itbuf_ = 0;
#endif

#if NRNMPI
    if (use_multisend_) {
        nrn_multisend_init();
    }
#endif

    if (n_npe_ != nrn_nthread) {
        if (npe_) {
            delete[] npe_;
        }
        npe_ = new NetParEvent[nrn_nthread];
        n_npe_ = nrn_nthread;
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        npe_[i].ithread_ = i;
        npe_[i].wx_ = 0.;
        npe_[i].ws_ = 0.;
        npe_[i].send(t, net_cvode_instance, nrn_threads + i);
    }
#if NRNMPI
    if (use_compress_) {
        idxout_ = 2;
        t_exchange_ = t;
        dt1_ = 1. / dt;
        usable_mindelay_ = floor(mindelay_ * dt1_ + 1e-9) * dt;
        assert(usable_mindelay_ >= dt && (usable_mindelay_ * dt1_) < 255);
    } else {
#if nrn_spikebuf_size > 0
        if (spbufout_) {
            spbufout_->nspike = 0;
        }
#endif
    }
    nout_ = 0;
    nsend_ = nsendmax_ = nrecv_ = nrecv_useful_ = 0;
    if (nrnmpi_numprocs > 0) {
        if (nrn_nthread > 0) {
#if NRN_ENABLE_THREADS
            if (!mut_) {
                MUTCONSTRUCT(1)
            }
#endif
        } else {
            MUTDESTRUCT
        }
    }
#endif  // NRNMPI
        // if (nrnmpi_myid == 0){printf("usable_mindelay_ = %g\n", usable_mindelay_);}
}

#if !NRNMPI
void nrn_spike_exchange(NrnThread*) {}
#else
void nrn_spike_exchange(NrnThread* nt) {
    nrn::Instrumentor::phase p_spike_exchange("spike-exchange");
    if (!active_) {
        return;
    }
#if NRNMPI
    if (use_multisend_) {
        nrn_multisend_receive(nt);
        return;
    }
#endif
    if (use_compress_) {
        nrn_spike_exchange_compressed(nt);
        return;
    }
    TBUF
#if TBUFSIZE
    nrnmpi_barrier();
#endif
    TBUF
    double wt;
    int i, n;
    nsend_ += nout_;
    if (nsendmax_ < nout_) {
        nsendmax_ = nout_;
    }
#if nrn_spikebuf_size > 0
    spbufout_->nspike = nout_;
#endif
    wt = nrnmpi_wtime();
    if (nrnmpi_step_wait_ >= 0.) {
        nrnmpi_barrier();
        nrnmpi_step_wait_ += nrnmpi_wtime() - wt;
    }
    n = nrnmpi_spike_exchange(&ovfl_, &nout_, nin_, spikeout_, &spikein_, &icapacity_);
    wt_ = nrnmpi_wtime() - wt;
    wt = nrnmpi_wtime();
    TBUF
#if TBUFSIZE
    tbuf_[itbuf_++] = (unsigned long) nout_;
    tbuf_[itbuf_++] = (unsigned long) n;
#endif

    errno = 0;
    // if (n > 0) {
    // printf("%d nrn_spike_exchange sent %d received %d\n", nrnmpi_myid, nout_, n);
    //}
    nout_ = 0;
    if (n == 0) {
        if (max_histogram_) {
            vector_vec(max_histogram_)[0] += 1.;
        }
        TBUF
        return;
    }
    nrecv_ += n;
    if (max_histogram_) {
        int mx = 0;
        if (n > 0) {
            for (i = nrnmpi_numprocs - 1; i >= 0; --i) {
#if nrn_spikebuf_size == 0
                if (mx < nin_[i]) {
                    mx = nin_[i];
                }
#else
                if (mx < spbufin_[i].nspike) {
                    mx = spbufin_[i].nspike;
                }
#endif
            }
        }
        int ms = vector_capacity(max_histogram_) - 1;
        mx = (mx < ms) ? mx : ms;
        vector_vec(max_histogram_)[mx] += 1.;
    }
#if nrn_spikebuf_size > 0
    for (i = 0; i < nrnmpi_numprocs; ++i) {
        int j;
        int nn = spbufin_[i].nspike;
        if (nn > nrn_spikebuf_size) {
            nn = nrn_spikebuf_size;
        }
        for (j = 0; j < nn; ++j) {
            auto iter = gid2in_.find(spbufin_[i].gid[j]);
            if (iter != gid2in_.end()) {
                PreSyn* ps = iter->second;
                ps->send(spbufin_[i].spiketime[j], net_cvode_instance, nt);
                ++nrecv_useful_;
            }
        }
    }
    n = ovfl_;
#endif  // nrn_spikebuf_size > 0
    for (i = 0; i < n; ++i) {
        auto iter = gid2in_.find(spikein_[i].gid);
        if (iter != gid2in_.end()) {
            PreSyn* ps = iter->second;
            ps->send(spikein_[i].spiketime, net_cvode_instance, nt);
            ++nrecv_useful_;
        }
    }
    wt1_ = nrnmpi_wtime() - wt;
    TBUF
}

void nrn_spike_exchange_compressed(NrnThread* nt) {
    if (!active_) {
        return;
    }
    TBUF
#if TBUFSIZE
    nrnmpi_barrier();
#endif
    TBUF
    assert(!cvode_active_);
    double wt;
    int i, n, idx;
    nsend_ += nout_;
    if (nsendmax_ < nout_) {
        nsendmax_ = nout_;
    }
    assert(nout_ < 0x10000);
    spfixout_[1] = (unsigned char) (nout_ & 0xff);
    spfixout_[0] = (unsigned char) (nout_ >> 8);

    wt = nrnmpi_wtime();
    if (nrnmpi_step_wait_ >= 0.) {
        nrnmpi_barrier();
        nrnmpi_step_wait_ += nrnmpi_wtime() - wt;
    }
    n = nrnmpi_spike_exchange_compressed(localgid_size_,
                                         ag_send_size_,
                                         ag_send_nspike_,
                                         &ovfl_capacity_,
                                         &ovfl_,
                                         spfixout_,
                                         spfixin_,
                                         &spfixin_ovfl_,
                                         nin_);
    wt_ = nrnmpi_wtime() - wt;
    wt = nrnmpi_wtime();
    TBUF
#if TBUFSIZE
    tbuf_[itbuf_++] = (unsigned long) nout_;
    tbuf_[itbuf_++] = (unsigned long) n;
#endif
    errno = 0;
    // if (n > 0) {
    // printf("%d nrn_spike_exchange sent %d received %d\n", nrnmpi_myid, nout_, n);
    //}
    nout_ = 0;
    idxout_ = 2;
    if (n == 0) {
        if (max_histogram_) {
            vector_vec(max_histogram_)[0] += 1.;
        }
        t_exchange_ = nt->_t;
        TBUF
        return;
    }
    nrecv_ += n;
    if (max_histogram_) {
        int mx = 0;
        if (n > 0) {
            for (i = nrnmpi_numprocs - 1; i >= 0; --i) {
                if (mx < nin_[i]) {
                    mx = nin_[i];
                }
            }
        }
        int ms = vector_capacity(max_histogram_) - 1;
        mx = (mx < ms) ? mx : ms;
        vector_vec(max_histogram_)[mx] += 1.;
    }
    if (nrn_use_localgid_) {
        int idxov = 0;
        for (i = 0; i < nrnmpi_numprocs; ++i) {
            int j, nnn;
            int nn = nin_[i];
            if (nn) {
                if (i == nrnmpi_myid) {  // skip but may need to increment idxov.
                    if (nn > ag_send_nspike_) {
                        idxov += (nn - ag_send_nspike_) * (1 + localgid_size_);
                    }
                    continue;
                }
                Gid2PreSyn* gps = localmaps_[i].get();
                if (nn > ag_send_nspike_) {
                    nnn = ag_send_nspike_;
                } else {
                    nnn = nn;
                }
                idx = 2 + i * ag_send_size_;
                for (j = 0; j < nnn; ++j) {
                    // order is (firetime,gid) pairs.
                    double firetime = spfixin_[idx++] * dt + t_exchange_;
                    int lgid = (int) spfixin_[idx];
                    idx += localgid_size_;
                    auto iter = gps->find(lgid);
                    if (iter != gps->end()) {
                        PreSyn* ps = iter->second;
                        ps->send(firetime + 1e-10, net_cvode_instance, nt);
                        ++nrecv_useful_;
                    }
                }
                for (; j < nn; ++j) {
                    double firetime = spfixin_ovfl_[idxov++] * dt + t_exchange_;
                    int lgid = (int) spfixin_ovfl_[idxov];
                    idxov += localgid_size_;
                    auto iter = gps->find(lgid);
                    if (iter != gps->end()) {
                        PreSyn* ps = iter->second;
                        ps->send(firetime + 1e-10, net_cvode_instance, nt);
                        ++nrecv_useful_;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < nrnmpi_numprocs; ++i) {
            int j;
            int nn = nin_[i];
            if (nn > ag_send_nspike_) {
                nn = ag_send_nspike_;
            }
            idx = 2 + i * ag_send_size_;
            for (j = 0; j < nn; ++j) {
                // order is (firetime,gid) pairs.
                double firetime = spfixin_[idx++] * dt + t_exchange_;
                int gid = spupk(spfixin_ + idx);
                idx += localgid_size_;
                auto iter = gid2in_.find(gid);
                if (iter != gid2in_.end()) {
                    PreSyn* ps = iter->second;
                    ps->send(firetime + 1e-10, net_cvode_instance, nt);
                    ++nrecv_useful_;
                }
            }
        }
        n = ovfl_;
        idx = 0;
        for (i = 0; i < n; ++i) {
            double firetime = spfixin_ovfl_[idx++] * dt + t_exchange_;
            int gid = spupk(spfixin_ovfl_ + idx);
            idx += localgid_size_;
            auto iter = gid2in_.find(gid);
            if (iter != gid2in_.end()) {
                PreSyn* ps = iter->second;
                ps->send(firetime + 1e-10, net_cvode_instance, nt);
                ++nrecv_useful_;
            }
        }
    }
    t_exchange_ = nt->_t;
    wt1_ = nrnmpi_wtime() - wt;
    TBUF
}

static void mk_localgid_rep() {
    // how many gids are there on this machine
    // and can they be compressed into one byte
    int ngid = 0;
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        if (ps->output_index_ >= 0) {
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
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        if (ps->output_index_ >= 0) {
            ps->localgid_ = (unsigned char) ngid;
            sbuf[ngid] = ps->output_index_;
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
    localmaps_.clear();
    localmaps_.resize(nrnmpi_numprocs);

    for (int i = 0; i < nrnmpi_numprocs; ++i)
        if (i != nrnmpi_myid) {
            localmaps_[i].reset(new Gid2PreSyn());
        }

    // fill in the maps
    for (int i = 0; i < nrnmpi_numprocs; ++i)
        if (i != nrnmpi_myid) {
            sbuf = rbuf + i * (ngidmax + 1);
            ngid = *(sbuf++);
            for (int k = 0; k < ngid; ++k) {
                auto iter = gid2in_.find(int(sbuf[k]));
                if (iter != gid2in_.end()) {
                    PreSyn* ps = iter->second;
                    (*localmaps_[i])[k] = ps;
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
// The remaining possibility is fake_out=2 which only does a send for
// the gids owned by this cpu. This, followed by a nrn_spike_exchange(),
// ensures that all the target cells, regardless of what rank they are on
// will get the spike delivered and nobody gets it twice.

void nrn_fake_fire(int gid, double spiketime, int fake_out) {
    PreSyn* ps{nullptr};
    if (fake_out < 2) {
        auto iter = gid2in_.find(gid);
        if (iter != gid2in_.end()) {
            ps = iter->second;
            // printf("nrn_fake_fire %d %g\n", gid, spiketime);
            ps->send(spiketime, net_cvode_instance, nrn_threads);
#if NRNMPI
            ++nrecv_useful_;
#endif
        }
    } else if (fake_out && !ps) {
        auto iter = gid2out_.find(gid);
        if (iter != gid2out_.end()) {
            ps = iter->second;
            // printf("nrn_fake_fire fake_out %d %g\n", gid, spiketime);
            ps->send(spiketime, net_cvode_instance, nrn_threads);
#if NRNMPI
            ++nrecv_useful_;
#endif
        }
    }
}

static void alloc_space() {
    if (!netcon_sym_) {
        netcon_sym_ = hoc_lookup("NetCon");
#if NRNMPI
        ocapacity_ = 100;
        spikeout_ = (NRNMPI_Spike*) hoc_Emalloc(ocapacity_ * sizeof(NRNMPI_Spike));
        hoc_malchk();
        icapacity_ = 100;
        spikein_ = (NRNMPI_Spike*) hoc_Emalloc(icapacity_ * sizeof(NRNMPI_Spike));
        hoc_malchk();
        nin_ = (int*) hoc_Emalloc(nrnmpi_numprocs * sizeof(int));
        hoc_malchk();
#if nrn_spikebuf_size > 0
        spbufout_ = (NRNMPI_Spikebuf*) hoc_Emalloc(sizeof(NRNMPI_Spikebuf));
        hoc_malchk();
        spbufin_ = (NRNMPI_Spikebuf*) hoc_Emalloc(nrnmpi_numprocs * sizeof(NRNMPI_Spikebuf));
        hoc_malchk();
#endif
#endif
    }
}

void BBS::set_gid2node(int gid, int nid) {
    alloc_space();
#if NRNMPI
    if (nid == nrnmpi_myid) {
#else
    {
#endif
        // printf("gid %d defined on %d\n", gid, nrnmpi_myid);
        if (gid2in_.find(gid) != gid2in_.end()) {
            hoc_execerr_ext(
                "gid=%d already exists as an input port. Setup all the output ports on this "
                "process before using them as input ports.",
                gid);
        }
        if (gid2out_.find(gid) != gid2out_.end()) {
            hoc_execerr_ext("gid=%d already exists on this process as an output port", gid);
        }
        gid2out_[gid] = nullptr;
    }
}

static int gid_donot_remove = 0;  // avoid  gid2in_, gid2out removal when iterating

void nrn_cleanup_presyn(PreSyn* ps) {
#if NRNMPI
    nrn_multisend_cleanup_presyn(ps);
#endif
    if (gid_donot_remove) {
        return;
    }
    if (ps->output_index_ >= 0) {
        gid2out_.erase(ps->output_index_);
        ps->output_index_ = -1;
        ps->gid_ = -1;
    }
    if (ps->gid_ >= 0) {
        gid2in_.erase(ps->gid_);
        ps->gid_ = -1;
    }
}

void nrnmpi_gid_clear(int arg) {
    if (arg == 0 || arg == 3 || arg == 4) {
        nrn_partrans_clear();
#if NRNMPI
        nrnmpi_split_clear();
#endif
    }
    if (arg == 0 || arg == 2 || arg == 4) {
        nrnmpi_multisplit_clear();
    }
    if (arg == 2 || arg == 3) {
        return;
    }
    gid_donot_remove = 1;
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        if (ps && gid2in_.find(ps->gid_) == gid2in_.end()) {
            if (arg == 4) {
                delete ps;
            } else {
#if NRNMPI
                nrn_multisend_cleanup_presyn(ps);
#endif
                ps->gid_ = -1;
                ps->output_index_ = -1;
                if (ps->dil_.empty()) {
                    delete ps;
                }
            }
        }
    }
    for (const auto& iter: gid2in_) {
        PreSyn* ps = iter.second;
        if (arg == 4) {
            delete ps;
        } else {
#if NRNMPI
            nrn_multisend_cleanup_presyn(ps);
#endif
            ps->gid_ = -1;
            ps->output_index_ = -1;
            if (ps->dil_.empty()) {
                delete ps;
            }
        }
    }
    gid_donot_remove = 0;
    gid2in_.clear();
    gid2out_.clear();
}

int nrn_gid_exists(int gid) {
    alloc_space();
    auto iter = gid2out_.find(gid);
    if (iter != gid2out_.end()) {
        PreSyn* ps = iter->second;
        // printf("%d gid %d exists\n", nrnmpi_myid, gid);
        if (ps) {
            return (ps->output_index_ >= 0 ? 3 : 2);
        } else {
            return 1;
        }
    }
    return 0;
}
int BBS::gid_exists(int gid) {
    return nrn_gid_exists(gid);
}

double BBS::threshold() {
    int gid = int(chkarg(1, 0., MD));
    auto iter = gid2out_.find(gid);
    if (iter == gid2out_.end() || iter->second == NULL) {
        hoc_execerror("gid not associated with spike generation location", 0);
    }
    PreSyn* ps = iter->second;
    if (ifarg(2)) {
        ps->threshold_ = *getarg(2);
    }
    return ps->threshold_;
}

void BBS::cell() {
    int gid = int(chkarg(1, 0., MD));
    alloc_space();
    if (gid2in_.find(gid) != gid2in_.end()) {
        hoc_execerr_ext(
            "gid=%d is in the input list. Must register with pc.set_gid2node prior to connecting",
            gid);
    }
    if (gid2out_.find(gid) == gid2out_.end()) {
        hoc_execerr_ext("gid=%d has not been set on rank %d", gid, nrnmpi_myid);
    }
    Object* ob = *hoc_objgetarg(2);
    if (!ob || ob->ctemplate != netcon_sym_->u.ctemplate) {
        check_obj_type(ob, "NetCon");
    }
    NetCon* nc = (NetCon*) ob->u.this_pointer;
    PreSyn* ps = nc->src_;
    if (!ps) {
        hoc_execerr_ext("pc.cell second arg, %s, has no source", hoc_object_name(ob));
    }
    if (ps->gid_ >= 0 && ps->gid_ != gid) {
        hoc_execerr_ext("Can't associate gid %d. PreSyn already associated with gid %d.",
                        gid,
                        ps->gid_);
    }
    gid2out_[gid] = ps;
    ps->gid_ = gid;
    if (ifarg(3) && !chkarg(3, 0., 1.)) {
        ps->output_index_ = -2;  // prevents destruction of PreSyn
    } else {
        ps->output_index_ = gid;
    }
}

void BBS::outputcell(int gid) {
    auto iter = gid2out_.find(gid);
    nrn_assert(iter != gid2out_.end());
    PreSyn* ps = iter->second;
    assert(ps);
    ps->output_index_ = gid;
    ps->gid_ = gid;
}

void BBS::spike_record(int gid, IvocVect* spikevec, IvocVect* gidvec) {
    if (gid >= 0) {
        all_spiketvec = NULL, all_spikegidvec = NULL;  // invalidate global spike vectors

        auto iter = gid2out_.find(gid);
        nrn_assert(iter != gid2out_.end());
        PreSyn* ps = iter->second;
        assert(ps);
        ps->record(spikevec, gidvec, gid);
    } else {  // record all output spikes.
        all_spiketvec = spikevec,
        all_spikegidvec = gidvec;  // track global spike vectors (optimisation)
        for (const auto& iter: gid2out_) {
            PreSyn* ps = iter.second;
            if (ps->output_index_ >= 0) {
                ps->record(all_spiketvec, all_spikegidvec, ps->output_index_);
            }
        }
    }
}

void BBS::spike_record(IvocVect* gids, IvocVect* spikevec, IvocVect* gidvec) {
    int sz = vector_capacity(gids);
    // invalidate global spike vectors
    all_spiketvec = nullptr;
    all_spikegidvec = nullptr;
    double* pd = vector_vec(gids);
    for (int i = 0; i < sz; ++i) {
        int gid = int(pd[i]);
        auto iter = gid2out_.find(gid);
        nrn_assert(iter != gid2out_.end());
        PreSyn* ps = iter->second;
        assert(ps);
        ps->record(spikevec, gidvec, gid);
    }
}

static Object* gid2obj_(int gid) {
    Object* cell = 0;
    // printf("%d gid2obj gid=%d\n", nrnmpi_myid, gid);
    auto iter = gid2out_.find(gid);
    nrn_assert(iter != gid2out_.end());
    PreSyn* ps = iter->second;
    // printf(" found\n");
    assert(ps);
    cell = ps->ssrc_ ? nrn_sec2cell(ps->ssrc_) : ps->osrc_;
    // printf(" return %s\n", hoc_object_name(cell));
    return cell;
}

Object** BBS::gid2obj(int gid) {
    return hoc_temp_objptr(gid2obj_(gid));
}

Object** BBS::gid2cell(int gid) {
    Object* cell = 0;
    // printf("%d gid2obj gid=%d\n", nrnmpi_myid, gid);
    auto iter = gid2out_.find(gid);
    nrn_assert(iter != gid2out_.end());
    PreSyn* ps = iter->second;
    // printf(" found\n");
    assert(ps);
    if (ps->ssrc_) {
        cell = nrn_sec2cell(ps->ssrc_);
    } else {
        cell = ps->osrc_;
        // but if it is a POINT_PROCESS in a section
        // that is inside an object ... (probably has a WATCH statement)
        Section* sec = ob2pntproc(cell)->sec;
        Object* c2;
        if (sec && (c2 = nrn_sec2cell(sec))) {
            if (c2) {
                cell = c2;
            }
        }
    }
    // printf(" return %s\n", hoc_object_name(cell));
    return hoc_temp_objptr(cell);
}

Object** BBS::gid_connect(int gid) {
    Object* target = *hoc_objgetarg(2);
    if (!is_point_process(target)) {
        hoc_execerror("arg 2 must be a point process", 0);
    }
    alloc_space();
    PreSyn* ps;
    auto iter_out = gid2out_.find(gid);
    if (iter_out != gid2out_.end()) {
        // the gid is owned by this machine so connect directly
        ps = iter_out->second;
        if (!ps) {
            hoc_execerr_ext("gid %d owned by %d but no associated cell", gid, nrnmpi_myid);
        }
    } else {
        auto iter_in = gid2in_.find(gid);
        if (iter_in != gid2in_.end()) {
            // the gid stub already exists
            ps = iter_in->second;
            // printf("%d connect %s from already existing %d\n", nrnmpi_myid,
            // hoc_object_name(target), gid);
        } else {
            // printf("%d connect %s from new PreSyn for %d\n", nrnmpi_myid,
            // hoc_object_name(target), gid);
            ps = new PreSyn({}, nullptr, nullptr);
            net_cvode_instance->psl_append(ps);
            gid2in_[gid] = ps;
            ps->gid_ = gid;
        }
    }
    NetCon* nc;
    Object** po;
    if (ifarg(3)) {
        po = hoc_objgetarg(3);
        if (!*po || (*po)->ctemplate != netcon_sym_->u.ctemplate) {
            check_obj_type(*po, "NetCon");
        }
        nc = (NetCon*) ((*po)->u.this_pointer);
        if (nc->target_ != ob2pntproc(target)) {
            hoc_execerror("target is different from 3rd arg NetCon target", 0);
        }
        nc->replace_src(ps);
    } else {
        nc = new NetCon(ps, target);
        po = hoc_temp_objvar(netcon_sym_, nc);
        nc->obj_ = *po;
    }
    return po;
}

static int timeout_ = 20;
int nrn_set_timeout(int timeout) {
    int tt;
    tt = timeout_;
    timeout_ = timeout;
    return tt;
}

void BBS::netpar_solve(double tstop) {
    // temporary check to be eventually replaced by verify_structure()
    if (tree_changed) {
        setup_topology();
    }
    if (v_structure_change) {
        v_setup_vectors();
    }
    if (diam_changed) {
        recalc_diam();
    }
    // if cvode_active, and anything at all has changed, should call re_init

#if NRNMPI
    double mt, md;
    tstopunset;
    if (cvode_active_) {
        mt = 1e-9;
        md = mindelay_;
    } else {
        mt = dt;
        md = mindelay_ - 1e-10;
    }
    if (md < mt) {
        if (nrnmpi_myid == 0) {
            hoc_execerror("mindelay is 0", "(or less than dt for fixed step method)");
        } else {
            return;
        }
    }
    double wt;

    nrnmpi_barrier();       // make sure all integrations start about the same
    nrn_timeout(timeout_);  // time to avoid spurious timeouts while waiting
                            // at the next MPI_collective.
    wt = nrnmpi_wtime();
    if (cvode_active_) {
        ncs2nrn_integrate(tstop);
    } else {
        ncs2nrn_integrate(tstop * (1. + 1e-11));
    }
    impl_->integ_time_ += nrnmpi_wtime() - wt;
    impl_->integ_time_ -= (npe_ ? (npe_[0].wx_ + npe_[0].ws_) : 0.);
#if NRNMPI
    if (use_multisend_) {
        for (int i = 0; i < n_multisend_interval; ++i) {
            nrn_multisend_receive(nrn_threads);
        }
    } else {
        nrn_spike_exchange(nrn_threads);
    }
#else
    nrn_spike_exchange(nrn_threads);
#endif
    nrn_timeout(0);
    impl_->wait_time_ += wt_;
    impl_->send_time_ += wt1_;
    if (npe_) {
        impl_->wait_time_ += npe_[0].wx_;
        impl_->send_time_ += npe_[0].ws_;
        npe_[0].wx_ = npe_[0].ws_ = 0.;
    };
// printf("%d netpar_solve exit t=%g tstop=%g mindelay_=%g\n",nrnmpi_myid, t, tstop, mindelay_);
#else  // not NRNMPI
    ncs2nrn_integrate(tstop);
#endif
    tstopunset;
}

int nrnthread_all_spike_vectors_return(std::vector<double>& spiketvec,
                                       std::vector<int>& spikegidvec) {
    assert(spiketvec.size() == spikegidvec.size());
    if (spiketvec.size()) {
        /* Optimisation:
         *  if all_spiketvec and all_spikegidvec are set (pc.spike_record with gid=-1 was called)
         *  and they are still reference counted (obj_->refcount), then copy over incoming vectors.
         */
        if (all_spiketvec != NULL && all_spiketvec->obj_ != NULL &&
            all_spiketvec->obj_->refcount > 0 && all_spikegidvec != NULL &&
            all_spikegidvec->obj_ != NULL && all_spikegidvec->obj_->refcount > 0) {
            all_spiketvec->buffer_size(spiketvec.size() + all_spiketvec->size());
            all_spikegidvec->buffer_size(spikegidvec.size() + all_spikegidvec->size());
            all_spiketvec->vec().insert(all_spiketvec->end(), spiketvec.begin(), spiketvec.end());
            all_spikegidvec->vec().insert(all_spikegidvec->end(),
                                          spikegidvec.begin(),
                                          spikegidvec.end());

        } else {  // different underlying vectors for PreSyns
            for (size_t i = 0; i < spikegidvec.size(); ++i) {
                auto iter = gid2out_.find(spikegidvec[i]);
                if (iter != gid2out_.end()) {
                    PreSyn* ps = iter->second;
                    ps->record(spiketvec[i]);
                }
            }
        }
    }
    return 1;
}


static double set_mindelay(double maxdelay) {
    double mindelay = maxdelay;
    last_maxstep_arg_ = maxdelay;
    if (nrn_use_selfqueue_ || net_cvode_instance->localstep() || nrn_nthread > 1) {
        hoc_Item* q;
        if (net_cvode_instance->psl_)
            ITERATE(q, net_cvode_instance->psl_) {
                PreSyn* ps = (PreSyn*) VOIDITM(q);
                double md = ps->mindelay();
                if (mindelay > md) {
                    mindelay = md;
                }
            }
    }
#if NRNMPI
    else {
        for (const auto& iter: gid2in_) {
            PreSyn* ps = iter.second;
            double md = ps->mindelay();
            if (mindelay > md) {
                mindelay = md;
            }
        }
    }
    if (nrnmpi_use) {
        active_ = 1;
    }
    if (use_compress_) {
        if (mindelay / dt > 255) {
            mindelay = 255 * dt;
        }
    }

    // printf("%d netpar_mindelay local %g now calling nrnmpi_mindelay\n", nrnmpi_myid, mindelay);
    //	double st = time();
    mindelay_ = nrnmpi_mindelay(mindelay);
    min_interprocessor_delay_ = mindelay_;
    //	add_wait_time(st);
    // printf("%d local min=%g  global min=%g\n", nrnmpi_myid, mindelay, mindelay_);
    if (mindelay_ < 1e-9 && nrn_use_selfqueue_) {
        nrn_use_selfqueue_ = 0;
        double od = mindelay_;
        mindelay = set_mindelay(maxdelay);
        if (nrnmpi_myid == 0) {
            Printf(
                "Notice: The global minimum NetCon delay is %g, so turned off the "
                "cvode.queue_mode\n",
                od);
            Printf("   use_self_queue option. The interprocessor minimum NetCon delay is %g\n",
                   mindelay);
        }
    }
    errno = 0;
    return mindelay;
#else
    mindelay_ = mindelay;
    min_interprocessor_delay_ = mindelay_;
    return mindelay;
#endif  // NRNMPI
}

double BBS::netpar_mindelay(double maxdelay) {
#if NRNMPI
    nrn_multisend_setup();
#endif
    double tt = set_mindelay(maxdelay);
    return tt;
}

void BBS::netpar_spanning_statistics(int* nsend, int* nsendmax, int* nrecv, int* nrecv_useful) {
#if NRNMPI
    *nsend = nsend_;
    *nsendmax = nsendmax_;
    *nrecv = nrecv_;
    *nrecv_useful = nrecv_useful_;
#endif
}

// unfortunately, ivocvect.h conflicts with STL
IvocVect* BBS::netpar_max_histogram(IvocVect* mh) {
#if NRNMPI
    IvocVect* h = max_histogram_;
    if (max_histogram_) {
        max_histogram_ = NULL;
    }
    if (mh) {
        max_histogram_ = mh;
    }
    return h;
#else
    return nullptr;
#endif
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

0: Allgather
1: multisend implemented as MPI_ISend

n_multisend_interval 1 or 2 per minimum interprocessor NetCon delay
 that concept valid for all methods

Note that Allgather allows spike compression and an allgather spike buffer
 with size chosen at setup time.  All methods allow bin queueing.

The multisend methods should allow two phase multisend.

Note that, in principle, MPI_ISend allows the source to send the index
 of the target PreSyn to avoid a hash table lookup (even with a two phase
 variant)

Not all variation are useful. e.g. it is pointless to combine Allgather and
n_multisend_interval=2.
The whole point is to make the
spike transfer initiation as lowcost as possible since that is what causes
most load imbalance. I.e. since 10K more spikes arrive than are sent, spikes
received per processor per interval are much more statistically
balanced than spikes sent per processor per interval. And presently
DCMF multisend injects 10000 messages per spike into the network which
is quite expensive.

See case 8 of nrn_multisend_receive_time for the xchng_meth properties
*/

int nrnmpi_spike_compress(int nspike, bool gid_compress, int xchng_meth) {
#if NRNMPI
    if (nrnmpi_numprocs < 2) {
        return 0;
    }
    if (nspike >= 0) {  // otherwise don't set any multisend properties
        n_multisend_interval = (xchng_meth & 4) ? 2 : 1;
        use_multisend_ = (xchng_meth & 1) == 1;
        use_phase2_ = (xchng_meth & 8) ? 1 : 0;
        if (use_multisend_) {
            assert(NRNMPI);
        }
        nrn_multisend_cleanup();
    }
    if (nspike >= 0) {
        ag_send_nspike_ = 0;
        if (spfixout_) {
            free(spfixout_);
            spfixout_ = 0;
        }
        if (spfixin_) {
            free(spfixin_);
            spfixin_ = 0;
        }
        if (spfixin_ovfl_) {
            free(spfixin_ovfl_);
            spfixin_ovfl_ = 0;
        }
        localmaps_.clear();
    }
    if (nspike == 0) {  // turn off
        use_compress_ = false;
        nrn_use_localgid_ = false;
    } else if (nspike > 0) {  // turn on
        if (cvode_active_) {
            if (nrnmpi_myid == 0) {
                hoc_warning("ParallelContext.spike_compress cannot be used with cvode active", 0);
            }
            use_compress_ = false;
            nrn_use_localgid_ = false;
            return 0;
        }
        use_compress_ = true;
        ag_send_nspike_ = nspike;
        nrn_use_localgid_ = false;
        if (gid_compress) {
            // we can only do this after everything is set up
            mk_localgid_rep();
            if (!nrn_use_localgid_ && nrnmpi_myid == 0) {
                Printf(
                    "Notice: gid compression did not succeed. Probably more than 255 cells on one "
                    "cpu.\n");
            }
        }
        if (!nrn_use_localgid_) {
            localgid_size_ = sizeof(unsigned int);
        }
        ag_send_size_ = 2 + ag_send_nspike_ * (1 + localgid_size_);
        spfixout_capacity_ = ag_send_size_ + 50 * (1 + localgid_size_);
        spfixout_ = (unsigned char*) hoc_Emalloc(spfixout_capacity_);
        hoc_malchk();
        spfixin_ = (unsigned char*) hoc_Emalloc(nrnmpi_numprocs * ag_send_size_);
        hoc_malchk();
        ovfl_capacity_ = 100;
        spfixin_ovfl_ = (unsigned char*) hoc_Emalloc(ovfl_capacity_ * (1 + localgid_size_));
        hoc_malchk();
    }
    return ag_send_nspike_;
#else
    return 0;
#endif
}

PreSyn* nrn_gid2outputpresyn(int gid) {  // output PreSyn
    auto iter = gid2out_.find(gid);
    if (iter != gid2out_.end()) {
        return iter->second;
    }
    return NULL;
}

Object* nrn_gid2obj(int gid) {
    return gid2obj_(gid);
}

PreSyn* nrn_gid2presyn(int gid) {  // output PreSyn
    auto iter = gid2out_.find(gid);
    nrn_assert(iter != gid2out_.end());
    return iter->second;
}

void nrn_gidout_iter(PFIO callback) {
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        if (ps) {
            int gid = ps->gid_;
            Object* c = gid2obj_(gid);
            (*callback)(gid, c);
        }
    }
}

#include "nrncore_write.h"
extern int* nrn_prop_param_size_;
extern short* nrn_is_artificial_;
static int weightcnt(NetCon* nc) {
    return nc->cnt_;
    //  return nc->target_ ? pnt_receive_size[nc->target_->prop->_type]: 1;
}

size_t nrncore_netpar_bytes() {
    size_t ntot, nin, nout, nnet, nweight;
    ntot = nin = nout = nnet = nweight = 0;
    size_t npnt = 0;
    if (0 && nrnmpi_myid == 0) {
        printf("size Presyn=%ld NetCon=%ld Point_process=%ld Prop=%ld\n",
               sizeof(PreSyn),
               sizeof(NetCon),
               sizeof(Point_process),
               sizeof(Prop));
    }
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        if (ps) {
            nout += 1;
            int n = ps->dil_.size();
            nnet += n;
            for (auto nc: ps->dil_) {
                nweight += weightcnt(nc);
                if (nc->target_) {
                    npnt += 1;
                }
            }
        }
    }
    // printf("output Presyn npnt = %ld nweight = %ld\n", npnt, nweight);
    for (const auto& iter: gid2in_) {
        PreSyn* ps = iter.second;
        if (ps) {
            nin += 1;
            int n = ps->dil_.size();
            nnet += n;
            for (auto nc: ps->dil_) {
                nweight += weightcnt(nc);
                if (nc->target_) {
                    npnt += 1;
                }
            }
        }
    }
    // printf("after input Presyn total npnt = %ld  nweight = %ld\n", npnt, nweight);
    ntot = (nin + nout) * sizeof(PreSyn) + nnet * sizeof(NetCon) + nweight * sizeof(double);
    //  printf("%d rank output Presyn %ld  input Presyn %ld  NetCon %ld  bytes %ld\n",
    //    nrnmpi_myid, nout, nin, nnet, ntot);
    return ntot;
}
