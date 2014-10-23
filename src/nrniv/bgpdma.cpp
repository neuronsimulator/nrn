// included by netpar.cpp

/*
Overall exchange strategy

When a cell spikes, it immediately does a DCMF_Multicast of
(int gid, double spiketime) to all the target machines that have
cells that need to receive this spike by spiketime + delay
I'd like to cycle through a list of mconfig.nconnections so that
I don't have to wait for my single connection to complete the previous
broadcast when there is a high density of generated spikes but I need
to take care of my bus error issues first.

In order to minimize the number of nrnmpi_bgp_conserve tests
(and potentially abandon them altogether if I can ever guarantee
that exchange time is less than half the computation time), I divide the
minimum delay integration intervals into two equal subintervals.
So if a spike is generated in an even subinterval, I do not have
to include it in the conservation check until the end of the next even
subinterval.

When a spike is received (DMA interrupt) it is placed in even or odd
buffers (depending on whether the coded gid is positive or negative)

At the end of a computation subinterval the even or odd buffer spikes
are enqueued in the priority queue after checking that the number
of spikes sent is equal to the number of spikes sent.
*/

extern "C" {

extern IvocVect* vector_arg(int);
extern void vector_resize(IvocVect*, int);

extern void (*nrntimeout_call)();

}

// The initial idea behind TWOPHASE is to avoid the large overhead of
// initiating a send of the up to 10k list of target hosts when a cell fires.
// I.e. when there are a small number of cells on a processor, this causes
// load balance problems.
// Load balance shuld be better if the send is distributed to a much smaller
// set of targets, which, when they receive the spike, pass it on to a neighbor
// set. A non-exclusive alternative to this is the use of RECORD_REPLAY
// which give a very fast initiation but we have not been able to get that
// to complete in the sense of all the targets receiving their spikes before
// the conservation step.
// We expect that TWOPHASE will work best in combination with ENQUEUE=2
// which has the greatest amount of overlap between computation
// and communication.
// Note: the old implementation assumed that input PreSyn did not need
// a BGP_DMASend pointer so used the PreSyn.bgp.srchost_ element to
// help figure out the target_hosts_ list for the output PreSyn.bgp.dma_send_.
// Since bgp.srchost_ is used only for setup, it can be overwritten at phase2
// setup time with a bgp.dma_send_ so as to pass on the spike to the
// phase2 list of target hosts.

// set to 1 if you have problems with Record_Replay when some cells send
// spikes to fewer than 4 hosts.
#define WORK_AROUND_RECORD_BUG 0

#define TWOPHASE 1 // 0 no longer allowed since 672:544c61a730ec

#if BGPDMA & 2
#include <dcmf_multisend.h>
#include <dcmf.h>
#define DCMFTICK DCMF_Tick();
#define DCMFTIMEBASE DCMF_Timebase()
#endif

#if !defined(DCMFTICK)
#define DCMFTICK 0
#define DCMFTIMEBASE 0
#endif

static unsigned long long dmasend_time_;
static int n_xtra_cons_check_;
#define MAXNCONS 10
#if MAXNCONS
static int xtra_cons_hist_[MAXNCONS+1];
#endif

// asm/msr.h no longer compiles on my machine.
// only for basic testing of logic when not on blue gene/p
#define USE_RDTSCL 0

// only use if careful not to overrun the buffer during a simulation
#if 0 && (BGPDMA > 1 || USE_RDTSCL)
#define TBUFSIZE (1<<15)
#else
#define TBUFSIZE 0
#endif

#if TBUFSIZE
static unsigned long tbuf_[TBUFSIZE];
static int itbuf_;
#if USE_RDTSCL // but can have many accuracy problems on recent cpus.
/* for rdtscl() */
//#include <asm/msr.h>
#define rdtscl(a) a++
static unsigned long t__;
#define TBUF {rdtscl(t__); tbuf_[itbuf_++] = t__;}
#else
#define TBUF tbuf_[itbuf_++] = (unsigned long)DCMF_Timebase();
#endif // not USE_RDTSCLL
#else
#define TBUF /**/
#endif

// ENQUEUE 0 means to  BGP_ReceiveBuffer buffer -> PreSyn.send
// ENQUEUE 1 means to BGP_ReceiveBuffer buffer -> psbuf -> PreSyn.send
// ENQUEUE 2 means to BGP_ReceiveBuffer.incoming -> PrySyn.send
// Note that ENQUEUE 2 give more overlap between computation and exchange
// since the enqueuing takes place during computation except for those
// remaining during conservation.
#define ENQUEUE 2

#if ENQUEUE == 2
static unsigned long enq2_find_time_;
static unsigned long enq2_enqueue_time_; // includes enq_find_time_
#endif

#if TWOPHASE
#define PHASE2BUFFER_SIZE 2048 // power of 2
#define PHASE2BUFFER_MASK (PHASE2BUFFER_SIZE - 1)
struct Phase2Buffer {
	PreSyn* ps;
	double spiketime;
};
#endif

#include <structpool.h>

declareStructPool(SpkPool, NRNMPI_Spike)
implementStructPool(SpkPool, NRNMPI_Spike)

#define BGP_RECEIVEBUFFER_SIZE 10000
class BGP_ReceiveBuffer {
public:
	BGP_ReceiveBuffer();
	virtual ~BGP_ReceiveBuffer();
	void init(int index);
	void incoming(int gid, double spiketime);
	void enqueue();
	int index_;
	int size_;
	int count_;
	int maxcount_;
	int busy_;
	int nsend_, nrecv_; // for checking conservation
	int nsend_cell_; // cells that spiked this interval.
	unsigned long long timebase_;
	NRNMPI_Spike** buffer_;
	SpkPool* pool_;

