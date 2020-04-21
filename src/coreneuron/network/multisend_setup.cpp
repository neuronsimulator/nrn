#include <cstdio>
#include <cmath>
#include <numeric>

#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/mpi/nrnmpidec.h"
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

#if NRN_MULTISEND
namespace coreneuron {
using Gid2IPS = std::map<int, InputPreSyn*>;
using Gid2PS = std::map<int, PreSyn*>;

#if DEBUG
template <typename T>
static void celldebug(const char* p, T& map) {
    std::string fname = std::string("debug.") + std::to_string(nrnmpi_myid);
    std::ofstream f(fname, std::ios::app);
    f << std::endl << p << std::endl;
    int rank = nrnmpi_myid;
    f << "  " << std::setw(2) << std::setfill('0') << rank << ":";
    for (const auto& m: map) {
        int gid = m.first;
        f << "  " << std::setw(2) << std::setfill('0') << gid << ":";
        fprintf(f, " %2d", gid);
    }
    f << std::endl;
}

static void
alltoalldebug(const char* p, const std::vector<int>& s, const std::vector<int>& scnt, const std::vector<int>& sdispl, const std::vector<int>& r, const std::vector<int>& rcnt, const std::vector<int>& rdispl) {
    std::string fname = std::string("debug.") + std::to_string(nrnmpi_myid);
    std::ofstream f(fname, std::ios::app);
    f << std::endl << p << std::endl;
    int rank = nrnmpi_myid;
    f << "  rank " << rank << std::endl;
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        f << "    s" << i << " : " << scnt[i] << " " << sdispl[i] << " :";
        for (int j = sdispl[i]; j < sdispl[i + 1]; ++j) {
            f << "  " << std::setw(2) << std::setfill('0') << s[j] << ":";
        }
        f << std::endl;
    }
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        f << "    r" << i << " : " << rcnt[i] << " " << rdispl[i] << " :";
        for (int j = rdispl[i]; j < rdispl[i + 1]; ++j) {
            f << "  " << std::setw(2) << std::setfill('0') << r[j] << ":";
        }
        f << std::endl;
    }
}
#else
template <typename T>
static void celldebug(const char*, T&) {
}
static void alltoalldebug(const char*, const std::vector<int>&, const std::vector<int>&, const std::vector<int>&, const std::vector<int>&, const std::vector<int>&, const std::vector<int>&) {
}
#endif

#if DEBUG
void phase1debug(int* targets_phase1) {
    std::string fname = std::string("debug.") + std::to_string(nrnmpi_myid);
    std::ofstream f(fname, std::ios::app);
    f << std::endl << "phase1debug " << nrnmpi_myid;
    for (auto& g: gid2out) {
        PreSyn* ps = g.second;
        f << std::endl <<  " " << std::setw(2) << std::setfill('0') << ps->gid_ << ":";
        int* ranks = targets_phase1 + ps->multisend_index_;
        int n = ranks[1];
        ranks += 2;
        for (int i = 0; i < n; ++i) {
            f << " " << std::setw(2) << std::setfill('0') << ranks[i];
        }
    }
    f << std::endl;
}

void phase2debug(int* targets_phase2) {
    std::string fname = std::string("debug.") + std::to_string(nrnmpi_myid);
    std::ofstream f(fname, std::ios::app);
    f << std::endl << "phase2debug " << nrnmpi_myid;
    for (auto& g: gid2in) {
        int gid = g.first;
        InputPreSyn* ps = g.second;
        f << std::endl <<  " " << std::setw(2) << std::setfill('0') << gid << ":";
        int j = ps->multisend_phase2_index_;
        if (j >= 0) {
            int* ranks = targets_phase2 + j;
            int cnt = ranks[0];
            ranks += 1;
            for (int i = 0; i < cnt; ++i) {
                f << " " << std::setw(2) << std::setfill('0') << ranks[i];
            }
        }
    }
    f << std::endl;
}
#endif

static std::vector<int> newoffset(const std::vector<int>& acnt) {
    std::vector<int> aoff(acnt.size() + 1);
    aoff[0] = 0;
    std::partial_sum(acnt.begin(), acnt.end(), aoff.begin() + 1);
    return aoff;
}

// input: scnt, sdispl; output: rcnt, rdispl
static std::pair<std::vector<int>, std::vector<int>> all2allv_helper(const std::vector<int>& scnt) {
    int np = nrnmpi_numprocs;
    std::vector<int> c(np, 1);
    std::vector<int> rdispl = newoffset(c);
    std::vector<int> rcnt(np, 0);
    nrnmpi_int_alltoallv(scnt.data(), c.data(), rdispl.data(), rcnt.data(), c.data(), rdispl.data());
    rdispl = newoffset(rcnt);
    return std::make_pair(std::move(rcnt), std::move(rdispl));
}

