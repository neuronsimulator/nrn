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
#include <numeric>

#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/network/tnode.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"

// experiment starting with identical cell ordering
// groupindex aleady defined that keeps identical cells together
// begin with leaf to root ordering
namespace coreneuron {
using VTN = VecTNode;             // level of nodes
using VVTN = std::vector<VTN>;    // group of levels
using VVVTN = std::vector<VVTN>;  // groups

// verify level in groups of nident identical nodes
void chklevel(VTN& level, size_t nident = 8) {}

// first child before second child, etc
// if same parent level, then parent order
// if not same parent, then earlier parent (no parent earlier than parent)
// if same parents, then children order
// if no parents then nodevec_index order.
static bool sortlevel_cmp(TNode* a, TNode* b) {
    // when starting with leaf to root order
    // note that leaves are at max level and all roots at level 0
    bool result = false;
    // since cannot have an index < 0, just add 1 to level
    size_t palevel = a->parent ? 1 + a->parent->level : 0;
    size_t pblevel = b->parent ? 1 + b->parent->level : 0;
    if (palevel < pblevel) {          // only used when starting leaf to root order
        result = true;                // earlier level first
    } else if (palevel == pblevel) {  // always true when starting root to leaf
        if (palevel == 0) {           // a and b are roots
            if (a->nodevec_index < b->nodevec_index) {
                result = true;
            }
        } else {  // parent order (already sorted with proper treenode_order)
            if (a->treenode_order < b->treenode_order) {  // children order
                result = true;
            } else if (a->treenode_order == b->treenode_order) {
                if (a->parent->treenode_order < b->parent->treenode_order) {
                    result = true;
                }
            }
        }
    }
    return result;
}

static void sortlevel(VTN& level) {
    std::sort(level.begin(), level.end(), sortlevel_cmp);

    for (size_t i = 0; i < level.size(); ++i) {
        level[i]->treenode_order = i;
    }
}

// TODO: refactor since sortlevel() is traversing the nodes in same order
static void set_treenode_order(VVTN& levels) {
    size_t order = 0;
    for (auto& level: levels) {
        for (auto* nd: level) {
            nd->treenode_order = order++;
        }
    }
}

#if CORENRN_DEBUG
// every level starts out with no race conditions involving both
// parent and child in the same level. Can we arrange things so that
// every level has at least 32 nodes?
static size_t g32(TNode* nd) {
    return nd->nodevec_index / warpsize;
}

static bool is_parent_race(TNode* nd) {  // vitiating
    size_t pg = g32(nd);
    for (const auto& child: nd->children) {
        if (pg == g32(child)) {
            return true;
        }
    }
    return false;
}
#endif

// less than 32 apart
static bool is_parent_race2(TNode* nd) {  // vitiating
    size_t pi = nd->nodevec_index;
    for (const auto& child: nd->children) {
        if (child->nodevec_index - pi < warpsize) {
            return true;
        }
    }
    return false;
}

#if CORENRN_DEBUG
static bool is_child_race(TNode* nd) {  // potentially handleable by atomic
    if (nd->children.size() < 2) {
        return false;
    }
    if (nd->children.size() == 2) {
        return g32(nd->children[0]) == g32(nd->children[1]);
    }
    std::set<size_t> s;
    for (const auto& child: nd->children) {
        std::size_t gc = g32(child);
        if (s.find(gc) != s.end()) {
            return true;
        }
        s.insert(gc);
    }
    return false;
}
#endif

static bool is_child_race2(TNode* nd) {  // potentially handleable by atomic
    if (nd->children.size() < 2) {
        return false;
    }
    if (nd->children.size() == 2) {
        size_t c0 = nd->children[0]->nodevec_index;
        size_t c1 = nd->children[1]->nodevec_index;
        c0 = (c0 < c1) ? (c1 - c0) : (c0 - c1);
        return c0 < warpsize;
    }
    size_t ic0 = nd->children[0]->nodevec_index;
    for (size_t i = 1; i < nd->children.size(); ++i) {
        size_t ic = nd->children[i]->nodevec_index;
        if (ic - ic0 < warpsize) {
            return true;
        }
        ic0 = ic;
    }
    return false;
}

size_t dist2child(TNode* nd) {
    size_t d = 1000;
    size_t pi = nd->nodevec_index;
    for (const auto& child: nd->children) {
        std::size_t d1 = child->nodevec_index - pi;
        if (d1 < d) {
            d = d1;
        }
    }
    return d;
}

// from stackoverflow.com
template <typename T>
static void move_range(size_t start, size_t length, size_t dst, std::vector<T>& v) {
    typename std::vector<T>::iterator first, middle, last;
    if (start < dst) {
        first = v.begin() + start;
        middle = first + length;
        last = v.begin() + dst;
    } else {
        first = v.begin() + dst;
        middle = v.begin() + start;
        last = middle + length;
    }
    std::rotate(first, middle, last);
}

static void move_nodes(size_t start, size_t length, size_t dst, VTN& nodes) {
    nrn_assert(dst <= nodes.size());
    nrn_assert(start + length <= dst);
    move_range(start, length, dst, nodes);

    // check correctness of move
    for (size_t i = start; i < dst - length; ++i) {
        nrn_assert(nodes[i]->nodevec_index == i + length);
    }
    for (size_t i = dst - length; i < dst; ++i) {
        nrn_assert(nodes[i]->nodevec_index == start + (i - (dst - length)));
    }

    // update nodevec_index
    for (size_t i = start; i < dst; ++i) {
        nodes[i]->nodevec_index = i;
    }
}

#if CORENRN_DEBUG
// least number of nodes to move after nd to eliminate prace
static size_t need2move(TNode* nd) {
    size_t d = dist2child(nd);
    return warpsize - ((nd->nodevec_index % warpsize) + d);
}

static void how_many_warpsize_groups_have_only_leaves(VTN& nodes) {
    size_t n = 0;
    for (size_t i = 0; i < nodes.size(); i += warpsize) {
        bool r = true;
        for (size_t j = 0; j < warpsize; ++j) {
            if (!nodes[i + j]->children.empty()) {
                r = false;
                break;
            }
        }
        if (r) {
            printf("warpsize group %ld starting at level %ld\n", i / warpsize, nodes[i]->level);
            ++n;
        }
    }
    printf("number of warpsize groups with only leaves = %ld\n", n);
}

static void pr_race_situation(VTN& nodes) {
    size_t prace2 = 0;
    size_t prace = 0;
    size_t crace = 0;
    for (size_t i = nodes.size() - 1; nodes[i]->level != 0; --i) {
        TNode* nd = nodes[i];
        if (is_parent_race2(nd)) {
            ++prace2;
        }
        if (is_parent_race(nd)) {
            printf("level=%ld i=%ld d=%ld n=%ld",
                   nd->level,
                   nd->nodevec_index,
                   dist2child(nd),
                   need2move(nd));
            for (const auto& cnd: nd->children) {
                printf("   %ld %ld", cnd->level, cnd->nodevec_index);
            }
            printf("\n");
            ++prace;
        }
        if (is_child_race(nd)) {
            ++crace;
        }
    }
    printf("prace=%ld  crace=%ld prace2=%ld\n", prace, crace, prace2);
}
#endif

static size_t next_leaf(TNode* nd, VTN& nodes) {
    size_t i = 0;
    for (i = nd->nodevec_index - 1; i > 0; --i) {
        if (nodes[i]->children.empty()) {
            return i;
        }
    }
    //  nrn_assert(i > 0);
    return 0;
}

static void checkrace(TNode* nd, VTN& nodes) {
    for (size_t i = nd->nodevec_index; i < nodes.size(); ++i) {
        if (is_parent_race2(nodes[i])) {
            //      printf("checkrace %ld\n", i);
        }
    }
}

static bool eliminate_race(TNode* nd, size_t d, VTN& nodes, TNode* look) {
    // printf("eliminate_race %ld %ld\n", nd->nodevec_index, d);
    // opportunistically move that number of leaves
    // error if no leaves left to move.
    size_t i = look->nodevec_index;
    while (d > 0) {
        i = next_leaf(nodes[i], nodes);
        if (i == 0) {
            return false;
        }
        size_t n = 1;
        while (nodes[i - 1]->children.empty() && n < d) {
            --i;
            ++n;
        }
        // printf("  move_nodes src=%ld len=%ld dest=%ld\n", i, n, nd->nodevec_index);
        move_nodes(i, n, nd->nodevec_index + 1, nodes);
        d -= n;
    }
    checkrace(nd, nodes);
    return true;
}

static void eliminate_prace(TNode* nd, VTN& nodes) {
    size_t d = warpsize - dist2child(nd);
    bool b = eliminate_race(nd, d, nodes, nd);
    if (0 && !b) {
        printf("could not eliminate prace for g=%ld  c=%ld l=%ld o=%ld   %ld\n",
               nd->groupindex,
               nd->cellindex,
               nd->level,
               nd->treenode_order,
               nd->hash);
    }
}

static void eliminate_crace(TNode* nd, VTN& nodes) {
    size_t c0 = nd->children[0]->nodevec_index;
    size_t c1 = nd->children[1]->nodevec_index;
    size_t d = warpsize - ((c0 > c1) ? (c0 - c1) : (c1 - c0));
    TNode* cnd = nd->children[0];
    bool b = eliminate_race(cnd, d, nodes, nd);
    if (0 && !b) {
        printf("could not eliminate crace for g=%ld  c=%ld l=%ld o=%ld   %ld\n",
               nd->groupindex,
               nd->cellindex,
               nd->level,
               nd->treenode_order,
               nd->hash);
    }
}

static void question2(VVTN& levels) {
    // number of compartments in the group
    std::size_t nnode = std::accumulate(levels.begin(),
                                        levels.end(),
                                        0,
                                        [](std::size_t s, const VTN& l) { return s + l.size(); });
    VTN nodes(nnode);  // store the sorted nodes from analyze function
    nnode = 0;
    for (const auto& level: levels) {
        for (const auto& l: level) {
            nodes[nnode++] = l;
        }
    }
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->nodevec_index = i;
    }