	void enqueue1();
	void enqueue2();
	PreSyn** psbuf_;
#if TWOPHASE
	void phase2send();
	int phase2_head_;
	int phase2_tail_;
	int phase2_nsend_cell_, phase2_nsend_;
	Phase2Buffer* phase2_buffer_;
#endif
};

#if TWOPHASE
static int use_phase2_;
static void setup_phase2();
#define NTARGET_HOSTS_PHASE1 ntarget_hosts_phase1_
#else
#define NTARGET_HOSTS_PHASE1 ntarget_hosts_
#endif

class BGP_DMASend {
public:
	BGP_DMASend();
	virtual ~BGP_DMASend();
	void send(int gid, double t);
	int ntarget_hosts_;
	int* target_hosts_;
	NRNMPI_Spike spk_;
#if 0
	// There is no possibility of send2self because an output PreSyn
	// is never in the gid2in_ table.
	int send2self_; // if 1 then send spikes to this host also
#endif
#if TWOPHASE
	int ntarget_hosts_phase1_;
#endif
#if HAVE_DCMF_RECORD_REPLAY
	unsigned persist_id_;
#endif
};

#if TWOPHASE
class BGP_DMASend_Phase2 {
public:
	BGP_DMASend_Phase2();
	virtual ~BGP_DMASend_Phase2();
	NRNMPI_Spike spk_;

	void send_phase2(int gid, double t, BGP_ReceiveBuffer*);
	int ntarget_hosts_phase2_;
	int* target_hosts_phase2_;

#if HAVE_DCMF_RECORD_REPLAY
	unsigned persist_id_;
#endif
};
#endif

static BGP_ReceiveBuffer* bgp_receive_buffer[BGP_INTERVAL];
static int current_rbuf, next_rbuf;
#if BGP_INTERVAL == 2
// note that if a spike is supposed to be received by bgp_receive_buffer[1]
// then during transmission its gid is complemented.
#endif

BGP_ReceiveBuffer::BGP_ReceiveBuffer() {
	busy_ = 0;
	count_ = 0;
	size_ = BGP_RECEIVEBUFFER_SIZE;
	buffer_ = new NRNMPI_Spike*[size_];
	pool_ = new SpkPool(BGP_RECEIVEBUFFER_SIZE);
	psbuf_ = 0;
#if ENQUEUE == 1
	psbuf_ = new PreSyn*[size_];
#endif
#if TWOPHASE
	phase2_buffer_ = new Phase2Buffer[PHASE2BUFFER_SIZE];
	phase2_head_ = phase2_tail_ = 0;
#endif
}
BGP_ReceiveBuffer::~BGP_ReceiveBuffer() {
	assert(busy_ == 0);
	for (int i = 0; i < count_; ++i) {
		pool_->hpfree(buffer_[i]);
	}
	delete [] buffer_;
	delete pool_;
	if (psbuf_) delete [] psbuf_;
#if TWOPHASE
	delete [] phase2_buffer_;
#endif
}
void BGP_ReceiveBuffer::init(int index) {
	index_ = index;
	timebase_ = 0;
	nsend_cell_ = nsend_ = nrecv_ = busy_ = maxcount_ = 0;
	for (int i = 0; i < count_; ++i) {
		pool_->hpfree(buffer_[i]);
	}
	count_ = 0;
#if TWOPHASE
	phase2_head_ = phase2_tail_ = 0;
	phase2_nsend_cell_ = phase2_nsend_ = 0;
#endif	
}
void BGP_ReceiveBuffer::incoming(int gid, double spiketime) {
//printf("%d %p.incoming %g %g %d\n", nrnmpi_myid, this, t, spk->spiketime, spk->gid);
	assert(busy_ == 0);
	busy_ = 1;
#if 0 && ENQUEUE == 2
	// this section is potential race if both ReceiveBuffer called at
	// once, but in fact this is only called from within messager advance.
	// Note that this does not work as we occasionally get a timeout        
	// during conservation checking. For this reason we return to buffer    
	// usage but overlap through a call to enqueue on both ReceiveBuffer    
	// on each messager advance
	unsigned long long tb = DCMFTIMEBASE;
	PreSyn* ps;
	assert(gid2in_->find(gid, ps));
	enq2_find_time_ += (unsigned long)(DCMFTIMEBASE - tb);
	ps->send(spiketime, net_cvode_instance, nrn_threads);
	enq2_enqueue_time_ += (unsigned long)(DCMFTIMEBASE - tb);
#else
	if (count_ >= size_) {
		size_ *= 2;
		NRNMPI_Spike** newbuf = new NRNMPI_Spike*[size_];
		for (int i = 0; i < count_; ++i) {
			newbuf[i] = buffer_[i];
		}
		delete [] buffer_;
		buffer_ = newbuf;
		if (psbuf_) {
			delete [] psbuf_;
			psbuf_ = new PreSyn*[size_];
		}
	}
	NRNMPI_Spike* spk = pool_->alloc();
	spk->gid = gid;
	spk->spiketime = spiketime;
	buffer_[count_++] = spk;
	if (maxcount_ < count_) { maxcount_ = count_; }
#endif
	++nrecv_;
	busy_ = 0;	
}
void BGP_ReceiveBuffer::enqueue() {
//printf("%d %p.enqueue count=%d t=%g nrecv=%d nsend=%d\n", nrnmpi_myid, this, t, count_, nrecv_, nsend_);
	assert(busy_ == 0);
	busy_ = 1;
#if 1
	for (int i=0; i < count_; ++i) {
		NRNMPI_Spike* spk = buffer_[i];
		PreSyn* ps;
#if ENQUEUE == 2
		unsigned long long tb = DCMFTIMEBASE;
#endif

#if WORK_AROUND_RECORD_BUG
		if (!gid2in_->find(spk->gid, ps)) {
#if ENQUEUE == 2
			enq2_find_time_ += (unsigned long)(DCMFTIMEBASE - tb);
#endif
			pool_->hpfree(spk);
			continue;
		}
#else
		assert(gid2in_->find(spk->gid, ps));
#endif

#if TWOPHASE
		if (use_phase2_ && ps->bgp.dma_send_phase2_) {
			// cannot do directly because busy_;
			//ps->bgp.dma_send_phase2_->send_phase2(spk->gid, spk->spiketime, this);
			Phase2Buffer& pb = phase2_buffer_[phase2_head_++];
			phase2_head_ &= PHASE2BUFFER_MASK;
			assert(phase2_head_ != phase2_tail_);
			pb.ps = ps;
			pb.spiketime = spk->spiketime;
			
		}
#endif
#if ENQUEUE == 2
		enq2_find_time_ += (unsigned long)(DCMFTIMEBASE - tb);
#endif
		ps->send(spk->spiketime, net_cvode_instance, nrn_threads);
		pool_->hpfree(spk);
#if ENQUEUE == 2
		enq2_enqueue_time_ += (unsigned long)(DCMFTIMEBASE - tb);
#endif
	}
#endif
	count_ = 0;
#if ENQUEUE != 2
	nrecv_ = 0;
	nsend_ = 0;
	nsend_cell_ = 0;
#endif
	busy_ = 0;
#if TWOPHASE
	phase2send();
#endif
}

