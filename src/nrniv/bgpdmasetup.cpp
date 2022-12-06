/*
For very large numbers of processors and cells and fanout, it is taking
a long time to figure out each cells target list given the input gids
(gid2in) on each host. e.g 240 seconds for 2^25 cells, 1k connections
per cell, and 128K cores; and 340 seconds for two phase excchange.
To reduce this setup time we experiment with a very different algorithm in which
we construct a gid target host list on host gid%nhost and copy that list to
the source host owning the gid.
*/
#include "oc_ansi.h"

#include "oc_ansi.h"

#if 0
void celldebug(const char* p, Gid2PreSyn& map) {
	FILE* f;
	char fname[100];
	Sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	fprintf(f, "\n%s\n", p);
	int rank = nrnmpi_myid;
	fprintf(f, "  %2d:", rank);
	for (const auto& iter: map) {
		int gid = iter.first;
		fprintf(f, " %2d", gid);
	}
	fprintf(f, "\n");
	fclose(f);
}

void alltoalldebug(const char* p, int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl){
	FILE* f;
	char fname[100];
	Sprintf(fname, "debug.%d", nrnmpi_myid);
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
void celldebug(const char* p, Gid2PreSyn& map) {}
void alltoalldebug(const char* p, int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl) {}
#endif

#if 0
void phase1debug() {
	FILE* f;
	char fname[100];
	Sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	fprintf(f, "\nphase1debug %d", nrnmpi_myid);
	for (const auto* iter: gid2out_) {
		PreSyn* ps = iter.second;
		fprintf(f, "\n %2d:", ps->gid_);
		BGP_DMASend* bs = ps->bgp.dma_send_;
		for (int i=0; i < bs->ntarget_hosts_; ++i) {
			fprintf(f, " %2d", bs->target_hosts_[i]);
		}
	}
	fprintf(f, "\n");
	fclose(f);
}

void phase2debug() {
	FILE* f;
	char fname[100];
	Sprintf(fname, "debug.%d", nrnmpi_myid);
	f = fopen(fname, "a");
	fprintf(f, "\nphase2debug %d", nrnmpi_myid);
	for (const auto& iter: gid2in_) {
		PreSyn* ps = iter.second;
		fprintf(f, "\n %2d:", ps->gid_);
		BGP_DMASend_Phase2* bs = ps->bgp.dma_send_phase2_;
	    if (bs) {
		for (int i=0; i < bs->ntarget_hosts_phase2_; ++i) {
			fprintf(f, " %2d", bs->target_hosts_phase2_[i]);
		}
	    }
	}
	fprintf(f, "\n");
	fclose(f);
}
#endif

static void del(int* a) {
    if (a) {
        delete[] a;
    }
}

static int* newintval(int val, int size) {
    if (size == 0) {
        return 0;
    }
    int* x = new int[size];
    for (int i = 0; i < size; ++i) {
        x[i] = val;
    }
    return x;
}

static int* newoffset(int* acnt, int size) {
    int* aoff = new int[size + 1];
    aoff[0] = 0;
    for (int i = 0; i < size; ++i) {
        aoff[i + 1] = aoff[i] + acnt[i];
    }
    return aoff;
}


// input scnt, sdispl ; output, newly allocated rcnt, rdispl
static void all2allv_helper(int* scnt, int* sdispl, int*& rcnt, int*& rdispl) {
    int np = nrnmpi_numprocs;
    int* c = newintval(1, np);
    rdispl = newoffset(c, np);
    rcnt = newintval(0, np);
    nrnmpi_int_alltoallv(scnt, c, rdispl, rcnt, c, rdispl);
    del(c);
    del(rdispl);
    rdispl = newoffset(rcnt, np);
}

/*
define following to 1 if desire space/performance information such as:
all2allv_int gidin to intermediate space=1552 total=37345104 time=0.000495835
all2allv_int gidout space=528 total=37379376 time=1.641e-05
all2allv_int lists space=3088 total=37351312 time=4.4708e-05
*/
#define all2allv_perf 0
// input s, scnt, sdispl ; output, newly allocated r, rcnt, rdispl
static void
all2allv_int(int* s, int* scnt, int* sdispl, int*& r, int*& rcnt, int*& rdispl, const char* dmes) {
#if all2allv_perf
    double tm = nrnmpi_wtime();
#endif
    int np = nrnmpi_numprocs;
    all2allv_helper(scnt, sdispl, rcnt, rdispl);
    r = newintval(0, rdispl[np]);

    nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
    alltoalldebug(dmes, s, scnt, sdispl, r, rcnt, rdispl);

    // when finished with r, rcnt, rdispl, caller should del them.
#if all2allv_perf
    if (nrnmpi_myid == 0) {
        int nb = 4 * nrnmpi_numprocs + sdispl[nrnmpi_numprocs] + rdispl[nrnmpi_numprocs];
        tm = nrnmpi_wtime() - tm;
        printf("all2allv_int %s space=%d total=%llu time=%g\n", dmes, nb, nrn_mallinfo(0), tm);
    }
#endif
}

class TarList {
  public:
    TarList();
    virtual ~TarList();
    virtual void alloc();
    int size;
    int* list;
    int rank;
    int* indices;  // indices of list for groups of phase2 targets.
                   // If indices is not null, then size is one less than
                   // the size of the indices list where indices[size] = the size of
                   // the list. Indices[0] is 0 and list[indices[i]] is the rank
                   // to send the ith group of phase2 targets.
};

using Int2TarList = std::unordered_map<int, std::unique_ptr<TarList>>;

TarList::TarList() {
    size = 0;
    list = 0;
    rank = -1;
    indices = 0;
}
TarList::~TarList() {
    del(list);
    del(indices);
}

void TarList::alloc() {
    if (size) {
        list = new int[size];
    }
}

#include <nrnisaac.h>
static void* ranstate;

static void random_init(int i) {
    if (!ranstate) {
        ranstate = nrnisaac_new();
    }
    nrnisaac_init(ranstate, nrnmpi_myid + 1);
}

static unsigned int get_random() {
    return nrnisaac_uint32_pick(ranstate);
}

static int iran(int i1, int i2) {
    // discrete uniform random integer from i2 to i2 inclusive. Must
    // work if i1 == i2
    if (i1 == i2) {
        return i1;
    }
    int i3 = i1 + get_random() % (i2 - i1 + 1);
    return i3;
}

static void phase2organize(TarList* tl) {
    // copied and modified from old specify_phase2_distribution of bgpdma.cpp
    int n, nt;
    nt = tl->size;
    n = int(sqrt(double(nt)));
    // change to about 20
    if (n > 1) {  // do not bother if not many connections
        // equal as possible group sizes
        tl->indices = new int[n + 1];
        tl->indices[n] = tl->size;
        tl->size = n;
        for (int i = 0; i < n; ++i) {
            tl->indices[i] = (i * nt) / n;
        }
        // Note: not sure the following is true anymore but it could be.
        // This distribution is very biased (if 0 is a phase1 target
        // it is always a phase2 sender. So now choose a random
        // target in the subset and make that the phase2 sender
        // (need to switch the indices[i] target and the one chosen)
        for (int i = 0; i < n; ++i) {
            int i1 = tl->indices[i];
            int i2 = tl->indices[i + 1] - 1;
            // need discrete uniform random integer from i1 to i2
            int i3 = iran(i1, i2);
            int itar = tl->list[i1];
            tl->list[i1] = tl->list[i3];
            tl->list[i3] = itar;
        }
    }
}

/*
Setting up target lists uses a lot of temporary memory. It is conceiveable
that this can be done prior to creating any cells or connections. I.e.
gid2out is presently known from pc.set_gid2node(gid,...). Gid2in is presenly
known from NetCon = pc.gid_connect(gid, target) and it is quite a style
and hoc network programming change to use something like pc.need_gid(gid)
before cells with their synapses are created since one would have to imagine
that the hoc network setup code would have to be executed in a virtual
or 'abstract' fashion without actually creating, cells, targets, or NetCons.
Anyway, to potentially support this in the future, we write setup_target_lists
to not use any PreSyn information.
*/

static int setup_target_lists(int**);
static void fill_dma_send_lists(int, int*);

static void setup_presyn_dma_lists() {
    // Create and attach BGP_DMASend instances to output Presyn
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        // only ones that generate spikes. eg. multisplit
        // registers a gid and even associates with a cell piece, but
        // that piece may not send spikes.
        if (ps->output_index_ >= 0) {
            bgpdma_cleanup_presyn(ps);
            ps->bgp.dma_send_ = new BGP_DMASend();
        }
    }

    // Need to use the bgp union slot for dma_send_phase2_.
    // Only will be non-NULL if the input is a phase 2 sender.
    for (const auto& iter: gid2in_) {
        PreSyn* ps = iter.second;
        ps->bgp.srchost_ = 0;
    }

    int* r;
    int sz = setup_target_lists(&r);
    fill_dma_send_lists(sz, r);
    del(r);

    //	phase1debug();
    //	phase2debug();
}