    //  how_many_warpsize_groups_have_only_leaves(nodes);

    // Here we need to make sure that the dependent nodes
    // belong to separate warps

    // work backward and check the distance from parent to children.
    // if parent in different group (warp?) then there is no vitiating race.
    // if children in different group (warp?) then ther is no race (satisfied by
    // atomic).
    // If there is a vitiating race, then figure out how many nodes
    // need to be inserted just before the parent to avoid the race.
    //   It is not clear if we should prioritize safe nodes (when moved they
    //   do not introduce a race) and/or contiguous nodes (probably, to keep
    //   the low hanging fruit together).
    //   At least, moved nodes should have proper tree order and not themselves
    //   introduce a race at their new location.  Leaves are nice in that there
    //   are no restrictions in movement toward higher indices.
    //   Note that unless groups of 32 are inserted, it may be the case that
    //   races are generated at greater indices since otherwise a portion of
    //   each group is placed into the next group. This would not be an issue
    //   if, in fact, the stronger requirement of every parent having
    //   pi (parent index) + 32 <= ci (child index) is demanded instead of merely being in different
    //   warpsize. One nice thing about adding warpsize nodes is that it does not disturb any
    //   existing contiguous groups except the moved group which gets divided between parent
    //   warpsize and child, where the nodes past the parent get same relative indices in the next
    //   warpsize