void BGP_ReceiveBuffer::enqueue1() {
//printf("%d %lx.enqueue count=%d t=%g nrecv=%d nsend=%d\n", nrnmpi_myid, (long)this, t, count_, nrecv_, nsend_);
	assert(busy_ == 0);
	busy_ = 1;
	for (int i=0; i < count_; ++i) {
		NRNMPI_Spike* spk = buffer_[i];
		PreSyn* ps;
		assert(gid2in_->find(spk->gid, ps));
		psbuf_[i] = ps;
#if TWOPHASE
		if (use_phase2_ && ps->bgp.dma_send_phase2_) {
			// cannot do directly because busy_;
			//ps->bgp.dma_send_phase2_->send_phase2(spk->gid, spk->spiketime, this);
			Phase2Buffer& pb = phase2_buffer_[phase2_head_++];
			phase2_head_ &= PHASE2BUFFER_MASK;
			assert(phase2_head_ != phase2_tail_);
			pb.ps = ps;
			pb.spiketime = spk->spiketime;
			
		}
#endif
	}
	busy_ = 0;
#if TWOPHASE
	phase2send();
#endif
}

void BGP_ReceiveBuffer::enqueue2() {
//printf("%d %lx.enqueue count=%d t=%g nrecv=%d nsend=%d\n", nrnmpi_myid, (long)this, t, count_, nrecv_, nsend_);
	assert(busy_ == 0);
	busy_ = 1;
	for (int i=0; i < count_; ++i) {
		NRNMPI_Spike* spk = buffer_[i];
		PreSyn* ps = psbuf_[i];
		ps->send(spk->spiketime, net_cvode_instance, nrn_threads);
		pool_->hpfree(spk);
	}
	count_ = 0;
	nrecv_ = 0;
	nsend_ = 0;
	nsend_cell_ = 0;
	busy_ = 0;
}

#if TWOPHASE
void BGP_ReceiveBuffer::phase2send() {
	while (phase2_head_ != phase2_tail_) {
		Phase2Buffer& pb = phase2_buffer_[phase2_tail_++];
		phase2_tail_ &= PHASE2BUFFER_MASK;
		pb.ps->bgp.dma_send_phase2_->send_phase2(pb.ps->gid_, pb.spiketime, this);
	}
}
#endif

// number of DCMF_Multicast_t to cycle through when not using recordreplay
#define NSEND 10

#if BGPDMA > 1
extern "C" {
extern void getMemSize(long long *mem);
extern void getUsedMem(long long *mem);
extern void getFreeMem(long long *mem);
}
#endif


#if BGPDMA & 2

#define PIPEWIDTH 16

static DCMF_Opcode_t* hints_;
//static DCMF_Protocol_t protocol __attribute__((__aligned__(16)));
//static DCMF_Multicast_Configuration_t mconfig;
struct MyProtocolWrapper {
	DCMF_Protocol_t protocol __attribute__((__aligned__(16)));
} __attribute__((__aligned__(16)));
static MyProtocolWrapper* mpw;
static DCMF_Multicast_Configuration_t* mconfig;

struct MyMulticastInfo {
	DCMF_Request_t request __attribute__((__aligned__(16)));
        DCQuad msginfo __attribute__((__aligned__(16)));
	DCMF_Multicast_t msend;
	DCMF_Callback_t cb_done;
	bool req_in_use;
	bool record; // when recordreplay, first time Multicast, thereafter  Restart
}__attribute__((__aligned__(16)));
// NSEND of them for cycling, or, if recordreplay, max_persist_ids of them
static int n_mymulticast_; // NSEND or max_persist_ids
static struct MyMulticastInfo* mci_;

// maybe we will use this when a rank sends to itself.
static void spk_ready (int gid, double spiketime) {
	assert(0);
//printf("%d spk_ready %d %g\n", nrnmpi_myid, gid, spiketime);
	// to avoid a race condition (since we may be in the middle of
	// retrieving an item from the event queue) store the incoming
	// events in a receive buffer
	int i = 0;
#if BGP_INTERVAL == 2
	if (gid < 0) {
		gid = ~gid;
		i = 1;
	}
#endif
	bgp_receive_buffer[i]->incoming(gid, spiketime);
	++nrecv_;
}

