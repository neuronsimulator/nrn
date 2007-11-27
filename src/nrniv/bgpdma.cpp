// included by netpar.cpp
extern "C" {
extern void nrnmpi_int_allgatherv(int*, int*, int*, int*);
extern void nrnmpi_int_gather(int*, int*, int, int);
extern void nrnmpi_int_gatherv(int*, int, int*, int*, int*, int);

extern void nrnmpi_bgp_comm();
extern void nrnmpi_bgp_multisend(NRNMPI_Spike*, int, int*);
extern int nrnmpi_bgp_single_advance(NRNMPI_Spike*);
extern int nrnmpi_bgp_conserve(int nsend, int nrecv);
}

class BGP_DMASend {
public:
	BGP_DMASend();
	virtual ~BGP_DMASend();
	void send(int gid, double t);
	int ntarget_hosts_;
	int* target_hosts_;
	NRNMPI_Spike spk_;
};

// static int bgpdma_nsend_, bgpdma_nrecv_; // declared earlier for initialization

static int bgp_advance() {
	NRNMPI_Spike spk_;
	PreSyn* ps;
	int i = 0;
	while(nrnmpi_bgp_single_advance(&spk_)) {
		i += 1;
		assert(gid2in_->find(spk_.gid, ps));
		ps->send(spk_.spiketime, net_cvode_instance);
	}
	nrecv_ += i;
	return i;
}

BGP_DMASend::BGP_DMASend() {
	ntarget_hosts_ = 0;
	target_hosts_ = nil;
}

BGP_DMASend::~BGP_DMASend() {
	if (target_hosts_) {
		delete [] target_hosts_;
	}
}

void BGP_DMASend::send(int gid, double t) {
	spk_.gid = gid;
	spk_.spiketime = t;
	nrnmpi_bgp_multisend(&spk_, ntarget_hosts_, target_hosts_);
	bgpdma_nsend_ += ntarget_hosts_;
	nsend_ += 1;
	bgpdma_nrecv_ += bgp_advance();
}




static void determine_source_hosts();
static void determine_targid_count_on_srchost(int* src, int* send);
static void determine_targids_on_srchost(int* s, int* scnt, int* sdispl,
    int* r, int* rcnt, int* rdispl);
static void determine_target_hosts();
static int gathersrcgid(int hostbegin, int totalngid, int* ngid,
	int* thishostgid, int* n, int* displ, int bsize, int* buf);

void bgp_dma_receive() {
//	nrn_spike_exchange();
	bgpdma_nrecv_ += bgp_advance();
	while (nrnmpi_bgp_conserve(bgpdma_nsend_, bgpdma_nrecv_) != 0) {
		bgpdma_nrecv_ += bgp_advance();
	}
	bgpdma_nrecv_ = 0; bgpdma_nsend_ = 0;
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

void bgpdma_cleanup_presyn(PreSyn* ps) {
	if (ps->output_index_ >= 0 && ps->bgp.dma_send_) {
		delete ps->bgp.dma_send_;
		ps->bgp.dma_send_ = 0;
	}
}

void bgp_dma_setup() {
	nrnmpi_bgp_comm();
	// although we only care about the set of hosts that gid2out_
	// sends spikes to (source centric). We do not want to send
	// the entire list of gid2in (which may be 10000 times larger
	// than gid2out) from every machine to every machine.
	// so we accomplish the task it two phases the first of which
	// involves allgather with a total receive buffer size of number
	// of cells (even that is too large and we will split it up
	// into chunks). And the second, an
	// allreduce with receive buffer size of number of hosts.

	// gid2in_ gets spikes from which hosts.
	determine_source_hosts();

	// gid2out_ sends spikes to which hosts
	determine_target_hosts();
}

void determine_source_hosts() {
	int i, nsrcgid, ihost, jhost;
	PreSyn* ps;

	// some target PreSyns may not have any input
	// so initialize all to -1
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		assert(ps->output_index_ < 0);
		ps->bgp.srchost_ = -1;
	}}}

	// how many ngid src gids on this machine (PreSyn generates the
	// spikes) also create the BGP_DMASend instances and attach to PreSyn
	nsrcgid = 0;
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			++nsrcgid;
			bgpdma_cleanup_presyn(ps);
			ps->bgp.dma_send_ = new BGP_DMASend();
		}
	}}}
	// store source gids in an array for later transfer.
	int* gids = nsrcgid ? new int[nsrcgid] : 0;
	i = 0;
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			gids[i] = ps->gid_;
			++i;
		}
	}}}
	
	// how many src gids on each machine
	int* host_nsrcgid = new int[nrnmpi_numprocs];
	nrnmpi_int_allgather(&nsrcgid, host_nsrcgid, 1);
