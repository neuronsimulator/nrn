/*
For very large numbers of processors and cells and fanout, it is taking
a long time to figure out each cells target list given the input gids
(gid2in) on each host. e.g 240 seconds for 2^25 cells, 1k connections
per cell, and 128K cores; and 340 seconds for two phase excchange.
To reduce this setup time we experiment with very different algorithm in which
we construct a gid target host list on host gid%nhost and copy that list to
the source host owning the gid.
*/

#if 0
void celldebug(const char* p, Gid2PreSyn* map) {
	FILE* f;
	char fname[100];
	sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	PreSyn* ps;
	fprintf(f, "\n%s\n", p);
	int rank = nrnmpi_myid;
	fprintf(f, "  %2d:", rank);
	NrnHashIterate(Gid2PreSyn, map, PreSyn*, ps) {
		fprintf(f, " %2d", ps->gid_);
	}}}
	fprintf(f, "\n");
	fclose(f);
}

void alltoalldebug(const char* p, int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl){
	FILE* f;
	char fname[100];
	sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	fprintf(f, "\n%s\n", p);
	int rank = nrnmpi_myid;
	fprintf(f, "  rank %d\n", rank);
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		fprintf(f, "    s%d : %d %d :", i, scnt[i], sdispl[i]);
		for (int j = sdispl[i]; j < sdispl[i+1]; ++j) {
			fprintf(f, " %2d", s[j]);
		}
		fprintf(f, "\n");
	}
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		fprintf(f, "    r%d : %d %d :", i, rcnt[i], rdispl[i]);
		for (int j = rdispl[i]; j < rdispl[i+1]; ++j) {
			fprintf(f, " %2d", r[j]);
		}
		fprintf(f, "\n");
	}
	fclose(f);
}
#else
void celldebug(const char* p, Gid2PreSyn* map) {}
void alltoalldebug(const char* p, int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl){}
#endif

#if 0
void phase1debug() {
	FILE* f;
	char fname[100];
	sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	PreSyn* ps;
	fprintf(f, "\nphase1debug %d", nrnmpi_myid);
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		fprintf(f, "\n %2d:", ps->gid_);
		BGP_DMASend* bs = ps->bgp.dma_send_;
		for (int i=0; i < bs->ntarget_hosts_; ++i) {
			fprintf(f, " %2d", bs->target_hosts_[i]);
		}
	}}}
	fprintf(f, "\n");
	fclose(f);
}

void phase2debug() {
	FILE* f;
	char fname[100];
	sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	PreSyn* ps;
	fprintf(f, "\nphase2debug %d", nrnmpi_myid);
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		fprintf(f, "\n %2d:", ps->gid_);
		BGP_DMASend_Phase2* bs = ps->bgp.dma_send_phase2_;
	    if (bs) {
		for (int i=0; i < bs->ntarget_hosts_phase2_; ++i) {
			fprintf(f, " %2d", bs->target_hosts_phase2_[i]);
		}
	    }
	}}}
	fprintf(f, "\n");
	fclose(f);
}
#endif

static void del(int* a) {
	if (a) {
		delete [] a;
	}
}

static int* newintval(int val, int size) {
	if (size == 0) { return 0; }
	int* x = new int[size];
	for (int i=0; i < size; ++i) {
		x[i] = val;
	}
	return x;
}

static int* newoffset(int* acnt, int size) {
	int* aoff = new int[size+1];
	aoff[0] = 0;
	for (int i=0; i < size; ++i) {
		aoff[i+1] = aoff[i] + acnt[i];
	}
	return aoff;
}


// input scnt, sdispl ; output, newly allocated rcnt, rdispl
static void all2allv_helper(int* scnt, int* sdispl, int*& rcnt, int*& rdispl) {
	int i;
	int np = nrnmpi_numprocs;
	int* c = newintval(1, np);
	rdispl = newoffset(c, np);
	rcnt = newintval(0, np);
	nrnmpi_int_alltoallv(scnt, c, rdispl, rcnt, c, rdispl);
	del(c);
	del(rdispl);
	rdispl = newoffset(rcnt, np);
}

//input s, scnt, sdispl ; output, newly allocated r, rcnt, rdispl
static void all2allv_int(int* s, int* scnt, int* sdispl, int*& r, int*& rcnt, int*& rdispl, const char* dmes) {
	int np = nrnmpi_numprocs;
	all2allv_helper(scnt, sdispl, rcnt, rdispl);
	r = newintval(0, rdispl[np]);

	nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
	alltoalldebug(dmes, s, scnt, sdispl, r, rcnt, rdispl);

	// when finished with r, rcnt, rdispl, caller should del them.
}