static void fill_dma_send_lists(int sz, int* r) {
    // sequence of gid, size, [totalsize], list
    // Note that totalsize is there only for output gid's and use_phase2_.
    // Using this sequence, copy lists to proper phase
    // 1 and phase 2 lists. (Phase one lists found in gid2out_ and phase
    // two lists found in gid2in_.
    for (int i = 0; i < sz;) {
        int gid = r[i++];
        int size = r[i++];
        PreSyn* ps{nullptr};
        if (use_phase2_) {  // look in gid2in first
            auto iter = gid2in_.find(gid);
            if (iter != gid2in_.end()) {
                ps = iter->second;
                BGP_DMASend_Phase2* bsp = new BGP_DMASend_Phase2();
                ps->bgp.dma_send_phase2_ = bsp;
                bsp->ntarget_hosts_phase2_ = size;
                int* p = newintval(0, size);
                bsp->target_hosts_phase2_ = p;
                // printf("%d %d phase2 size=%d\n", nrnmpi_myid, gid, bsp->ntarget_hosts_phase2_);
                for (int j = 0; j < size; ++j) {
                    p[j] = r[i++];
                    assert(p[j] != nrnmpi_myid);
                }
            }
        }
        if (!ps) {  // phase 1 target list (or whole list if use_phase2 is 0)
            auto iter = gid2out_.find(gid);
            nrn_assert(iter != gid2out_.end());
            ps = iter->second;
            BGP_DMASend* bs = ps->bgp.dma_send_;
            bs->ntarget_hosts_phase1_ = size;
            if (use_phase2_ == 0) {
                bs->ntarget_hosts_ = size;
            } else {
                bs->ntarget_hosts_ = r[i++];
            }
            int* p = newintval(0, size);
            bs->target_hosts_ = p;
            // printf("%d %d phase1 size=%d\n", nrnmpi_myid, gid, bs->ntarget_hosts_);
            for (int j = 0; j < size; ++j) {
                p[j] = r[i++];
                // There never was a possibility of send2self
                // because an output presyn is never in gid2in_.
                assert(p[j] != nrnmpi_myid);
            }
        }
    }
    // compute max_ntarget_host and max_multisend_targets
    max_ntarget_host = 0;
    max_multisend_targets = 0;
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        if (ps->output_index_ >= 0) {  // only ones that generate spikes
            BGP_DMASend* bs = ps->bgp.dma_send_;
            if (max_ntarget_host < bs->ntarget_hosts_) {
                max_ntarget_host = bs->ntarget_hosts_;
            }
            if (max_multisend_targets < bs->NTARGET_HOSTS_PHASE1) {
                max_multisend_targets = bs->NTARGET_HOSTS_PHASE1;
            }
        }
    }
    if (use_phase2_)
        for (const auto& iter: gid2in_) {
            PreSyn* ps = iter.second;
            BGP_DMASend_Phase2* bsp = ps->bgp.dma_send_phase2_;
            if (bsp && max_multisend_targets < bsp->ntarget_hosts_phase2_) {
                max_multisend_targets = bsp->ntarget_hosts_phase2_;
            }
        }
}

