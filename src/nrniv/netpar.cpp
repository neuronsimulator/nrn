#include <../../nrnconf.h>
#include <nrnmpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#define ALTHASH 1
#if ALTHASH
#include <nrnhash_alt.h>
#else
#include <nrnhash.h>
#endif
#include <bbs.h>

#undef MD
#define MD 2147483648.

class PreSyn;

// hash table where buckets are binary search maps
declareNrnHash(Gid2PreSyn, int, PreSyn*)
implementNrnHash(Gid2PreSyn, int, PreSyn*)

#include <errno.h>
#include <netcon.h>
#include <cvodeobj.h>
#include <netcvode.h>

#define BGP_INTERVAL 2
#if BGP_INTERVAL == 2
static int n_bgp_interval;
#endif

static Symbol* netcon_sym_;
static Gid2PreSyn* gid2out_;
static Gid2PreSyn* gid2in_;
static double t_exchange_;
static double dt1_; // 1/dt
static void alloc_space();

extern "C" {
extern NetCvode* net_cvode_instance;
extern double t, dt;
extern int cvode_active_;
extern Point_process* ob2pntproc(Object*);
extern int nrn_use_selfqueue_;
extern void nrn_pending_selfqueue(double, NrnThread*);
extern int vector_capacity(IvocVect*); //ivocvect.h conflicts with STL
extern double* vector_vec(IvocVect*);
extern Object* nrn_sec2cell(Section*);
extern void ncs2nrn_integrate(double tstop);
extern void nrn_fake_fire(int gid, double firetime, int fake_out);
int nrnmpi_spike_compress(int nspike, bool gid_compress, int xchng_meth);
void nrn_cleanup_presyn(PreSyn*);
int nrn_set_timeout(int);
void nrnmpi_gid_clear(int);
extern void nrn_partrans_clear();
void nrn_spike_exchange_init();
extern double nrn_bgp_receive_time(int);

// BGPDMA can be 0,1,2,3,6,7
// (BGPDMA & 1) > 0 means multisend ISend allowed
// (BGPDMA & 2) > 0 means multisend Blue Gene/P DMA allowed
// (BGPDMA & 4) > 0 means multisend Blue Gene/P DMA Record Replay allowed
#if !defined(BGPDMA)
#define BGPDMA 0
#endif

#if BGPDMA == 0
double nrn_bgp_receive_time(int) { return 0.; }
#endif

#if PARANEURON
extern void nrnmpi_split_clear();
#endif
extern void nrnmpi_multisplit_clear();
}

static double set_mindelay(double maxdelay);

#if NRNMPI

#include "../nrnmpi/mpispike.h"

extern "C" {
void nrn_timeout(int);
void nrn_spike_exchange(NrnThread*);
extern int nrnmpi_int_allmax(int);
extern void nrnmpi_int_allgather(int*, int*, int);
void nrn2ncs_outputevent(int netcon_output_index, double firetime);
}

#ifdef USENCS
extern int ncs_bgp_sending_info( int ** );
extern int ncs_bgp_target_hosts( int, int** );
extern int ncs_bgp_target_info( int ** );
extern int ncs_bgp_mindelays( int **, double ** );

//get minimum delays for all presyn objects in gid2in_
int ncs_netcon_mindelays( int**hosts, double **delays )
{
    return ncs_bgp_mindelays(hosts, delays);
}

double ncs_netcon_localmindelay( int srcgid )
{
    PreSyn *ps;
    gid2out_->find( srcgid, ps );
    assert(ps);
    
    return ps->mindelay();
}

//get the number of netcons for an object, if it sends here
int ncs_netcon_count( int srcgid, bool localNetCons )
{
    PreSyn *ps;
    int flag = false;
    if( localNetCons )
        gid2out_->find( srcgid, ps );
    else
        gid2in_->find( srcgid, ps );
    if( !ps ) {  //no cells on this cpu receive from the given gid
        fprintf( stderr, "should never happen!\n" );
        return 0;
    }
    
    return ps->dil_.count();
}