class TarList {
public:
	TarList();
	virtual ~TarList();
	void alloc();
	int size;
	int* list;
	int gid;
	int rank;
#if TWOPHASE
	int* indices; // indices of list for groups of phase2 targets.
	// If indices is not null, then size is then size is one less than
	// the size of the indices list where indices[size] = the size of
	// the list. Indices[0] is 0 and list[indices[i]] is the rank
	// to send the ith group of phase2 targets.
#endif
};

declareNrnHash(Int2TarList, int, TarList*)
implementNrnHash(Int2TarList, int, TarList*)
static Int2TarList* gid2tarlist;

TarList::TarList() {
	size = 0;
	list = 0;
	gid = -1;
	rank = -1;
#if TWOPHASE
	indices = 0;
#endif
}
TarList::~TarList() {
	del(list);
#if TWOPHASE
	del(indices);
#endif
}
void TarList::alloc() {
	if (size) {
		list = new int[size];
	}
}

static void random_init(int);
static int iran(int, int);

static void phase2organize(TarList* tl) {
	// copied and modified from old specify_phase2_distribution of bgpdma.cpp
	int n, nt;
	nt = tl->size;
	n = int(sqrt(double(nt)));
	// change to about 20
	if (n > 1) { // do not bother if not many connections
		tl->indices = new int[n+1];
		tl->indices[n] = tl->size;
		tl->size = n;
		for (int i=0; i < n; ++i) {
			tl->indices[i] = (i * nt)/n;
		}
		// Note: not sure the following is true anymore but it could be.
		// This distribution is very biased (if 0 is a phase1 target
		// it is always a phase2 sender. So now choose a random
		// target in the subset and make that the phase2 sender
		// (need to switch the indices[i] target and the one chosen)
		for (int i=0; i < n; ++i) {
			int i1 = tl->indices[i];
			int i2 = tl->indices[i+1]-1;
			// need discrete uniform random integer from i1 to i2
			int i3 = iran(i1, i2);
			int itar = tl->list[i1];
			tl->list[i1] = tl->list[i3];
			tl->list[i3] = itar;
		}
	}
}

static void target_list_sizes() {
	int *s, *r, *scnt, *rcnt, *sdispl, *rdispl;
	int nhost = nrnmpi_numprocs;

	celldebug("output gid", gid2out_);
	celldebug("input gid", gid2in_);

	// Create and attach BGP_DMASend instances to output Presyn
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		if (ps->output_index_ >= 0) {
			bgpdma_cleanup_presyn(ps);
			ps->bgp.dma_send_ = new BGP_DMASend();
		}
	}}}

#if TWOPHASE
	// need to use the bgp union slot for dma_send_phase2_
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		ps->bgp.srchost_ = 0;
	}}}
