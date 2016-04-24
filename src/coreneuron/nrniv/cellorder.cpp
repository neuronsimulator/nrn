#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/cellorder.h"

#ifdef _OPENACC
#include <openacc.h>
#endif

int use_interleave_permute;
InterleaveInfo* interleave_info; // nrn_nthread array

InterleaveInfo::InterleaveInfo() {
  nstride = 0;
  stride = NULL;
  firstnode = NULL;
  lastnode = NULL;
  cellsize = NULL;
}

InterleaveInfo::~InterleaveInfo() {
  if (stride) {
    delete [] stride;
    delete [] firstnode;
    delete [] lastnode;
    delete [] cellsize;
  }
}

void create_interleave_info() {
  destroy_interleave_info();
  interleave_info = new InterleaveInfo[nrn_nthread];
}

void destroy_interleave_info() {
  if (interleave_info) {
    delete [] interleave_info;
    interleave_info = NULL;
  }
}

int* interleave_order(int ith, int ncell, int nnode, int* parent) {
  // ensure parent of root = -1
  for (int i=0; i < ncell; ++i) {
    if (parent[i] == 0) { parent[i] = -1; }
  }

  int nstride, *stride, *firstnode, *lastnode, *cellsize;

  int* order = node_order(ncell, nnode, parent,
    nstride, stride, firstnode, lastnode, cellsize);

  if (interleave_info) {
    InterleaveInfo& ii = interleave_info[ith];
    ii.nstride = nstride;
    ii.stride = stride;
    ii.firstnode = firstnode;
    ii.lastnode = lastnode;
    ii.cellsize = cellsize;
if (0 && ith == 0) {
  printf("ith=%d nstride=%d ncell=%d nnode=%d\n", ith, nstride, ncell, nnode);
  for (int i=0; i < ncell; ++i) {
    printf("icell=%d cellsize=%d first=%d last=%d\n", i, cellsize[i], firstnode[i], lastnode[i]);
  }
  for (int i=0; i < nstride; ++i) {
    printf("istride=%d stride=%d\n", i, stride[i]);
  }
}
  }

  return order;
}

#if 1
#define GPU_V(i) nt->_actual_v[i]
#define GPU_A(i) nt->_actual_a[i]
#define GPU_B(i) nt->_actual_b[i]
#define GPU_D(i) nt->_actual_d[i]
#define GPU_RHS(i) nt->_actual_rhs[i]
#define GPU_PARENT(i) nt->_v_parent_index[i]

// How does the interleaved permutation with stride get used in
// triagularization?

// each cell in parallel regardless of inhomogeneous topology
static void triang_interleaved(NrnThread* nt, int icell, int icellsize, int nstride, int* stride, int* lastnode) {
  int i = lastnode[icell];
  for (int istride = nstride-1; istride >= 0; --istride) {
    if (istride < icellsize) { // only first icellsize strides matter
      // what is the index
      int ip = GPU_PARENT(i);
#ifndef _OPENACC
      nrn_assert (ip >= 0); // if (ip < 0) return;
#endif
      double p = GPU_A(i) / GPU_D(i);
      GPU_D(ip) -= p * GPU_B(i);
      GPU_RHS(ip) -= p * GPU_RHS(i);
      i -= stride[istride];
    }
  }
}
    
// back substitution?
static void bksub_interleaved(NrnThread* nt, int icell, int icellsize, int nstride, int* stride, int* firstnode) {
  if (nstride){} // otherwise unused
  int i=firstnode[icell];
  GPU_RHS(icell) /= GPU_D(icell); // the root
  for (int istride=0; istride < icellsize; ++istride) {
    int ip = GPU_PARENT(i);
#ifndef _OPENACC
    nrn_assert(ip >= 0);
#endif
    GPU_RHS(i) -= GPU_B(i) * GPU_RHS(ip);
    GPU_RHS(i) /= GPU_D(i);
    i += stride[istride+1];
  }
}

#if INTERLEAVE_DEBUG
static int** cell_indices_debug(NrnThread& nt, InterleaveInfo& ii) {
  int ncell = nt.ncell;
  int nnode = nt.end;
  int* parents = nt._v_parent_index;

  // we expect the nodes to be interleave ordered with smallest cell first
  // establish consistency with ii.
  // first ncell parents are -1
  for (int i=0; i < ncell; ++i) {
    nrn_assert(parents[i] == -1);
  }
  int* sz;
  int* cell;
  sz = new int[ncell];
  cell = new int[nnode];
  for (int i=0; i < ncell; ++i) {
    sz[i] = 0;
    cell[i] = i;
  }
  for (int i=ncell; i < nnode; ++i) {
    cell[i] = cell[parents[i]];
    sz[cell[i]] += 1;
  }

  // cells are in inceasing sz order;
  for (int i=1; i < ncell; ++i) {
    nrn_assert(sz[i-1] <= sz[i]);
  }
  // same as ii.cellsize
  for (int i=0; i < ncell; ++i) {
    nrn_assert(sz[i] == ii.cellsize[i]);
  }

  int** cellindices = new int*[ncell];
  for (int i=0; i < ncell; ++i) {
    cellindices[i] = new int[sz[i]];
    sz[i] = 0; // restart sz counts
  }
  for (int i=ncell; i < nnode; ++i) {
    cellindices[cell[i]][sz[cell[i]]] = i;
    sz[cell[i]] += 1;
  }
  // cellindices first and last same as ii first and last
  for (int i=0; i < ncell; ++i) {
    nrn_assert(cellindices[i][0] == ii.firstnode[i]);
    nrn_assert(cellindices[i][sz[i]-1] == ii.lastnode[i]);
  }

  delete [] sz;
  delete [] cell;

  return cellindices;
}

static int*** cell_indices_threads;
void mk_cell_indices() {
  cell_indices_threads = new int**[nrn_nthread];
  for (int i=0; i < nrn_nthread; ++i) {
    NrnThread& nt = nrn_threads[i];
    if (nt.ncell) {
      cell_indices_threads[i] = cell_indices_debug(nt, interleave_info[i]);
    }else{
      cell_indices_threads[i] = NULL;
    }
  }
}
#endif //INTERLEAVE_DEBUG

#ifdef ENABLE_CUDA_INTERFACE
void solve_interleaved_launcher(NrnThread *nt, InterleaveInfo *info, int ncell);
#endif

void solve_interleaved(int ith) {
  NrnThread* nt = nrn_threads + ith;
  InterleaveInfo& ii = interleave_info[ith];
  int nstride = ii.nstride;
  int* stride = ii.stride;
  int* firstnode = ii.firstnode;
  int* lastnode = ii.lastnode;
  int* cellsize = ii.cellsize;
  int ncell = nt->ncell;
  int stream_id = nt->stream_id;

  #ifdef ENABLE_CUDA_INTERFACE
    NrnThread* d_nt = (NrnThread*) acc_deviceptr(nt);
    InterleaveInfo* d_info = (InterleaveInfo*) acc_deviceptr(interleave_info+ith);
    solve_interleaved_launcher(d_nt, d_info, ncell);
  #else
    #pragma acc parallel loop present(nt[0:1], stride[0:nstride], firstnode[0:ncell],\
    lastnode[0:ncell], cellsize[0:ncell]) if(nt->compute_gpu) async(stream_id)
    for (int icell = 0; icell < ncell; ++icell) {
      int icellsize = cellsize[icell];
      triang_interleaved(nt, icell, icellsize, nstride, stride, lastnode);
      bksub_interleaved(nt, icell, icellsize, nstride, stride, firstnode);
    }
    #pragma acc wait(nt->stream_id)
  #endif
}

#endif