// async callback for receiver's side of multisend  
static DCMF_Request_t * msend_recv(const DCQuad  * msginfo,
			    unsigned          nquads,
			    unsigned          senderID,
			    const unsigned    sndlen,
			    unsigned          connid,
			    void            * arg,
			    unsigned        * rcvlen,
			    char           ** rcvbuf,
			    unsigned        * pipewidth,
			    DCMF_Callback_t * cb_done)
{
  unsigned long long tb = DCMF_Timebase();
  *rcvlen = 0;
  *rcvbuf = 0;
  * pipewidth       = PIPEWIDTH;
  cb_done->clientdata = 0;
  cb_done->function = 0;

  double t = *((double*)&msginfo->w0);
  int gid = *((int*)&msginfo->w2);
//  printf("%d msend_recv %d %g\n", nrnmpi_myid, spk->gid, spk->spiketime);
	int i = 0;
#if BGP_INTERVAL == 2
	if (gid < 0) {
		gid = ~gid;
		i = 1;
	}
#endif
	bgp_receive_buffer[i]->incoming(gid, t);
	++nrecv_;
  bgp_receive_buffer[i]->timebase_ += DCMF_Timebase() - tb;
  return NULL;
}

// Multisend_multicast callback
#if DCMF_VERSION_MAJOR >= 2
static void  multicast_done(void* arg, DCMF_Error_t *) {
#else
static void  multicast_done(void* arg) {
#endif
	bool* a = (bool*)arg;
	*a = false;
}

#endif //BGPDMA & 2

static int max_ntarget_host;
// For one phase sending, max_multisend_targets is max_ntarget_host.
// For two phase sending, it is the maximum of all the
// ntarget_hosts_phase1 and ntarget_hosts_phase2.
static int max_multisend_targets;

double nrn_bgp_receive_time(int type) { // and others
	double rt = 0.;
	switch(type) {
	case 2: //in msend_recv
		if (!use_bgpdma_) { return rt; }
		for (int i = 0; i < n_bgp_interval; ++i) {
			rt += bgp_receive_buffer[i]->timebase_ * DCMFTICK;
		}
		break;
	case 3: // in BGP_DMAsend::send
		if (!use_bgpdma_) { return rt; }
		rt = dmasend_time_ * DCMFTICK;
		break;
	case 4: // number of extra conservation checks
		rt = double(n_xtra_cons_check_);
		// and if there is second vector arg then also return the histogram
#if MAXNCONS
		if (ifarg(2) && use_bgpdma_) {
			IvocVect* vec = vector_arg(2);
			vector_resize(vec, MAXNCONS+1);
			for (int i=0; i <= MAXNCONS; ++i) {
				vector_vec(vec)[i] = double(xtra_cons_hist_[i]);
			}
		}
#endif // MAXNCONS
#if TBUFSIZE
		if (ifarg(3)) {
			IvocVect* vec = vector_arg(3);
			vector_resize(vec, itbuf_+1);
			for (int i=0; i <= itbuf_; ++i) {
				vector_vec(vec)[i] = double(tbuf_[i]);
			}
			vector_vec(vec)[itbuf_] = DCMFTICK;
		}
#endif
		break;
#if ALTHASH
	case 5:
		rt = double(gid2in_->max_chain_length());
		break;
	case 6:
		rt = double(gid2in_->nclash());
		break;
	case 7:
		rt = double(gid2in_->nfind());
		break;
#endif
	case 8: // exchange method properties
		// bit 0-1: 0 allgather, 1 multisend as MPI_ISend,
		// 2 multisend DCMF, 3 multisend DCMF with record replay
		// bit 2: n_bgp_interval, 0 means one interval, 1 means 2
		// bit 3: number of phases, 0 means 1 phase, 1 means 2
		// bit 4: 1 means althash used
		// bit 5: 1 means enqueue separated into two parts for timeing
	    {
		int meth = use_dcmf_record_replay ? 3 : use_bgpdma_;
		int p = meth + 4*(n_bgp_interval == 2 ? 1 : 0)
#if TWOPHASE
			+ 8*use_phase2_
#endif
			+ 16*(ALTHASH == 1 ? 1 : 0)
			+ 32*ENQUEUE;
		rt = double(p);
	    }
		break;
#if BGPDMA > 1
	case 9: // getMemSize
	  { long long mem; getMemSize(&mem);
		rt = double(mem);
		break;
	  }
	case 10: // getUsedMem
	  { long long mem; getUsedMem(&mem);
		rt = double(mem);
		break;
	  }
	case 11: // getFreeMem
	  { long long mem; getFreeMem(&mem);
		rt = double(mem);
		break;
	  }
#endif

	case 12: // greatest length multisend
	  {
		rt = double(max_multisend_targets);
		break;
	  }
	}
	return rt;
}

extern "C" {
extern void nrnmpi_bgp_comm();
extern void nrnmpi_bgp_multisend(NRNMPI_Spike*, int, int*);
extern int nrnmpi_bgp_single_advance(NRNMPI_Spike*);
extern int nrnmpi_bgp_conserve(int nsend, int nrecv);
}

static void bgp_dma_init() {
	for (int i = 0; i < n_bgp_interval; ++i) {
		bgp_receive_buffer[i]->init(i);
	}
	current_rbuf = 0;
	next_rbuf = n_bgp_interval - 1;
#if TBUFSIZE
	itbuf_ = 0;
#endif
#if BGPDMA & 2
	if (use_bgpdma_ == 2) for (int i=0; i < n_mymulticast_; ++i) {
		mci_[i].req_in_use = false;
	}
#endif
	dmasend_time_ = 0;
#if ENQUEUE == 2
	enq2_find_time_ = enq2_enqueue_time_ = 0;
#endif
	n_xtra_cons_check_ = 0;
#if MAXNCONS
	for (int i=0; i <= MAXNCONS; ++i) {
		xtra_cons_hist_[i] = 0;
	}
#endif // MAXNCONS
}

static int bgp_advance() {
	NRNMPI_Spike spk;
	PreSyn* ps;
	int i = 0;
	while(nrnmpi_bgp_single_advance(&spk)) {
		i += 1;
		int j = 0;
#if BGP_INTERVAL == 2
		if (spk.gid < 0) {
			spk.gid = ~spk.gid;
			j = 1;
		}
#endif
		bgp_receive_buffer[j]->incoming(spk.gid, spk.spiketime);
	}
	nrecv_ += i;
	return i;
}

#if BGPDMA
void nrnbgp_messager_advance() {
#if BGPDMA & 2
	if (use_bgpdma_ > 1) { DCMF_Messager_advance(); }
#endif
#if BGPDMA & 1
	if (use_bgpdma_ == 1) { bgp_advance(); }
#endif
#if ENQUEUE == 2
	bgp_receive_buffer[current_rbuf]->enqueue();
#endif
}
#endif

BGP_DMASend::BGP_DMASend() {
	ntarget_hosts_ = 0;
	target_hosts_ = NULL;
#if 0
	send2self_ = 0;
#endif
#if TWOPHASE
	ntarget_hosts_phase1_ = 0;
#endif
}

BGP_DMASend::~BGP_DMASend() {
	if (target_hosts_) {
		delete [] target_hosts_;
	}
}

#if TWOPHASE
BGP_DMASend_Phase2::BGP_DMASend_Phase2() {
	ntarget_hosts_phase2_ = 0;
	target_hosts_phase2_ = 0;
}

BGP_DMASend_Phase2::~BGP_DMASend_Phase2() {
	if (target_hosts_phase2_) {
		delete [] target_hosts_phase2_;
	}
}
#endif

static	int isend;

// helps debugging when core dump since otherwise cannot tell where
// BGP_DMASend::send fails
#if 0
static void mymulticast(DCMF_Multicast_t* arg) {
	DCMF_Multicast(arg);
}
static void myrestart(DCMF_Request_t* arg) {
	DCMF_Restart(arg);
}
#endif

void BGP_DMASend::send(int gid, double t) {
	unsigned long long tb = DCMFTIMEBASE;
  if (NTARGET_HOSTS_PHASE1) {
	spk_.gid = gid;
	spk_.spiketime = t;
#if BGP_INTERVAL == 2
	bgp_receive_buffer[next_rbuf]->nsend_ += ntarget_hosts_;
	bgp_receive_buffer[next_rbuf]->nsend_cell_ += 1;
	if (next_rbuf == 1) {
		spk_.gid = ~spk_.gid;
	}
#else
	bgp_receive_buffer[0]->nsend_ += ntarget_hosts_;
	bgp_receive_buffer[0]->nsend_cell_ += 1;
#endif
	nsend_ += 1;
#if BGPDMA & 2
    if (use_bgpdma_ == 2) {

	MyMulticastInfo* mci;
#if HAVE_DCMF_RECORD_REPLAY
	if (use_dcmf_record_replay) {
		mci = mci_ + persist_id_;
	}else{
#else
	{
#endif
		mci = mci_ + isend;
	}

	int acnt = 0;
	while (mci->req_in_use) {
		++acnt;
		nrnbgp_messager_advance();
	}
//	if (acnt > 10) { printf("%d multicast %d not done\n", nrnmpi_myid, msend.connection_id);}
	mci->req_in_use = true;
//printf("%d multisend %d %g\n", nrnmpi_myid, gid, t);
	*((double*)&(mci->msginfo.w0)) = spk_.spiketime;
	*((int*)&(mci->msginfo.w2)) = spk_.gid;
	mci->msend.nranks = (unsigned int)NTARGET_HOSTS_PHASE1;
	mci->msend.ranks = (unsigned int*) target_hosts_;

//printf("%d DCMF_Multicast %d %g %d\n", nrnmpi_myid, msend.connection_id, t, gid);
#if HAVE_DCMF_RECORD_REPLAY
	if (use_dcmf_record_replay) {
		if (mci->record) {
#if 0
			mymulticast(&mci->msend);
#else
			DCMF_Multicast(&mci->msend);
#endif
			mci->record = false;
		}else{
#if 0
			myrestart(&mci->request);
#else
			DCMF_Restart(&mci->request);
#endif
		}
	}else{
#else
	{
#endif
		DCMF_Multicast(&mci->msend);
		isend = (++isend)%n_mymulticast_;
	}
    }
#endif // BGPDMA & 2
#if BGPDMA & 1
    if (use_bgpdma_ == 1) {
	nrnmpi_bgp_multisend(&spk_, NTARGET_HOSTS_PHASE1, target_hosts_);
    }
#endif
  }
#if 0
	// I am given to understand that multisend cannot send to itself
	// one is never supposed to send to oneself since an output presyn
	// can never be in the gid2in_ table (ie. the assert below would fail).
	if (send2self_) {
		PreSyn* ps;
		assert(gid2in_->find(gid, ps));
		ps->send(t, net_cvode_instance, nrn_threads);
	}
#endif
	dmasend_time_ += DCMFTIMEBASE - tb;
}

#if TWOPHASE

void BGP_DMASend_Phase2::send_phase2(int gid, double t, BGP_ReceiveBuffer* rb) {
	unsigned long long tb = DCMFTIMEBASE;
  if (ntarget_hosts_phase2_) {
	spk_.gid = gid;
	spk_.spiketime = t;
#if BGP_INTERVAL == 2
	if (rb->index_ == 1) {
		spk_.gid = ~spk_.gid;
	}
#endif
	rb->phase2_nsend_cell_ += 1;
	rb->phase2_nsend_ += ntarget_hosts_phase2_;
#if BGPDMA & 2
    if (use_bgpdma_ == 2) {

	MyMulticastInfo* mci;
#if HAVE_DCMF_RECORD_REPLAY
	if (use_dcmf_record_replay) {
		mci = mci_ + persist_id_;
	}else{
#else
	{
#endif
		mci = mci_ + isend;
	}

	int acnt = 0;
	while (mci->req_in_use) {
		++acnt;
		nrnbgp_messager_advance();
	}
//	if (acnt > 10) { printf("%d multicast %d not done\n", nrnmpi_myid, msend.connection_id);}
	mci->req_in_use = true;
//printf("%d multisend %d %g\n", nrnmpi_myid, gid, t);
	*((double*)&(mci->msginfo.w0)) = spk_.spiketime;
	*((int*)&(mci->msginfo.w2)) = spk_.gid;
	mci->msend.nranks = (unsigned int)ntarget_hosts_phase2_;
	mci->msend.ranks = (unsigned int*) target_hosts_phase2_;

//printf("%d DCMF_Multicast %d %g %d\n", nrnmpi_myid, msend.connection_id, t, gid);
#if HAVE_DCMF_RECORD_REPLAY
	if (use_dcmf_record_replay) {
		if (mci->record) {
			DCMF_Multicast(&mci->msend);
			mci->record = false;
		}else{
			DCMF_Restart(&mci->request);
		}
	}else{
#else
	{
#endif
		DCMF_Multicast(&mci->msend);
		isend = (++isend)%n_mymulticast_;
	}
    }
#endif // BGPDMA & 2
#if BGPDMA & 1
    if (use_bgpdma_ == 1) {
	nrnmpi_bgp_multisend(&spk_, ntarget_hosts_phase2_, target_hosts_phase2_);
    }
#endif
  }
	dmasend_time_ += DCMFTIMEBASE - tb;
}
#endif // TWOPHASE


static void determine_source_hosts();
static void determine_targid_count_on_srchost(int* src, int* send);
static void determine_targids_on_srchost(int* s, int* scnt, int* sdispl,
    int* r, int* rcnt, int* rdispl);
static void determine_target_hosts();
static int gathersrcgid(int hostbegin, int totalngid, int* ngid,
	int* thishostgid, int* n, int* displ, int bsize, int* buf);

void bgp_dma_receive(NrnThread* nt) {
//	nrn_spike_exchange();
	assert(nt == nrn_threads);
	TBUF
	double w1, w2;
	int ncons = 0;
	int& s = bgp_receive_buffer[current_rbuf]->nsend_;
	int& r = bgp_receive_buffer[current_rbuf]->nrecv_;
#if ENQUEUE == 2
	unsigned long tfind, tsend;
#endif
	w1 = nrnmpi_wtime();
#if BGPDMA & 2
    if (use_bgpdma_ == 2) {
	nrnbgp_messager_advance();
	TBUF
#if ENQUEUE == 2
	// want the overlap with computation, not conserve
	tfind = enq2_find_time_;
	tsend = enq2_enqueue_time_ - enq2_find_time_;
#endif
	// demonstrates that most of the time here is due to load imbalance
#if TBUFSIZE
	nrnmpi_barrier();
#endif
	TBUF
	while (nrnmpi_bgp_conserve(s, r) != 0) {
		nrnbgp_messager_advance();
		++ncons;
	}
	TBUF
	n_xtra_cons_check_ += ncons;
    }
#endif
#if BGPDMA & 1
    if (use_bgpdma_ == 1) {
	nrnbgp_messager_advance();
	TBUF
#if ENQUEUE == 2
	// want the overlap with computation, not conserve
	tfind = enq2_find_time_;
	tsend = enq2_enqueue_time_ - enq2_find_time_;
#endif
#if TBUFSIZE
	nrnmpi_barrier();
#endif
	TBUF
	while (nrnmpi_bgp_conserve(s, r) != 0) {
		nrnbgp_messager_advance();
		++ncons;
	}
	TBUF
    }
#endif
	w1 = nrnmpi_wtime() - w1;
	w2 = nrnmpi_wtime();
#if TBUFSIZE
	tbuf_[itbuf_++] = (unsigned long)ncons;
	tbuf_[itbuf_++] = (unsigned long)bgp_receive_buffer[current_rbuf]->nsend_cell_;
	tbuf_[itbuf_++] = (unsigned long)s;
	tbuf_[itbuf_++] = (unsigned long)r;
	tbuf_[itbuf_++] = (unsigned long)dmasend_time_;
#if TWOPHASE
	if (use_phase2_) {
		tbuf_[itbuf_++] = (unsigned long)bgp_receive_buffer[current_rbuf]->phase2_nsend_cell_;
		tbuf_[itbuf_++] = (unsigned long)bgp_receive_buffer[current_rbuf]->phase2_nsend_;
	}
#endif
#endif
#if (BGPMDA & 2) && MAXNCONS
	if (ncons > MAXNCONS) { ncons = MAXNCONS; }
	++xtra_cons_hist_[ncons];
#endif // MAXNCONS
#if ENQUEUE == 0
	bgp_receive_buffer[current_rbuf]->enqueue();
#endif
#if ENQUEUE == 1
	bgp_receive_buffer[current_rbuf]->enqueue1();
	TBUF
	bgp_receive_buffer[current_rbuf]->enqueue2();
#endif
#if ENQUEUE == 2
	bgp_receive_buffer[current_rbuf]->enqueue();
	s = r =  bgp_receive_buffer[current_rbuf]->nsend_cell_ = 0;
#if TWOPHASE
	bgp_receive_buffer[current_rbuf]->phase2_nsend_cell_ = 0;
	bgp_receive_buffer[current_rbuf]->phase2_nsend_ = 0;
#endif
	enq2_find_time_ = 0;
	enq2_enqueue_time_ = 0;
#if TBUFSIZE
	tbuf_[itbuf_++] = tfind;
	tbuf_[itbuf_++] = tsend;
#endif
#endif // ENQUEUE == 2
	wt1_ = nrnmpi_wtime() - w2;
	wt_ = w1;
#if BGP_INTERVAL == 2
//printf("%d reverse buffers %g\n", nrnmpi_myid, t);
	if (n_bgp_interval == 2) {
		current_rbuf = next_rbuf;
		next_rbuf = ((next_rbuf + 1)&1);
	}
#endif
	TBUF
}

void bgp_dma_send(PreSyn* ps, double t) {
#if 0
	if (nrn_use_localgid_) {
		nrn_outputevent(ps->localgid_, t);
	}else{
		nrn2ncs_outputevent(ps->output_index_, t);
	}
#endif
	ps->bgp.dma_send_->send(ps->output_index_, t);
}

void bgpdma_send_init(PreSyn* ps) {
}

void bgpdma_cleanup_presyn(PreSyn* ps) {
	if (ps->bgp.dma_send_) {
		if (ps->output_index_ >= 0) {
			delete ps->bgp.dma_send_;
			ps->bgp.dma_send_ = 0;
		}
#if TWOPHASE
		if (ps->output_index_ < 0) {
			delete ps->bgp.dma_send_phase2_;
			ps->bgp.dma_send_phase2_ = 0;
		}
#endif
	}
}

static void bgpdma_cleanup() {
	nrntimeout_call = 0;
#if BGPDMA & 2
	if (hints_) {
		delete [] hints_;
		hints_ = 0;
		delete [] mconfig->connectionlist;
		delete [] mci_;
	}
#endif
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		bgpdma_cleanup_presyn(ps);
	}}}
#if TWOPHASE
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		bgpdma_cleanup_presyn(ps);
	}}}
