/*
For very large numbers of processors and cells and fanout, it is taking
a long time to figure out each cells target list given the input gids
(gid2in) on each host. e.g 240 seconds for 2^25 cells, 1k connections
per cell, and 128K cores; and 340 seconds for two phase excchange.
To reduce this setup time we experiment with very different algorithm in which
we construct a gid target host list on host gid%nhost and copy that list to
the source host owning the gid.
*/

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

class TarList {
public:
	TarList();
	virtual ~TarList();
	void alloc();
	int size;
	int* list;
	int gid;
	int rank;
};

declareNrnHash(Int2TarList, int, TarList*)
implementNrnHash(Int2TarList, int, TarList*)
static Int2TarList* gid2tarlist;

TarList::TarList() {
	size = 0;
	list = 0;
	gid = -1;
	rank = -1;
}
TarList::~TarList() {
	del(list);
}
void TarList::alloc() {
	if (size) {
		list = new int[size];
	}
}

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

	// How many items will be sent from this host to the intermediate?
	// gid%nhost rank.
	// s is number of input gids from target
	s = newintval(0, nhost);
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		++s[ps->gid_%nhost];
	}}}

	// How many items will be received by this (intermediate) host?
	scnt = newintval(1, nhost);
	sdispl = newoffset(scnt, nhost);
	// r is number of input gids received by intermediate
	r = newintval(0, nhost);
	nrnmpi_int_alltoallv(s, scnt, sdispl, r, scnt, sdispl);
	alltoalldebug("# gidin to intermediate", s, scnt, sdispl, r, scnt, sdispl);
	del(scnt);
	del(sdispl);
	scnt = s; // Number of items sent from this rank.
	rcnt = r; // Number of items received by this intermediate rank.

	// displacements for the message
	sdispl = newoffset(scnt, nhost);
	rdispl = newoffset(rcnt, nhost);

	// The alltoallv message (list of gids).
	// s are the input gids from target.	
	s = newintval(0, sdispl[nhost]);
	// r are the input gids received by intermediate.
	r = newintval(0, rdispl[nhost]);
	NrnHashIterate(Gid2PreSyn, gid2in_, PreSyn*, ps) {
		s[sdispl[ps->gid_%nhost]++] = ps->gid_;
	}}}
	// Restore sdispl for the message.
	del(sdispl);
	sdispl = newoffset(scnt, nhost);
	nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
	alltoalldebug("gidin to intermediate", s, scnt, sdispl, r, rcnt, rdispl);
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
	// This is a good place to organize for two-phase exchange as well

	// How many target lists are desired by the source rank?
	// Ironically, for round robin distributions, the target lists are
	// already on the proper source rank so the following code should
	// be tested for random distributions of gids.
	s = newintval(0, nhost);
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		++s[ps->gid_%nhost];
	}}}

	// How many items will be received by this (intermediate) host?
	// Note that if multiple target lists from the same intermediate
	// host will be sent to this source rank, the order is consistent
	// with gid2out.
	scnt = newintval(1, nhost);
	sdispl = newoffset(scnt, nhost);
	r = newintval(0, nhost);
	nrnmpi_int_alltoallv(s, scnt, sdispl, r, scnt, sdispl);
	alltoalldebug("# gidout", s, scnt, sdispl, r, scnt, sdispl);
	del(scnt);
	del(sdispl);
	scnt = s; // Number of items sent from this rank.
	rcnt = r; // Number of items received by this intermediate rank.

	// Which target lists are desired by the source rank?
	// displacements for the message
	sdispl = newoffset(scnt, nhost);
	rdispl = newoffset(rcnt, nhost);

	// The alltoallv message (list of gids).
	s = newintval(0, sdispl[nhost]);
	r = newintval(0, rdispl[nhost]);
	NrnHashIterate(Gid2PreSyn, gid2out_, PreSyn*, ps) {
		s[sdispl[ps->gid_%nhost]++] = ps->gid_;
	}}}
	// Restore sdispl for the message.
	del(sdispl);
	sdispl = newoffset(scnt, nhost);
	nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
	alltoalldebug("gidout", s, scnt, sdispl, r, rcnt, rdispl);
	
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
				if (gid2out_->find(s->target_hosts_[k], ps1)) {
					--s->ntarget_hosts_;
					s->send2self_ = 1;
				}
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
}

static void construct_target_lists() {
}

static void copy_target_lists_to_source() {
}