// return is vector and its size. The vector encodes a sequence of
// gid, target list size, and target list
static int setup_target_lists(int** r_return) {
    int *s, *r, *scnt, *rcnt, *sdispl, *rdispl;
    int nhost = nrnmpi_numprocs;

    celldebug("output gid", gid2out_);
    celldebug("input gid", gid2in_);

    // What are the target ranks for a given input gid. All the ranks
    // with the same input gid send that gid to the intermediate
    // gid%nhost rank. The intermediate rank can then construct the
    // list of target ranks for the gids it gets.

    // scnt is number of input gids from target
    scnt = newintval(0, nhost);
    for (const auto& iter: gid2in_) {
        int gid = iter.first;
        ++scnt[gid % nhost];
    }

    // s are the input gids from target to be sent to the various intermediates
    sdispl = newoffset(scnt, nhost);
    s = newintval(0, sdispl[nhost]);
    for (const auto& iter: gid2in_) {
        int gid = iter.first;
        s[sdispl[gid % nhost]++] = gid;
    }
    // Restore sdispl for the message.
    del(sdispl);
    sdispl = newoffset(scnt, nhost);

    all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "gidin to intermediate");
    del(s);
    del(scnt);
    del(sdispl);
    // r is the gids received by this intermediate rank from all other ranks.

    // Construct hash table for finding the target rank list for a given gid.
    Int2TarList gid2tarlist;
    // Now figure out the size of the target list for each distinct gid in r.
    for (int i = 0; i < rdispl[nhost]; ++i) {
        const int gid = r[i];
        auto& tl = gid2tarlist[gid];  // default-construct a new std::unique_ptr<TarList> if needed
        if (!tl) {
            tl.reset(new TarList());  // constructor initialises `size` to zero
        }
        ++(tl->size);
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
    for (auto& iter: gid2tarlist) {
        TarList* tl = iter.second.get();
        tl->alloc();
        tl->size = 0;
    }

    // fill the target lists
    for (int rank = 0; rank < nhost; ++rank) {
        int b = rdispl[rank];
        int e = rdispl[rank + 1];
        for (int i = b; i < e; ++i) {
            const auto iter = gid2tarlist.find(r[i]);
            if (iter != gid2tarlist.end()) {
                TarList* tl = iter->second.get();
                tl->list[tl->size] = rank;
                tl->size++;
            }
        }
    }
    del(r);
    del(rcnt);
    del(rdispl);

    // Now the intermediate hosts have complete target lists and
    // the sources know the intermediate host from the gid2out_ map.
    // We could potentially organize here for two-phase exchange as well.

    // Which target lists are desired by the source rank?

    // Ironically, for round robin distributions, the target lists are
    // already on the proper source rank so the following code should
    // be tested for random distributions of gids.
    // How many on the source rank?
    scnt = newintval(0, nhost);
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        int gid = iter.first;
        if (ps->output_index_ >= 0) {  // only ones that generate spikes
            ++scnt[gid % nhost];
        }
    }
    sdispl = newoffset(scnt, nhost);

    // what are the gids of those target lists
    s = newintval(0, sdispl[nhost]);
    for (const auto& iter: gid2out_) {
        PreSyn* ps = iter.second;
        int gid = iter.first;
        if (ps->output_index_ >= 0) {  // only ones that generate spikes
            s[sdispl[gid % nhost]++] = gid;
        }
    }
    // Restore sdispl for the message.
    del(sdispl);
    sdispl = newoffset(scnt, nhost);
    all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "gidout");

    // fill in the tl->rank for phase 1 target lists
    // r is an array of source spiking gids
    // tl is list associating input gids with list of target ranks.
    for (int rank = 0; rank < nhost; ++rank) {
        int b = rdispl[rank];
        int e = rdispl[rank + 1];
        for (int i = b; i < e; ++i) {
            // note that there may be input gids with no corresponding
            // output gid so that the find may not return true and in
            // that case the tl->rank remains -1.
            // For example multisplit gids or simulation of a subset of
            // cells.
            const auto iter = gid2tarlist.find(r[i]);
            if (iter != gid2tarlist.end()) {
                TarList* tl = iter->second.get();
                tl->rank = rank;
            }
        }
    }
    del(s);
    del(scnt);
    del(sdispl);
    del(r);
    del(rcnt);
    del(rdispl);

    if (use_phase2_) {
        random_init(nrnmpi_myid + 1);
        for (const auto& iter: gid2tarlist) {
            TarList* tl = iter.second.get();
            if (tl->rank >= 0) {  // only if output gid is spike generating
                phase2organize(tl);
            }
        }
    }

    // For clarity, use the all2allv_int style of information flow
    // from source to destination as above (instead of the obsolete
    // section which was difficult to understand (It was last present at
    // 672:544c61a730ec))
    // and also use a uniform code
    // for copying one and two phase information from a TarList to
    // develop the s, scnt, and sdispl buffers. That is, a buffer list
    // section in s for either a one-phase list or the much shorter
    // (individually) lists for first and second phases, has a
    // gid, size, totalsize header for each list where totalsize
    // is only present if the gid is an output gid (for
    // BGP_DMASend.ntarget_host used for conservation).
    // Note that totalsize is tl->indices[tl->size]

    // how much to send to each rank
    scnt = newintval(0, nhost);
    for (const auto& iter: gid2tarlist) {
        TarList* tl = iter.second.get();
        if (tl->rank < 0) {
            // When the output gid does not generate spikes, that rank
            // is not interested if there is a target list for it.
            // If the output gid dies not exist, there is no rank.
            // In either case ignore this target list.
            continue;
        }
        if (tl->indices) {
            // indices[size] is the size of list but size of those
            // are the sublist phase 2 destination ranks which
            // don't get sent as part of the phase 2 target list.
            // Also there is a phase 1 target list of size so there
            // are altogether size+1 target lists.
            // (one phase 1 list and size phase 2 lists)
            scnt[tl->rank] += tl->size + 2;  // gid, size, list
            for (int i = 0; i < tl->size; ++i) {
                scnt[tl->list[tl->indices[i]]] += tl->indices[i + 1] - tl->indices[i] + 1;
                // gid, size, list
            }
        } else {
            // gid, list size, list
            scnt[tl->rank] += tl->size + 2;
        }
        if (use_phase2_) {
            // The phase 1 header has as its third element, the
            // total list size (needed for conservation);
            scnt[tl->rank] += 1;
        }
    }
    sdispl = newoffset(scnt, nhost);
    s = newintval(0, sdispl[nhost]);
    // what to send to each rank
    for (const auto& iter: gid2tarlist) {
        int gid = iter.first;
        TarList* tl = iter.second.get();
        if (tl->rank < 0) {
            continue;
        }
        if (tl->indices) {
            s[sdispl[tl->rank]++] = gid;
            s[sdispl[tl->rank]++] = tl->size;
            if (use_phase2_) {
                s[sdispl[tl->rank]++] = tl->indices[tl->size];
            }
            for (int i = 0; i < tl->size; ++i) {
                s[sdispl[tl->rank]++] = tl->list[tl->indices[i]];
            }
            for (int i = 0; i < tl->size; ++i) {
                int rank = tl->list[tl->indices[i]];
                s[sdispl[rank]++] = gid;
                assert(tl->indices[i + 1] > tl->indices[i]);
                s[sdispl[rank]++] = tl->indices[i + 1] - tl->indices[i] - 1;
                for (int j = tl->indices[i] + 1; j < tl->indices[i + 1]; ++j) {
                    s[sdispl[rank]++] = tl->list[j];
                }
            }
        } else {
            // gid, list size, list
            s[sdispl[tl->rank]++] = gid;
            s[sdispl[tl->rank]++] = tl->size;
            if (use_phase2_) {
                s[sdispl[tl->rank]++] = tl->size;
            }
            for (int i = 0; i < tl->size; ++i) {
                s[sdispl[tl->rank]++] = tl->list[i];
            }
        }
    }
    del(sdispl);
    sdispl = newoffset(scnt, nhost);
    all2allv_int(s, scnt, sdispl, r, rcnt, rdispl, "lists");
    del(s);
    del(scnt);
    del(sdispl);

    del(rcnt);
    int sz = rdispl[nhost];
    del(rdispl);
    *r_return = r;
    return sz;
}