//inject a spike into the appropriate netcon
void ncs_netcon_inject( int srcgid, int netconIndex, double spikeTime, bool localNetCons )
{
    PreSyn *ps;
    NetCvode* ns = net_cvode_instance;
    if( localNetCons )
        gid2out_->find( srcgid, ps );
    else
        gid2in_->find( srcgid, ps );
    if( !ps ) {  //no cells on this cpu receive from the given gid
        return;
    }
    
    //fprintf( stderr, "gid %d index %d!\n", srcgid, netconIndex );
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

int ncs_gid_receiving_info( int **presyngids ) {
    return ncs_bgp_target_info( presyngids );
}

//given the gid of a cell, retrieve its target count
int ncs_gid_sending_count( int **sendlist2build ) {
    if( !gid2out_ ) {
        fprintf( stderr, "gid2out_ not allocated\n" );
        return -1;
    }
    return ncs_bgp_sending_info( sendlist2build );
}

int ncs_target_hosts( int gid, int** targetnodes ) {
    return ncs_bgp_target_hosts( gid, targetnodes );
}

#endif

// for compressed gid info during spike exchange
bool nrn_use_localgid_;
void nrn_outputevent(unsigned char localgid, double firetime);
static Gid2PreSyn** localmaps_;

#define NRNSTAT 1
static int nsend_, nsendmax_, nrecv_, nrecv_useful_;
#if NRNSTAT
static IvocVect* max_histogram_;
#endif 

static int ocapacity_; // for spikeout_
// require it to be smaller than  min_interprocessor_delay.
static double wt_; // wait time for nrnmpi_spike_exchange
static double wt1_; // time to find the PreSyns and send the spikes.
static bool use_compress_;
static int spfixout_capacity_;
static int idxout_;
static void nrn_spike_exchange_compressed(NrnThread*);
#endif // NRNMPI

#if BGPDMA & 4
#define HAVE_DCMF_RECORD_REPLAY 1
#else
#define HAVE_DCMF_RECORD_REPLAY 0
#endif

#if BGPDMA
int use_dcmf_record_replay;
int use_bgpdma_; // can be 0, 1, or 2 : allgather, multisend (ISend, bgpdma)
static void bgp_dma_setup();
static void bgp_dma_init();
static void bgp_dma_receive(NrnThread*);
extern void bgp_dma_send(PreSyn*, double t);
static void bgpdma_cleanup_presyn(PreSyn*);
#endif

static int active_;
static double usable_mindelay_;
static double min_interprocessor_delay_;
static double mindelay_; // the one actually used. Some of our optional algorithms
static double last_maxstep_arg_;
static NetParEvent* npe_; // nrn_nthread of them
static int n_npe_; // just to compare with nrn_nthread

#if NRNMPI
// for combination of threads and mpi.
#if USE_PTHREAD
static MUTDEC
#endif
static int seqcnt_;
static NrnThread* last_nt_;
#endif

#if NRN_MUSIC
#include "nrnmusic.cpp"
#endif

NetParEvent::NetParEvent(){
	wx_ = ws_ = 0.;
	ithread_ = -1;
}
NetParEvent::~NetParEvent(){
}
void NetParEvent::send(double tt, NetCvode* nc, NrnThread* nt){
	nc->event(tt + usable_mindelay_, this, nt);
}
void NetParEvent::deliver(double tt, NetCvode* nc, NrnThread* nt){
	int seq;
	if (nrn_use_selfqueue_) { //first handle pending flag=1 self events
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
	MUTLOCK
	seq = ++seqcnt_;
	MUTUNLOCK
      if (seq == nrn_nthread) {
	last_nt_ = nt;
#if BGPDMA
	if (use_bgpdma_) {
		bgp_dma_receive(nt);
	}else{
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
void NetParEvent::pgvts_deliver(double tt, NetCvode* nc){
	assert(nrn_nthread == 1);
	deliver(tt, nc, nrn_threads);
}

void NetParEvent::pr(const char* m, double tt, NetCvode* nc){
	printf("%s NetParEvent %d t=%.15g tt-t=%g\n", m, ithread_, tt, tt - nrn_threads[ithread_]._t);
}

DiscreteEvent* NetParEvent::savestate_save(){
	//pr("savestate_save", 0, net_cvode_instance);
	NetParEvent* npe =  new NetParEvent();
	npe->ithread_ = ithread_;
	return npe;
}

DiscreteEvent* NetParEvent::savestate_read(FILE* f){
	int i;
	char buf[100];
	assert(fgets(buf, 100, f));
	assert(sscanf(buf, "%d\n", &i) == 1);
	//printf("NetParEvent::savestate_read %d\n", i);
	NetParEvent* npe = new NetParEvent();
	npe->ithread_ = i;
	return npe;
}

void NetParEvent::savestate_write(FILE* f){
	//pr("savestate_write", 0, net_cvode_instance);
	fprintf(f, "%d\n%d\n", NetParEventType, ithread_);
}

void NetParEvent::savestate_restore(double tt, NetCvode* nc){
#if NRNMPI
	if (use_compress_) {
		t_exchange_ = t;
	}
#endif
	if (ithread_ == 0) {
		//npe_->pr("savestate_restore", tt, nc);
		for (int i=0; i < nrn_nthread; ++i) {
			nc->event(tt, npe_+i, nrn_threads + i);
		}
	}
}

#if NRNMPI
inline static void sppk(unsigned char* c, int gid) {
	for (int i = localgid_size_-1; i >= 0; --i) {
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
	if (!active_) { return; }
	MUTLOCK
	nout_++;
	int i = idxout_;
	idxout_ += 2;
	if (idxout_ >= spfixout_capacity_) {
		spfixout_capacity_ *= 2;
		spfixout_ = (unsigned char*)hoc_Erealloc(spfixout_, spfixout_capacity_*sizeof(unsigned char)); hoc_malchk();
	}
	spfixout_[i++] = (unsigned char)((firetime - t_exchange_)*dt1_ + .5);
	spfixout_[i] = localgid;
//printf("%d idx=%d lgid=%d firetime=%g t_exchange_=%g [0]=%d [1]=%d\n", nrnmpi_myid, i, (int)localgid, firetime, t_exchange_, (int)spfixout_[i-1], (int)spfixout_[i]);
	MUTUNLOCK
}

#ifndef USENCS
void nrn2ncs_outputevent(int gid, double firetime) {
	if (!active_) { return; }
	MUTLOCK
    if (use_compress_) {
	nout_++;
	int i = idxout_;
	idxout_ += 1 + localgid_size_;
	if (idxout_ >= spfixout_capacity_) {
		spfixout_capacity_ *= 2;
		spfixout_ = (unsigned char*)hoc_Erealloc(spfixout_, spfixout_capacity_*sizeof(unsigned char)); hoc_malchk();
	}
//printf("%d nrnncs_outputevent %d %.20g %.20g %d\n", nrnmpi_myid, gid, firetime, t_exchange_,
//(int)((unsigned char)((firetime - t_exchange_)*dt1_ + .5)));
	spfixout_[i++] = (unsigned char)((firetime - t_exchange_)*dt1_ + .5);
//printf("%d idx=%d firetime=%g t_exchange_=%g spfixout=%d\n", nrnmpi_myid, i, firetime, t_exchange_, (int)spfixout_[i-1]);
	sppk(spfixout_+i, gid);
//printf("%d idx=%d gid=%d spupk=%d\n", nrnmpi_myid, i, gid, spupk(spfixout_+i));
    }else{
#if nrn_spikebuf_size == 0
	int i = nout_++;
	if (i >= ocapacity_) {
		ocapacity_ *= 2;
		spikeout_ = (NRNMPI_Spike*)hoc_Erealloc(spikeout_, ocapacity_*sizeof(NRNMPI_Spike)); hoc_malchk();
	}		
//printf("%d cell %d in slot %d fired at %g\n", nrnmpi_myid, gid, i, firetime);
	spikeout_[i].gid = gid;
	spikeout_[i].spiketime = firetime;
#else
	int i = nout_++;
	if (i >= nrn_spikebuf_size) {
		i -= nrn_spikebuf_size;
		if (i >= ocapacity_) {
			ocapacity_ *= 2;
			spikeout_ = (NRNMPI_Spike*)hoc_Erealloc(spikeout_, ocapacity_*sizeof(NRNMPI_Spike)); hoc_malchk();
		}		
		spikeout_[i].gid = gid;
		spikeout_[i].spiketime = firetime;
	}else{
		spbufout_->gid[i] = gid;
		spbufout_->spiketime[i] = firetime;
	}
#endif
    }
	MUTUNLOCK
//printf("%d cell %d in slot %d fired at %g\n", nrnmpi_myid, gid, i, firetime);
}
#endif //USENCS
#endif // NRNMPI

static int nrn_need_npe() {
	
	int b = 0;
	if (active_) { b = 1; }
	if (nrn_use_selfqueue_) { b = 1; }
	if (nrn_nthread > 1) { b = 1; }
	if (b) {
		if (last_maxstep_arg_ == 0) {
			last_maxstep_arg_ =   100.;
		}
		set_mindelay(last_maxstep_arg_);
	}else{
		if (npe_) {
			delete [] npe_;
			npe_ = NULL;
			n_npe_ = 0;
		}
	}
	return b;
}

static void calc_actual_mindelay() {
	//reasons why mindelay_ can be smaller than min_interprocessor_delay
	// are use_bgpdma_ when BGP_INTERVAL == 2
#if BGPDMA && (BGP_INTERVAL == 2)
	if (use_bgpdma_ && n_bgp_interval == 2) {
		mindelay_ = min_interprocessor_delay_ / 2.;
	}else{
		mindelay_ = min_interprocessor_delay_;
	}
#endif
}

#if BGPDMA
#include "bgpdma.cpp"
#else
#define TBUFSIZE 0
#define TBUF /**/
#endif

void nrn_spike_exchange_init() {
#ifdef USENCS
    bgp_dma_setup();
    return;
#endif
//printf("nrn_spike_exchange_init\n");
	if (!nrn_need_npe()) { return; }
//	if (!active_ && !nrn_use_selfqueue_) { return; }
	alloc_space();
//printf("nrnmpi_use=%d active=%d\n", nrnmpi_use, active_);
	calc_actual_mindelay();	
	usable_mindelay_ = mindelay_;
	if (cvode_active_ == 0 && nrn_nthread > 1) {
		usable_mindelay_ -= dt;
	}
	if ((usable_mindelay_ < 1e-9) || (cvode_active_ == 0 && usable_mindelay_ < dt)) {
		if (nrnmpi_myid == 0) {
			hoc_execerror("usable mindelay is 0", "(or less than dt for fixed step method)");
		}else{
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

#if BGPDMA
	if (use_bgpdma_) {
		bgp_dma_init();
	}
#endif

	if (n_npe_ != nrn_nthread) {
		if (npe_) { delete [] npe_; }
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
	dt1_ = 1./dt;
	usable_mindelay_ = floor(mindelay_ * dt1_ + 1e-9) * dt;
	assert (usable_mindelay_ >= dt && (usable_mindelay_ * dt1_) < 255);
    }else{
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
#if USE_PTHREAD
			if (!mut_) {
				MUTCONSTRUCT(1)
			}
#endif
		}else{
			MUTDESTRUCT
		}
	}
#endif // NRNMPI
	//if (nrnmpi_myid == 0){printf("usable_mindelay_ = %g\n", usable_mindelay_);}
}

#if NRNMPI
void nrn_spike_exchange(NrnThread* nt) {
	if (!active_) { return; }
#if BGPDMA
	if (use_bgpdma_) {
		bgp_dma_receive(nt);
		return;
	}
#endif
	if (use_compress_) { nrn_spike_exchange_compressed(nt); return; }
	TBUF
#if TBUFSIZE
	nrnmpi_barrier();
#endif
	TBUF
	double wt;
	int i, n;
#if NRNSTAT
	nsend_ += nout_;
	if (nsendmax_ < nout_) { nsendmax_ = nout_; }
#endif
#if nrn_spikebuf_size > 0
	spbufout_->nspike = nout_;
#endif
	wt = nrnmpi_wtime();
	n = nrnmpi_spike_exchange();
	wt_ = nrnmpi_wtime() - wt;
	wt = nrnmpi_wtime();
	TBUF
#if TBUFSIZE
	tbuf_[itbuf_++] = (unsigned long)nout_;
	tbuf_[itbuf_++] = (unsigned long)n;
#endif

	errno = 0;
//if (n > 0) {
//printf("%d nrn_spike_exchange sent %d received %d\n", nrnmpi_myid, nout_, n);
//}
	nout_ = 0;
	if (n == 0) {
#if NRNSTAT
		if (max_histogram_) { vector_vec(max_histogram_)[0] += 1.; }
#endif
		TBUF
		return;
	}
#if NRNSTAT
	nrecv_ += n;
	if (max_histogram_) {
		int mx = 0;
		if (n > 0) {
			for (i=nrnmpi_numprocs-1 ; i >= 0; --i) {
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
		int ms = vector_capacity(max_histogram_)-1;
		mx = (mx < ms) ? mx : ms;
		vector_vec(max_histogram_)[mx] += 1.;
	}
#endif // NRNSTAT
#if nrn_spikebuf_size > 0
	for (i = 0; i < nrnmpi_numprocs; ++i) {
		int j;
		int nn = spbufin_[i].nspike;
		if (nn > nrn_spikebuf_size) { nn = nrn_spikebuf_size; }
		for (j=0; j < nn; ++j) {
			PreSyn* ps;
			if (gid2in_->find(spbufin_[i].gid[j], ps)) {
				ps->send(spbufin_[i].spiketime[j], net_cvode_instance, nt);
#if NRNSTAT
				++nrecv_useful_;
#endif
			}
		}
	}
	n = ovfl_;
#endif // nrn_spikebuf_size > 0
	for (i = 0; i < n; ++i) {
		PreSyn* ps;
		if (gid2in_->find(spikein_[i].gid, ps)) {
			ps->send(spikein_[i].spiketime, net_cvode_instance, nt);
#if NRNSTAT
			++nrecv_useful_;
#endif
		}
	}
	wt1_ = nrnmpi_wtime() - wt;
	TBUF
}
		
void nrn_spike_exchange_compressed(NrnThread* nt) {
	if (!active_) { return; }
	TBUF
#if TBUFSIZE
	nrnmpi_barrier();
#endif
	TBUF
	assert(!cvode_active_);
	double wt;
	int i, n, idx;
#if NRNSTAT
	nsend_ += nout_;
	if (nsendmax_ < nout_) { nsendmax_ = nout_; }
#endif
	assert(nout_ < 0x10000);
	spfixout_[1] = (unsigned char)(nout_ & 0xff);
	spfixout_[0] = (unsigned char)(nout_>>8);

	wt = nrnmpi_wtime();
	n = nrnmpi_spike_exchange_compressed();
	wt_ = nrnmpi_wtime() - wt;
	wt = nrnmpi_wtime();
	TBUF
#if TBUFSIZE             
        tbuf_[itbuf_++] = (unsigned long)nout_;
        tbuf_[itbuf_++] = (unsigned long)n;
#endif
	errno = 0;
//if (n > 0) {
//printf("%d nrn_spike_exchange sent %d received %d\n", nrnmpi_myid, nout_, n);
//}
	nout_ = 0;
	idxout_ = 2;
	if (n == 0) {
#if NRNSTAT
		if (max_histogram_) { vector_vec(max_histogram_)[0] += 1.; }
#endif
		t_exchange_ = nrn_threads->_t;
		TBUF
		return;
	}
#if NRNSTAT
	nrecv_ += n;
	if (max_histogram_) {
		int mx = 0;
		if (n > 0) {
			for (i=nrnmpi_numprocs-1 ; i >= 0; --i) {
				if (mx < nin_[i]) {
					mx = nin_[i];
				}
			}
		}
		int ms = vector_capacity(max_histogram_)-1;
		mx = (mx < ms) ? mx : ms;
		vector_vec(max_histogram_)[mx] += 1.;
	}
#endif // NRNSTAT
    if (nrn_use_localgid_) {
	int idxov = 0;
	for (i = 0; i < nrnmpi_numprocs; ++i) {
		int j, nnn;
		int nn = nin_[i];
	    if (nn) {
		if (i == nrnmpi_myid) { // skip but may need to increment idxov.
			if (nn > ag_send_nspike_) {
				idxov += (nn - ag_send_nspike_)*(1 + localgid_size_);
			}
			continue;
		}
		Gid2PreSyn* gps = localmaps_[i];
		if (nn > ag_send_nspike_) {
			nnn = ag_send_nspike_;
		}else{
			nnn = nn;
		}
		idx = 2 + i*ag_send_size_;
		for (j=0; j < nnn; ++j) {
			// order is (firetime,gid) pairs.
			double firetime = spfixin_[idx++]*dt + t_exchange_;
			int lgid = (int)spfixin_[idx];
			idx += localgid_size_;
			PreSyn* ps;
			if (gps->find(lgid, ps)) {
				ps->send(firetime + 1e-10, net_cvode_instance, nt);
#if NRNSTAT
				++nrecv_useful_;
#endif
			}
		}
		for ( ; j < nn; ++j) {
			double firetime = spfixin_ovfl_[idxov++]*dt + t_exchange_;
			int lgid = (int)spfixin_ovfl_[idxov];
			idxov += localgid_size_;
			PreSyn* ps;
			if (gps->find(lgid, ps)) {
				ps->send(firetime+1e-10, net_cvode_instance, nt);
#if NRNSTAT
				++nrecv_useful_;
#endif
			}
		}
	    }
	}
    }else{
	for (i = 0; i < nrnmpi_numprocs; ++i) {
		int j;
		int nn = nin_[i];
		if (nn > ag_send_nspike_) { nn = ag_send_nspike_; }
		idx = 2 + i*ag_send_size_;
		for (j=0; j < nn; ++j) {
			// order is (firetime,gid) pairs.
			double firetime = spfixin_[idx++]*dt + t_exchange_;
			int gid = spupk(spfixin_ + idx);
			idx += localgid_size_;
			PreSyn* ps;
			if (gid2in_->find(gid, ps)) {
				ps->send(firetime+1e-10, net_cvode_instance, nt);
#if NRNSTAT
				++nrecv_useful_;
#endif
			}
		}
	}
	n = ovfl_;
	idx = 0;
	for (i = 0; i < n; ++i) {
		double firetime = spfixin_ovfl_[idx++]*dt + t_exchange_;
		int gid = spupk(spfixin_ovfl_ + idx);
		idx += localgid_size_;
		PreSyn* ps;
		if (gid2in_->find(gid, ps)) {
			ps->send(firetime+1e-10, net_cvode_instance, nt);
#if NRNSTAT
			++nrecv_useful_;
#endif
		}
	}
    }
	t_exchange_ = nrn_threads->_t;
	wt1_ = nrnmpi_wtime() - wt;
	TBUF
}

static void mk_localgid_rep() {
	int i, j, k;
	PreSyn* ps;

	// how many gids are there on this machine
	// and can they be compressed into one byte
	int ngid = 0;
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			++ngid;
		}
	}}}
	int ngidmax = nrnmpi_int_allmax(ngid);
	if (ngidmax > 256) {
		//do not compress
		return;
	}
	localgid_size_ = sizeof(unsigned char);
	nrn_use_localgid_ = true;

	// allocate Allgather receive buffer (send is the nrnmpi_myid one)
	int* rbuf = new int[nrnmpi_numprocs*(ngidmax + 1)];
	int* sbuf = new int[ngidmax + 1];

	sbuf[0] = ngid;
	++sbuf;
	ngid = 0;
	// define the local gid and fill with the gids on this machine
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			ps->localgid_ = (unsigned char)ngid;
			sbuf[ngid] = ps->output_index_;
			++ngid;
		}
	}}}
	--sbuf;

	// exchange everything
	nrnmpi_int_allgather(sbuf, rbuf, ngidmax+1);
	delete [] sbuf;
	errno = 0;

	// create the maps
	// there is a lot of potential for efficiency here. i.e. use of
	// perfect hash functions, or even simple Vectors.
	localmaps_ = new Gid2PreSyn*[nrnmpi_numprocs];
	localmaps_[nrnmpi_myid] = 0;
	for (i = 0; i < nrnmpi_numprocs; ++i) if (i != nrnmpi_myid) {
		// how many do we need?
		sbuf = rbuf + i*(ngidmax + 1);
		ngid = *(sbuf++); // at most
		// of which we actually use...
		for (k=0, j=0; k < ngid; ++k) {
			if (gid2in_ && gid2in_->find(int(sbuf[k]), ps)) {
				++j;
			}
		}
		// oh well. there is probably a rational way to choose but...
		localmaps_[i] = new Gid2PreSyn((j > 19) ? 19 : j+1);
	}

	// fill in the maps
	for (i = 0; i < nrnmpi_numprocs; ++i) if (i != nrnmpi_myid) {
		sbuf = rbuf + i*(ngidmax + 1);
		ngid = *(sbuf++);
		for (k=0; k < ngid; ++k) {
			if (gid2in_ && gid2in_->find(int(sbuf[k]), ps)) {
#if ALTHASH
				localmaps_[i]->insert(k, ps);
#else
				(*localmaps_[i])[k] = ps;
#endif
			}
		}
	}

	// cleanup
	delete [] rbuf;
}

#endif // NRNMPI

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
	assert(gid2in_);
	PreSyn* ps;
	if (gid2in_->find(gid, ps)) {
		assert(ps);
//printf("nrn_fake_fire %d %g\n", gid, spiketime);
		ps->send(spiketime, net_cvode_instance, nrn_threads);
#if NRNSTAT
		++nrecv_useful_;
#endif
	}else if (fake_out && gid2out_->find(gid, ps)) {
		assert(ps);
//printf("nrn_fake_fire fake_out %d %g\n", gid, spiketime);
		ps->send(spiketime, net_cvode_instance, nrn_threads);
#if NRNSTAT
		++nrecv_useful_;
#endif
	}

}