#endif

	// How many items will be sent from this host to the intermediate?
	// gid%nhost rank.
	// scnt is number of input gids from target
	scnt = newintval(0, nhost);
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		++scnt[ps->gid_%nhost];
	}}}

	// s are the input gids from target to be sent to the various intermediates
	sdispl = newoffset(scnt, nhost);
	s = newintval(0, sdispl[nhost]);
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		s[sdispl[ps->gid_%nhost]++] = ps->gid_;
	}}}
	// Restore sdispl for the message.
	del(sdispl);
	sdispl = newoffset(scnt, nhost);

	all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "gidin to intermediate");
	del(s);
	del(scnt);
	del(sdispl);
	// r is the gids received by this intermediate rank from all other ranks.

	gid2tarlist = new Int2TarList(1000);
	// Now figure out the size of the target list for each distinct gid in r.
	for (int i=0; i < rdispl[nhost]; ++i) {
#if ALTHASH
		TarList* tl;
		if (gid2tarlist->find(r[i], tl)) {
			tl->size += 1;
		}else{
			tl = new TarList();
			tl->size = 1;
			gid2tarlist->insert(r[i], tl);
			tl->gid = r[i];
		}
		assert(tl->gid == r[i]);
#else
		assert(0);
#endif
	}

	// Conceptually, now the intermediate is the mpi source and the gid
	// sources are the mpi destination in regard to target lists.
	// It would be possible at this point, but confusing,
	// to allocate a s[rdispl[nhost]] and figure out scnt and sdispl by
	// by getting the counts and gids from the ranks that own the source
	// gids. In this way we could organize s without having to allocate
	// individual target lists on the intermediate and then allocate
	// another large s buffer to receive a copy of them. However for
	// this processing we already require two large buffers for input
	// gid's so there is no real savings of space.
	// So let's do the simple obvious sequence and now complete the
	// target lists.

	// Allocate the target lists (and set size to 0 (we will recount when filling).
	NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
		tl->alloc();
		tl->size = 0;
	}}}

	// fill the target lists
	for (int rank=0; rank < nhost; ++rank) {
		int b = rdispl[rank];
		int e = b + rcnt[rank];
		for (int i=b; i < e; ++i) {
			TarList* tl;
			if (gid2tarlist->find(r[i], tl)) {
				tl->list[tl->size] = rank;
				tl->size++;
			}
		}
	}
	del(r);
	del(rcnt);
	del(rdispl);

	// Now the intermediate hosts have complete target lists and
	// the sources know the intermediate host from the gid2out_ map and
	// the ps->gid_.
	// We could potentially organize here for two-phase exchange as well.

	// How many target lists are desired by the source rank?
	// Ironically, for round robin distributions, the target lists are
	// already on the proper source rank so the following code should
	// be tested for random distributions of gids.
	scnt = newintval(0, nhost);
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		++scnt[ps->gid_%nhost];
	}}}
	sdispl = newoffset(scnt, nhost);

	// what are the gids of those target lists
	s = newintval(0, sdispl[nhost]);
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		s[sdispl[ps->gid_%nhost]++] = ps->gid_;
	}}}
	// Restore sdispl for the message.
	del(sdispl);
	sdispl = newoffset(scnt, nhost);
	all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "gidout");
	