//if (nrnmpi_myid == 0) {
//for (i=0; i < nrnmpi_numprocs; ++i) {
//printf("i=%d host_nsrcgid=%d\n", i, host_nsrcgid[i]);
//}
//}

	// to assess allgatherv requirements, what is the total number
	// of src PreSyn
	long totalngid = 0;
	int maxngid = 0;
	for (i=0; i < nrnmpi_numprocs; ++i) {
		totalngid += host_nsrcgid[i];
		if (maxngid < host_nsrcgid[i]) {
			maxngid = host_nsrcgid[i];
		}
	}
	// get the srcgids from everywhere and fill the src PreSyn host
	// field. Assume there might be more cells than buffer space.
	int bufsize = 10000; // can get at least this
	// since we routinely allocate things of this size it can be at least...
	bufsize = (bufsize < nrnmpi_numprocs) ? nrnmpi_numprocs : bufsize;
	bufsize = (maxngid < bufsize) ? bufsize : maxngid; // guarantee at least enough for any one host
	bufsize = (totalngid < bufsize) ? totalngid : bufsize; // but we certainly do not need more than this
	int* n = new int[nrnmpi_numprocs];
	int* displ = new int[nrnmpi_numprocs+1];
	int* buf = new int[bufsize];
//printf("%d bufsize=%d\n", nrnmpi_myid, bufsize);
	for (ihost = 0; ihost < nrnmpi_numprocs; ) {
		jhost = gathersrcgid(ihost, totalngid, host_nsrcgid, gids,
			n, displ, bufsize, buf);
		for (; ihost < jhost; ++ihost) {
			for (i = displ[ihost]; i < displ[ihost+1]; ++i) {
				int gid = buf[i];
				if (gid2in_ && gid2in_->find(gid, ps)) {
					ps->bgp.srchost_ = ihost;
//printf("%d ihost=%d jhost=%d i=%d gid=%d\n", nrnmpi_myid, ihost, jhost, i, gid);
				}
			}
		}
	}

	delete [] buf;
	delete [] displ;
	delete [] host_nsrcgid;
	if (gids) delete [] gids;

#if 0
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
printf("%d target gid=%d srchost=%d\n", nrnmpi_myid, ps->gid_, ps->bgp.srchost_);
	}}}
#endif

}

int gathersrcgid(int hostbegin, int totalngid, int* ngid, int* thishostgid,
    int* n, int* displ, int bsize, int* buf) {
	int i, hostend;
	for (i=0; i < hostbegin; ++i) {
		n[i] = 0;
		displ[i] = 0;
	}
	displ[i] = 0;
	for (; i < nrnmpi_numprocs; ++i) {
		if ((displ[i] + ngid[i]) > bsize) {
			break;
		}
		n[i] = ngid[i];
		displ[i+1] = displ[i] + n[i];
		hostend = i+1;
	}
	for (; i < nrnmpi_numprocs; ++i) {
		n[i] = 0;
		displ[i+1] = displ[i];
	}
	int* me = nil;
	if (nrnmpi_myid >= hostbegin && nrnmpi_myid < hostend) {
		me = thishostgid;
	}
#if 0
printf("%d hostbegin=%d hostend=%d totalngid=%d bsize=%d\n",
nrnmpi_myid, hostbegin, hostend, totalngid, bsize);
printf("%d thishostgid=%lx me=%lx\n", nrnmpi_myid, thishostgid, me);
for (i=0; i < nrnmpi_numprocs; ++i) {
printf("%d i=%d n=%d displ=%d\n", nrnmpi_myid, i, n[i], displ[i]);
}
#endif
	nrnmpi_int_allgatherv(me, buf, n, displ);
	return hostend;
}

void determine_target_hosts() {
	PreSyn* ps;
	int i;
	// how many target gids
	int ntargid = 0;
	// how many distinct gids this host needs from each host
	int* srchost_count = new int[nrnmpi_numprocs];
	for (i=0; i < nrnmpi_numprocs; ++i) {
		srchost_count[i] = 0;
	}
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		assert(ps->output_index_ < 0);
		assert(ps->bgp.srchost_ >= -1 && ps->bgp.srchost_ < nrnmpi_numprocs);
		if (ps->bgp.srchost_ >= 0) {
			++ntargid;
			++srchost_count[ps->bgp.srchost_];
		}
	}}}
	int* srchost_displ = new int[nrnmpi_numprocs + 1];
	srchost_displ[0] = 0;
	for (i=0; i < nrnmpi_numprocs; ++i) {
		srchost_displ[i+1] = srchost_displ[i] + srchost_count[i];
	}
