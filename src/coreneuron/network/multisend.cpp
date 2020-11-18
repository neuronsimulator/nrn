#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/network/netcvode.hpp"

/*
Overall exchange strategy

When a cell spikes, it immediately does a multisend of
(int gid, double spiketime) to all the target machines that have
cells that need to receive this spike by spiketime + delay.
The MPI implementation does not block due to use of MPI_Isend.

In order to minimize the number of nrnmpi_multisend_conserve tests
(and potentially abandon them altogether if I can ever guarantee
that exchange time is less than half the computation time), I divide the
minimum delay integration intervals into two equal subintervals.
So if a spike is generated in an even subinterval, I do not have
to include it in the conservation check until the end of the next even
subinterval.

When a spike is received (generally MPI_Iprobe, MPI_Recv) it is placed in
even or odd buffers (depending on whether the coded gid is positive or negative)

At the end of a computation subinterval the even or odd buffer spikes
are enqueued in the priority queue after checking that the number
of spikes sent is equal to the number of spikes sent.
*/

// The initial idea behind the optional phase2 is to avoid the large overhead of
// initiating a send of the up to 10k list of target hosts when a cell fires.
// I.e. when there are a small number of cells on a processor, this causes
// load balance problems.
// Load balance should be better if the send is distributed to a much smaller
// set of targets, which, when they receive the spike, pass it on to a neighbor
// set. A non-exclusive alternative to this is the use of RECORD_REPLAY
// which give a very fast initiation but we have not been able to get that
// to complete in the sense of all the targets receiving their spikes before
// the conservation step.
// We expect that phase2 will work best in combination with ENQUEUE=2
// which has the greatest amount of overlap between computation
// and communication.
namespace coreneuron {
bool use_multisend_;
bool use_phase2_;
int n_multisend_interval = 2;

#if NRN_MULTISEND

static int n_xtra_cons_check_;
#define MAXNCONS 10
#if MAXNCONS
static int xtra_cons_hist_[MAXNCONS + 1];
#endif

// ENQUEUE 0 means to  Multisend_ReceiveBuffer buffer -> InputPreSyn.send
// ENQUEUE 1 means to Multisend_ReceiveBuffer buffer -> psbuf -> InputPreSyn.send
// ENQUEUE 2 means to Multisend_ReceiveBuffer.incoming -> InputPrySyn.send
// Note that ENQUEUE 2 give more overlap between computation and exchange
// since the enqueuing takes place during computation except for those
// remaining during conservation.
#define ENQUEUE 2

#if ENQUEUE == 2
static unsigned long enq2_find_time_;
static unsigned long enq2_enqueue_time_;  // includes enq_find_time_
#endif

#define PHASE2BUFFER_SIZE 2048  // power of 2
#define PHASE2BUFFER_MASK (PHASE2BUFFER_SIZE - 1)
struct Phase2Buffer {
    InputPreSyn* ps;
    double spiketime;
    int gid;
};

#define MULTISEND_RECEIVEBUFFER_SIZE 10000
class Multisend_ReceiveBuffer {
  public:
    Multisend_ReceiveBuffer();
    virtual ~Multisend_ReceiveBuffer();
    void init(int index);
    void incoming(int gid, double spiketime);
    void enqueue();
    int index_;
    int size_;
    int count_;
    int maxcount_;
    bool busy_ = false;
    int nsend_, nrecv_;  // for checking conservation
    int nsend_cell_;     // cells that spiked this interval.
    NRNMPI_Spike** buffer_;

    void enqueue1();
    void enqueue2();
    InputPreSyn** psbuf_;