static void alloc_space() {
	if (!gid2out_) {
		netcon_sym_ = hoc_lookup("NetCon");
#if ALTHASH
		gid2out_ = new Gid2PreSyn(1000);
		gid2in_ = new Gid2PreSyn(500000);
#else
		gid2out_ = new Gid2PreSyn(211);
		gid2in_ = new Gid2PreSyn(2311);
#endif
#if NRNMPI
		ocapacity_  = 100;
		spikeout_ = (NRNMPI_Spike*)hoc_Emalloc(ocapacity_*sizeof(NRNMPI_Spike)); hoc_malchk();
		icapacity_  = 100;
		spikein_ = (NRNMPI_Spike*)hoc_Emalloc(icapacity_*sizeof(NRNMPI_Spike)); hoc_malchk();
		nin_ = (int*)hoc_Emalloc(nrnmpi_numprocs*sizeof(int)); hoc_malchk();
#if nrn_spikebuf_size > 0
spbufout_ = (NRNMPI_Spikebuf*)hoc_Emalloc(sizeof(NRNMPI_Spikebuf)); hoc_malchk();
spbufin_ = (NRNMPI_Spikebuf*)hoc_Emalloc(nrnmpi_numprocs*sizeof(NRNMPI_Spikebuf)); hoc_malchk();
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
//printf("gid %d defined on %d\n", gid, nrnmpi_myid);
		PreSyn* ps;
		char m[200];
		if (gid2in_->find(gid, ps)) {
			sprintf(m, "gid=%d already exists as an input port", gid);
			hoc_execerror(m, "Setup all the output ports on this process before using them as input ports.");
		}
		if (gid2out_->find(gid, ps)) {
			sprintf(m, "gid=%d already exists on this process as an output port", gid);
			hoc_execerror(m, 0);                            
		}
#if ALTHASH
		gid2out_->insert(gid, NULL);
#else
		(*gid2out_)[gid] = NULL;
#endif
//		gid2out_->insert(pair<const int, PreSyn*>(gid, NULL));
	}
}