    //  let's see how well we can do by opportunistically moving leaves to
    //  separate parents from children by warpsize (ie is_parent_prace2 is false)
    //  Hopefully, we won't run out of leaves before eliminating all
    //  is_parent_prace2

    if (0 && nodes.size() % warpsize != 0) {
        size_t nnode = nodes.size() - levels[0].size();
        printf("warp of %ld cells has %ld nodes in last cycle %ld\n",
               levels[0].size(),
               nnode % warpsize,
               nnode / warpsize + 1);
    }

    //  pr_race_situation(nodes);

    // eliminate parent and children races using leaves
    // traverse all the children (no roots)
    for (size_t i = nodes.size() - 1; i >= levels[0].size(); --i) {
        TNode* nd = nodes[i];
        if (is_child_race2(nd)) {
            eliminate_crace(nd, nodes);
            i = nd->nodevec_index;
        }
        if (is_parent_race2(nd)) {
            eliminate_prace(nd, nodes);
            i = nd->nodevec_index;
        }
    }
    // copy nodes indices to treenode_order
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->treenode_order = i;
    }
}

// analyze each group of cells
// the cells are grouped based on warp balance (lpt) algorithm
static void analyze(VVTN& levels) {
    // sort each level with respect to parent level order
    // earliest parent level first.

    // treenode order can be anything as long as first children < second
    // children etc.. After sorting a level, the order will be correct for
    // that level, ranging from [0:level.size]
    for (auto& level: levels) {
        chklevel(level);  // does nothing
        for (const auto& nd: level) {
            for (size_t k = 0; k < nd->children.size(); ++k) {
                nd->children[k]->treenode_order = k;
            }
        }
    }

    for (auto& level: levels) {
        sortlevel(level);
        chklevel(level);  // does nothing
    }

    set_treenode_order(levels);
}

