/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/*
To be included by a file that desires rendezvous rank exchange functionality.
Need to define HAVEWANT_t, HAVEWANT_alltoallv, and HAVEWANT2Int
*/

#ifdef have2want_h
#error "This implementation can only be included once"
/* The static function names could involve a macro name. */
#endif

#define have2want_h

#include "coreneuron/utils/nrnoc_aux.hpp"

/*

A rank owns a set of HAVEWANT_t keys and wants information associated with
a set of HAVEWANT_t keys owned by unknown ranks.  Owners do not know which
ranks want their information. Ranks that want info do not know which ranks
own that info.

The have_to_want function returns two new vectors of keys along with
associated count and displacement vectors of length nhost and nhost+1
respectively. Note that a send_to_want_displ[i+1] =
  send_to_want_cnt[i] + send_to_want_displ[i] .

send_to_want[send_to_want_displ[i] to send_to_want_displ[i+1]] contains
the keys from this rank for which rank i wants information.

recv_from_have[recv_from_have_displ[i] to recv_from_have_displ[i+1] contains
the keys from which rank i is sending information to this rank.

Note that on rank i, the order of keys in the rank j area of send_to_want
is the same order of keys on rank j in the ith area in recv_from_have.

The rendezvous_rank function is used to parallelize this computation
and minimize memory usage so that no single rank ever needs to know all keys.
*/

#ifndef HAVEWANT_t
#define HAVEWANT_t int
#endif
namespace coreneuron {
// round robin default rendezvous rank function
static int default_rendezvous(HAVEWANT_t key) {
    return key % nrnmpi_numprocs;
}

static int* cnt2displ(int* cnt) {
    int* displ = new int[nrnmpi_numprocs + 1];
    displ[0] = 0;
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        displ[i + 1] = displ[i] + cnt[i];
    }
    return displ;
}

static int* srccnt2destcnt(int* srccnt) {
    int* destcnt = new int[nrnmpi_numprocs];
#if NRNMPI
    if (nrnmpi_numprocs > 1) {
        nrnmpi_int_alltoall(srccnt, destcnt, 1);
    } else
#endif
    {
        for (int i = 0; i < nrnmpi_numprocs; ++i) {
            destcnt[i] = srccnt[i];
        }
    }
    return destcnt;
}

static void rendezvous_rank_get(HAVEWANT_t* data,
                                int size,
                                HAVEWANT_t*& sdata,
                                int*& scnt,
                                int*& sdispl,
                                HAVEWANT_t*& rdata,
                                int*& rcnt,
                                int*& rdispl,
                                int (*rendezvous_rank)(HAVEWANT_t)) {
    int nhost = nrnmpi_numprocs;

    // count what gets sent
    scnt = new int[nhost];
    for (int i = 0; i < nhost; ++i) {
        scnt[i] = 0;
    }
    for (int i = 0; i < size; ++i) {
        int r = (*rendezvous_rank)(data[i]);
        ++scnt[r];
    }

    sdispl = cnt2displ(scnt);
    rcnt = srccnt2destcnt(scnt);
    rdispl = cnt2displ(rcnt);
    sdata = new HAVEWANT_t[sdispl[nhost]];
    rdata = new HAVEWANT_t[rdispl[nhost]];
    // scatter data into sdata by recalculating scnt.
    for (int i = 0; i < nhost; ++i) {
        scnt[i] = 0;
    }
    for (int i = 0; i < size; ++i) {
        int r = (*rendezvous_rank)(data[i]);
        sdata[sdispl[r] + scnt[r]] = data[i];
        ++scnt[r];
    }
#if NRNMPI
    if (nhost > 1) {
        HAVEWANT_alltoallv(sdata, scnt, sdispl, rdata, rcnt, rdispl);
    } else
#endif
    {
        for (int i = 0; i < sdispl[nhost]; ++i) {
            rdata[i] = sdata[i];
        }
    }
}