#if 0
printf("%d ntargid=%d  last srchost_displ=%d\n", nrnmpi_myid, ntargid, srchost_displ[nrnmpi_numprocs]);
for (i=0; i < nrnmpi_numprocs; ++i) {
	printf("%d i=%d srchost_count=%d srchost_displ=%d\n",
	nrnmpi_myid, i, srchost_count[i], srchost_displ[i]);
}
#endif
	// recount srchost_count while organizing the
	// list of target gids to be organized in ihost order
	int* targid_on_tar = ntargid ? new int[ntargid] : 0;
	for (i=0; i < nrnmpi_numprocs; ++i) {
		srchost_count[i] = 0;
	}
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		i = ps->bgp.srchost_;
		if (i >= 0) {
			targid_on_tar[srchost_displ[i] + srchost_count[i]] = ps->gid_;
			++srchost_count[i];
		}
	}}}
	
	// now is a good time to use the DMA transfer capabilities.
	int* tarcounts = new int[nrnmpi_numprocs];
	int* tardispl = new int[nrnmpi_numprocs+1];

	determine_targid_count_on_srchost(srchost_count, tarcounts);
	tardispl[0] = 0;
	for (i=0; i < nrnmpi_numprocs; ++i) {
		tardispl[i+1] = tarcounts[i] + tardispl[i];
	}
	int n = tardispl[nrnmpi_numprocs];
	int* targid_on_src = n ? new int[n] : 0;
	// and here is another opportunity for DMA
	determine_targids_on_srchost(targid_on_tar, srchost_count, srchost_displ,
		targid_on_src, tarcounts, tardispl);
	
	// on a src cell basis, what is the size of DMASend.target_hosts.
	if (gid2out_) for (i=0; i < n; ++i) {
		assert(gid2out_->find(targid_on_src[i], ps));
		++ps->bgp.dma_send_->ntarget_hosts_;
	}
	// allocate and set to 0 for recount
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		BGP_DMASend* s = ps->bgp.dma_send_;
		s->target_hosts_ = new int[s->ntarget_hosts_];
		s->ntarget_hosts_ = 0;
	}}}
	if (gid2out_) for (i=0; i < nrnmpi_numprocs; ++i) {
		for (int j = tardispl[i] ; j < tardispl[i+1]; ++j) {
			assert(gid2out_->find(targid_on_src[j], ps));
			BGP_DMASend* s = ps->bgp.dma_send_;
			s->target_hosts_[s->ntarget_hosts_++] = i;
		}
	}

	if (targid_on_src) delete [] targid_on_src;
	delete [] tardispl;
	delete [] tarcounts;
	if (targid_on_tar) delete [] targid_on_tar;
	delete [] srchost_displ;
	delete [] srchost_count;
#if 0
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		BGP_DMASend* s = ps->bgp.dma_send_;
		for (i=0; i < s->ntarget_hosts_; ++i) {
printf("%d gid=%d i=%d targethost=%d\n", nrnmpi_myid, ps->gid_, i, s->target_hosts_[i]);
		}
	}}}
#endif
}

void determine_targid_count_on_srchost(int* src, int* tarcounts) {
	int i;
#if 0
for (i=0; i < nrnmpi_numprocs; ++i) {
printf("%d i=%d srchostcnt=%d\n", nrnmpi_myid, i, src[i]);
}
#endif
	for (i=0; i < nrnmpi_numprocs; ++i) {
//printf("%d i=%d src=%d\n", nrnmpi_myid, i, src[i]);
		nrnmpi_int_gather(src+i, tarcounts, 1, i);
#if 0
if (i == nrnmpi_myid) {
for (int j = 0; j < nrnmpi_numprocs; ++j) {
printf("%d gather i=%d j=%d tarcounts=%d\n", nrnmpi_myid, i, j, tarcounts[j]);
}}
#endif
	}
}

void determine_targids_on_srchost(int* s, int* scnt, int* sdispl,
    int* r, int* rcnt, int* rdispl) {
	int i;
	for (i=0; i< nrnmpi_numprocs; ++i) {
		nrnmpi_int_gatherv(
			s + sdispl[i], scnt[i],
			r, rcnt, rdispl,
			i
		);
	}
}

