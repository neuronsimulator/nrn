/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/*
Below, the sense of permutation, is reversed. Though consistent, forward
permutation should be defined as (and the code should eventually transformed)
so that
  v: original vector
  p: forward permutation
  pv: permuted vector
  pv[i] = v[p[i]]
and
  pinv: inverse permutation
  pv[pinv[i]] = v[i]
Note: pinv[p[i]] = i = p[pinv[i]]
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

#include "nrnoc/multicore.h"
#include "oc/nrnassrt.h"
#include "permute_utils.hpp"

namespace neuron {

void node_permute(std::vector<int>& vec, const std::vector<int>& permute) {
    for (int i = 0; i < vec.size(); ++i) {
        if (vec[i] >= 0) {
            vec[i] = permute[vec[i]];
        }
    }
}

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

const std::vector<int> nrn_index_sort(int* values, int n) {
    std::vector<std::pair<int, int>> vi(n);
    for (int i = 0; i < n; ++i) {
        vi[i].first = values[i];
        vi[i].second = i;
    }
    std::sort(vi.begin(), vi.end(), nrn_index_sort_cmp);
    std::vector<int> sort_indices(n);
    for (int i = 0; i < n; ++i) {
        sort_indices[i] = vi[i].second;
    }
    return std::move(sort_indices);
}

void sort_ml(Memb_list* ml) {
    auto isrt = nrn_index_sort(ml->nodeindices, ml->nodecount);
    forward_permute(ml->nodeindices, isrt, ml->nodecount);
    forward_permute(ml->nodelist, isrt, ml->nodecount);
    forward_permute(ml->prop, isrt, ml->nodecount);
    forward_permute(ml->pdata, isrt, ml->nodecount);
}

}  // namespace neuron