void prgroupsize(VVVTN& groups) {
#if CORENRN_DEBUG
    for (size_t i = 0; i < groups[0].size(); ++i) {
        printf("%5ld\n", i);
        for (const auto& group: groups) {
            printf(" %5ld", group[i].size());
        }
        printf("\n");
    }
#endif
}

// group index primary, treenode_order secondary
static bool final_nodevec_cmp(TNode* a, TNode* b) {
    bool result = false;
    if (a->groupindex < b->groupindex) {
        result = true;
    } else if (a->groupindex == b->groupindex) {
        if (a->treenode_order < b->treenode_order) {
            result = true;
        }
    }
    return result;
}

static void set_nodeindex(VecTNode& nodevec) {
    for (size_t i = 0; i < nodevec.size(); ++i) {
        nodevec[i]->nodevec_index = i;
    }
}

void group_order2(VecTNode& nodevec, size_t groupsize, size_t ncell) {
    size_t maxlevel = level_from_root(nodevec);

    // reset TNode.groupindex
    size_t nwarp = warp_balance(ncell, nodevec);

    // work on a cellgroup as a vector of levels. ie only possible race is
    // two children in same warpsize

    // every warp deals with a group of cells
    // the cell dispatching to the available groups is done through the warp_balance function (lpt
    // algo)
    VVVTN groups(nwarp ? nwarp : (ncell / groupsize + ((ncell % groupsize) ? 1 : 0)));

    for (auto& group: groups) {
        group.resize(maxlevel + 1);
    }

    // group the cells according to their groupindex and according to their level (see
    // level_from_root)
    for (const auto& nd: nodevec) {
        groups[nd->groupindex][nd->level].push_back(nd);
    }

    prgroupsize(groups);  // debugging

    // deal with each group
    for (auto& group: groups) {
        analyze(group);
        question2(group);
    }

    // final nodevec order according to group_index and treenode_order
    std::sort(nodevec.begin() + ncell, nodevec.end(), final_nodevec_cmp);
    set_nodeindex(nodevec);
}
}  // namespace coreneuron
