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

template <typename T>
struct Data {
    std::vector<T> data{};
    std::vector<int> cnt{};
    std::vector<int> displ{};
};

static std::vector<int> cnt2displ(const std::vector<int>& cnt) {
    std::vector<int> displ(nrnmpi_numprocs + 1);
    std::partial_sum(cnt.cbegin(), cnt.cend(), displ.begin() + 1);
    return displ;
}

static std::vector<int> srccnt2destcnt(std::vector<int> srccnt) {
    std::vector<int> destcnt(nrnmpi_numprocs);
    nrnmpi_int_alltoall(srccnt.data(), destcnt.data(), 1);
    return destcnt;
}

template <typename T, typename F>
static std::tuple<Data<T>, Data<T>> rendezvous_rank_get(const std::vector<T>& data,
                                                        F alltoall_function) {
    int nhost = nrnmpi_numprocs;

    Data<T> s;
    // count what gets sent
    s.cnt = std::vector<int>(nhost);

    for (const auto& e: data) {
        int r = rendezvous_rank<T>(e);
        s.cnt[r] += 1;
    }

    s.displ = cnt2displ(s.cnt);
    s.data.resize(s.displ[nhost] + 1);

    Data<T> r;
    r.cnt = srccnt2destcnt(s.cnt);
    r.displ = cnt2displ(r.cnt);
    r.data.resize(r.displ[nhost]);
    // scatter data into sdata by recalculating s.cnt.
    std::fill(s.cnt.begin(), s.cnt.end(), 0);
    for (const auto& e: data) {
        int rank = rendezvous_rank<T>(e);
        s.data[s.displ[rank] + s.cnt[rank]] = e;
        s.cnt[rank] += 1;
    }
    alltoall_function(s, r);
    return {s, r};
}

template <typename T, typename F>
std::pair<Data<T>, Data<T>> have_to_want(const std::vector<T>& have,
                                         const std::vector<T>& want,
                                         F alltoall_function) {
    // 1) Send have and want to the rendezvous ranks.
    // 2) Rendezvous rank matches have and want.
    // 3) Rendezvous ranks tell the want ranks which ranks own the keys
    // 4) Ranks that want tell owner ranks where to send.

    int nhost = nrnmpi_numprocs;

    // 1) Send have and want to the rendezvous ranks.

    // hash table of key2rank. Will also need it for matching have and want
    std::unordered_map<T, int> havekey2rank{};
    {
        auto [_, have_r] = rendezvous_rank_get<T>(have, alltoall_function);
        // assume it is an error if two ranks have the same key so create
        havekey2rank.reserve(have_r.displ[nhost] + 1);
        for (int r = 0; r < nhost; ++r) {
            for (int i = 0; i < have_r.cnt[r]; ++i) {
                T key = have_r.data[have_r.displ[r] + i];
                if (havekey2rank.find(key) != havekey2rank.end()) {
                    hoc_execerr_ext(
                        "internal error in have_to_want: key %lld owned by multiple ranks\n",
                        (long long) key);
                }
                havekey2rank[key] = r;
            }
        }
    }

    auto [want_s, want_r] = rendezvous_rank_get<T>(want, alltoall_function);

    // 2) Rendezvous rank matches have and want.
    //    we already have made the havekey2rank map.
    // Create an array parallel to want_r_data which contains the ranks that
    // have that data.
    int n = want_r.displ[nhost];
    std::vector<int> want_r_ownerranks(n);
    for (int r = 0; r < nhost; ++r) {
        for (int i = 0; i < want_r.cnt[r]; ++i) {
            int ix = want_r.displ[r] + i;
            T key = want_r.data[ix];
            auto search = havekey2rank.find(key);
            if (search == havekey2rank.end()) {
                hoc_execerr_ext(
                    "internal error in have_to_want: key = %lld is wanted but does not exist\n",
                    (long long) key);
            }
            want_r_ownerranks[ix] = search->second;
        }
    }

    // 3) Rendezvous ranks tell the want ranks which ranks own the keys
    // The ranks that want keys need to know the ranks that own those keys.
    // The want_s_ownerranks will be parallel to the want_s_data.
    // That is, each item defines the rank from which information associated
    // with that key is coming from
    std::vector<int> want_s_ownerranks(want_s.displ[nhost]);
    if (nrn_sparse_partrans > 0) {
        nrnmpi_int_alltoallv_sparse(want_r_ownerranks.data(),
                                    want_r.cnt.data(),
                                    want_r.displ.data(),
                                    want_s_ownerranks.data(),
                                    want_s.cnt.data(),
                                    want_s.displ.data());
    } else {
        nrnmpi_int_alltoallv(want_r_ownerranks.data(),
                             want_r.cnt.data(),
                             want_r.displ.data(),
                             want_s_ownerranks.data(),
                             want_s.cnt.data(),
                             want_s.displ.data());
    }

    // 4) Ranks that want tell owner ranks where to send.
    // Finished with the rendezvous ranks. The ranks that want keys know the
    // owner ranks for those keys. The next step is for the want ranks to
    // tell the owner ranks where to send.
    // The parallel want_s_ownerranks and want_s_data are now uselessly ordered
    // by rendezvous rank. Reorganize so that want ranks can tell owner ranks
    // what they want.
    n = want_s.displ[nhost];
    for (int i = 0; i < nhost; ++i) {
        want_s.cnt[i] = 0;
    }
    std::vector<T> old_want_s_data(n);
    std::swap(old_want_s_data, want_s.data);
    // compute the counts
    for (int i = 0; i < n; ++i) {
        int r = want_s_ownerranks[i];
        ++want_s.cnt[r];
    }
    want_s.displ = cnt2displ(want_s.cnt);
    for (int i = 0; i < nhost; ++i) {
        want_s.cnt[i] = 0;
    }  // recount while filling
    for (int i = 0; i < n; ++i) {
        int r = want_s_ownerranks[i];
        T key = old_want_s_data[i];
        want_s.data[want_s.displ[r] + want_s.cnt[r]] = key;
        ++want_s.cnt[r];
    }

    Data<T> new_want_r{};
    new_want_r.cnt = srccnt2destcnt(want_s.cnt);
    new_want_r.displ = cnt2displ(new_want_r.cnt);
    new_want_r.data.resize(new_want_r.displ[nhost]);
    alltoall_function(want_s, new_want_r);
    // now the want_r_data on the have_ranks are grouped according to the ranks
    // that want those keys.

    return {new_want_r, want_s};
}