#endif
}

static void bgptimeout() {
	printf("%d timeout %d %d %d\n", nrnmpi_myid, current_rbuf,
		bgp_receive_buffer[current_rbuf]->nsend_,
		bgp_receive_buffer[current_rbuf]->nrecv_
	);
}

#if WORK_AROUND_RECORD_BUG
static void ensure_ntarget_gt_3(BGP_DMASend* bs) {
	// work around for bug in RecordReplay
	if (bs->ntarget_hosts_ > 3) { return; }
	int nold = bs->ntarget_hosts_;
	int* old = bs->target_hosts_;
	bs->target_hosts_ = new int[4];
	for (int i=0; i < nold; ++i) {
		bs->target_hosts_[i] = old[i];
	}
	delete [] old;
	int h = (nrnmpi_myid + 4)%nrnmpi_numprocs;
	while (bs->ntarget_hosts_ < 4) {
		int b = 0;
		for (int i=0; i < bs->ntarget_hosts_; ++i) {
			if (h == bs->target_hosts_[i]) {
				h = (h + 4)%nrnmpi_numprocs;
				b = 1;
				break;
			}
		}
		if (b == 0) {
			bs->target_hosts_[bs->ntarget_hosts_++] = h;
			h = (h + 1)%nrnmpi_numprocs;
		}
	}
	bs->ntarget_hosts_phase1_ = bs->ntarget_hosts_;
}
#endif