static void have_to_want(HAVEWANT_t* have,
                         int have_size,
                         HAVEWANT_t* want,
                         int want_size,
                         HAVEWANT_t*& send_to_want,
                         int*& send_to_want_cnt,
                         int*& send_to_want_displ,
                         HAVEWANT_t*& recv_from_have,
                         int*& recv_from_have_cnt,
                         int*& recv_from_have_displ,
                         int (*rendezvous_rank)(HAVEWANT_t)) {
    // 1) Send have and want to the rendezvous ranks.
    // 2) Rendezvous rank matches have and want.
    // 3) Rendezvous ranks tell the want ranks which ranks own the keys
    // 4) Ranks that want tell owner ranks where to send.

    int nhost = nrnmpi_numprocs;

    // 1) Send have and want to the rendezvous ranks.
    HAVEWANT_t *have_s_data, *have_r_data;
    int *have_s_cnt, *have_s_displ, *have_r_cnt, *have_r_displ;
    rendezvous_rank_get(have,
                        have_size,
                        have_s_data,
                        have_s_cnt,
                        have_s_displ,
                        have_r_data,
                        have_r_cnt,
                        have_r_displ,
                        rendezvous_rank);
    // assume it is an error if two ranks have the same key so create
    // hash table of key2rank. Will also need it for matching have and want
    HAVEWANT2Int havekey2rank = HAVEWANT2Int();
    for (int r = 0; r < nhost; ++r) {
        for (int i = 0; i < have_r_cnt[r]; ++i) {
            HAVEWANT_t key = have_r_data[have_r_displ[r] + i];
            if (havekey2rank.find(key) != havekey2rank.end()) {
                char buf[200];
                sprintf(buf, "key %lld owned by multiple ranks\n", (long long) key);
                hoc_execerror(buf, 0);
            }
            havekey2rank[key] = r;
        }
    }
    delete[] have_s_data;
    delete[] have_s_cnt;
    delete[] have_s_displ;
    delete[] have_r_data;
    delete[] have_r_cnt;
    delete[] have_r_displ;

    HAVEWANT_t *want_s_data, *want_r_data;
    int *want_s_cnt, *want_s_displ, *want_r_cnt, *want_r_displ;
    rendezvous_rank_get(want,
                        want_size,
                        want_s_data,
                        want_s_cnt,
                        want_s_displ,
                        want_r_data,
                        want_r_cnt,
                        want_r_displ,
                        rendezvous_rank);

    // 2) Rendezvous rank matches have and want.
    //    we already have made the havekey2rank map.
    // Create an array parallel to want_r_data which contains the ranks that
    // have that data.
    int n = want_r_displ[nhost];
    int* want_r_ownerranks = new int[n];
    for (int r = 0; r < nhost; ++r) {
        for (int i = 0; i < want_r_cnt[r]; ++i) {
            int ix = want_r_displ[r] + i;
            HAVEWANT_t key = want_r_data[ix];
            if (havekey2rank.find(key) == havekey2rank.end()) {
                char buf[200];
                sprintf(buf, "key = %lld is wanted but does not exist\n", (long long) key);
                hoc_execerror(buf, 0);
            }
            want_r_ownerranks[ix] = havekey2rank[key];
        }
    }
    delete[] want_r_data;

    // 3) Rendezvous ranks tell the want ranks which ranks own the keys
    // The ranks that want keys need to know the ranks that own those keys.
    // The want_s_ownerranks will be parallel to the want_s_data.
    // That is, each item defines the rank from which information associated
    // with that key is coming from
    int* want_s_ownerranks = new int[want_s_displ[nhost]];
#if NRNMPI
    if (nhost > 1) {
        nrnmpi_int_alltoallv(want_r_ownerranks,
                             want_r_cnt,
                             want_r_displ,
                             want_s_ownerranks,
                             want_s_cnt,
                             want_s_displ);
    } else
#endif
    {
        for (int i = 0; i < want_r_displ[nhost]; ++i) {
            want_s_ownerranks[i] = want_r_ownerranks[i];
        }
    }
    delete[] want_r_ownerranks;
    delete[] want_r_cnt;
    delete[] want_r_displ;

    // 4) Ranks that want tell owner ranks where to send.
    // Finished with the rendezvous ranks. The ranks that want keys know the
    // owner ranks for those keys. The next step is for the want ranks to
    // tell the owner ranks where to send.
    // The parallel want_s_ownerranks and want_s_data are now uselessly ordered
    // by rendezvous rank. Reorganize so that want ranks can tell owner ranks
    // what they want.
    n = want_s_displ[nhost];
    delete[] want_s_displ;
    for (int i = 0; i < nhost; ++i) {
        want_s_cnt[i] = 0;
    }
    HAVEWANT_t* old_want_s_data = want_s_data;
    want_s_data = new HAVEWANT_t[n];
    // compute the counts
    for (int i = 0; i < n; ++i) {
        int r = want_s_ownerranks[i];
        ++want_s_cnt[r];
    }
    want_s_displ = cnt2displ(want_s_cnt);
    for (int i = 0; i < nhost; ++i) {
        want_s_cnt[i] = 0;
    }  // recount while filling
    for (int i = 0; i < n; ++i) {
        int r = want_s_ownerranks[i];
        HAVEWANT_t key = old_want_s_data[i];
        want_s_data[want_s_displ[r] + want_s_cnt[r]] = key;
        ++want_s_cnt[r];
    }
    delete[] want_s_ownerranks;
    delete[] old_want_s_data;
    want_r_cnt = srccnt2destcnt(want_s_cnt);
    want_r_displ = cnt2displ(want_r_cnt);
    want_r_data = new HAVEWANT_t[want_r_displ[nhost]];
#if NRNMPI
    if (nhost > 1) {
        HAVEWANT_alltoallv(
            want_s_data, want_s_cnt, want_s_displ, want_r_data, want_r_cnt, want_r_displ);
    } else
#endif
    {
        for (int i = 0; i < want_s_displ[nhost]; ++i) {
            want_r_data[i] = want_s_data[i];
        }
    }
    // now the want_r_data on the have_ranks are grouped according to the ranks
    // that want those keys.

    send_to_want = want_r_data;
    send_to_want_cnt = want_r_cnt;
    send_to_want_displ = want_r_displ;
    recv_from_have = want_s_data;
    recv_from_have_cnt = want_s_cnt;
    recv_from_have_displ = want_s_displ;
}
}  // namespace coreneuron