static int gid2in_donot_remove = 0; // avoid  gid2in_ removal when iterating gid2in_

void nrn_cleanup_presyn(PreSyn* ps) {
#if BGPDMA
	bgpdma_cleanup_presyn(ps);
#endif
	PreSyn* pss;
	if (ps->gid_ >= 0 && gid2in_ && gid2in_donot_remove == 0) {
		gid2in_->remove(ps->gid_);
	}
}

void nrnmpi_gid_clear(int arg) {
	if (arg == 0 || arg == 3 || arg == 4) {
		nrn_partrans_clear();
#if PARANEURON
		nrnmpi_split_clear();
#endif
	}
	if (arg == 0 || arg == 2 || arg == 4) { nrnmpi_multisplit_clear(); }
	if (arg == 2 || arg == 3) { return; }
	if (!gid2out_) { return; }
	PreSyn* ps, *psi;
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps && !gid2in_->find(ps->gid_, psi)) {
		    if (arg == 4) {
			delete ps;
		    }else{
#if BGPDMA
			bgpdma_cleanup_presyn(ps);
#endif
			ps->gid_ = -1;
			ps->output_index_ = -1;
			if (ps->dil_.count() == 0) {
				delete ps;
			}
		    }
		}
	}}}
	gid2in_donot_remove = 1;
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
	    if (arg == 4) {
		delete ps;
	    }else{
#if BGPDMA
		bgpdma_cleanup_presyn(ps);
#endif
		ps->gid_ = -1;
		ps->output_index_ = -1;
		if (ps->dil_.count() == 0) {
			delete ps;
		}
	    }
	}}}
	gid2in_donot_remove = 0;