#define FASTSETUP 1
#if FASTSETUP
#include "bgpdmasetup.cpp"
#endif

void bgp_dma_setup() {
	bgpdma_cleanup();
	if (use_bgpdma_ == 0) { return; }
	//not sure this is useful for debugging when stuck in a collective.
	//nrntimeout_call = bgptimeout;
	double wt = nrnmpi_wtime();
	nrnmpi_bgp_comm();
	//if (nrnmpi_myid == 0) printf("bgp_dma_setup()\n");
	// although we only care about the set of hosts that gid2out_
	// sends spikes to (source centric). We do not want to send
	// the entire list of gid2in (which may be 10000 times larger
	// than gid2out) from every machine to every machine.
	// so we accomplish the task in two phases the first of which
	// involves allgather with a total receive buffer size of number
	// of cells (even that is too large and we will split it up
	// into chunks). And the second, an
	// allreduce with receive buffer size of number of hosts.
	max_ntarget_host = 0;
	max_multisend_targets = 0;

#if FASTSETUP
	// completely new algorithm does one and two phase.
	setup_presyn_dma_lists();
#else // obsolete
	// see 672:544c61a730ec
#error "FASTSETUP required"
#endif // obsolete (not FASTSETUP)

	if (!bgp_receive_buffer[0]) {
		bgp_receive_buffer[0] = new BGP_ReceiveBuffer();
	}
#if BGP_INTERVAL == 2
	if (n_bgp_interval == 2 && !bgp_receive_buffer[1]) {
		bgp_receive_buffer[1] = new BGP_ReceiveBuffer();
	}
#endif
#if BGPDMA & 2
  if (use_bgpdma_ == 2) {
	// going to leak these since no way to unregister
	mpw = new MyProtocolWrapper;
	mconfig = new DCMF_Multicast_Configuration_t;
	// one would think that everyone can use the same hints and so they
	// can be allocated according to the maximum ntarget_hosts_.
	hints_ = new DCMF_Opcode_t[nrnmpi_numprocs];
	for (int i = 0; i < nrnmpi_numprocs; ++i) {
		hints_[i] = DCMF_PT_TO_PT_SEND;
	}
	// I am also guessing everyone can use the same mconfig.
#if HAVE_DCMF_RECORD_REPLAY
	unsigned max_persist_ids = 0;
	if (use_dcmf_record_replay) {
		PreSyn* ps;
		NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
			if (ps->output_index_ >= 0 && ps->bgp.dma_send_->ntarget_hosts_ > 0) {
				ps->bgp.dma_send_->persist_id_ = max_persist_ids++;
#if WORK_AROUND_RECORD_BUG
				ensure_ntarget_gt_3(ps->bgp.dma_send_);
#endif
			}
		}}}
	}
	if (max_persist_ids > 0) { // may want to check for too many as well
		mconfig->protocol = DCMF_MEMFIFO_MCAST_RECORD_REPLAY_PROTOCOL;
		mconfig->max_persist_ids = max_persist_ids;
		mconfig->max_msgs = max_multisend_targets;
		n_mymulticast_ = max_persist_ids;
	}else{
		mconfig->protocol = DCMF_MEMFIFO_DMA_MSEND_PROTOCOL;
		mconfig->max_persist_ids = 0;
		mconfig->max_msgs = 0;
		n_mymulticast_ = NSEND;
#if TWOPHASE
		if (use_phase2_) {
			n_mymulticast_ *= 100;
		}
#endif
	}
