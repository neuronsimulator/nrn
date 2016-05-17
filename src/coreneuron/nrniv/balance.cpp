// use LPT algorithm to balance cells so all warps have similar number
// of compartments.
// NB: Ideally we'd balance so that warps have similar ncycle. But we do not
// know how to predict warp quality without an apriori set of cells to
// fill the warp. For large numbers of cells in a warp,
// it is a justifiable speculation to presume that there will be very
// few holes in warp filling. I.e., ncycle = ncompart/warpsize

// competing objectives are to keep identical cells together and also
// balance warps.

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/tnode.h"
#include "coreneuron/nrniv/lpt.h"

static size_t ncore = 1024; // cores on gpu

// order the ncell nodevec roots for balance and return a displacement
// vector specifying the contiguous roots for a warp.
// The return vector should be freed by the caller.
// On entry, nodevec is ordered so that each cell type is together and
// largest cells first. On exit, nodevec is ordered so that warp i
// should contain roots nodevec[displ[i]:displ[i+1]]

std::vector<size_t>* warp_balance(size_t ncell, VecTNode& nodevec) {
  if (ncell == 0) { return 0; }

  // cellsize vector and location of types.
  std::vector<size_t>cellsize(ncell);
  std::vector<size_t>typedispl;
  size_t total_compart = 0;
  typedispl.push_back(0); // types are already in order
  for (size_t i = 0; i < ncell; ++i) {
    cellsize[i] = nodevec[i]->treesize;
    total_compart += cellsize[i];
    if (i == 0 ||nodevec[i]->hash != nodevec[i-1]->hash) {
      typedispl.push_back(typedispl.back() + 1);
    }else{
      typedispl.back() += 1;
    }
  }

  size_t cells_per_type = ncell/(typedispl.size() - 1);

  size_t ideal_ncycle = total_compart/ncore;
  size_t ideal_compart_per_warp = total_compart/(ncore/warpsize);
  size_t avg_cells_per_warp = total_compart/(ncell*ncore/warpsize);

  size_t min_cells_per_warp = 0;
  for (size_t i = 0, sz = 0; sz < ideal_compart_per_warp; ++i) {
    ++min_cells_per_warp;
    sz += cellsize[i];
  }

  // balance when order is unrestricted (identical cells not together)
  // i.e. pieces are cellsize
  double best_balance = 0.0;
  std::vector<size_t>* inwarp = lpt(ncore/warpsize, cellsize, &best_balance);
  printf("best_balance=%g ncell=%ld ntype=%ld nwarp=%ld\n",
    best_balance, ncell, typedispl.size() - 1, ncore/warpsize);

  // order the roots for balance


  delete inwarp;

  return NULL;
}

