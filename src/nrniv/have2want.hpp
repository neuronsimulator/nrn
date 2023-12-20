#pragma once

#include <numeric>
#include <unordered_map>
#include <vector>

/*
A rank owns a set of T keys and wants information associated with
a set of T keys owned by unknown ranks.  Owners do not know which
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

// round robin rendezvous rank function
template <typename T>
int rendezvous_rank(const T& key) {
    return key % nrnmpi_numprocs;
}

static int* cnt2displ(const int* cnt) {
    int* displ = new int[nrnmpi_numprocs + 1];
    displ[0] = 0;
    std::partial_sum(cnt, cnt + nrnmpi_numprocs, displ + 1);
    return displ;
}

static int* srccnt2destcnt(int* srccnt) {
    int* destcnt = new int[nrnmpi_numprocs];
    nrnmpi_int_alltoall(srccnt, destcnt, 1);
    return destcnt;
}

template <typename T, typename F>
static void rendezvous_rank_get(T* data,
                                int size,
                                T*& sdata,
                                int*& scnt,
                                int*& sdispl,
                                T*& rdata,
                                int*& rcnt,
                                int*& rdispl,
                                F alltoall_function) {
    int nhost = nrnmpi_numprocs;

    // count what gets sent
    scnt = new int[nhost];
    for (int i = 0; i < nhost; ++i) {
        scnt[i] = 0;
    }
    for (int i = 0; i < size; ++i) {
        int r = rendezvous_rank<T>(data[i]);
        ++scnt[r];
    }

    sdispl = cnt2displ(scnt);
    rcnt = srccnt2destcnt(scnt);
    rdispl = cnt2displ(rcnt);
    sdata = new T[sdispl[nhost] + 1];  // ensure not 0 size
    rdata = new T[rdispl[nhost] + 1];  // ensure not 0 size
    // scatter data into sdata by recalculating scnt.
    for (int i = 0; i < nhost; ++i) {
        scnt[i] = 0;
    }
    for (int i = 0; i < size; ++i) {
        int r = rendezvous_rank<T>(data[i]);
        sdata[sdispl[r] + scnt[r]] = data[i];
        ++scnt[r];
    }
    alltoall_function(sdata, scnt, sdispl, rdata, rcnt, rdispl);
}

template <typename T, typename F>
void have_to_want(T* have,
                  int have_size,
                  T* want,
                  int want_size,
                  T*& send_to_want,
                  int*& send_to_want_cnt,
                  int*& send_to_want_displ,
                  T*& recv_from_have,
                  int*& recv_from_have_cnt,
                  int*& recv_from_have_displ,
                  F alltoall_function) {
    // 1) Send have and want to the rendezvous ranks.
    // 2) Rendezvous rank matches have and want.
    // 3) Rendezvous ranks tell the want ranks which ranks own the keys
    // 4) Ranks that want tell owner ranks where to send.

    int nhost = nrnmpi_numprocs;

    // 1) Send have and want to the rendezvous ranks.
    T *have_s_data, *have_r_data;
    int *have_s_cnt, *have_s_displ, *have_r_cnt, *have_r_displ;
    rendezvous_rank_get<T>(have,
                           have_size,
                           have_s_data,
                           have_s_cnt,
                           have_s_displ,
                           have_r_data,
                           have_r_cnt,
                           have_r_displ,
                           alltoall_function);
    delete[] have_s_cnt;
    delete[] have_s_displ;
    delete[] have_s_data;
    // assume it is an error if two ranks have the same key so create
    // hash table of key2rank. Will also need it for matching have and want
    std::unordered_map<T, int> havekey2rank(have_r_displ[nhost] + 1);  // ensure not empty.
    for (int r = 0; r < nhost; ++r) {
        for (int i = 0; i < have_r_cnt[r]; ++i) {
            T key = have_r_data[have_r_displ[r] + i];
            if (havekey2rank.find(key) != havekey2rank.end()) {
                hoc_execerr_ext(
                    "internal error in have_to_want: key %lld owned by multiple ranks\n",
                    (long long) key);
            }
            havekey2rank[key] = r;
        }
    }
    delete[] have_r_data;
    delete[] have_r_cnt;
    delete[] have_r_displ;

    T *want_s_data, *want_r_data;
    int *want_s_cnt, *want_s_displ, *want_r_cnt, *want_r_displ;
    rendezvous_rank_get<T>(want,
                           want_size,
                           want_s_data,
                           want_s_cnt,
                           want_s_displ,
                           want_r_data,
                           want_r_cnt,
                           want_r_displ,
                           alltoall_function);

    // 2) Rendezvous rank matches have and want.
    //    we already have made the havekey2rank map.
    // Create an array parallel to want_r_data which contains the ranks that
    // have that data.
    int n = want_r_displ[nhost];
    int* want_r_ownerranks = new int[n];
    for (int r = 0; r < nhost; ++r) {
        for (int i = 0; i < want_r_cnt[r]; ++i) {
            int ix = want_r_displ[r] + i;
            T key = want_r_data[ix];
            auto search = havekey2rank.find(key);
            if (search == havekey2rank.end()) {
                hoc_execerr_ext(
                    "internal error in have_to_want: key = %lld is wanted but does not exist\n",
                    (long long) key);
            }
            want_r_ownerranks[ix] = search->second;
        }
    }
    delete[] want_r_data;

    // 3) Rendezvous ranks tell the want ranks which ranks own the keys
    // The ranks that want keys need to know the ranks that own those keys.
    // The want_s_ownerranks will be parallel to the want_s_data.
    // That is, each item defines the rank from which information associated
    // with that key is coming from
    int* want_s_ownerranks = new int[want_s_displ[nhost]];
    if (nrn_sparse_partrans > 0) {
        nrnmpi_int_alltoallv_sparse(want_r_ownerranks,
                                    want_r_cnt,
                                    want_r_displ,
                                    want_s_ownerranks,
                                    want_s_cnt,
                                    want_s_displ);
    } else {
        nrnmpi_int_alltoallv(want_r_ownerranks,
                             want_r_cnt,
                             want_r_displ,
                             want_s_ownerranks,
                             want_s_cnt,
                             want_s_displ);
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
    T* old_want_s_data = want_s_data;
    want_s_data = new T[n];
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
        T key = old_want_s_data[i];
        want_s_data[want_s_displ[r] + want_s_cnt[r]] = key;
        ++want_s_cnt[r];
    }
    delete[] want_s_ownerranks;
    delete[] old_want_s_data;
    want_r_cnt = srccnt2destcnt(want_s_cnt);
    want_r_displ = cnt2displ(want_r_cnt);
    want_r_data = new T[want_r_displ[nhost]];
    alltoall_function(want_s_data, want_s_cnt, want_s_displ, want_r_data, want_r_cnt, want_r_displ);
    // now the want_r_data on the have_ranks are grouped according to the ranks
    // that want those keys.

    send_to_want = want_r_data;
    send_to_want_cnt = want_r_cnt;
    send_to_want_displ = want_r_displ;
    recv_from_have = want_s_data;
    recv_from_have_cnt = want_s_cnt;
    recv_from_have_displ = want_s_displ;
}
