#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/cellorder.h"

//calculate the endnode vector and verify that nodes are in cell order.
// the root of the ith cell is node[i]
// the rest of the ith cell is node[endnode[i]] : node[endnode[i+1]]

int* calculate_endnodes(int ncell, int nnode, int* parent) {
  int* cell = new int[nnode]; // each node knows the cell index
  int* endnode = new int[ncell + 1];
  endnode[0] = ncell; // index of first non-root node of cell 0.
  for (int i=0; i < ncell; ++i) {
    cell[i] = i;
    endnode[cell[i]+1] = i;
    assert(parent[i] < 0); // all root nodes must be at beginning
  }
  for (int i=ncell; i < nnode; ++i) {
    cell[i] = cell[parent[i]];
    endnode[cell[i]+1] = i;
    if (cell[i] < cell[i-1]) { // cell not contiguous!
      printf("node %d is in cell %d but node %d is in cell %d\n",i, cell[i], i-1, cell[i-1]);
    }
    nrn_assert(cell[i] >= cell[i-1]);
  }

  delete [] cell;
  return endnode;
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
  stride = new int[nstride];
  for (int i=0; i < nstride; ++i) {
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
        permute[k++] = endnode[j] + i;
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

static int* contiguous_cell_block_order(int ncell, int nnode, int* parent) {
  int* p = new int[nnode];
  int* cell = new int[nnode]; // each node knows the cell index
  int* cellsize = new int[ncell];

  for (int i=0; i < ncell; ++i) {
    cell[i] = i;
    cellsize[i] = 0; // does not inlcude root
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

  // identify permutation for the roots ... and reinit cellsize
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

if (1) { // test
  for (int i=0; i < nnode; ++i) {
    int par = parent[p[i]];
    if (par >= 0) {
      par = p[par];
    }
    printf("parent[%d] = %d\n", i, par);
  }
}

  return p;
}

int* interleave_order(int ncell, int nnode, int* parent) {
  // not used yet
  int nstride = 0;
  int* stride = NULL;
  int* firstnode = NULL;
  int* lastnode = NULL;
  int* cellsize = NULL;

  // ensure parent of root = -1
  for (int i=0; i < ncell; ++i) {
    if (parent[i] == 0) { parent[i] = -1; }
  }

  // permute into contiguous cell block order
  int* p1 = contiguous_cell_block_order(ncell, nnode, parent);
  assert(p1);

  int* endnodes = calculate_endnodes(ncell, nnode, parent);

  int* order = interleave_permutations(ncell, endnodes,
    nstride, stride, firstnode, lastnode, cellsize);

  return order;
}

#if 0
#define GPU_V(i) v[i]
#define GPU_A(i) a[i]
#define GPU_B(i) b[i]
#define GPU_D(i) d[i]
#define GPU_RHS(i) rhs[i]
#define GPU_PARENT(i) _v_parent_index[i]

static double *v, *a, *b, *d, *rhs;
static int* _v_parent_index;

// How does the interleaved permutation with stride get used in
// triagularization?

// each cell in parallel regardless of inhomogeneous topology
void triang(int icell, int sz, int nstride, int* stride, int* lastnode) {
  int i = lastnode[icell];
  for (int istride = nstride-1; istride >= 0; --istride) {
    if (istride >= sz) { continue; }  // only first sz strides matter
    // what is the index
    int ip = GPU_PARENT(i);
    //assert (ip >= 0); // if (ip < 0) return;
    double p = GPU_A(i) / GPU_D(i);
    GPU_D(ip) -= p * GPU_B(i);
    GPU_RHS(ip) -= p * GPU_RHS(i);
    i -= stride[istride];
  }
}
    
// back substitution?
void bksub(int icell, int sz, int nstride, int* stride, int* firstnode) {
  int i=firstnode[icell];
  int ip = GPU_PARENT(i);
  //assert(ip >= 0);
  GPU_RHS(ip) /= GPU_D(ip); // the root
  for (int istride=0; istride < nstride; ++istride) {
    if (istride >= sz) { continue; }
    int ip = GPU_PARENT(i);
    //assert(ip >= 0);
    GPU_RHS(i) -= GPU_B(i) * GPU_RHS(ip);
    GPU_RHS(i) /= GPU_D(i);
    i += stride[istride];
  }
}

#endif
