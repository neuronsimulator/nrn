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

//calculate the endnode vector and verify that nodes are in cell order.
// the root of the ith cell is node[i]
// the rest of the ith cell is node[endnode[i]] : node[endnode[i+1]]

int* calculate_endnodes(int ncell, int nnode, int* parent) {
  int* cell = new int[nnode]; // each node knows the cell index
  int* cellsize = new int[ncell+1];
  for (int i=0; i < ncell; ++i) {
    cell[i] = i;
    cellsize[cell[i]] = 0;
    assert(parent[i] < 0); // all root nodes must be at beginning
  }
  for (int i=ncell; i < nnode; ++i) {
    cell[i] = cell[parent[i]];
    cellsize[cell[i]] += 1;
    if (cell[i] < cell[i-1] && parent[i-1] >= ncell) { // cell not contiguous! (except at root)
      printf("node %d is in cell %d but node %d is in cell %d\n",i, cell[i], i-1, cell[i-1]);
    }
    nrn_assert(cell[i] >= cell[i-1] || parent[i-1] < ncell);
  }

  // integrate to get displacments
  int dspl = ncell;
  for (int i=0; i <= ncell; ++i) {
    int sz = cellsize[i];
    cellsize[i] = dspl;
    dspl += sz;
  }
  delete [] cell;
  return cellsize;
}


// permutation that transforms cell group ordering to interleaved ordering
// Optimal for large number of homogenous (same topology) cells
// but a bit naive for
// inhomogeneous cells as it does not optimize the parent order.
// Also naive for homogeneous cells if more compute resources than cells
// as we imagine each core is devoted to an entire cell.
// Also naive for inhomogenoues cells as the largest cell takes the most effort.
// Returns new permutation vector of length endnode[ncell] as well as
// a new stride vector of length equal to the size of the largest cell.
// Also return index vectors for firstnode (non-root) of cell i and lastnode
// of cell i and number of non-root nodes for each cell

int* interleave_permutations(int ncell, int* endnode,
  int& nstride, int*& stride,
  int*& firstnode, int*& lastnode, int*& sz
){
  int* permute = new int[endnode[ncell]];

  // nstride is size of largest cell (absent root)
  // also construct vectors of cell sizes (without root) and firstnode,lastnode
  nstride = 0;
  firstnode = new int[ncell];
  lastnode = new int[ncell];
  sz = new int[ncell];
  for (int i=0; i < ncell; ++i) {
    sz[i] = endnode[i+1] - endnode[i];
    if (sz[i] > nstride) {
      nstride = sz[i];
    }
  }

  // do not know if it matters to start from beginning or end
  // start from beginning
  // initialize
  stride = new int[nstride+1]; // back substitution will access stride[nstride]
  for (int i=0; i <= nstride; ++i) {
    stride[i] = 0;
  }
  
  // decrease sz array values by 1 for each stride and count number of
  // positives. That is the stride.
  // Meanwhile the permutation follows that interleaving
  int k = 0; // permute index
  // keep original cell order
  for (int j = 0; j < ncell; ++j) {
    firstnode[j] = -1;
    lastnode[j] = -1;
    permute[k++] = j;
  }
  for (int i=0; i < nstride; ++i) {
    int n = 0;
    for (int j = 0; j < ncell; ++j) {
      if (sz[j] > 0) {
        ++n;
        if (firstnode[j] < 0) { firstnode[j] = k; }
        lastnode[j] = k;
        permute[endnode[j] + i] = k++;
      }
      --sz[j]; // original size is sz[j] + nstride
    }
    stride[i] = n;
  }

  // rebuild sz
  for (int i=0; i < ncell; ++i) {
    sz[i] = endnode[i+1] - endnode[i];
  }

  return permute;
}

