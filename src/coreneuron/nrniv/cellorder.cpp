#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/cellorder.h"
#include "coreneuron/nrniv/tnode.h"
#include "coreneuron/nrniv/lpt.h"

#include "coreneuron/nrniv/node_permute.h" // for print_quality
#include <set>

#ifdef _OPENACC
#include <openacc.h>
#endif

int use_interleave_permute;
InterleaveInfo* interleave_info; // nrn_nthread array

InterleaveInfo::InterleaveInfo() {
  nwarp = 0;
  nstride = 0;
  stridedispl = NULL;
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
  if (stridedispl) {
    delete [] stridedispl;
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

// more precise visualization of the warp quality
// can be called after admin2
static void print_quality2(int iwarp, InterleaveInfo& ii, int* parent, int* order) {
  int nodebegin = ii.lastnode[iwarp];
  int nodeend = ii.lastnode[iwarp+1];
  int* stride = ii.stride + ii.stridedispl[iwarp];
  int ncycle = ii.cellsize[iwarp];

  int nnode = ii.lastnode[ii.nwarp];
  int* p = new int[nnode];
  for (int i=0; i < nnode; ++i) { p[i] = parent[i]; }
  permute_ptr(p, nnode, order);
  node_permute(p, nnode, order);  

  int inode = nodebegin;

  size_t nx = 0; // number of idle cores on all cycles. X
  size_t ncacheline = 0;; // number of parent memory cacheline accesses.
                 //   assmue warpsize is max number in a cachline so all o
  size_t ncr = 0; // number of child race. nchild-1 of same parent in same cycle

  for (int icycle=0; icycle < ncycle; ++icycle) {
    int s = stride[icycle];
    int lastp = -2;
    printf("  ");
    std::set<int> crace; // how many children have same parent in a cycle
    for (int icore=0; icore < warpsize; ++icore) {
      char ch = '.';
      if (icore < s) {
        int par = p[inode];
        if (crace.find(par) != crace.end()) {
          ch = 'r';
          ++ncr;
        }else{
          crace.insert(par);
        }

        if (par != lastp+1) {
          ch = (ch == 'r') ? 'R' : 'o';
          ++ncacheline;
        }
        lastp = p[inode++];
      }else{
        ch = 'X';
        ++nx;
      }
      printf("%c", ch);
    }
    printf("\n");
  }

  printf("warp %d:  %d nodes, %d cycles, %ld idle, %ld cache access, %ld child races\n",
    iwarp, nodeend - nodebegin, ncycle, nx, ncacheline, ncr);

  delete [] p;
}

static void print_quality1(int iwarp, InterleaveInfo& ii, int ncell, int* parent, int* order) {
  int* stride = ii.stride;
  int cellbegin = iwarp*warpsize;
  int cellend = cellbegin + warpsize;
  cellend = (cellend < stride[0]) ? cellend : stride[0];

  int ncycle = ii.cellsize[cellend - 1]; // since largest last
  int nnode = ii.lastnode[ncell-1];
  int* p = new int[nnode];
  for (int i=0; i < nnode; ++i) { p[i] = parent[i]; }
  permute_ptr(p, nnode, order);
  node_permute(p, nnode, order);  

  ncell = cellend - cellbegin;

  size_t n = 0; // number of nodes in warp (not including roots)
  size_t nx = 0; // number of idle cores on all cycles. X
  size_t ncacheline = 0;; // number of parent memory cacheline accesses.
                 //   assmue warpsize is max number in a cachline so all o

  int inode = ii.firstnode[cellbegin];
  for (int icycle=0; icycle < ncycle; ++icycle) {
    int s = stride[icycle] - cellbegin;
    int sbegin = ncell - s;
    int lastp = -2;
    printf("  ");
    for (int icore=0; icore < warpsize; ++icore) {
      char ch = '.';
      if (icore < ncell && icore >= sbegin) {
        int par = p[inode + icore - sbegin];
        if (par != lastp+1) {
          ch = 'o';
          ++ncacheline;
        }
        lastp = par;
        ++n;
      }else{
        ch = 'X';
        ++nx;
      }
      printf("%c", ch);
    }
    printf("\n");
    inode += ii.stride[icycle+1];
  }

  printf("warp %d:  %ld nodes, %d cycles, %ld idle, %ld cache access\n",
    iwarp, n, ncycle, nx, ncacheline);

  delete [] p;
}

static void warp_balance(int ith, InterleaveInfo& ii) {
  if (use_interleave_permute != 2) { return; }
  size_t nwarp = size_t(ii.nwarp);
  std::vector<size_t> v(nwarp);
  for (size_t i = 0; i < nwarp; ++i) {
    v[i] = size_t(ii.stridedispl[i+1] - ii.stridedispl[i]);
  }
  double bal = load_balance(v);
  printf("thread %d nwarp=%ld  balance=%g\n", ith, nwarp, bal);
}

int* interleave_order(int ith, int ncell, int nnode, int* parent) {
  // ensure parent of root = -1
  for (int i=0; i < ncell; ++i) {
    if (parent[i] == 0) { parent[i] = -1; }
  }

  int nwarp, nstride, *stride, *firstnode, *lastnode, *cellsize, *stridedispl;

  int* order = node_order(ncell, nnode, parent, nwarp,
    nstride, stride, firstnode, lastnode, cellsize, stridedispl);

  if (interleave_info) {
    InterleaveInfo& ii = interleave_info[ith];
    ii.nwarp = nwarp;
    ii.nstride = nstride;
    ii.stridedispl = stridedispl;
    ii.stride = stride;
    ii.firstnode = firstnode;
    ii.lastnode = lastnode;
    ii.cellsize = cellsize;
if (0 && ith == 0 && use_interleave_permute == 1) {
  printf("ith=%d nstride=%d ncell=%d nnode=%d\n", ith, nstride, ncell, nnode);
  for (int i=0; i < ncell; ++i) {
    printf("icell=%d cellsize=%d first=%d last=%d\n", i, cellsize[i], firstnode[i], lastnode[i]);
  }
  for (int i=0; i < nstride; ++i) {
    printf("istride=%d stride=%d\n", i, stride[i]);
  }
}
    if (ith == 0 && use_interleave_permute == 1) {
      print_quality1(0, interleave_info[ith], ncell, parent, order);
    }
    if (ith == 0 && use_interleave_permute == 2) {
      print_quality2(0, interleave_info[ith], parent, order);
    }
    warp_balance(ith, interleave_info[ith]);
  }

  return order;
}

#if INTERLEAVE_DEBUG  // only the cell per core style
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

// icore ranges [0:warpsize) ; stride[ncycle]
static void triang_interleaved2(NrnThread* nt, int icore, int ncycle, int* stride, int lastnode) {
  int icycle = ncycle - 1;
  int istride = stride[icycle];
  int i = lastnode - istride + icore;
#if !defined(_OPENACC)
int ii = i;
#endif
  for (;;) { // ncycle loop
#if !defined(_OPENACC)
// serial test, gpu does this in parallel
for (int icore = 0; icore < warpsize; ++icore) {
int i = ii + icore;
#endif
    if (icore < istride) { // most efficient if istride equal  warpsize
      // what is the index
      int ip = GPU_PARENT(i);
      double p = GPU_A(i) / GPU_D(i);
      GPU_D(ip) -= p * GPU_B(i);
      GPU_RHS(ip) -= p * GPU_RHS(i);
    }
#if !defined(_OPENACC)
}
#endif
    if (icycle == 0) { break; }
    --icycle;
    istride = stride[icycle];
    i -= istride;
#if !defined(_OPENACC)
ii -= istride;
#endif
  }
}
    
// icore ranges [0:warpsize) ; stride[ncycle]
static void bksub_interleaved2(NrnThread* nt, int root, int lastroot,
  int icore, int ncycle, int* stride, int firstnode
) {

#if !defined(_OPENACC)
  for (int i = root; i < lastroot; i += 1) {
#else
  for (int i = root; i < lastroot; i += warpsize) {
#endif
    GPU_RHS(i) /= GPU_D(i); // the root
  }

  int i = firstnode + icore;
#if !defined(_OPENACC)
int ii = i;
#endif
  for (int icycle = 0; icycle < ncycle; ++icycle) {
   int istride = stride[icycle];
#if !defined(_OPENACC)
// serial test, gpu does this in parallel
for (int icore = 0; icore < warpsize; ++icore) {
int i = ii + icore;
#endif
     if (icore < istride) {
      int ip = GPU_PARENT(i);
      GPU_RHS(i) -= GPU_B(i) * GPU_RHS(ip);
      GPU_RHS(i) /= GPU_D(i);
    }      
    i += istride;
#if !defined(_OPENACC)
}
ii += istride;
#endif
  }
}

#ifdef ENABLE_CUDA_INTERFACE
void solve_interleaved_launcher(NrnThread *nt, InterleaveInfo *info, int ncell);
#endif

int temp1[1024] = {0};
int temp2[1024] = {0};
int temp3[1024] = {0};

void solve_interleaved2(int ith) {
static int foo = 1;
  NrnThread* nt = nrn_threads + ith;
  InterleaveInfo& ii = interleave_info[ith];
  int nwarp = ii.nwarp;
  if (nwarp == 0) { return; }
  int* ncycles = ii.cellsize; // nwarp of these
  int* stridedispl = ii.stridedispl; // nwarp+1 of these
  int* strides = ii.stride; // sum ncycles of these (bad since ncompart/warpsize)
  int* rootbegin = ii.firstnode; // nwarp+1 of these
  int* nodebegin = ii.lastnode; // nwarp+1 of these
#ifdef _OPENACC
  int nstride = stridedispl[nwarp];
  int stream_id = nt->stream_id;
#endif

  int ncore = nwarp*warpsize;
  #if 0 && defined(ENABLE_CUDA_INTERFACE) // not implemented
    NrnThread* d_nt = (NrnThread*) acc_deviceptr(nt);
    InterleaveInfo* d_info = (InterleaveInfo*) acc_deviceptr(interleave_info+ith);
    solve_interleaved2_launcher(d_nt, d_info);
  #else
#ifdef _OPENACC
//    #pragma acc kernels loop gang(1), vector(32) present(nt[0:1], strides[0:nstride],...
    #pragma acc parallel loop present(nt[0:1], strides[0:nstride], ncycles[0:nwarp], stridedispl[0:nwarp+1], rootbegin[0:nwarp+1], nodebegin[0:nwarp+1]) if(nt->compute_gpu) async(stream_id)
#endif
    for (int icore = 0; icore < ncore; ++icore) {
      int iwarp = icore / warpsize; // figure out the >> value
      int ic = icore & (warpsize-1); // figure out the & mask
      int ncycle = ncycles[iwarp];
      int* stride = strides + stridedispl[iwarp];
      int root = rootbegin[iwarp];
      int lastroot = rootbegin[iwarp+1];
      int firstnode = nodebegin[iwarp];
      int lastnode = nodebegin[iwarp+1];
//temp1[icore] = ic;
//temp2[icore] = ncycle;
//temp3[icore] = stride - strides;
#if !defined(_OPENACC)
if (ic == 0) { // serial test mode. triang and bksub do all cores in warp
#endif
      triang_interleaved2(nt, ic, ncycle, stride, lastnode);
      bksub_interleaved2(nt, root + ic, lastroot, ic, ncycle, stride, firstnode);
#if !defined(_OPENACC)
} // serial test mode
#endif
    }
#ifdef _OPENACC
    #pragma acc wait(nt->stream_id)
#endif
  #endif
if (foo == 1) { return; }
foo = 0;
  for (int i=0; i < ncore; ++i) {
    printf("%d => %d %d %d\n", i, temp1[i], temp2[i], temp3[i]);
  }
}

void solve_interleaved(int ith) {
  if (use_interleave_permute != 1) {
    solve_interleaved2(ith);
    return;
  }
  NrnThread* nt = nrn_threads + ith;
  InterleaveInfo& ii = interleave_info[ith];
  int nstride = ii.nstride;
  int* stride = ii.stride;
  int* firstnode = ii.firstnode;
  int* lastnode = ii.lastnode;
  int* cellsize = ii.cellsize;
  int ncell = nt->ncell;
#if _OPENACC
  int stream_id = nt->stream_id;
#endif

  #ifdef ENABLE_CUDA_INTERFACE
    NrnThread* d_nt = (NrnThread*) acc_deviceptr(nt);
    InterleaveInfo* d_info = (InterleaveInfo*) acc_deviceptr(interleave_info+ith);
    solve_interleaved_launcher(d_nt, d_info, ncell);
  #else
#ifdef _OPENACC
    #pragma acc parallel loop present(nt[0:1], stride[0:nstride], firstnode[0:ncell],\
    lastnode[0:ncell], cellsize[0:ncell]) if(nt->compute_gpu) async(stream_id)
#endif
    for (int icell = 0; icell < ncell; ++icell) {
      int icellsize = cellsize[icell];
      triang_interleaved(nt, icell, icellsize, nstride, stride, lastnode);
      bksub_interleaved(nt, icell, icellsize, nstride, stride, firstnode);
    }
#ifdef _OPENACC
    #pragma acc wait(nt->stream_id)
#endif
  #endif
}

#endif