#if ALTHASH
	gid2in_->remove_all();
	gid2out_->remove_all();
#else
	int i;
	for (i = gid2out_->size_ - 1; i >= 0; --i) {
		gid2out_->at(i).clear();
	}
	for (i = gid2in_->size_ - 1; i >= 0; --i) {
		gid2in_->at(i).clear();
	}
#endif
}

int BBS::gid_exists(int gid) {
	PreSyn* ps;
	alloc_space();
	if (gid2out_->find(gid, ps)) {
//printf("%d gid %d exists\n", nrnmpi_myid, gid);
		if (ps) {
			return (ps->output_index_ >= 0 ? 3 : 2);
		}else{
			return 1;
		}
	}
	return 0;
}

double BBS::threshold() {
	int gid = int(chkarg(1, 0., MD));
	PreSyn* ps;
	assert(gid2out_->find(gid, ps));
	assert(ps);
	if (ifarg(2)) {
		ps->threshold_ = *getarg(2);
	}
	return ps->threshold_;
}

void BBS::cell() {
	int gid = int(chkarg(1, 0., MD));
	PreSyn* ps;
	if (gid2in_->find(gid, ps)) {
		char buf[100];
		sprintf(buf, "gid=%d is in the input list. Must register prior to connecting", gid);
		hoc_execerror(buf, 0);
	}
	if (gid2out_->find(gid, ps) == 0) {
		char buf[100];
		sprintf(buf, "gid=%d has not been set on rank %d", gid, nrnmpi_myid);
		hoc_execerror(buf, 0);
	}
	Object* ob = *hoc_objgetarg(2);
	if (!ob || ob->ctemplate != netcon_sym_->u.ctemplate) {
		check_obj_type(ob, "NetCon");
	}
	NetCon* nc = (NetCon*)ob->u.this_pointer;
	ps = nc->src_;
//printf("%d cell %d %s\n", nrnmpi_myid, gid, hoc_object_name(ps->ssrc_ ? nrn_sec2cell(ps->ssrc_) : ps->osrc_));
#if ALTHASH
	gid2out_->insert(gid, ps);
#else
	(*gid2out_)[gid] = ps;
#endif
	ps->gid_ = gid;
	if (ifarg(3) && !chkarg(3, 0., 1.)) {
		ps->output_index_ = -2; //prevents destruction of PreSyn
	}else{
		ps->output_index_ = gid;
	}
}