#define all2allv_perf 1
// input: s, scnt, sdispl; output: r, rdispl
static std::pair<std::vector<int>, std::vector<int>>
all2allv_int(const std::vector<int>& s, const std::vector<int>& scnt, const std::vector<int>& sdispl, const char* dmes) {
#if all2allv_perf
    double tm = nrn_wtime();
#endif
    int np = nrnmpi_numprocs;

    std::vector<int> rcnt;
    std::vector<int> rdispl;
    std::tie(rcnt, rdispl) = all2allv_helper(scnt);
    std::vector<int> r(rdispl[np], 0);
    nrnmpi_int_alltoallv(s.data(), scnt.data(), sdispl.data(), r.data(), rcnt.data(), rdispl.data());
    alltoalldebug(dmes, s, scnt, sdispl, r, rcnt, rdispl);

#if all2allv_perf
    if (nrnmpi_myid == 0) {
        int nb = 4 * nrnmpi_numprocs + sdispl[nrnmpi_numprocs] + rdispl[nrnmpi_numprocs];
        tm = nrn_wtime() - tm;
        printf("all2allv_int %s space=%d total=%g time=%g\n", dmes, nb, nrn_mallinfo(), tm);
    }
#endif
    return std::make_pair(std::move(r), std::move(rdispl));
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

using Int2TarList = std::map<int, TarList*>;

TarList::TarList()
  : size(0), list(nullptr), rank(-1), indices(nullptr)
{}

TarList::~TarList() {
    delete[] list;
    delete[] indices;
}

void TarList::alloc() {
    if (size) {
        list = new int[size];
    }
}

// for two phase

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
    int nt = tl->size;
    int n = int(sqrt(double(nt)));
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

static std::vector<int> setup_target_lists(bool);
static void fill_multisend_lists(bool, const std::vector<int>&, int*&, int*&);

void nrn_multisend_setup_targets(bool use_phase2, int*& targets_phase1, int*& targets_phase2) {
    auto r = setup_target_lists(use_phase2);

    // initialize as unused
    for (auto& g: gid2out) {
        PreSyn* ps = g.second;
        ps->multisend_index_ = -1;
    }

    // Only will be not -1 if non-nullptr input is a phase 2 sender.
    for (auto& g: gid2in) {
        InputPreSyn* ps = g.second;
        ps->multisend_phase2_index_ = -1;
    }

    fill_multisend_lists(use_phase2, r, targets_phase1, targets_phase2);

    // phase1debug(targets_phase1);
    // phase2debug(targets_phase2);
}

// Some notes about threads and the rank lists.
// Assume all MPI message sent and received from a single thread (0).
// gid2in and gid2out are rank wide lists for all threads
//
static void fill_multisend_lists(bool use_phase2,
                                 const std::vector<int>& r,
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
    for (int i = 0; i < r.size();) {
        InputPreSyn* ips = nullptr;
        int gid = r[i++];
        int size = r[i++];
        if (use_phase2) {  // look in gid2in first
            auto gid2in_it = gid2in.find(gid);
            if (gid2in_it != gid2in.end()) {  // phase 2 target list
                ips = gid2in_it->second;
                ips->multisend_phase2_index_ = phase2_index;
                phase2_index += 1 + size;  // count + ranks
                i += size;
            }
        }
        if (!ips) {  // phase 1 target list (or whole list if use_phase2 is 0)
            auto gid2out_it = gid2out.find(gid);
            assert(gid2out_it != gid2out.end());
            PreSyn* ps = gid2out_it->second;
            ps->multisend_index_ = phase1_index;
            phase1_index += 2 + size;  // total + count + ranks
            if (use_phase2) {
                i++;
            }
            i += size;
        }
    }

    targets_phase1 = new int[phase1_index];
    targets_phase2 = new int[phase2_index];

    // printf("%d sz=%d\n", nrnmpi_myid, r.size());
    for (int i = 0; i < r.size();) {
        InputPreSyn* ips = nullptr;
        int gid = r[i++];
        int size = r[i++];
        if (use_phase2) {  // look in gid2in first
            auto gid2in_it = gid2in.find(gid);
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
            auto gid2out_it = gid2out.find(gid);
            assert(gid2out_it != gid2out.end());
            PreSyn* ps = gid2out_it->second;
            int p = ps->multisend_index_;
            int* ranks = targets_phase1 + p;
            int total = size;
            if (use_phase2) {
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
    for (auto& g: gid2out) {
        PreSyn* ps = g.second;
        if (ps->output_index_ >= 0) {  // only ones that generate spikes
            int i = ps->multisend_index_;
            max_ntarget_host = std::max(targets_phase1[i], max_ntarget_host);
            max_multisend_targets = std::max(targets_phase1[i + 1], max_multisend_targets);
        }
    }
    if (use_phase2) {
        for (auto& g: gid2in) {
            InputPreSyn* ps = g.second;
            int i = ps->multisend_phase2_index_;
            if (i >= 0) {
                max_multisend_targets = std::max(max_multisend_targets, targets_phase2[i]);
            }
        }
    }
}

// Return the vector encoding a sequence of gid, target list size, and target list
static std::vector<int> setup_target_lists(bool use_phase2) {
    int nhost = nrnmpi_numprocs;

    // Construct hash table for finding the target rank list for a given gid.
    Int2TarList gid2tarlist;

    celldebug<Gid2PS>("output gid", gid2out);
    celldebug<Gid2IPS>("input gid", gid2in);

    // What are the target ranks for a given input gid. All the ranks
    // with the same input gid send that gid to the intermediate
    // gid%nhost rank. The intermediate rank can then construct the
    // list of target ranks for the gids it gets.

    {
        // scnt1 is number of input gids from target
        std::vector<int> scnt1(nhost, 0);
        for (const auto& g: gid2in) {
            int gid = g.first;
            ++scnt1[gid % nhost];
        }

        // s1 are the input gids from target to be sent to the various intermediates
        const std::vector<int> sdispl1 = newoffset(scnt1);
        // Make an usable copy
        auto sdispl1_ = sdispl1;
        std::vector<int> s1(sdispl1[nhost], 0);
        for (const auto& g: gid2in) {
            int gid = g.first;
            s1[sdispl1_[gid % nhost]++] = gid;
        }

        std::vector<int> r1;
        std::vector<int> rdispl1;
        std::tie(r1, rdispl1) = all2allv_int(s1, scnt1, sdispl1, "gidin to intermediate");
        // r1 is the gids received by this intermediate rank from all other ranks.

        // Now figure out the size of the target list for each distinct gid in r1.
        for (const auto& gid: r1) {
            if (gid2tarlist.find(gid) == gid2tarlist.end()) {
                gid2tarlist[gid] = new TarList{};
                gid2tarlist[gid]->size = 0;
            }
            auto tar = gid2tarlist[gid];
            ++(tar->size);
        }

        // Conceptually, now the intermediate is the mpi source and the gid
        // sources are the mpi destination in regard to target lists.
        // It would be possible at this point, but confusing,
        // to allocate a s[rdispl1[nhost]] and figure out scnt and sdispl by
        // by getting the counts and gids from the ranks that own the source
        // gids. In this way we could organize s without having to allocate
        // individual target lists on the intermediate and then allocate
        // another large s buffer to receive a copy of them. However for
        // this processing we already require two large buffers for input
        // gid's so there is no real savings of space.
        // So let's do the simple obvious sequence and now complete the
        // target lists.

        // Allocate the target lists (and set size to 0 (we will recount when filling).
        for (const auto& g: gid2tarlist) {
            TarList* tl = g.second;
            tl->alloc();
            tl->size = 0;
        }

        // fill the target lists
        for (int rank = 0; rank < nhost; ++rank) {
            int b = rdispl1[rank];
            int e = rdispl1[rank + 1];
            for (int i = b; i < e; ++i) {
                const auto itl_it = gid2tarlist.find(r1[i]);
                if (itl_it != gid2tarlist.end()) {
                    TarList* tl = itl_it->second;
                    tl->list[tl->size] = rank;
                    tl->size++;
                }
            }
        }
    }

    {
        // Now the intermediate hosts have complete target lists and
        // the sources know the intermediate host from the gid2out_ map.
        // We could potentially organize here for two-phase exchange as well.

        // Which target lists are desired by the source rank?

        // Ironically, for round robin distributions, the target lists are
        // already on the proper source rank so the following code should
        // be tested for random distributions of gids.
        // How many on the source rank?
        std::vector<int> scnt2(nhost, 0);
        for (auto& g: gid2out) {
            int gid = g.first;
            PreSyn* ps = g.second;
            if (ps->output_index_ >= 0) {  // only ones that generate spikes
                ++scnt2[gid % nhost];
            }
        }
        const auto sdispl2 = newoffset(scnt2);
        auto sdispl2_ = sdispl2;

        // what are the gids of those target lists
        std::vector<int> s2(sdispl2[nhost], 0);
        for (auto& g: gid2out) {
            int gid = g.first;
            PreSyn* ps = g.second;
            if (ps->output_index_ >= 0) {  // only ones that generate spikes
                s2[sdispl2_[gid % nhost]++] = gid;
            }
        }
        std::vector<int> r2;
        std::vector<int> rdispl2;
        std::tie(r2, rdispl2) = all2allv_int(s2, scnt2, sdispl2, "gidout");

        // fill in the tl->rank for phase 1 target lists
        // r2 is an array of source spiking gids
        // tl is list associating input gids with list of target ranks.
        for (int rank = 0; rank < nhost; ++rank) {
            int b = rdispl2[rank];
            int e = rdispl2[rank + 1];
            for (int i = b; i < e; ++i) {
                // note that there may be input gids with no corresponding
                // output gid so that the find may not return true and in
                // that case the tl->rank remains -1.
                // For example multisplit gids or simulation of a subset of
                // cells.
                const auto itl_it = gid2tarlist.find(r2[i]);
                if (itl_it != gid2tarlist.end()) {
                    TarList* tl = itl_it->second;
                    tl->rank = rank;
                }
            }
        }
    }

    if (use_phase2) {
        random_init(nrnmpi_myid + 1);
        for (const auto& gid2tar: gid2tarlist) {
            TarList* tl = gid2tar.second;
            if (tl->rank >= 0) {  // only if output gid is spike generating
                phase2organize(tl);
            }
        }
    }

    // For clarity, use the all2allv_int style of information flow
    // from source to destination as above
    // and also use a uniform code
    // for copying one and two phase information from a TarList to
    // develop the s, scnt, and sdispl3 buffers. That is, a buffer list
    // section in s for either a one-phase list or the much shorter
    // (individually) lists for first and second phases, has a
    // gid, size, totalsize header for each list where totalsize
    // is only present if the gid is an output gid (for
    // NrnMultisend_Send.ntarget_host used for conservation).
    // Note that totalsize is tl->indices[tl->size]

    // how much to send to each rank
    std::vector<int> scnt3(nhost, 0);
    for (const auto& gid2tar: gid2tarlist) {
        TarList* tl = gid2tar.second;
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
            scnt3[tl->rank] += tl->size + 2;  // gid, size, list
            for (int i = 0; i < tl->size; ++i) {
                scnt3[tl->list[tl->indices[i]]] += tl->indices[i + 1] - tl->indices[i] + 1;
                // gid, size, list
            }
        } else {
            // gid, list size, list
            scnt3[tl->rank] += tl->size + 2;
        }
        if (use_phase2) {
            // The phase 1 header has as its third element, the
            // total list size (needed for conservation);
            scnt3[tl->rank] += 1;
        }
    }
    const auto sdispl4 = newoffset(scnt3);
    auto sdispl4_ = sdispl4;
    std::vector<int> s3(sdispl4[nhost], 0);
    // what to send to each rank
    for (const auto& gid2tar: gid2tarlist) {
        int gid = gid2tar.first;
        TarList* tl = gid2tar.second;
        if (tl->rank < 0) {
            continue;
        }
        if (tl->indices) {
            s3[sdispl4_[tl->rank]++] = gid;
            s3[sdispl4_[tl->rank]++] = tl->size;
            if (use_phase2) {
                s3[sdispl4_[tl->rank]++] = tl->indices[tl->size];
            }
            for (int i = 0; i < tl->size; ++i) {
                s3[sdispl4_[tl->rank]++] = tl->list[tl->indices[i]];
            }
            for (int i = 0; i < tl->size; ++i) {
                int rank = tl->list[tl->indices[i]];
                s3[sdispl4_[rank]++] = gid;
                assert(tl->indices[i + 1] > tl->indices[i]);
                s3[sdispl4_[rank]++] = tl->indices[i + 1] - tl->indices[i] - 1;
                for (int j = tl->indices[i] + 1; j < tl->indices[i + 1]; ++j) {
                    s3[sdispl4_[rank]++] = tl->list[j];
                }
            }
        } else {
            // gid, list size, list
            s3[sdispl4_[tl->rank]++] = gid;
            s3[sdispl4_[tl->rank]++] = tl->size;
            if (use_phase2) {
                s3[sdispl4_[tl->rank]++] = tl->size;
            }
            for (int i = 0; i < tl->size; ++i) {
                s3[sdispl4_[tl->rank]++] = tl->list[i];
            }
        }
        delete tl;
    }
    std::vector<int> r_return;
    std::vector<int> rdispl3;
    std::tie(r_return, rdispl3) = all2allv_int(s3, scnt3, sdispl4, "lists");
    return r_return;
}
}  // namespace coreneuron
#endif  // NRN_MULTISEND