int* cell_size_order(int ncell, int nnode, int* parent) {
  int* p = new int[nnode];
  int* cell = new int[nnode]; // each node knows the cell index
  int* cellsize = new int[ncell];

  for (int i=0; i < ncell; ++i) {
    cell[i] = i;
    cellsize[i] = 0; // does not include root
    assert(parent[i] < 0); // all root nodes at beginning
  }

  for (int i=ncell; i < nnode; ++i) {
    cell[i] = cell[parent[i]];
    cellsize[cell[i]] += 1;
  }
  
  // sort by increasing cell size
  // required by interleaved gaussian elimination
  int* cellpermute = nrn_index_sort(cellsize, ncell);
  
  for (int i = 0; i < nnode; ++i) {
    if (i < ncell) {
      p[cellpermute[i]] = i;
    }else{
      p[i] = i;
    }
  }

  delete [] cell;
  delete [] cellsize;
  delete [] cellpermute;

  return p;
}

static int* contiguous_cell_block_order(int ncell, int nnode, int* parent) {
  int* p = new int[nnode];
  int* cell = new int[nnode]; // each node knows the cell index
  int* cellsize = new int[ncell];

  for (int i=0; i < ncell; ++i) {
    cell[i] = i;
    cellsize[i] = 0; // does not include root
    assert(parent[i] < 0); // all root nodes at beginning
  }

  for (int i=ncell; i < nnode; ++i) {
    cell[i] = cell[parent[i]];
    cellsize[cell[i]] += 1;
  }
  
  int* celldispl = new int[ncell+1];
  celldispl[0] = ncell;
  for (int i=0; i < ncell; ++i) {
    celldispl[i+1] = celldispl[i] + cellsize[i];
  }

  // cellpermute order for the roots ... and reinit cellsize
  for (int i=0; i < ncell; ++i) {
    p[i] = i;
    cellsize[i] = 0;
  }

  // remaining nodes in cell block order
  for (int i=ncell; i < nnode; ++i) {
    int icell = cell[i];
    p[i] = celldispl[icell] + cellsize[icell]++;
  }

  delete [] cellsize;
  delete [] celldispl;
  delete [] cell;

  return p;
}

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

  // permute so cell roots are in order of increasing cell size
  int* p_cellsize = cell_size_order(ncell, nnode, parent);
  // update parent so roots are cellsize order
  int* parent1 = new int[nnode];
  for (int i=0; i < nnode; ++i) {
    int par = parent[i];
    if (par >= 0) {
      par = p_cellsize[par];
    }
    parent1[i] = par;
  }

  // permute into contiguous cell
  int* p1 = contiguous_cell_block_order(ncell, nnode, parent1);

  // end_index = p1[start_index]
  int* newparent = new int[nnode];
  for (int i=0; i < nnode; ++i) {
    int par = parent1[i];
    if (par >= 0) {
      par = p1[par];
    }
    newparent[p1[i]] = par;
  }
#if 0  // newparent in contiguous cell block orderr
  for (int i=0; i < nnode; ++i) {
    printf("parent[%d] = %d\n", i, newparent[i]);
  }
#endif

  int* endnodes = calculate_endnodes(ncell, nnode, newparent);
#if 0
  for (int i=0; i <= ncell; ++i) {
    printf("endnodes[%d] = %d\n", i, endnodes[i]);
  }
#endif

  int nstride, *stride, *firstnode, *lastnode, *cellsize;

  int* order = interleave_permutations(ncell, endnodes,
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

  int* combined = new int[nnode];
  for (int i=0; i < nnode; ++i) {
    combined[i] = order[p1[p_cellsize[i]]];
  }

#if 0
  // test the combination. should be interleaved
  for (int i=0; i < nnode; ++i) {
    int par = parent[i];
    if (par >= 0) {
      par = combined[par];
    }
    newparent[combined[i]] = par;
  }
  for (int i=0; i < nnode; ++i) {
    printf("parent[%d] = %d\n", i, newparent[i]);
  }
#endif

  delete [] newparent;
  delete [] order;
  delete [] p_cellsize;

  return combined;
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
      //assert (ip >= 0); // if (ip < 0) return;
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
    //assert(ip >= 0);
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