    void phase2send();
    int phase2_head_;
    int phase2_tail_;
    int phase2_nsend_cell_, phase2_nsend_;
    Phase2Buffer* phase2_buffer_;
};

#define MULTISEND_INTERVAL 2
static Multisend_ReceiveBuffer* multisend_receive_buffer[MULTISEND_INTERVAL];
static int current_rbuf, next_rbuf;
#if MULTISEND_INTERVAL == 2
// note that if a spike is supposed to be received by multisend_receive_buffer[1]
// then during transmission its gid is complemented.
#endif

static int* targets_phase1_;
static int* targets_phase2_;

void nrn_multisend_send(PreSyn* ps, double t, NrnThread* nt) {
    int i = ps->multisend_index_;
    if (i >= 0) {
        // format is cnt, cnt_phase1, array of target ranks.
        // Valid for one or two phase.
        int* ranks = targets_phase1_ + i;
        int cnt = ranks[0];
        int cnt_phase1 = ranks[1];
        ranks += 2;
        NRNMPI_Spike spk;
        spk.gid = ps->output_index_;
        spk.spiketime = t;
        if (next_rbuf == 1) {
            spk.gid = ~spk.gid;
        }
        if (nt == nrn_threads) {
            multisend_receive_buffer[next_rbuf]->nsend_ += cnt;
            multisend_receive_buffer[next_rbuf]->nsend_cell_ += 1;
            nrnmpi_multisend(&spk, cnt_phase1, ranks);
        } else {
            assert(0);
        }
    }
}

static void multisend_send_phase2(InputPreSyn* ps, int gid, double t) {
    int i = ps->multisend_phase2_index_;
    assert(i >= 0);
    // format is cnt_phase2, array of target ranks
    int* ranks = targets_phase2_ + i;
    int cnt_phase2 = ranks[0];
    ranks += 1;
    NRNMPI_Spike spk;
    spk.gid = gid;
    spk.spiketime = t;
    nrnmpi_multisend(&spk, cnt_phase2, ranks);
}

Multisend_ReceiveBuffer::Multisend_ReceiveBuffer() {
    busy_ = false;
    count_ = 0;
    size_ = MULTISEND_RECEIVEBUFFER_SIZE;
    buffer_ = new NRNMPI_Spike*[size_];
    psbuf_ = 0;
#if ENQUEUE == 1
    psbuf_ = new InputPreSyn*[size_];
#endif
    phase2_buffer_ = new Phase2Buffer[PHASE2BUFFER_SIZE];
    phase2_head_ = phase2_tail_ = 0;
}
Multisend_ReceiveBuffer::~Multisend_ReceiveBuffer() {
    nrn_assert(!busy_);
    for (int i = 0; i < count_; ++i) {
        delete buffer_[i];
    }
    delete[] buffer_;
    if (psbuf_)
        delete[] psbuf_;
    delete[] phase2_buffer_;
}
void Multisend_ReceiveBuffer::init(int index) {
    index_ = index;
    nsend_cell_ = nsend_ = nrecv_ = maxcount_ = 0;
    busy_ = false;
    for (int i = 0; i < count_; ++i) {
        delete buffer_[i];
    }
    count_ = 0;

    phase2_head_ = phase2_tail_ = 0;
    phase2_nsend_cell_ = phase2_nsend_ = 0;
}
void Multisend_ReceiveBuffer::incoming(int gid, double spiketime) {
    // printf("%d %p.incoming %g %g %d\n", nrnmpi_myid, this, t, spk->spiketime, spk->gid);
    nrn_assert(!busy_);
    busy_ = true;

    if (count_ >= size_) {
        size_ *= 2;
        NRNMPI_Spike** newbuf = new NRNMPI_Spike*[size_];
        for (int i = 0; i < count_; ++i) {
            newbuf[i] = buffer_[i];
        }
        delete[] buffer_;
        buffer_ = newbuf;
        if (psbuf_) {
            delete[] psbuf_;
            psbuf_ = new InputPreSyn*[size_];
        }
    }
    NRNMPI_Spike* spk = new NRNMPI_Spike();
    spk->gid = gid;
    spk->spiketime = spiketime;
    buffer_[count_++] = spk;
    if (maxcount_ < count_) {
        maxcount_ = count_;
    }

    ++nrecv_;
    busy_ = false;
}
void Multisend_ReceiveBuffer::enqueue() {
    // printf("%d %p.enqueue count=%d t=%g nrecv=%d nsend=%d\n", nrnmpi_myid, this, t, count_,
    // nrecv_, nsend_);
    nrn_assert(!busy_);
    busy_ = true;

    for (int i = 0; i < count_; ++i) {
        NRNMPI_Spike* spk = buffer_[i];
        InputPreSyn* ps;
        std::map<int, InputPreSyn*>::iterator gid2in_it;

        gid2in_it = gid2in.find(spk->gid);
        assert(gid2in_it != gid2in.end());
        ps = gid2in_it->second;

        if (use_phase2_ && ps->multisend_phase2_index_ >= 0) {
            Phase2Buffer& pb = phase2_buffer_[phase2_head_++];
            phase2_head_ &= PHASE2BUFFER_MASK;
            assert(phase2_head_ != phase2_tail_);
            pb.ps = ps;
            pb.spiketime = spk->spiketime;
            pb.gid = spk->gid;
        }

        ps->send(spk->spiketime, net_cvode_instance, nrn_threads);
        delete spk;
    }

    count_ = 0;
#if ENQUEUE != 2
    nrecv_ = 0;
    nsend_ = 0;
    nsend_cell_ = 0;
#endif
    busy_ = false;
    phase2send();
}

void Multisend_ReceiveBuffer::enqueue1() {
    // printf("%d %lx.enqueue count=%d t=%g nrecv=%d nsend=%d\n", nrnmpi_myid, (long)this, t,
    // count_, nrecv_, nsend_);
    nrn_assert(!busy_);
    busy_ = true;
    for (int i = 0; i < count_; ++i) {
        NRNMPI_Spike* spk = buffer_[i];
        InputPreSyn* ps;
        std::map<int, InputPreSyn*>::iterator gid2in_it;

        gid2in_it = gid2in.find(spk->gid);
        assert(gid2in_it != gid2in.end());
        ps = gid2in_it->second;
        psbuf_[i] = ps;
        if (use_phase2_ && ps->multisend_phase2_index_ >= 0) {
            Phase2Buffer& pb = phase2_buffer_[phase2_head_++];
            phase2_head_ &= PHASE2BUFFER_MASK;
            assert(phase2_head_ != phase2_tail_);
            pb.ps = ps;
            pb.spiketime = spk->spiketime;
            pb.gid = spk->gid;
        }
    }
    busy_ = false;
    phase2send();
}

void Multisend_ReceiveBuffer::enqueue2() {
    // printf("%d %lx.enqueue count=%d t=%g nrecv=%d nsend=%d\n", nrnmpi_myid, (long)this, t,
    // count_, nrecv_, nsend_);
    nrn_assert(!busy_);
    busy_ = false;
    for (int i = 0; i < count_; ++i) {
        NRNMPI_Spike* spk = buffer_[i];
        InputPreSyn* ps = psbuf_[i];
        ps->send(spk->spiketime, net_cvode_instance, nrn_threads);
        delete spk;
    }
    count_ = 0;
    nrecv_ = 0;
    nsend_ = 0;
    nsend_cell_ = 0;
    busy_ = false;
}

void Multisend_ReceiveBuffer::phase2send() {
    while (phase2_head_ != phase2_tail_) {
        Phase2Buffer& pb = phase2_buffer_[phase2_tail_++];
        phase2_tail_ &= PHASE2BUFFER_MASK;
        int gid = pb.gid;
        if (index_) {
            gid = ~gid;
        }
        multisend_send_phase2(pb.ps, gid, pb.spiketime);
    }
}

static int max_ntarget_host;
// For one phase sending, max_multisend_targets is max_ntarget_host.
// For two phase sending, it is the maximum of all the
// ntarget_hosts_phase1 and ntarget_hosts_phase2.
static int max_multisend_targets;

void nrn_multisend_init() {
    for (int i = 0; i < n_multisend_interval; ++i) {
        multisend_receive_buffer[i]->init(i);
    }
    current_rbuf = 0;
    next_rbuf = n_multisend_interval - 1;
#if ENQUEUE == 2
    enq2_find_time_ = enq2_enqueue_time_ = 0;
#endif
    n_xtra_cons_check_ = 0;
#if MAXNCONS
    for (int i = 0; i <= MAXNCONS; ++i) {
        xtra_cons_hist_[i] = 0;
    }
#endif  // MAXNCONS
}

static int multisend_advance() {
    NRNMPI_Spike spk;
    int i = 0;
    while (nrnmpi_multisend_single_advance(&spk)) {
        i += 1;
        int j = 0;
#if MULTISEND_INTERVAL == 2
        if (spk.gid < 0) {
            spk.gid = ~spk.gid;
            j = 1;
        }
#endif
        multisend_receive_buffer[j]->incoming(spk.gid, spk.spiketime);
    }
    return i;
}

#if NRN_MULTISEND
void nrn_multisend_advance() {
    if (use_multisend_) {
        multisend_advance();
#if ENQUEUE == 2
        multisend_receive_buffer[current_rbuf]->enqueue();
#endif
    }
}
#endif

void nrn_multisend_receive(NrnThread* nt) {
    //	nrn_spike_exchange();
    assert(nt == nrn_threads);
    //	double w1, w2;
    int ncons = 0;
    int& s = multisend_receive_buffer[current_rbuf]->nsend_;
    int& r = multisend_receive_buffer[current_rbuf]->nrecv_;
//	w1 = nrn_wtime();
#if NRN_MULTISEND & 1
    if (use_multisend_) {
        nrn_multisend_advance();
        nrnmpi_barrier();
        nrn_multisend_advance();
        // with two phase we expect conservation to hold and ncons should
        // be 0.
        while (nrnmpi_multisend_conserve(s, r) != 0) {
            nrn_multisend_advance();
            ++ncons;
        }
    }
#endif
//	w1 = nrn_wtime() - w1;
//	w2 = nrn_wtime();

#if ENQUEUE == 0
    multisend_receive_buffer[current_rbuf]->enqueue();
#endif
#if ENQUEUE == 1
    multisend_receive_buffer[current_rbuf]->enqueue1();
    multisend_receive_buffer[current_rbuf]->enqueue2();
#endif
#if ENQUEUE == 2
    multisend_receive_buffer[current_rbuf]->enqueue();
    s = r = multisend_receive_buffer[current_rbuf]->nsend_cell_ = 0;

    multisend_receive_buffer[current_rbuf]->phase2_nsend_cell_ = 0;
    multisend_receive_buffer[current_rbuf]->phase2_nsend_ = 0;

    enq2_find_time_ = 0;
    enq2_enqueue_time_ = 0;
#endif  // ENQUEUE == 2
//	wt1_ = nrn_wtime() - w2;
//	wt_ = w1;
#if MULTISEND_INTERVAL == 2
    // printf("%d reverse buffers %g\n", nrnmpi_myid, t);
    if (n_multisend_interval == 2) {
        current_rbuf = next_rbuf;
        next_rbuf = ((next_rbuf + 1) & 1);
    }
#endif
}

void nrn_multisend_cleanup() {
    if (targets_phase1_) {
        delete[] targets_phase1_;
        targets_phase1_ = nullptr;
    }

    if (targets_phase2_) {
        delete[] targets_phase2_;
        targets_phase2_ = nullptr;
    }

    // cleanup MultisendReceiveBuffer here as well
}

void nrn_multisend_setup() {
    nrn_multisend_cleanup();
    if (!use_multisend_) {
        return;
    }
    nrnmpi_multisend_comm();
    // if (nrnmpi_myid == 0) printf("multisend_setup()\n");
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

    // completely new algorithm does one and two phase.
    nrn_multisend_setup_targets(use_phase2_, targets_phase1_, targets_phase2_);

    if (!multisend_receive_buffer[0]) {
        multisend_receive_buffer[0] = new Multisend_ReceiveBuffer();
    }
#if MULTISEND_INTERVAL == 2
    if (n_multisend_interval == 2 && !multisend_receive_buffer[1]) {
        multisend_receive_buffer[1] = new Multisend_ReceiveBuffer();
    }
#endif
}
#endif  // NRN_MULTISEND
}  // namespace coreneuron