#if TWOPHASE
	// fill in the tl->rank for phase 1 target lists
	for (int rank=0; rank < nhost; ++rank) {
		int b = rdispl[rank];
		int e = rdispl[rank+1];
		for (int i=b; i < e; ++i) {
			TarList* tl;
			if (gid2tarlist->find(r[i], tl)) {
				tl->rank = rank;
			}
		}
	}
	del(s); del(scnt); del(sdispl); del(r); del(rcnt); del(rdispl);

	if (use_phase2_) {
		// For conservation, the source BGP_DMASend.ntarget_host
		// needs to be the total number of destinations for that
		// gid and not just the number of phase 1 destinations
		// which will go into BGP_DMASend.ntarget_hosts_phase1.
		// Here, we send those ntarget_host values. Note that
		// the counts and displacements we just deleted are exactly
		// what is needed. However to avoid confusion, send the info
		// using our standard style. The payload is (gid, targetsize)
		// pairs.
		scnt = newintval(0, nhost);
		NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
			scnt[tl->rank] += 2;
		}}}
		sdispl = newoffset(scnt, nhost);
		s = newintval(0, sdispl[nhost]);
		NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
			s[sdispl[tl->rank]++] = tl->gid;
			s[sdispl[tl->rank]++] = tl->size;
		}}}
		del(sdispl);
		sdispl = newoffset(scnt, nhost);
		all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "phase1 ntarget_hosts");
		for (int i=0; i < rdispl[nhost]; i += 2) {
			PreSyn* ps;
			assert(gid2out_->find(r[i], ps));
			ps->bgp.dma_send_->ntarget_hosts_ = r[i+1];
		}
		del(s); del(scnt); del(sdispl); del(r); del(rcnt); del(rdispl);
	}

	if (use_phase2_) {
		random_init(nrnmpi_myid + 1);
		NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
			phase2organize(tl);
		}}}
	}

	// For clarity, use the all2allv_int style of information flow
	// from source to destination as above (see obsolete section below
	// which is difficult to understand) and also use a uniform code
	// for copying one and two phase information from a TarList to
	// develop the s, scnt, and sdispl buffers. That is, a buffer list
	// section in s for either a one-phase list or the much shorter
	// (individually) lists for first and second phases, has a
	// gid, size, header for each list. Thus, if n blocks have a
	// total of m target ranks in their list to send to a rank, the
	// scnt sent to that rank is m + 2*n.
	
	// how much to send to each rank
	scnt = newintval(0, nhost);
	NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
		if (tl->indices) {
			// indices[size] is the size of list but size of those
			// are the sublist phase 2 destination ranks which
			// don't get sent as part of the phase 2 target list.
			// Also there is a phase 1 target list of size so there
			// are altogether size+1 target lists.
			// (one phase 1 list and size phase 2 lists)
			scnt[tl->rank] += tl->size + 2;
			for (int i=0; i < tl->size; ++i) {
				scnt[tl->list[tl->indices[i]]] +=
					tl->indices[i+1] - tl->indices[i] + 1;
			}
		}else{
			// gid, list size, list
			scnt[tl->rank] += tl->size + 2;
		}
	}}}
	sdispl = newoffset(scnt, nhost);
	s = newintval(0, sdispl[nhost]);
	// what to send to each rank
	NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
		if (tl->indices) {
			s[sdispl[tl->rank]++] = tl->gid;
			s[sdispl[tl->rank]++] = tl->size;
			for (int i = 0; i < tl->size; ++i) {
				s[sdispl[tl->rank]++] = tl->list[tl->indices[i]];
			}
			for (int i = 0; i < tl->size; ++i) {
				int rank = tl->list[tl->indices[i]];
				s[sdispl[rank]++] = tl->gid;
				assert(tl->indices[i+1] > tl->indices[i]);
				s[sdispl[rank]++] = tl->indices[i+1] - tl->indices[i] - 1;
				for (int j = tl->indices[i] + 1; j < tl->indices[i+1]; ++j) {
					s[sdispl[rank]++] = tl->list[j];
				}
			}
			
		}else{
			// gid, list size, list
			s[sdispl[tl->rank]++] = tl->gid;
			s[sdispl[tl->rank]++] = tl->size;
			for (int i = 0; i < tl->size; ++i) {
				s[sdispl[tl->rank]++] = tl->list[i];
			}
		}
		delete tl;
	}}}
	sdispl = newoffset(scnt, nhost);
	all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "lists");
	del(s);
	del(scnt);
	del(sdispl);

	// Using the r gid, size, list info, copy lists to proper phase 1 and
	// phase 2 lists. (Phase one lists found in gid2out_ and phase two
	// lists found in gid2in_.
	del(rcnt);
	int sz = rdispl[nhost];
	del(rdispl);
	for (int i = 0; i < sz;) {
		int gid = r[i++];
		int size = r[i++];
		PreSyn* ps = 0;
		if (use_phase2_) { // look in gid2in first
		    if (gid2in_->find(gid, ps)) { // phase 2 target list
			BGP_DMASend_Phase2* bsp = new BGP_DMASend_Phase2();
			ps->bgp.dma_send_phase2_ = bsp;
			bsp->ntarget_hosts_phase2_ = size;
			int* p = newintval(0, size);
			bsp->target_hosts_phase2_ = p;
//printf("%d %d phase2 size=%d\n", nrnmpi_myid, gid, bsp->ntarget_hosts_phase2_);
			for (int j = 0; j < size; ++j) {
				p[j] = r[i++];
				assert(p[j] != nrnmpi_myid);
			}
		    }
		}
		if (!ps) { // phase 1 target list (or whole list if use_phase2 is 0)
			assert(gid2out_->find(gid, ps));
			BGP_DMASend* bs =  ps->bgp.dma_send_;
			bs->ntarget_hosts_phase1_ = size;
			if (use_phase2_ == 0) {
				// otherwise filled in prior to phase2_organize.
				bs->ntarget_hosts_ = size;
			}
			int* p = newintval(0, size);
			bs->target_hosts_ = p;
//printf("%d %d phase1 size=%d\n", nrnmpi_myid, gid, bs->ntarget_hosts_);
			for (int j = 0; j < size; ++j) {
				p[j] = r[i++];
				// There never was a possibility of send2self
				// because an output presyn is never in gid2in_.
				assert(p[j] != nrnmpi_myid);
			}
		}
	}
	del(r);
//	phase1debug();
//	phase2debug();

