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

#include <stdio.h>
#include <stdlib.h>
#include "node_permute.h"
#include <vector>

static int* permute_;
    

template <typename T>
void permute(T* data, size_t cnt, size_t sz, int layout, size_t* p) {
  // data(p[icnt], isz) <- data(icnt, isz)
  // this does not change data, merely permutes it.
  // assert len(p) == cnt
  size_t n = cnt*sz;
  if (n < 1) { return; }

  if (layout == 0) { // for SoA, n might be larger due to cnt padding
    n = nrn_soa_padded_size(cnt, layout)*sz;
  }

  T* data_orig = new T[n];
  for (size_t i = 0; i < n: ++i) {
    data_orig[i] = data[i];
  }

  for (size_t icnt = 0; icnt < cnt; ++icnt) {
    for (size_t isz = 0; isz < sz; ++isz) {
      // note that when layout==0, nrn_i_layout takes into account SoA padding.
      int i = nrn_i_layout(icnt, cnt, isz, sz, layout);
      int ip = nrn_i_layout(p[icnt], cnt, isz, sz, layout);
      data[ip] = data_orig[i];
    }
  }

  delete [] data_orig;
}

void update_pdata_values(Memb_list* ml, int type, NrnThread* nt) {
  // assumes AoS to SoA transformation already made since we are using
  // nrn_i_layout to determine indices into both ml->pdata and into target data
  int psz = nrn_prop_dparam_size_[type];
  if (psz == 0) { return; }
  if (nrn_is_artificial_[type]) { return; }
  int* semantics = memb_func[type].dparam_semantics;
  if (!semantics) { return; }
  int* pdata = ml->pdata;
  int layout = nrn_mech_data_layout_[type];
  int cnt = ml->nodecount;
  // ml padding does not matter (but target padding does matter)

  // interesting semantics are -1 (area), -5 (pointer), or 0-999 (ion variables)
  for (int i; i < psz; ++i) {
    int s = semantics[i];
    if (s == -1) { // area
      int area0 = nt._actual_area - nt._data; // includes padding if relevant
      int* p_target = nt.permute_;
      for (int iml = 0; iml < cnt; ++iml) {
        int* pd = pdata + nrn_i_layout(iml, cnt, i, psz, layout);
        // *pd is the original integer into nt._data . Needs to be replaced
        // by the permuted value

        // This is ok whether or not area changed by padding?
        // since old *pd updated appropriately by earlier AoS to SoA
        // transformation
        int ix = *pd - area0; //original integer into area array.
        nrn_assert((ix >= 0) && (ix < nt.end));
        int ixnew = p_target[ix];
        *pd = ixnew + area0;
      }
    }else if (s == -5) { // assume pointer to membrane voltage
      int v0 = nt._actual_v - nt._data;
      for (int iml = 0; iml < cnt; ++iml) {
        // same as for area semantics
      }
    }else if (s >= 0 && s < 1000) { // ion
      int etype = s;
      int elayout = nrn_mech_data_layout_[etype];
      Memb_list* eml = nt->_ml_list[etype];
      int edata0 = eml->data - nt._data;
      int ecnt = eml->nodecount;
      int esz = nrn_prop_param_size_[etype];
      int* p_target = eml->permute_;
      for (int iml = 0; iml < cnt; ++iml) {
        int* pd = pdata + nrn_i_layout(iml, cnt, i, psz, layout);
        int ix = *pd - edata0;
        // from ix determine i_ecnt and i_esz (need to permute i_ecnt)
        int i_ecnt, i_esz, padded_ecnt;
        if (elayout == 1) { // AoS
          padded_ecnt = ecnt;
          i_ecnt = ix / esz;
          i_esz = ix % esz;
        }else{ // SoA
          assert(elayout == 0);
          padded_ecnt = nrn_soa_padded_size(ecnt, elayout);
          i_ecnt = ix % padded_ecnt;
          i_esz = ix / padded_ecnt;
        }
        int i_ecnt_new = p_target(i_ecnt);
        int ix_new = nrn_i_layout(i_ecnt_new, e_cnt, i_esz, esz, elayout)
        *pd = ix_new + edata0;
      }
    }
  }
}


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

void permute_cleanup() {
  if (permute_) {
    delete [] permute_;
    permute_ = NULL;
  }
}

void node_permute(int* parents, int n) {
  // index is the original, permute_[index] is the new index (location)
  // if the value does not refer to an index, 
  // old value[i] is copied to new value[permute_[i]]
  // new value[permute_[i]] = old value[i]
  // new value[i] = old value[inverse_permute_[i]]

  // Apply the permutation to the data as read and before it is transformed
  // AoS to SoA or padded.
  permute_ = new int[n];
    
  // rotation permutation for testing
  for (int i=0; i < n; ++i) {
    permute_[i] = (i+1)%n; 
  }
   
  // now permute the parents
  int* tmp_parents = new int[n];
  for (int i=0; i < n; ++i) {
    tmp_parents[i] = parents[i];
  }
  for (int i=0; i < n; ++i) {
    int p = permute_[i];
    int parent = tmp_parents[i];
    if (parent >= 0) {
      parent = permute_[parent];
    }
    parents[p] = parent;
  }
  delete [] tmp_parents;
 
#if 0
  // verify tree constraint    
  for (int i=0; i < n; ++i) {  
    assert(parents[i] < i);  
  }
#endif
}  