void BBS::outputcell(int gid) {
	PreSyn* ps;
	assert(gid2out_->find(gid, ps));
	assert(ps);
	ps->output_index_ = gid;
	ps->gid_ = gid;
}

void BBS::spike_record(int gid, IvocVect* spikevec, IvocVect* gidvec) {
	PreSyn* ps;
    if (gid >= 0) {
	assert(gid2out_->find(gid, ps));
	assert(ps);
	ps->record(spikevec, gidvec, gid);
    }else{ // record all output spikes
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			ps->record(spikevec, gidvec, ps->output_index_);
		}
	}}}
    }
}

Object** BBS::gid2obj(int gid) {
	Object* cell = 0;
//printf("%d gid2obj gid=%d\n", nrnmpi_myid, gid);
	PreSyn* ps;
	assert(gid2out_->find(gid, ps));
//printf(" found\n");
	assert(ps);
	cell = ps->ssrc_ ? nrn_sec2cell(ps->ssrc_) : ps->osrc_;
//printf(" return %s\n", hoc_object_name(cell));
	return hoc_temp_objptr(cell);
}

Object** BBS::gid2cell(int gid) {
	Object* cell = 0;
//printf("%d gid2obj gid=%d\n", nrnmpi_myid, gid);
	PreSyn* ps;
	assert(gid2out_->find(gid, ps));
//printf(" found\n");
	assert(ps);
	if (ps->ssrc_) {
		cell = nrn_sec2cell(ps->ssrc_);
	}else{
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
//printf(" return %s\n", hoc_object_name(cell));
	return hoc_temp_objptr(cell);
}

Object** BBS::gid_connect(int gid) {
	Object* target = *hoc_objgetarg(2);
	if (!is_point_process(target)) {
		hoc_execerror("arg 2 must be a point process", 0);
	}
	alloc_space();
	PreSyn* ps;
	if (gid2out_->find(gid, ps)) {
		// the gid is owned by this machine so connect directly
		if (!ps) {
			char buf[100];
			sprintf(buf, "gid %d owned by %d but no associated cell", gid, nrnmpi_myid);
			hoc_execerror(buf, 0);
		}
	}else if (gid2in_->find(gid, ps)) {
		// the gid stub already exists
//printf("%d connect %s from already existing %d\n", nrnmpi_myid, hoc_object_name(target), gid);
	}else{
//printf("%d connect %s from new PreSyn for %d\n", nrnmpi_myid, hoc_object_name(target), gid);
		ps = new PreSyn(NULL, NULL, NULL);
		net_cvode_instance->psl_append(ps);
#if ALTHASH
		gid2in_->insert(gid, ps);
#else
		(*gid2in_)[gid] = ps;
#endif
		ps->gid_ = gid;
	}
	NetCon* nc;
	Object** po;
	if (ifarg(3)) {
		po = hoc_objgetarg(3);
		if (!*po || (*po)->ctemplate != netcon_sym_->u.ctemplate) {
			check_obj_type(*po, "NetCon");
		}
		nc = (NetCon*)((*po)->u.this_pointer);
		if (nc->target_ != ob2pntproc(target)) {
			hoc_execerror("target is different from 3rd arg NetCon target", 0);
		}
		nc->replace_src(ps);
	}else{
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
#if NRNMPI
	double mt, md;
	tstopunset;
	if (cvode_active_) {
		mt = 1e-9 ; md = mindelay_;
	}else{
		mt = dt ; md = mindelay_ - 1e-10;
	}
	if (md < mt) {
		if (nrnmpi_myid == 0) {
			hoc_execerror("mindelay is 0", "(or less than dt for fixed step method)");
		}else{
			return;
		}
	}
	double wt;

	nrnmpi_barrier(); // make sure all integrations start about the same
	nrn_timeout(timeout_); //time to avoid spurious timeouts while waiting
				// at the next MPI_collective.
	wt = nrnmpi_wtime();
	if (cvode_active_) {
		ncs2nrn_integrate(tstop);
	}else{
		ncs2nrn_integrate(tstop*(1.+1e-11));
	}
	impl_->integ_time_ += nrnmpi_wtime() - wt;
	impl_->integ_time_ -= (npe_ ? (npe_[0].wx_ + npe_[0].ws_) : 0.);
#if BGPDMA
	if (use_bgpdma_) {
#if BGP_INTERVAL == 2
		for (int i=0; i < n_bgp_interval; ++i) {
			bgp_dma_receive(nrn_threads);
		}
#else
		bgp_dma_receive(nrn_threads);
#endif
	}else{
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
//printf("%d netpar_solve exit t=%g tstop=%g mindelay_=%g\n",nrnmpi_myid, t, tstop, mindelay_);
#else // not NRNMPI
	ncs2nrn_integrate(tstop);
#endif
	tstopunset;
}

static double set_mindelay(double maxdelay) {
	double mindelay = maxdelay;
	last_maxstep_arg_ = maxdelay;
    if (nrn_use_selfqueue_ || net_cvode_instance->localstep() || nrn_nthread > 1 ) {
	hoc_Item* q;
	if (net_cvode_instance->psl_) ITERATE(q, net_cvode_instance->psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		double md = ps->mindelay();
		if (mindelay > md) {
			mindelay = md;
		}
	}
    }
#if NRNMPI
    else{
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		double md = ps->mindelay();
		if (mindelay > md) {
			mindelay = md;
		}
	}}}
    }
	if (nrnmpi_use) {active_ = 1;}
	if (use_compress_) {
		if (mindelay/dt > 255) {
			mindelay = 255*dt;
		}
	}

//printf("%d netpar_mindelay local %g now calling nrnmpi_mindelay\n", nrnmpi_myid, mindelay);
//	double st = time();
	mindelay_ = nrnmpi_mindelay(mindelay);
	min_interprocessor_delay_ = mindelay_;
//	add_wait_time(st);
//printf("%d local min=%g  global min=%g\n", nrnmpi_myid, mindelay, mindelay_);
	if (mindelay_ < 1e-9 && nrn_use_selfqueue_) {
		nrn_use_selfqueue_ = 0;
		double od = mindelay_;
		mindelay = set_mindelay(maxdelay);
		if (nrnmpi_myid == 0) {
printf("Notice: The global minimum NetCon delay is %g, so turned off the cvode.queue_mode\n", od);
printf("   use_self_queue option. The interprocessor minimum NetCon delay is %g\n", mindelay);
		}
	}
	errno = 0;
	return mindelay;
#else
	mindelay_ = mindelay;
	min_interprocessor_delay_ = mindelay_;
	return mindelay;
#endif //NRNMPI
}

