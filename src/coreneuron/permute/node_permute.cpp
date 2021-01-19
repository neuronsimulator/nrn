/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/*
Permute nodes.

To make gaussian elimination on gpu more efficient.

Permutation vector p[i] applied to a data vector, moves the data_original[i]
to data[p[i]].
That suffices for node properties such as area[i], a[i], b[i]. e.g.
  area[p[i]] <- area_original[i]

Notice that p on the left side is a forward permutation. On the right side
it serves as the inverse permutation.
area_original[i] <- area_permuted[p[i]]

but things
get a bit more complicated when the data is an integer index into the
original data.

For example:

parent[i] needs to be transformed so that
parent[p[i]] <- p[parent_original[i]] except that if parent_original[j] = -1
  then parent[p[j]] = -1

membrane mechanism nodelist ( a subset of nodes) needs to be at least
minimally transformed so that
nodelist_new[k] <- p[nodelist_original[k]]
This does not affect the order of the membrane mechanism property data.

However, computation is more efficient to permute (sort) nodelist_new so that
it follows as much as possible the permuted node ordering, ie in increasing
node order.  Consider this further mechanism specific nodelist permutation,
which is to be applied to the above nodelist_new, to be p_m, which has the same
size as nodelist. ie.
nodelist[p_m[k]] <- nodelist_new[k].

Notice the similarity to the parent case...
nodelist[p_m[k]] = p[nodelist_original[k]]

and now the membrane mechanism node data, does need to be permuted to have an
order consistent with the new nodelist. Since there are nm instances of the
mechanism each with sz data values (consider AoS layout).
The data permutation is
for k=[0:nm] for isz=[0:sz]
  data_m[p_m[k]*sz + isz] = data_m_original[k*sz + isz]

For an SoA layout the indexing is k + isz*nm (where nm may include padding).

A more complicated case is a mechanisms dparam array (nm instances each with
dsz values) Some of those values are indices into another mechanism (eg
pointers to ion properties) or voltage or area depending on the semantics of
the value. We can use the above data_m permutation but then need to update
the values according to the permutation of the object the value indexes into.
Consider the permutation of the target object to be p_t . Then a value
iold = pdata_m(k, isz) - data_t in AoS format
refers to k_t = iold % sz_t and isz_t = iold - k_t*sz_t
and for a target in SoA format isz_t = iold % nm_t and k_t = iold - isz_t*nm_t
ie k_t_new = p_m_t[k_t] so, for AoS, inew = k_t_new*sz_t + isz_t
or , for SoA, inew = k_t_new + isz_t*nm_t
so pdata_m(k, isz) = inew + data_t


*/

#include <vector>
#include <utility>
#include <algorithm>

