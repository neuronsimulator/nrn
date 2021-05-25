/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <cstdio>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>

#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/network/tnode.hpp"

// just for interleave_permute_type
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/utils/memory.h"


namespace coreneuron {
static size_t groupsize = 32;

/**
 * \brief Function to order trees by size, hash and nodeindex
 */
static bool tnode_earlier(TNode* a, TNode* b) {
    bool result = false;
    if (a->treesize < b->treesize) {  // treesize dominates
        result = true;
    } else if (a->treesize == b->treesize) {
        if (a->hash < b->hash) {  // if treesize same, keep identical trees together
            result = true;
        } else if (a->hash == b->hash) {
            result = a->nodeindex < b->nodeindex;  // identical trees ordered by nodeindex
        }
    }
    return result;
}

static bool ptr_tnode_earlier(TNode* a, TNode* b) {
    return tnode_earlier(a, b);
}

TNode::TNode(int ix) {
    nodeindex = ix;
    cellindex = 0;
    groupindex = 0;
    level = 0;
    hash = 0;
    treesize = 1;
    nodevec_index = 0;
    treenode_order = 0;
    parent = nullptr;
    children.reserve(2);
}

TNode::~TNode() {}

size_t TNode::mkhash() {  // call on all nodes in leaf to root order
    // concept from http://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
    std::sort(children.begin(), children.end(), ptr_tnode_earlier);
    hash = children.size();
    treesize = 1;
    for (size_t i = 0; i < children.size(); ++i) {  // need sorted by child hash
        hash ^= children[i]->hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        treesize += children[i]->treesize;
    }
    return hash;  // hash of leaf nodes is 0
}

static void tree_analysis(int* parent, int nnode, int ncell, VecTNode&);
static void node_interleave_order(int ncell, VecTNode&);
static void admin1(int ncell,
                   VecTNode& nodevec,
                   int& nwarp,
                   int& nstride,
                   int*& stride,
                   int*& firstnode,
                   int*& lastnode,
                   int*& cellsize);
static void admin2(int ncell,
                   VecTNode& nodevec,
                   int& nwarp,
                   int& nstride,
                   int*& stridedispl,
                   int*& strides,
                   int*& rootbegin,
                   int*& nodebegin,
                   int*& ncycles);
static void check(VecTNode&);
static void prtree(VecTNode&);

using TNI = std::pair<TNode*, int>;
using HashCnt = std::map<size_t, std::pair<TNode*, int>>;
using TNIVec = std::vector<TNI>;

static char* stree(TNode* nd) {
    char s[1000];

    if (nd->treesize > 100) {
        return strdup("");
    }
    s[0] = '(';
    s[1] = '\0';
    for (const auto& child: nd->children) {  // need sorted by child hash
        char* sr = stree(child);
        strcat(s, sr);
        free(sr);
    }
    strcat(s, ")");
    return strdup(s);
}

/*
assess the quality of the ordering. The measure is the size of a contiguous
list of nodes whose parents have the same order. How many contiguous lists
have that same size. How many nodes participate in that size list.
Modify the quality measure from experience with performance. Start with
list of (nnode, size_participation)
*/
static void quality(VecTNode& nodevec, size_t max = 32) {
    size_t qcnt = 0;  // how many contiguous nodes have contiguous parents

    // first ncell nodes are by definition in contiguous order
    for (const auto& n: nodevec) {
        if (n->parent != nullptr) {
            break;
        }
        qcnt += 1;
    }
    size_t ncell = qcnt;

    // key is how many parents in contiguous order
    // value is number of nodes that participate in that
    std::map<size_t, size_t> qual;
    size_t ip_last = 10000000000;
    for (size_t i = ncell; i < nodevec.size(); ++i) {
        size_t ip = nodevec[i]->parent->nodevec_index;
        // i%max == 0 means that if we start a warp with 8 and then have 32
        // the 32 is broken into 24 and 8. (modify if the arrangement during
        // gaussian elimination becomes more sophisticated.(
        if (ip == ip_last + 1 && i % max != 0) {  // contiguous
            qcnt += 1;
        } else {
            if (qcnt == 1) {
                // printf("unique %ld p=%ld ix=%d\n", i, ip, nodevec[i]->nodeindex);
            }
            qual[max] += (qcnt / max) * max;
            size_t x = qcnt % max;
            if (x) {
                qual[x] += x;
            }
            qcnt = 1;
        }
        ip_last = ip;
    }
    qual[max] += (qcnt / max) * max;
    size_t x = qcnt % max;
    if (x) {
        qual[x] += x;
    }

    // print result
    qcnt = 0;
#if DEBUG
    for (const auto& q: qual) {
        qcnt += q.second;
        printf("%6ld %6ld\n", q.first, q.second);
    }
#endif
#if DEBUG
    printf("qual.size=%ld  qual total nodes=%ld  nodevec.size=%ld\n",
           qual.size(),
           qcnt,
           nodevec.size());
#endif

    // how many race conditions. ie refer to same parent on different core
    // of warp (max cores) or parent in same group of max.
    size_t maxip = ncell;
    size_t nrace1 = 0;
    size_t nrace2 = 0;
    std::set<size_t> ipused;
    for (size_t i = ncell; i < nodevec.size(); ++i) {
        TNode* nd = nodevec[i];
        size_t ip = nd->parent->nodevec_index;
        if (i % max == 0) {
            maxip = i;
            ipused.clear();
        }
        if (ip >= maxip) {
            nrace1 += 1;
        } /*else*/
        {
            if (ipused.find(ip) != ipused.end()) {
                nrace2 += 1;
                if (ip >= maxip) {
                    // printf("race for parent %ld (parent in same group as multiple users))\n",
                    // ip);
                }
            } else {
                ipused.insert(ip);
            }
        }
    }
#if DEBUG
    printf("nrace = %ld (parent in same group of %ld nodes)\n", nrace1, max);
    printf("nrace = %ld (parent used more than once by same group of %ld nodes)\n", nrace2, max);
#endif
}

size_t level_from_root(VecTNode& nodevec) {
    size_t maxlevel = 0;
    for (auto& nd: nodevec) {
        if (nd->parent) {
            nd->level = nd->parent->level + 1;
            if (maxlevel < nd->level) {
                maxlevel = nd->level;
            }
        } else {
            nd->level = 0;
        }
    }
    return maxlevel;
}

size_t level_from_leaf(VecTNode& nodevec) {
    size_t maxlevel = 0;
    for (size_t i = nodevec.size() - 1; true; --i) {
        TNode* nd = nodevec[i];
        size_t lmax = 0;
        for (auto& child: nd->children) {
            if (lmax <= child->level) {
                lmax = child->level + 1;
            }
        }
        nd->level = lmax;
        if (maxlevel < lmax) {
            maxlevel = lmax;
        }
        if (i == 0) {
            break;
        }
    }
    return maxlevel;
}

/**
 * \brief Set the cellindex to distinguish the different cells.
 */
static void set_cellindex(int ncell, VecTNode& nodevec) {
    for (int i = 0; i < ncell; ++i) {
        nodevec[i]->cellindex = i;
    }
    for (size_t i = 0; i < nodevec.size(); ++i) {
        TNode& nd = *nodevec[i];
        for (size_t j = 0; j < nd.children.size(); ++j) {
            TNode* cnode = nd.children[j];
            cnode->cellindex = nd.cellindex;
        }
    }
}

/**
 * \brief Initialization of the groupindex (groups)
 *
 * The cells are groupped at a later stage based on a load balancing algorithm.
 * This is just an initialization function.
 */
static void set_groupindex(VecTNode& nodevec) {
    for (size_t i = 0; i < nodevec.size(); ++i) {
        TNode* nd = nodevec[i];
        if (nd->parent) {
            nd->groupindex = nd->parent->groupindex;
        } else {
            nd->groupindex = i / groupsize;
        }
    }
}

// how many identical trees and their levels
// print when more than one instance of a type
// reverse the sense of levels (all leaves are level 0) to get a good
// idea of the depth of identical subtrees.
static void ident_statistic(VecTNode& nodevec, size_t ncell) {
    // reverse sense of levels
    //  size_t maxlevel = level_from_leaf(nodevec);
    size_t maxlevel = level_from_root(nodevec);

    // # in each level
    std::vector<std::vector<size_t>> n_in_level(maxlevel + 1);
    for (auto& n: n_in_level) {
        n.resize(ncell / groupsize);
    }
    for (const auto& n: nodevec) {
        n_in_level[n->level][n->groupindex]++;
    }
    printf("n_in_level.size = %ld\n", n_in_level.size());
    for (size_t i = 0; i < n_in_level.size(); ++i) {
        printf("%5ld\n", i);
        for (const auto& n: n_in_level[i]) {
            printf(" %5ld", n);
        }
        printf("\n");
    }
}
#undef MSS

int* node_order(int ncell,
                int nnode,
                int* parent,
                int& nwarp,
                int& nstride,
                int*& stride,
                int*& firstnode,
                int*& lastnode,
                int*& cellsize,
                int*& stridedispl) {
    VecTNode nodevec;

    // nodevec[0:ncell] in increasing size, with identical trees together,
    // and otherwise nodeindex order
    // nodevec.size = nnode
    tree_analysis(parent, nnode, ncell, nodevec);
    check(nodevec);

    set_cellindex(ncell, nodevec);
    set_groupindex(nodevec);
    level_from_root(nodevec);

    // nodevec[ncell:nnode] cells are interleaved in nodevec[0:ncell] cell order
    if (interleave_permute_type == 1) {
        node_interleave_order(ncell, nodevec);
    } else {
        group_order2(nodevec, groupsize, ncell);
    }
    check(nodevec);

#if DEBUG
    for (int i = 0; i < ncell; ++i) {
        TNode& nd = *nodevec[i];
        printf("%d size=%ld hash=%ld ix=%d\n", i, nd.treesize, nd.hash, nd.nodeindex);
    }
#endif

    if (0)
        ident_statistic(nodevec, ncell);
    quality(nodevec);

    // the permutation
    int* nodeorder = new int[nnode];
    for (int i = 0; i < nnode; ++i) {
        TNode& nd = *nodevec[i];
        nodeorder[nd.nodeindex] = i;
    }

    // administrative statistics for gauss elimination
    if (interleave_permute_type == 1) {
        admin1(ncell, nodevec, nwarp, nstride, stride, firstnode, lastnode, cellsize);
    } else {
        //  admin2(ncell, nodevec, nwarp, nstride, stridedispl, stride, rootbegin, nodebegin,
        //  ncycles);
        admin2(ncell, nodevec, nwarp, nstride, stridedispl, stride, firstnode, lastnode, cellsize);
    }

    int ntopol = 1;
    for (int i = 1; i < ncell; ++i) {
        if (nodevec[i - 1]->hash != nodevec[i]->hash) {
            ntopol += 1;
        }
    }
#ifdef DEBUG
    printf("%d distinct tree topologies\n", ntopol);
#endif

    for (size_t i = 0; i < nodevec.size(); ++i) {
        delete nodevec[i];
    }

    return nodeorder;
}

void check(VecTNode& nodevec) {
    // printf("check\n");
    size_t nnode = nodevec.size();
    size_t ncell = 0;
    for (size_t i = 0; i < nnode; ++i) {
        nodevec[i]->nodevec_index = i;
        if (nodevec[i]->parent == nullptr) {
            ncell++;
        }
    }
    for (size_t i = 0; i < ncell; ++i) {
        nrn_assert(nodevec[i]->parent == nullptr);
    }
    for (size_t i = ncell; i < nnode; ++i) {
        TNode& nd = *nodevec[i];
        if (nd.parent->nodevec_index >= nd.nodevec_index) {
            printf("error i=%ld nodevec_index=%ld parent=%ld\n",
                   i,
                   nd.nodevec_index,
                   nd.parent->nodevec_index);
        }
        nrn_assert(nd.nodevec_index > nd.parent->nodevec_index);
    }
}

void prtree(VecTNode& nodevec) {
    size_t nnode = nodevec.size();
    for (size_t i = 0; i < nnode; ++i) {
        nodevec[i]->nodevec_index = i;
    }
    for (size_t i = 0; i < nnode; ++i) {
        TNode& nd = *nodevec[i];
        printf("%ld p=%d   c=%ld l=%ld o=%ld   ix=%d pix=%d\n",
               i,
               nd.parent ? int(nd.parent->nodevec_index) : -1,
               nd.cellindex,
               nd.level,
               nd.treenode_order,
               nd.nodeindex,
               nd.parent ? int(nd.parent->nodeindex) : -1);
    }
}

/**
 * \brief Perform tree preparation for interleaving strategies
 *
 * \param parent vector of parent indices
 * \param nnode number of compartments in the cells
 * \param ncell number of cells
 */
void tree_analysis(int* parent, int nnode, int ncell, VecTNode& nodevec) {
    // create empty TNodes (knowing only their index)
    nodevec.reserve(nnode);
    for (int i = 0; i < nnode; ++i) {
        nodevec.push_back(new TNode(i));
    }

    // determine the (sorted by hash) children of each node
    for (int i = nnode - 1; i >= ncell; --i) {
        nodevec[i]->parent = nodevec[parent[i]];
        nodevec[i]->mkhash();
        nodevec[parent[i]]->children.push_back(nodevec[i]);
    }

    // determine hash of the cells
    for (int i = 0; i < ncell; ++i) {
        nodevec[i]->mkhash();
    }

    // sort it by tree size (from smaller to larger)
    std::sort(nodevec.begin(), nodevec.begin() + ncell, tnode_earlier);
}

static bool interleave_comp(TNode* a, TNode* b) {
    bool result = false;
    if (a->treenode_order < b->treenode_order) {
        result = true;
    } else if (a->treenode_order == b->treenode_order) {
        if (a->cellindex < b->cellindex) {
            result = true;
        }
    }
    return result;
}

/**
 * \brief Naive interleaving strategy (interleave_permute_type == 1)
 *
 * Sort so nodevec[ncell:nnode] cell instances are interleaved. Keep the
 * secondary ordering with respect to treenode_order so each cell is still a tree.
 *
 * \param ncell number of cells (trees)
 * \param nodevec vector that contains compartments (nodes of the trees)
 */
void node_interleave_order(int ncell, VecTNode& nodevec) {
    int* order = new int[ncell];
    for (int i = 0; i < ncell; ++i) {
        order[i] = 0;
        nodevec[i]->treenode_order = order[i]++;
    }
    for (size_t i = 0; i < nodevec.size(); ++i) {
        TNode& nd = *nodevec[i];
        for (size_t j = 0; j < nd.children.size(); ++j) {
            TNode* cnode = nd.children[j];
            cnode->treenode_order = order[nd.cellindex]++;
        }
    }
    delete[] order;

    //  std::sort(nodevec.begin() + ncell, nodevec.end(), contig_comp);
    // Traversal of nodevec: From root to leaves (this is why we compute the tree node order)
    std::sort(nodevec.begin() + ncell, nodevec.end(), interleave_comp);

#if DEBUG
    for (size_t i = 0; i < nodevec.size(); ++i) {
        TNode& nd = *nodevec[i];
        printf("%ld cell=%ld ix=%d\n", i, nd.cellindex, nd.nodeindex);
    }
#endif
}

static void admin1(int ncell,
                   VecTNode& nodevec,
                   int& nwarp,
                   int& nstride,
                   int*& stride,
                   int*& firstnode,
                   int*& lastnode,
                   int*& cellsize) {
    firstnode = (int*) ecalloc_align(ncell, sizeof(int));
    lastnode = (int*) ecalloc_align(ncell, sizeof(int));
    cellsize = (int*) ecalloc_align(ncell, sizeof(int));

    nwarp = (ncell % warpsize == 0) ? (ncell / warpsize) : (ncell / warpsize + 1);

    for (int i = 0; i < ncell; ++i) {
        firstnode[i] = -1;
        lastnode[i] = -1;
        cellsize[i] = 0;
    }

    nstride = 0;
    for (size_t i = ncell; i < nodevec.size(); ++i) {
        TNode& nd = *nodevec[i];
        size_t ci = nd.cellindex;
        if (firstnode[ci] == -1) {
            firstnode[ci] = i;
        }
        lastnode[ci] = i;
        cellsize[ci] += 1;
        if (nstride < cellsize[ci]) {
            nstride = cellsize[ci];
        }
    }

    stride = (int*) ecalloc_align(nstride + 1, sizeof(int));
    for (int i = 0; i <= nstride; ++i) {
        stride[i] = 0;
    }
    for (size_t i = ncell; i < nodevec.size(); ++i) {
        TNode& nd = *nodevec[i];
        stride[nd.treenode_order - 1] += 1;  // -1 because treenode order includes root
    }
}

// for admin2 we allow the node organisation in warps of (say 4 cores per warp)
// ...............  ideal warp but unbalanced relative to warp with max cycles
// ...............  ncycle = 15, icore [0:4), all strides are 4.
// ...............
// ...............
//
// ..........       unbalanced relative to warp with max cycles
// ..........       ncycle = 10, not all strides the same because
// ..........       of need to avoid occasional race conditions.
//  .  . ..         icore [4:8) only 4 strides of 4
//
// ....................  ncycle = 20, uses only one core in the warp (cable)
//                       icore 8, all ncycle strides are 1

// One thing to be unhappy about is the large stride vector of size about
// number of compartments/warpsize. There are a lot of models where the
// stride for a warp is constant except for one cycle in the warp and that
// is easy to obtain when there are more than warpsize cells per warp.

static size_t stride_length(size_t begin, size_t end, VecTNode& nodevec) {
    // return stride length starting at i. Do not go past j.
    // max stride is warpsize.
    // At this time, only assume vicious parent race conditions matter.
    if (end - begin > warpsize) {
        end = begin + warpsize;
    }
    for (size_t i = begin; i < end; ++i) {
        TNode* nd = nodevec[i];
        nrn_assert(nd->nodevec_index == i);
        size_t diff = dist2child(nd);
        if (i + diff < end) {
            end = i + diff;
        }
    }
    return end - begin;
}

static void admin2(int ncell,
                   VecTNode& nodevec,
                   int& nwarp,
                   int& nstride,
                   int*& stridedispl,
                   int*& strides,
                   int*& rootbegin,
                   int*& nodebegin,
                   int*& ncycles) {
    // the number of groups is the number of warps needed
    // ncore is the number of warps * warpsize
    nwarp = nodevec[ncell - 1]->groupindex + 1;

    ncycles = (int*) ecalloc_align(nwarp, sizeof(int));
    stridedispl = (int*) ecalloc_align(nwarp + 1,
                                       sizeof(int));  // running sum of ncycles (start at 0)
    rootbegin = (int*) ecalloc_align(nwarp + 1, sizeof(int));  // index (+1) of first root in warp.
    nodebegin = (int*) ecalloc_align(nwarp + 1, sizeof(int));  // index (+1) of first node in warp.

    // rootbegin and nodebegin are the root index values + 1 of the last of
    // the sequence of constant groupindex
    rootbegin[0] = 0;
    for (size_t i = 0; i < size_t(ncell); ++i) {
        rootbegin[nodevec[i]->groupindex + 1] = i + 1;
    }
    nodebegin[0] = ncell;
    // We start from the leaves and go backwards towards the root
    for (size_t i = size_t(ncell); i < nodevec.size(); ++i) {
        nodebegin[nodevec[i]->groupindex + 1] = i + 1;
    }

    // ncycles, stridedispl, and nstride
    nstride = 0;
    stridedispl[0] = 0;
    for (size_t iwarp = 0; iwarp < (size_t) nwarp; ++iwarp) {
        size_t j = size_t(nodebegin[iwarp + 1]);
        int nc = 0;
        size_t i = nodebegin[iwarp];
        while (i < j) {
            i += stride_length(i, j, nodevec);
            ++nc;  // how many times the warp should loop in order to finish with all the tree
                   // depths (for all the trees of the warp/group)
        }
        ncycles[iwarp] = nc;
        stridedispl[iwarp + 1] = stridedispl[iwarp] + nc;
        nstride += nc;
    }

    // strides
    strides = (int*) ecalloc_align(nstride, sizeof(int));
    nstride = 0;
    for (size_t iwarp = 0; iwarp < (size_t) nwarp; ++iwarp) {
        size_t j = size_t(nodebegin[iwarp + 1]);
        size_t i = nodebegin[iwarp];
        while (i < j) {
            int k = stride_length(i, j, nodevec);
            i += k;
            strides[nstride++] = k;
        }
    }

#if DEBUG
    printf("warp rootbegin nodebegin stridedispl\n");
    for (int i = 0; i <= nwarp; ++i) {
        printf("%4d %4d %4d %4d\n", i, rootbegin[i], nodebegin[i], stridedispl[i]);
    }
#endif
}
}  // namespace coreneuron