#else // NOT TWOPHASE --- obsolete
	// r is the gids (whose target lists are desired)
	// received by this intermediate rank from all other ranks.
	// The rcnt, rdispl received by the intermediate from source gid rank
	// is the scnt, sdispl order we send back from the intermediate
	// to the source gid rank, just need to copy target list size
	// into an array
	// Give s and r reasonable names
	int *gid_source = s;
	int *gid_source_cnt = scnt;
	int *gid_source_displ = sdispl;
	int *gid_intermediate = r;
	int *gid_intermediate_cnt = rcnt;
	int *gid_intermediate_displ = rdispl;
	alltoalldebug("gid source intermediate", gid_source, gid_source_cnt,
gid_source_displ, gid_intermediate, gid_intermediate_cnt, gid_intermediate_displ);

	int *tlsize_source, *tlsize_intermediate;
	tlsize_source = newintval(0, gid_source_displ[nhost]);
	tlsize_intermediate = newintval(0, gid_intermediate_displ[nhost]);
	for (int rank=0; rank < nhost; ++rank) {
		int b = gid_intermediate_displ[rank];
		int e = gid_intermediate_displ[rank+1];
		for (int i=b; i < e; ++i) {
			TarList* tl;
			if (gid2tarlist->find(gid_intermediate[i], tl)) {
				tlsize_intermediate[i] = tl->size;
				tl->rank = rank;
			}
		}
	}
#if 0 //debug
	NrnHashIterate(Int2TarList, gid2tarlist, TarList*, tl) {
printf("%d NrnHashIterate rank=%d gid=%d size=%d\n", nrnmpi_myid, tl->rank, tl->gid, tl->size);
	}}}
#endif

	nrnmpi_int_alltoallv(tlsize_intermediate, gid_intermediate_cnt, gid_intermediate_displ,
		tlsize_source, gid_source_cnt, gid_source_displ);
	alltoalldebug("intermediate sizes", tlsize_intermediate, gid_intermediate_cnt, gid_intermediate_displ,
		tlsize_source, gid_source_cnt, gid_source_displ);
	del(tlsize_intermediate);

	// Need buffers for the full target list.
	// on the intermediate
	scnt = newintval(0, nhost);
	for (int rank=0; rank < nhost; ++rank) {
		int b = gid_intermediate_displ[rank];
		int e = gid_intermediate_displ[rank+1];
		for (int i=b; i < e; ++i) {
			TarList* tl;
			if (gid2tarlist->find(gid_intermediate[i], tl)) {
				scnt[rank] += tl->size;
			}
		}
	}
	sdispl = newoffset(scnt, nhost);
	s = newintval(0, sdispl[nhost]);
	int j=0;
	for (int rank=0; rank < nhost; ++rank) {
		int b = gid_intermediate_displ[rank];
		int e = gid_intermediate_displ[rank+1];
		for (int i=b; i < e; ++i) {
			TarList* tl;
			if (gid2tarlist->find(gid_intermediate[i], tl)) {
				for (int k = 0; k < tl->size; ++k) {
					s[j++] += tl->list[k];
				}
				delete tl;
			}
		}
	}
	del(gid_intermediate);
	del(gid_intermediate_cnt);
	del(gid_intermediate_displ);
	delete gid2tarlist;
			
	// on the source gid rank
	rcnt = newintval(0, nhost);
	for (int rank=0; rank < nhost; ++rank) {
		int b = gid_source_displ[rank];
		int e = gid_source_displ[rank+1];
		for (int i=b; i < e; ++i) {
			rcnt[rank] += tlsize_source[i];
		}
	}
	rdispl = newoffset(rcnt, nhost);
	r = newintval(0, rdispl[nhost]);

	nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
	alltoalldebug("target lists", s, scnt, sdispl, r, rcnt, rdispl);
	del(s);
	del(scnt);
	del(sdispl);
	j=0;
	max_ntarget_host = 0;
	for (int rank=0; rank < nhost; ++rank) {
		int b = gid_source_displ[rank];
		int e = gid_source_displ[rank+1];
		for (int i=b; i < e; ++i) {
			PreSyn* ps;
			assert(gid2out_->find(gid_source[i], ps));
			BGP_DMASend* s =  ps->bgp.dma_send_;
			s->ntarget_hosts_ = tlsize_source[i];
			if (max_ntarget_host < s->ntarget_hosts_) {
				max_ntarget_host = s->ntarget_hosts_;
			}
			s->target_hosts_ = newintval(0, s->ntarget_hosts_);;
			for (int k = 0; k < tlsize_source[i]; ++k) {
				s->target_hosts_[k] = r[j++];
				PreSyn* ps1;
				assert(gid2out_->find(s->target_hosts_[k], ps1) == 0);
			}
		}
	}
	del(r);
	del(rcnt);
	del(rdispl);
	del(tlsize_source);
	del(gid_source);
	del(gid_source_cnt);
	del(gid_source_displ);
#endif // obsolete
}