#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/nrn_setup.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/coreneuron.hpp"
namespace coreneuron {
template <typename T>
void permute(T* data, int cnt, int sz, int layout, int* p) {
    // data(p[icnt], isz) <- data(icnt, isz)
    // this does not change data, merely permutes it.
    // assert len(p) == cnt
    if (!p) {
        return;
    }
    int n = cnt * sz;
    if (n < 1) {
        return;
    }

    if (layout == 0) {  // for SoA, n might be larger due to cnt padding
        n = nrn_soa_padded_size(cnt, layout) * sz;
    }

    T* data_orig = new T[n];
    for (int i = 0; i < n; ++i) {
        data_orig[i] = data[i];
    }

    for (int icnt = 0; icnt < cnt; ++icnt) {
        for (int isz = 0; isz < sz; ++isz) {
            // note that when layout==0, nrn_i_layout takes into account SoA padding.
            int i = nrn_i_layout(icnt, cnt, isz, sz, layout);
            int ip = nrn_i_layout(p[icnt], cnt, isz, sz, layout);
            data[ip] = data_orig[i];
        }
    }

    delete[] data_orig;
}

int* inverse_permute(int* p, int n) {
    int* pinv = new int[n];
    for (int i = 0; i < n; ++i) {
        pinv[p[i]] = i;
    }
    return pinv;
}

static void invert_permute(int* p, int n) {
    int* pinv = inverse_permute(p, n);
    for (int i = 0; i < n; ++i) {
        p[i] = pinv[i];
    }
    delete[] pinv;
}

void update_pdata_values(Memb_list* ml, int type, NrnThread& nt) {
    // assumes AoS to SoA transformation already made since we are using
    // nrn_i_layout to determine indices into both ml->pdata and into target data
    int psz = corenrn.get_prop_dparam_size()[type];
    if (psz == 0) {
        return;
    }
    if (corenrn.get_is_artificial()[type]) {
        return;
    }
    int* semantics = corenrn.get_memb_func(type).dparam_semantics;
    if (!semantics) {
        return;
    }
    int* pdata = ml->pdata;
    int layout = corenrn.get_mech_data_layout()[type];
    int cnt = ml->nodecount;
    // ml padding does not matter (but target padding does matter)

    // interesting semantics are -1 (area), -5 (pointer), -9 (diam), or 0-999 (ion variables)
    for (int i = 0; i < psz; ++i) {
        int s = semantics[i];
        if (s == -1) {                               // area
            int area0 = nt._actual_area - nt._data;  // includes padding if relevant
            int* p_target = nt._permute;
            for (int iml = 0; iml < cnt; ++iml) {
                int* pd = pdata + nrn_i_layout(iml, cnt, i, psz, layout);
                // *pd is the original integer into nt._data . Needs to be replaced
                // by the permuted value

                // This is ok whether or not area changed by padding?
                // since old *pd updated appropriately by earlier AoS to SoA
                // transformation
                int ix = *pd - area0;  // original integer into area array.
                nrn_assert((ix >= 0) && (ix < nt.end));
                int ixnew = p_target[ix];
                *pd = ixnew + area0;
            }
        } else if (s == -9) {                        // diam
            int diam0 = nt._actual_diam - nt._data;  // includes padding if relevant
            int* p_target = nt._permute;
            for (int iml = 0; iml < cnt; ++iml) {
                int* pd = pdata + nrn_i_layout(iml, cnt, i, psz, layout);
                // *pd is the original integer into nt._data . Needs to be replaced
                // by the permuted value

                // This is ok whether or not diam changed by padding?
                // since old *pd updated appropriately by earlier AoS to SoA
                // transformation
                int ix = *pd - diam0;  // original integer into actual_diam array.
                nrn_assert((ix >= 0) && (ix < nt.end));
                int ixnew = p_target[ix];
                *pd = ixnew + diam0;
            }
        } else if (s == -5) {  // assume pointer to membrane voltage
            int v0 = nt._actual_v - nt._data;
            // same as for area semantics
            int* p_target = nt._permute;
            for (int iml = 0; iml < cnt; ++iml) {
                int* pd = pdata + nrn_i_layout(iml, cnt, i, psz, layout);
                int ix = *pd - v0;  // original integer into area array.
                nrn_assert((ix >= 0) && (ix < nt.end));
                int ixnew = p_target[ix];
                *pd = ixnew + v0;
            }
        } else if (s >= 0 && s < 1000) {  // ion
            int etype = s;
            int elayout = corenrn.get_mech_data_layout()[etype];
            Memb_list* eml = nt._ml_list[etype];
            int edata0 = eml->data - nt._data;
            int ecnt = eml->nodecount;
            int esz = corenrn.get_prop_param_size()[etype];
            int* p_target = eml->_permute;
            for (int iml = 0; iml < cnt; ++iml) {
                int* pd = pdata + nrn_i_layout(iml, cnt, i, psz, layout);
                int ix = *pd - edata0;
                // from ix determine i_ecnt and i_esz (need to permute i_ecnt)
                int i_ecnt, i_esz, padded_ecnt;
                if (elayout == 1) {  // AoS
                    padded_ecnt = ecnt;
                    i_ecnt = ix / esz;
                    i_esz = ix % esz;
                } else {  // SoA
                    assert(elayout == 0);
                    padded_ecnt = nrn_soa_padded_size(ecnt, elayout);
                    i_ecnt = ix % padded_ecnt;
                    i_esz = ix / padded_ecnt;
                }
                int i_ecnt_new = p_target[i_ecnt];
                int ix_new = nrn_i_layout(i_ecnt_new, ecnt, i_esz, esz, elayout);
                *pd = ix_new + edata0;
            }
        }
    }
}

void node_permute(int* vec, int n, int* permute) {
    for (int i = 0; i < n; ++i) {
        if (vec[i] >= 0) {
            vec[i] = permute[vec[i]];
        }
    }
}

void permute_ptr(int* vec, int n, int* p) {
    permute(vec, n, 1, 1, p);
}

void permute_data(double* vec, int n, int* p) {
    permute(vec, n, 1, 1, p);
}

void permute_ml(Memb_list* ml, int type, NrnThread& nt) {
    int sz = corenrn.get_prop_param_size()[type];
    int psz = corenrn.get_prop_dparam_size()[type];
    int layout = corenrn.get_mech_data_layout()[type];
    permute(ml->data, ml->nodecount, sz, layout, ml->_permute);
    permute(ml->pdata, ml->nodecount, psz, layout, ml->_permute);

    update_pdata_values(ml, type, nt);
}

int nrn_index_permute(int ix, int type, Memb_list* ml) {
    int* p = ml->_permute;
    if (!p) {
        return ix;
    }
    int layout = corenrn.get_mech_data_layout()[type];
    if (layout == 1) {
        int sz = corenrn.get_prop_param_size()[type];
        int i_cnt = ix / sz;
        int i_sz = ix % sz;
        return p[i_cnt] * sz + i_sz;
    } else {
        assert(layout == 0);
        int padded_cnt = nrn_soa_padded_size(ml->nodecount, layout);
        int i_cnt = ix % padded_cnt;
        int i_sz = ix / padded_cnt;
        return i_sz * padded_cnt + p[i_cnt];
    }
}

#if DEBUG
static void pr(const char* s, int* x, int n) {
  printf("%s:", s);
  for (int i=0; i < n; ++i) {
    printf("  %d %d", i, x[i]);
  }
  printf("\n");
}

static void pr(const char* s, double* x, int n) {
  printf("%s:", s);
  for (int i=0; i < n; ++i) {
    printf("  %d %g", i, x[i]);
  }
  printf("\n");
}
#endif

// note that sort_indices has the sense of an inverse permutation in that
// the value of sort_indices[0] is the index with the smallest value in the
// indices array

static bool nrn_index_sort_cmp(const std::pair<int, int>& a, const std::pair<int, int>& b) {
    bool result = false;
    if (a.first < b.first) {
        result = true;
    } else if (a.first == b.first) {
        if (a.second < b.second) {
            result = true;
        }
    }
    return result;
}

int* nrn_index_sort(int* values, int n) {
    std::vector<std::pair<int, int> > vi(n);
    for (int i = 0; i < n; ++i) {
        vi[i].first = values[i];
        vi[i].second = i;
    }
    std::sort(vi.begin(), vi.end(), nrn_index_sort_cmp);
    int* sort_indices = new int[n];
    for (int i = 0; i < n; ++i) {
        sort_indices[i] = vi[i].second;
    }
    return sort_indices;
}

void permute_nodeindices(Memb_list* ml, int* p) {
    // nodeindices values are permuted according to p (that per se does
    //  not affect vec).

    node_permute(ml->nodeindices, ml->nodecount, p);

    // Then the new node indices are sorted by
    // increasing index. Instances using the same node stay in same
    // original relative order so that their contributions to rhs, d (if any)
    // remain in same order (except for gpu parallelism).
    // That becomes ml->_permute

    ml->_permute = nrn_index_sort(ml->nodeindices, ml->nodecount);
    invert_permute(ml->_permute, ml->nodecount);
    permute_ptr(ml->nodeindices, ml->nodecount, ml->_permute);
}
}  // namespace coreneuron
