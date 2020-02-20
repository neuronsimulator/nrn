// use LPT algorithm to balance cells so all warps have similar number
// of compartments.
// NB: Ideally we'd balance so that warps have similar ncycle. But we do not
// know how to predict warp quality without an apriori set of cells to
// fill the warp. For large numbers of cells in a warp,
// it is a justifiable speculation to presume that there will be very
// few holes in warp filling. I.e., ncycle = ncompart/warpsize

// competing objectives are to keep identical cells together and also
// balance warps.
#include <algorithm>

#include "coreneuron/nrnconf.h"
#include "coreneuron/network/tnode.hpp"
#include "coreneuron/utils/lpt.hpp"

namespace coreneuron {
int cellorder_nwarp = 0;  // 0 means do not balance

// ordering by warp, then old order
bool warpcmp(const TNode* a, const TNode* b) {
    bool res = false;
    if (a->groupindex < b->groupindex) {
        res = true;
    } else if (a->groupindex == b->groupindex) {
        if (a->nodevec_index < b->nodevec_index) {
            res = true;
        }
    }
    return res;
}

// order the ncell nodevec roots for balance and return a displacement
// vector specifying the contiguous roots for a warp.
// The return vector should be freed by the caller.
// On entry, nodevec is ordered so that each cell type is together and
// largest cells first. On exit, nodevec is ordered so that warp i
// should contain roots nodevec[displ[i]:displ[i+1]]

size_t warp_balance(size_t ncell, VecTNode& nodevec) {
    if (ncell == 0) {
        return 0;
    }

    if (cellorder_nwarp == 0) {
        return 0;
    }
    size_t nwarp = size_t(cellorder_nwarp);
    // cannot be more warps than cells
    nwarp = (ncell < nwarp) ? ncell : nwarp;

    // cellsize vector and location of types.
    std::vector<size_t> cellsize(ncell);
    std::vector<size_t> typedispl;
    size_t total_compart = 0;
    typedispl.push_back(0);  // types are already in order
    for (size_t i = 0; i < ncell; ++i) {
        cellsize[i] = nodevec[i]->treesize;
        total_compart += cellsize[i];
        if (i == 0 || nodevec[i]->hash != nodevec[i - 1]->hash) {
            typedispl.push_back(typedispl.back() + 1);
        } else {
            typedispl.back() += 1;
        }
    }

    size_t ideal_compart_per_warp = total_compart / nwarp;

    size_t min_cells_per_warp = 0;
    for (size_t i = 0, sz = 0; sz < ideal_compart_per_warp; ++i) {
        ++min_cells_per_warp;
        sz += cellsize[i];
    }

    // balance when order is unrestricted (identical cells not together)
    // i.e. pieces are cellsize
    double best_balance = 0.0;
    std::vector<size_t>* inwarp = lpt(nwarp, cellsize, &best_balance);
    printf("best_balance=%g ncell=%ld ntype=%ld nwarp=%ld\n", best_balance, ncell,
           typedispl.size() - 1, nwarp);

    // order the roots for balance
    for (size_t i = 0; i < ncell; ++i) {
        TNode* nd = nodevec[i];
        nd->groupindex = (*inwarp)[i];
    }
    std::sort(nodevec.begin(), nodevec.begin() + ncell, warpcmp);
    for (size_t i = 0; i < nodevec.size(); ++i) {
        TNode* nd = nodevec[i];
        for (size_t j = 0; j < nd->children.size(); ++j) {
            nd->children[j]->groupindex = nd->groupindex;
        }
        nd->nodevec_index = i;
    }

    delete inwarp;

    return nwarp;
}
}  // namespace coreneuron
