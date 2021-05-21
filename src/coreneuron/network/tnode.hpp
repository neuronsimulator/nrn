/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#ifndef tnode_h
#define tnode_h

#include <vector>

// experiment with ordering strategies for Tree Nodes
namespace coreneuron {
class TNode;

using VecTNode = std::vector<TNode*>;

class TNode {
  public:
    TNode(int ix);
    virtual ~TNode();
    TNode* parent;
    VecTNode children;
    size_t mkhash();
    size_t hash;
    size_t treesize;
    size_t nodevec_index;
    size_t treenode_order;
    size_t level;
    size_t cellindex;
    size_t groupindex;
    int nodeindex;
};

size_t level_from_leaf(VecTNode&);
size_t level_from_root(VecTNode&);

/**
 * \brief Implementation of the advanced interleaving strategy (interleave_permute_type == 2)
 *
 * The main steps are the following:
 * 1. warp_balance function creates balanced groups of cells.
 * 2. The compartments/tree nodes populate the groups vector (VVVTN) based on their groudindex and
 * their level (see level_from_root).
 * 3. The analyze() & question2() functions (operating per group) make sure that each cell is still
 * a tree (treenode_order) and that the dependent nodes belong to separate warps.
 */
void group_order2(VecTNode&, size_t groupsize, size_t ncell);
size_t dist2child(TNode* nd);

/**
 * \brief Use of the LPT (Least Processing Time) algorithm to create balanced groups of cells.
 *
 * Competing objectives are to keep identical cells together and also balance warps.
 *
 * \param ncell number of cells
 * \param nodevec vector of compartments from all cells
 * \return number of warps
 */
size_t warp_balance(size_t ncell, VecTNode& nodevec);

#define warpsize 32
}  // namespace coreneuron
#endif
