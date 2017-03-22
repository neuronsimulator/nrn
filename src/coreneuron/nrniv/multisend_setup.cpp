#include <math.h>
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/multisend.h"
#include "coreneuron/nrnmpi/nrnmpidec.h"
#include "coreneuron/utils/memory_utils.h"
/*
For very large numbers of processors and cells and fanout, it is taking
a long time to figure out each cells target list given the input gids
(gid2in) on each host. e.g 240 seconds for 2^25 cells, 1k connections
per cell, and 128K cores; and 340 seconds for two phase excchange.
To reduce this setup time we experiment with a very different algorithm in which
we construct a gid target host list on host gid%nhost and copy that list to
the source host owning the gid.
*/

typedef std::map<int, InputPreSyn*> Gid2IPS;
typedef std::map<int, PreSyn*> Gid2PS;

#if 0
template <typename T>
static void celldebug(const char* p, T& map) {
    FILE* f;
    char fname[100];
    sprintf(fname, "debug.%d", nrnmpi_myid);
    f = fopen(fname, "a");
    fprintf(f, "\n%s\n", p);
    int rank = nrnmpi_myid;
    fprintf(f, "  %2d:", rank);
    for (typename T::iterator map_it = map.begin(); map_it != map.end(); ++map_it) {
        int gid = map_it->first;
        fprintf(f, " %2d", gid);
    }
    fprintf(f, "\n");
    fclose(f);
}