double BBS::netpar_mindelay(double maxdelay) {
#if BGPDMA
	bgp_dma_setup();
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
#if NRNMPI && NRNSTAT
	IvocVect* h = max_histogram_;
	if (max_histogram_) {
		max_histogram_ = NULL;
	}
	if (mh) {
		max_histogram_ = mh;
	}
	return h;
#else
	return NULL;
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

Allgather
multisend implemented as MPI_ISend
multisend DCMF (only for Blue Gene/P)
multisend record_replay (only for Blue Gene/P with recordreplay_v1r4m2.patch)

n_bgp_interval 1 or 2 per minimum interprocessor NetCon delay
 that concept valid for all methods

Note that Allgather allows spike compression and an allgather spike buffer
 with size chosen at setup time.  All methods allow bin queueing.

All the multisend methods should allow two phase multisend.

Note that, in principle, MPI_ISend allows the source to send the index   
 of the target PreSyn to avoid a hash table lookup (even with a two phase
 variant)

Not all variation are useful. e.g. it is pointless to combine Allgather and
n_bgp_interval=2.
RecordReplay should be best on the BG/P. The whole point is to make the
spike transfer initiation as lowcost as possible since that is what causes
most load imbalance. I.e. since 10K more spikes arrive than are sent, spikes
received per processor per interval are much more statistically
balanced than spikes sent per processor per interval. And presently
DCMF multisend injects 10000 messages per spike into the network which
is quite expensive. record replay avoids this overhead and the idea of
two phase multisend distributes the injection

See case 8 of nrn_bgp_receive_time for the xchng_meth properties
*/

int nrnmpi_spike_compress(int nspike, bool gid_compress, int xchng_meth) {
#if NRNMPI
	if (nrnmpi_numprocs < 2) { return 0; }
#if BGP_INTERVAL == 2
	n_bgp_interval = (xchng_meth & 4) ? 2 : 1;
#endif
#if BGPDMA
	use_bgpdma_ = (xchng_meth & 3);
	if (use_bgpdma_ == 3) {	assert(HAVE_DCMF_RECORD_REPLAY); }
#if TWOPHASE
	use_phase2_ = (xchng_meth & 8) ? 1 : 0;
	if (nrnmpi_myid == 0) {printf("use_phase2_ = %d\n", use_phase2_);}
#endif
#if HAVE_DCMF_RECORD_REPLAY
	use_dcmf_record_replay = (use_bgpdma_ == 3 ? 1 : 0);
	if (nrnmpi_myid == 0) {printf("use_dcmf_record_replay = %d\n", use_dcmf_record_replay);}
#endif
	if (use_bgpdma_ == 3) { use_bgpdma_ = 2; }
	if (use_bgpdma_ == 2) { assert(BGPDMA & 2); }
	if (use_bgpdma_ == 1) { assert(BGPDMA & 1); }
	if (nrnmpi_myid == 0) {printf("use_bgpdma_ = %d\n", use_bgpdma_);}
#else // BGPDMA == 0
	assert(xchng_meth == 0);
#endif
	if (nspike >= 0) {
		ag_send_nspike_ = 0;
		if (spfixout_) { free(spfixout_); spfixout_ = 0; }
		if (spfixin_) { free(spfixin_); spfixin_ = 0; }
		if (spfixin_ovfl_) { free(spfixin_ovfl_); spfixin_ovfl_ = 0; }
		if (localmaps_) {
			for (int i=0; i < nrnmpi_numprocs; ++i) if (i != nrnmpi_myid) {
				if (localmaps_[i]) { delete localmaps_[i]; }
			}
			delete [] localmaps_;
			localmaps_ = 0;
		}
	}
	if (nspike == 0) { // turn off
		use_compress_ = false;
		nrn_use_localgid_ = false;
	}else if (nspike > 0) { // turn on
		if (cvode_active_) {
if (nrnmpi_myid == 0) {hoc_warning("ParallelContext.spike_compress cannot be used with cvode active", 0);}
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
printf("Notice: gid compression did not succeed. Probably more than 255 cells on one cpu.\n");
			}
		}
		if (!nrn_use_localgid_) {
			localgid_size_ = sizeof(unsigned int);
		}
		ag_send_size_ = 2 + ag_send_nspike_*(1 + localgid_size_);
		spfixout_capacity_ = ag_send_size_ + 50*(1 + localgid_size_);
		spfixout_ = (unsigned char*)hoc_Emalloc(spfixout_capacity_); hoc_malchk();
		spfixin_ = (unsigned char*)hoc_Emalloc(nrnmpi_numprocs*ag_send_size_); hoc_malchk();
		ovfl_capacity_ = 100;
		spfixin_ovfl_ = (unsigned char*)hoc_Emalloc(ovfl_capacity_*(1 + localgid_size_)); hoc_malchk();
	}
	return ag_send_nspike_;
#else
	return 0;
#endif
}

PreSyn* nrn_gid2outputpresyn(int gid) { // output PreSyn
        PreSyn* ps;
        if (gid2out_->find(gid, ps)) {
	        return ps;
	}
	return NULL;
}