using namespace std;
template<class T>
void permute(vector<T>& vec, vector<size_t>& prmut, int dir) {
  assert(vec.size() == prmut.size());
  vector<T> tmpvec = vec;
  if (dir > 0) {
    for (size_t i = 0; i < prmut.size(); ++i) {
      vec[prmut[i]] = tmpvec[i];
    }
  }else{
    for (size_t i = 0; i < prmut.size(); ++i) {
      vec[i] = tmpvec[prmut[i]];
    }
  }    
}

void permute_data(double* vec, int n) {
  double* tmp_vec = new double[n];
  for (int i=0; i < n; ++i) {
    tmp_vec[i] = vec[i];
  }
  for (int i=0; i < n; ++i) {
    vec[permute_[i]] = tmp_vec[i];
  }
  delete [] tmp_vec;
}

static int compar(const void* p1, const void* p2, void* arg) {
  int ix1 = *((int*)p1);
  int ix2 = *((int*)p2);
  int* indices = (int*)arg;
  if (indices[ix1] < indices[ix2]) { return -1; }
  if (indices[ix1] > indices[ix2]) { return 1; }
  return 0;
}

// note that sort_indices has the sense of an inverse permutation in that
// the value of sort_indices[0] is the index with the smallest value in the
// indices array
static int* sortindices(int* indices, int n) {
  int* sort_indices = new int[n];
  for (int i=0; i < n; ++i) {
    sort_indices[i] = i;
  }
  qsort_r(sort_indices, n, sizeof(int), compar, indices);
  return sort_indices;
}

int* node_indices_permute(int* nodeindices, int n) {
}

template <typename T>
void node_data_permute(T* vec, size_t sz, int* psort, int n) {
}

int* permute_node_subset_data(
  double* vec, size_t sz,
  int* dvec, size_t dsz,
  int* indices, int n
){
  // node indices are permuted (that per se does not affect vec)
  // Then the new node indices are sorted by increasing index. That does
  // require sorting vec according to the sort_indices return value.
  int* psort = node_indices_permute(indices, n);
  node_data_permute(vec, sz, psort, n);
  node_data_permute(dvec, dsz, psort, n); // index values will need to be
    //changed based on semantics of the values containing indices.
    // eg. permute for area; ion type specific psort for ion index values.

  size_t nsz = n*sz;
  double* tmp_vec = new double[nsz];
  for (size_t i=0; i < nsz; ++i) {
    tmp_vec[i] = vec[i];  
  }

  int* tmp_dvec = NULL;
  if (dsz) {
    size_t ndsz = n*dsz;
    tmp_dvec = new int[ndsz];
    for (size_t i=0; i < ndsz; ++i) {
      tmp_dvec[i] = dvec[i];  
    }
  }

  int* tmp_indices = new int[n];
  for (int i=0; i < n; ++i) {
    tmp_indices[i] = permute_[indices[i]];
  }
  for (int i=0; i < n; ++i) {
    indices[i] = tmp_indices[i];
  }

  // the data is not permuted just because the node indices were permuted.
  // However after sorting, that sort becomes the data permutation.
  pr("indices", tmp_indices, n);
  int* sort_indices = sortindices(tmp_indices, n);
  pr("sort_indices", sort_indices, n);

  for (int i=0; i < n; ++i) {
    indices[i] = tmp_indices[sort_indices[i]];
    for (int j=0; j < sz; ++j) {
      vec[i*sz + j] = tmp_vec[sort_indices[i]*sz + j];
    }
    for (int j=0; j < dsz; ++j) {
      dvec[i*dsz + j] = tmp_dvec[sort_indices[i]*dsz + j];
    }
  }

  delete [] tmp_indices;
  delete [] tmp_vec;
  if (dsz) {
    delete [] tmp_dvec;
  }

  return sort_indices;
}

#if 1

int main() {
  int n = 5;
  int* parents = new int[n];
  parents[0] = -1;
  for (int i=1; i < n; ++i) {
    parents[i] = i - 1;
  }
  pr("parents", parents, n);
  node_permute(parents, n);
  pr("permute_", permute_, n);
  pr("parents", parents, n);
  delete [] parents;

  double* d = new double[n];
  for (int i=0; i < n; ++i) {
    d[i] = 10. + i;
  }
  pr("d", d, n);
  permute_data(d, n);
  pr("d", d, n);
  delete [] d;
  
  n = 3;
  int* ix = new int[n];
  ix[0] = 1;
  ix[1] = 3;
  ix[2] = 4;
  d = new double[n];
  for (int i=0; i < n; ++i) {
    d[i] = 10. + ix[i];
  }
  pr("ix", ix, n);
  pr("d", d, n);
  permute_node_subset_data(d, 1, NULL, 0, ix, n);
  pr("ix", ix, n);
  pr("d", d, n);
  delete [] ix;
  delete [] d;
  
  printf("test sort\n");
  n=5;
  ix = new int[n];
  for (int i=0; i < n; i++) {
    ix[i] = (i+1)%n;
  }
  pr("ix", ix, n);
  int* si = sortindices(ix, n);
  pr("si", si, n);
  delete [] ix;
  delete [] si;

  return 0;
}
#endif