#else
	mconfig->protocol = DCMF_MEMFIFO_DMA_MSEND_PROTOCOL;
	n_mymulticast_ = NSEND;
#if TWOPHASE
		if (use_phase2_) {
			n_mymulticast_ *= 100;
		}
#endif
#endif
	if (nrnmpi_myid == 0) {
		printf("n_mymulticast_ = %d\n", n_mymulticast_);
	}	
	mci_ = new MyMulticastInfo[n_mymulticast_];
	mconfig->cb_recv = msend_recv;
	mconfig->nconnections = n_mymulticast_; //max_multisend_targets;
	mconfig->connectionlist = new void*[n_mymulticast_];
	mconfig->clientdata = NULL;
	assert(DCMF_Multicast_register (&mpw->protocol, mconfig) == DCMF_SUCCESS);

	for (int i = 0; i < n_mymulticast_; ++i) {
		MyMulticastInfo* mci = mci_+ i;
		mci->record = true;
		mci->req_in_use = false;
		mci->cb_done.clientdata = (void*)&mci->req_in_use;
		mci->cb_done.function = multicast_done;
		mci->msend.registration = &mpw->protocol;
		mci->msend.request = &mci->request;
		mci->msend.cb_done = mci->cb_done;
		mci->msend.consistency = DCMF_MATCH_CONSISTENCY; //DCMF_RELAXED_CONSISTENCY; 
		mci->msend.connection_id = i;
		mci->msend.bytes = 0;
		mci->msend.src = NULL;
		mci->msend.opcodes = hints_;
		mci->msend.msginfo = &mci->msginfo;
		mci->msend.count = 1;
		mci->msend.op = DCMF_UNDEFINED_OP;
		mci->msend.dt = DCMF_UNDEFINED_DT;
#if HAVE_DCMF_RECORD_REPLAY
		mci->msend.persist_id = i;
#endif
	}
  }