static void
alltoalldebug(const char* p, int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl) {
    FILE* f;
    char fname[100];
    sprintf(fname, "debug.%d", nrnmpi_myid);
    f = fopen(fname, "a");
    fprintf(f, "\n%s\n", p);
    int rank = nrnmpi_myid;
    fprintf(f, "  rank %d\n", rank);
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        fprintf(f, "    s%d : %d %d :", i, scnt[i], sdispl[i]);
        for (int j = sdispl[i]; j < sdispl[i + 1]; ++j) {
            fprintf(f, " %2d", s[j]);
        }
        fprintf(f, "\n");
    }
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        fprintf(f, "    r%d : %d %d :", i, rcnt[i], rdispl[i]);
        for (int j = rdispl[i]; j < rdispl[i + 1]; ++j) {
            fprintf(f, " %2d", r[j]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}
#else
template <typename T>
static void celldebug(const char*, T&) {
}
static void alltoalldebug(const char*, int*, int*, int*, int*, int*, int*) {
}
#endif

#if 0
void phase1debug(int* targets_phase1) {
    FILE* f;
    char fname[100];
    sprintf(fname, "debug.%d", nrnmpi_myid);
    f = fopen(fname, "a");
    fprintf(f, "\nphase1debug %d", nrnmpi_myid);
    for (Gid2PS::iterator gid2out_it = gid2out.begin(); gid2out_it != gid2out.end(); ++gid2out_it) {
        PreSyn* ps = gid2out_it->second;
        fprintf(f, "\n %2d:", ps->gid_);
        int* ranks = targets_phase1 + ps->multisend_index_;
        int n = ranks[1];
        ranks += 2;
        for (int i = 0; i < n; ++i) {
            fprintf(f, " %2d", ranks[i]);
        }
    }
    fprintf(f, "\n");
    fclose(f);
}

void phase2debug(int* targets_phase2) {
    FILE* f;
    char fname[100];
    sprintf(fname, "debug.%d", nrnmpi_myid);
    f = fopen(fname, "a");
    fprintf(f, "\nphase2debug %d", nrnmpi_myid);
    for (Gid2IPS::iterator gid2in_it = gid2in.begin(); gid2in_it != gid2in.end(); ++gid2in_it) {
        int gid = gid2in_it->first;
        InputPreSyn* ps = gid2in_it->second;
        fprintf(f, "\n %2d:", gid);
        int j = ps->multisend_phase2_index_;
        if (j >= 0) {
            int* ranks = targets_phase2 + j;
            int cnt = ranks[0];
            ranks += 1;
            for (int i = 0; i < cnt; ++i) {
                fprintf(f, " %2d", ranks[i]);
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
static void all2allv_helper(int* scnt, int*& rcnt, int*& rdispl) {
    int np = nrnmpi_numprocs;
    int* c = newintval(1, np);
    rdispl = newoffset(c, np);
    rcnt = newintval(0, np);
    nrnmpi_int_alltoallv(scnt, c, rdispl, rcnt, c, rdispl);
    del(c);
    del(rdispl);
    rdispl = newoffset(rcnt, np);
}

#define all2allv_perf 1
// input s, scnt, sdispl ; output, newly allocated r, rcnt, rdispl
static void
all2allv_int(int* s, int* scnt, int* sdispl, int*& r, int*& rcnt, int*& rdispl, const char* dmes) {
#if all2allv_perf
    double tm = nrn_wtime();
#endif
    int np = nrnmpi_numprocs;
    all2allv_helper(scnt, rcnt, rdispl);
    r = newintval(0, rdispl[np]);

    nrnmpi_int_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
    alltoalldebug(dmes, s, scnt, sdispl, r, rcnt, rdispl);

// when finished with r, rcnt, rdispl, caller should del them.
#if all2allv_perf
    if (nrnmpi_myid == 0) {
        int nb = 4 * nrnmpi_numprocs + sdispl[nrnmpi_numprocs] + rdispl[nrnmpi_numprocs];
        tm = nrn_wtime() - tm;
        printf("all2allv_int %s space=%d total=%g time=%g\n", dmes, nb, nrn_mallinfo(), tm);
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

typedef std::map<int, TarList*> Int2TarList;
static Int2TarList* gid2tarlist;

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

// for two phase

#include <coreneuron/utils/randoms/nrnran123.h>
static nrnran123_State* ranstate;

static void random_init(int i) {
    if (!ranstate) {
        ranstate = nrnran123_newstream(i, 0);
    }
}

static unsigned int get_random() {
    return nrnran123_ipick(ranstate);
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

// end of twophase

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

static int setup_target_lists(int, int**);
static void fill_multisend_lists(int, int, int*, int*&, int*&);

void nrn_multisend_setup_targets(int use_phase2, int*& targets_phase1, int*& targets_phase2) {
    int* r;
    int sz = setup_target_lists(use_phase2, &r);

    // initialize as unused
    for (Gid2PS::iterator gid2out_it = gid2out.begin(); gid2out_it != gid2out.end(); ++gid2out_it) {
        PreSyn* ps = gid2out_it->second;
        ps->multisend_index_ = -1;
    }

    // Only will be not -1 if non-NULL input is a phase 2 sender.
    for (Gid2IPS::iterator gid2in_it = gid2in.begin(); gid2in_it != gid2in.end(); ++gid2in_it) {
        InputPreSyn* ps = gid2in_it->second;
        ps->multisend_phase2_index_ = -1;
    }

    fill_multisend_lists(use_phase2, sz, r, targets_phase1, targets_phase2);
    del(r);

    // phase1debug(targets_phase1);
    // phase2debug(targets_phase2);
}

// Some notes about threads and the rank lists.
// Assume all MPI message sent and received from a single thread (0).
// gid2in and gid2out are rank wide lists for all threads
//
static void fill_multisend_lists(int use_phase2,
                                 int sz,
                                 int* r,
                                 int*& targets_phase1,
                                 int*& targets_phase2) {
    // sequence of gid, size, [totalsize], list
    // Note that totalsize is there only for output gid's and use_phase2.
    // Using this sequence, copy lists to proper phase
    // 1 and phase 2 lists. (Phase one lists found in gid2out_ and phase
    // two lists found in gid2in_.
    int phase1_index = 0;
    int phase2_index = 0;
    // Count and fill in multisend_index and multisend_phase2_index_
    // From the counts can allocate targets_phase1 and targets_phase2
    // Then can iterate again and copy r to proper target locations.
    for (int i = 0; i < sz;) {
        InputPreSyn* ips = NULL;
        int gid = r[i++];
        int size = r[i++];
        if (use_phase2) {  // look in gid2in first
            Gid2IPS::iterator gid2in_it;
            gid2in_it = gid2in.find(gid);
            if (gid2in_it != gid2in.end()) {  // phase 2 target list
                ips = gid2in_it->second;
                ips->multisend_phase2_index_ = phase2_index;
                phase2_index += 1 + size;  // count + ranks
                for (int j = 0; j < size; ++j) {
                    i++;
                }
            }
        }
        if (!ips) {  // phase 1 target list (or whole list if use_phase2 is 0)
            Gid2PS::iterator gid2out_it;
            gid2out_it = gid2out.find(gid);
            assert(gid2out_it != gid2out.end());
            PreSyn* ps = gid2out_it->second;
            ps->multisend_index_ = phase1_index;
            phase1_index += 2 + size;  // total + count + ranks
            if (use_phase2 > 0) {
                i++;
            }
            for (int j = 0; j < size; ++j) {
                i++;
            }
        }
    }

    targets_phase1 = new int[phase1_index];
    targets_phase2 = new int[phase2_index];

    // printf("%d sz=%d\n", nrnmpi_myid, sz);
    for (int i = 0; i < sz;) {
        InputPreSyn* ips = NULL;
        int gid = r[i++];
        int size = r[i++];
        if (use_phase2) {  // look in gid2in first
            Gid2IPS::iterator gid2in_it;
            gid2in_it = gid2in.find(gid);
            if (gid2in_it != gid2in.end()) {  // phase 2 target list
                ips = gid2in_it->second;
                int p = ips->multisend_phase2_index_;
                int* ranks = targets_phase2 + p;
                ranks[0] = size;
                ranks += 1;
                // printf("%d i=%d gid=%d phase2 size=%d\n", nrnmpi_myid, i, gid, size);
                for (int j = 0; j < size; ++j) {
                    ranks[j] = r[i++];
                    // printf("%d   j=%d rank=%d\n", nrnmpi_myid, j, ranks[j]);
                    assert(ranks[j] != nrnmpi_myid);
                }
            }
        }
        if (!ips) {  // phase 1 target list (or whole list if use_phase2 is 0)
            Gid2PS::iterator gid2out_it;
            gid2out_it = gid2out.find(gid);
            assert(gid2out_it != gid2out.end());
            PreSyn* ps = gid2out_it->second;
            int p = ps->multisend_index_;
            int* ranks = targets_phase1 + p;
            int total = size;
            if (use_phase2 > 0) {
                total = r[i++];
            }
            ranks[0] = total;
            ranks[1] = size;
            ranks += 2;
            // printf("%d i=%d gid=%d phase1 size=%d total=%d\n", nrnmpi_myid, i, gid, size, total);
            for (int j = 0; j < size; ++j) {
                ranks[j] = r[i++];
                // printf("%d   j=%d rank=%d\n", nrnmpi_myid, j, ranks[j]);
                // There never was a possibility of send2self
                // because an output presyn is never in gid2in_.
                assert(ranks[j] != nrnmpi_myid);
            }
        }
    }

    // compute max_ntarget_host and max_multisend_targets
    int max_ntarget_host = 0;
    int max_multisend_targets = 0;
    for (Gid2PS::iterator gid2out_it = gid2out.begin(); gid2out_it != gid2out.end(); ++gid2out_it) {
        PreSyn* ps = gid2out_it->second;
        if (ps->output_index_ >= 0) {  // only ones that generate spikes
            int i = ps->multisend_index_;
            if (max_ntarget_host < targets_phase1[i]) {
                max_ntarget_host = targets_phase1[i];
            }
            if (max_multisend_targets < targets_phase1[i + 1]) {
                max_multisend_targets = targets_phase1[i + 1];
            }
        }
    }
    if (use_phase2)
        for (Gid2IPS::iterator gid2in_it = gid2in.begin(); gid2in_it != gid2in.end(); ++gid2in_it) {
            InputPreSyn* ps = gid2in_it->second;
            int i = ps->multisend_phase2_index_;
            if (i >= 0 && max_multisend_targets < targets_phase2[i]) {
                max_multisend_targets = targets_phase2[i];
            }
        }
}

// return is vector and its size. The vector encodes a sequence of
// gid, target list size, and target list
static int setup_target_lists(int use_phase2, int** r_return) {
    int *s, *r, *scnt, *rcnt, *sdispl, *rdispl;
    int nhost = nrnmpi_numprocs;

    celldebug<Gid2PS>("output gid", gid2out);
    celldebug<Gid2IPS>("input gid", gid2in);

    // What are the target ranks for a given input gid. All the ranks
    // with the same input gid send that gid to the intermediate
    // gid%nhost rank. The intermediate rank can then construct the
    // list of target ranks for the gids it gets.

    // scnt is number of input gids from target
    scnt = newintval(0, nhost);
    for (Gid2IPS::iterator gid2in_it = gid2in.begin(); gid2in_it != gid2in.end(); ++gid2in_it) {
        int gid = gid2in_it->first;
        ++scnt[gid % nhost];
    }

    // s are the input gids from target to be sent to the various intermediates
    sdispl = newoffset(scnt, nhost);
    s = newintval(0, sdispl[nhost]);
    for (Gid2IPS::iterator gid2in_it = gid2in.begin(); gid2in_it != gid2in.end(); ++gid2in_it) {
        int gid = gid2in_it->first;
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
    gid2tarlist = new Int2TarList;
    // Now figure out the size of the target list for each distinct gid in r.
    for (int i = 0; i < rdispl[nhost]; ++i) {
        TarList* tl;
        Int2TarList::iterator itl_it = gid2tarlist->find(r[i]);
        if (itl_it != gid2tarlist->end()) {
            tl = itl_it->second;
            tl->size += 1;
        } else {
            tl = new TarList();
            tl->size = 1;
            (*gid2tarlist)[r[i]] = tl;
        }
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
    for (Int2TarList::iterator itl_it = gid2tarlist->begin(); itl_it != gid2tarlist->end();
         ++itl_it) {
        TarList* tl = itl_it->second;
        tl->alloc();
        tl->size = 0;
    }

    // fill the target lists
    for (int rank = 0; rank < nhost; ++rank) {
        int b = rdispl[rank];
        int e = rdispl[rank + 1];
        for (int i = b; i < e; ++i) {
            TarList* tl;
            Int2TarList::iterator itl_it = gid2tarlist->find(r[i]);
            if (itl_it != gid2tarlist->end()) {
                tl = itl_it->second;
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
    for (Gid2PS::iterator gid2ps_it = gid2out.begin(); gid2ps_it != gid2out.end(); ++gid2ps_it) {
        int gid = gid2ps_it->first;
        PreSyn* ps = gid2ps_it->second;
        if (ps->output_index_ >= 0) {  // only ones that generate spikes
            ++scnt[gid % nhost];
        }
    }
    sdispl = newoffset(scnt, nhost);

    // what are the gids of those target lists
    s = newintval(0, sdispl[nhost]);
    for (Gid2PS::iterator gid2ps_it = gid2out.begin(); gid2ps_it != gid2out.end(); ++gid2ps_it) {
        int gid = gid2ps_it->first;
        PreSyn* ps = gid2ps_it->second;
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
            TarList* tl;
            // note that there may be input gids with no corresponding
            // output gid so that the find may not return true and in
            // that case the tl->rank remains -1.
            // For example multisplit gids or simulation of a subset of
            // cells.
            Int2TarList::iterator itl_it = gid2tarlist->find(r[i]);
            if (itl_it != gid2tarlist->end()) {
                tl = itl_it->second;
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

    if (use_phase2) {
        random_init(nrnmpi_myid + 1);
        for (Int2TarList::iterator itl_it = gid2tarlist->begin(); itl_it != gid2tarlist->end();
             ++itl_it) {
            TarList* tl = itl_it->second;
            if (tl->rank >= 0) {  // only if output gid is spike generating
                phase2organize(tl);
            }
        }
    }

    // For clarity, use the all2allv_int style of information flow
    // from source to destination as above
    // and also use a uniform code
    // for copying one and two phase information from a TarList to
    // develop the s, scnt, and sdispl buffers. That is, a buffer list
    // section in s for either a one-phase list or the much shorter
    // (individually) lists for first and second phases, has a
    // gid, size, totalsize header for each list where totalsize
    // is only present if the gid is an output gid (for
    // NrnMultisend_Send.ntarget_host used for conservation).
    // Note that totalsize is tl->indices[tl->size]

    // how much to send to each rank
    scnt = newintval(0, nhost);
    for (Int2TarList::iterator itl_it = gid2tarlist->begin(); itl_it != gid2tarlist->end();
         ++itl_it) {
        TarList* tl = itl_it->second;
        if (tl->rank < 0) {
            // When the output gid does not generate spikes, that rank
            // is not interested if there is a target list for it.
            // If the output gid does not exist, there is no rank.
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
        if (use_phase2) {
            // The phase 1 header has as its third element, the
            // total list size (needed for conservation);
            scnt[tl->rank] += 1;
        }
    }
    sdispl = newoffset(scnt, nhost);
    s = newintval(0, sdispl[nhost]);
    // what to send to each rank
    for (Int2TarList::iterator itl_it = gid2tarlist->begin(); itl_it != gid2tarlist->end();
         ++itl_it) {
        int gid = itl_it->first;
        TarList* tl = itl_it->second;
        if (tl->rank < 0) {
            continue;
        }
        if (tl->indices) {
            s[sdispl[tl->rank]++] = gid;
            s[sdispl[tl->rank]++] = tl->size;
            if (use_phase2) {
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
            if (use_phase2) {
                s[sdispl[tl->rank]++] = tl->size;
            }
            for (int i = 0; i < tl->size; ++i) {
                s[sdispl[tl->rank]++] = tl->list[i];
            }
        }
        delete tl;
    }
    delete gid2tarlist;
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