#endif
}

#ifdef USENCS

//give me data on which gids of this node send out APs
int ncs_bgp_sending_info( int **sendlist2build )
{
	int nsrcgid = 0;
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			++nsrcgid;
		}
	}}}

	*sendlist2build = nsrcgid ? new int[nsrcgid] : 0;
	int i = 0;
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			(*sendlist2build)[i] = ps->gid_;
			++i;
		}
	}}}
    
    //printf( "Node %d sees first hand %d gids send\n", nrnmpi_myid, nsrcgid );
    return nsrcgid;
}


//function to access the sending information of a presyn
int ncs_bgp_target_hosts( int gid, int** targetnodes )
{
    PreSyn* ps;
    assert(gid2out_->find(gid, ps));
    if( ps->bgp.dma_send_ ) {
        (*targetnodes) = ps->bgp.dma_send_->ntarget_hosts_? new int[ps->bgp.dma_send_->ntarget_hosts_] : 0;
        return ps->bgp.dma_send_->ntarget_hosts_;
    }
    
    return 0;
}

//iterate over gid2in_ just so I can see what is in there
int ncs_bgp_target_info( int **presyngids )
{
    //(*presyngids) = 0;

	int i, nsrcgid;
	PreSyn* ps;
    nsrcgid = 0;

	// some target PreSyns may not have any input
	// so initialize all to -1
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		assert(ps->output_index_ < 0);
		if( ps->bgp.srchost_ != -1 )  //has input
        {
            ++nsrcgid;
            //printf( "Node %d: Presyn for gid %d has src %d\n", nrnmpi_myid, ps->gid_, ps->bgp.srchost_ );
        }
	}}}
    
    (*presyngids) = nsrcgid ? new int[nsrcgid] : 0;
    
    i=0;
    NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		assert(ps->output_index_ < 0);
		if( ps->bgp.srchost_ != -1 )  //has input
        {
            (*presyngids)[i] = ps->gid_;
            ++i;
        }
	}}}
    
    return nsrcgid;
}

int ncs_bgp_mindelays( int **srchost, double **delays )
{
    int i, nsrcgid=0;

	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
        assert(ps->output_index_ < 0);
        if( ps->bgp.srchost_ != -1 )  //has input
        {
            ++nsrcgid;
        }
	}}}

    (*delays) = nsrcgid ? new double[nsrcgid] : 0;
    (*srchost) = nsrcgid ? new int[nsrcgid] : 0;
    
    i=0;
    NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		assert(ps->output_index_ < 0);
		if( ps->bgp.srchost_ != -1 )  //has input
        {
            (*delays)[i] = ps->mindelay();
            (*srchost)[i] = ps->bgp.srchost_;
            ++i;
        }
	}}}
    
    return nsrcgid;
}

#endif //USENCS

