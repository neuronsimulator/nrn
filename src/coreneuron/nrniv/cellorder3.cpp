#include <stdio.h>
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/cellorder.h"
#include "coreneuron/nrniv/tnode.h"

#include <map>
#include <set>
#include <algorithm>
#include <string.h>

using namespace std;


// experiment starting with identical cell ordering
// groupindex aleady defined that keeps identical cells together
// begin with leaf to root ordering

typedef VecTNode VTN; // level of nodes
typedef vector<VTN> VVTN;  // group of levels
typedef vector<VVTN> VVVTN; // groups

// verify level in groups of nident identical nodes
void chklevel(VTN& level, size_t nident = 8) {
  nrn_assert(level.size() % nident == 0);
  for (size_t i = 0; i <  level.size(); ++i) {
    size_t j = nident*int(i/nident);
    nrn_assert(level[i]->hash == level[j]->hash);
  }
}

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
  size_t palevel = a->parent ?  1 + a->parent->level : 0;
  size_t pblevel = b->parent ?  1 + b->parent->level : 0;
  if (palevel < pblevel) { // only used when starting leaf to root order
    result = true; // earlier level first
  }else if (palevel == pblevel) { // alwayse true when starting root to leaf
    if (palevel == 0) { // a and b are roots
      if (a->nodevec_index < b->nodevec_index) {
        result = true;
      }
    }else{ // parent order (already sorted with proper treenode_order
      if (a->treenode_order < b->treenode_order) { // children order
        result = true;
      }else if (a->treenode_order == b->treenode_order) {
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

#if 0
printf("after sortlevel\n");
for (size_t i = 0; i < level.size(); ++i) {
TNode* nd = level[i];
printf("ilev=%ld i=%ld plev=%ld pi=%ld phash=%ld ord=%ld hash=%ld\n",
nd->level, i, nd->parent?nd->parent->level:0,
nd->parent?nd->parent->treenode_order:0, nd->parent?nd->parent->hash:0,
nd->treenode_order, nd->hash);
}
chklevel(level);
#endif

  for (size_t i=0; i < level.size(); ++i) {
    level[i]->treenode_order = i;
  }
}

static void set_treenode_order(VVTN& levels) {
  size_t order = 0;
  for (size_t i = 0; i < levels.size(); ++i) {
    for (size_t j = 0; j < levels[i].size(); ++j) {
      TNode* nd = levels[i][j];
      nd->treenode_order = order++;
    }
  }
}

// every level starts out with no race conditions involving both
// parent and child in the same level. Can we arrange things so that
// every level has at least 32 nodes?
static void question2(VVTN& levels, size_t max = 32) {
  size_t nnode = 0;
  for (size_t i = 0; i < levels.size(); ++i) {
    nnode += levels[i].size();
  }
  VTN nodes(nnode);
  nnode = 0;
  for (size_t i = 0; i < levels.size(); ++i) {
    for (size_t j=0;j < levels[i].size(); ++j) {
      nodes[nnode++] = levels[i][j];      
    }
  }
  for (size_t i=0; i < nodes.size(); ++i) {
    nodes[i]->nodevec_index = i;
  }

  // work backward and check the distance from parent to children.
  // if parent in different group then there is no vitiating race.
  // if children in different group then ther is no race (satisfied by
  // atomic).
  // If there is a vitiating race, then figure out how many nodes
  // need to be inserted just before the parent to avoid the race.
  //   It is not clear if we should prioritize safe nodes (when moved they
  //   do not introduce a race) and/or contiguous nodes (probably, to keep
  //   the low hanging fruit together).
  //   At least, moved nodes should have proper tree order and not themselves
  //   introduce a race at their new location.  Leaves are nice in that there
  //   are no restrictions in movement toward higher indices.

  nrn_assert(nodes.size()%max == 0); // for now
  for (size_t i = nodes.size() - 1; i >= levels[0].size(); --i) {
    TNode* nd = nodes[i];
    size_t i32 = nd->nodevec_index/max;
    size_t p32 = nd->parent->nodevec_index/max;
    if (p32 == i32) { // parent in same 32group
      size_t diff = nd->nodevec_index - nd->parent->nodevec_index;
      printf("level=%ld i=%ld ip=%ld diff=%ld\n", nd->level, nd->nodevec_index, nd->parent->nodevec_index, diff);
//    }else if (i%8 == 0 && p32 < i32-1) {
//      printf("movable between %ld and %ld\n", p32, i32);
    }
  }

  // move along in groups of 32 and see if there are any races.
  
}

// size of groups with contiguous parents for each level
static void question(VVTN& levels) {
  for (size_t i = 0; i < levels.size(); ++i) {
    printf("%3ld %5ld", i, levels[i].size());
    size_t iplast = 100000000;
    size_t nsame = 0;
    for (size_t j=0; j < levels[i].size(); ++j) {
      TNode* nd = levels[i][j];
      if (nd->parent == NULL) {
        nsame += 1;
      }else if (nd->parent->treenode_order == iplast + 1) {
        nsame += 1;
        iplast = nd->parent->treenode_order;
      }else{
        if (nsame) { printf(" %3ld", nsame); }
        nsame = 1;
        iplast = nd->parent->treenode_order;
      }
    }
    if (nsame) { printf(" %3ld", nsame); }
    printf("\n");
  }
}

static void analyze(VVTN& levels) {
  // sort each level with respect to parent level order
  // earliest parent level first.

  //treenode order can be anything as long as first children < second
  // children etc.. After sorting a level, the order will be correct for
  // that level, ranging from [0:level.size]
  for (size_t i = 0; i < levels.size(); ++i) {
    chklevel(levels[i]);
    for (size_t j = 0; j < levels[i].size(); ++j) {
      TNode* nd = levels[i][j];
      for (size_t k = 0; k < nd->children.size(); ++k) {
        nd->children[k]->treenode_order = k;
      }
    }
  }

  for (size_t i = 0 ; i < levels.size(); ++i) {
    sortlevel(levels[i]);
    chklevel(levels[i]);
  }

  set_treenode_order(levels);
}

void prgroupsize(VVVTN& groups) {
  for (size_t i=0; i < groups[0].size(); ++i) {
    printf("%5ld\n", i);
    for (size_t j=0; j < groups.size(); ++j) {
      printf(" %5ld", groups[j][i].size());   
    }
    printf("\n");
  }
}

// group index primary, treenode_order secondary
static bool final_nodevec_cmp(TNode* a, TNode* b) {
  bool result = false;
  if (a->groupindex < b->groupindex) {
    result = true;
  }else if (a->groupindex == b->groupindex) {
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
  //return;
  printf("enter group_order2\n");
#if 1
  size_t maxlevel = level_from_root(nodevec);
#else
  size_t maxlevel = level_from_leaf(nodevec);
  // reverse the level numbering so leaves are at maxlevel.
  // also make all roots have level 0
  for (size_t i = 0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    nd->level = maxlevel - nd->level;
    if (nd->parent == NULL) {
      nd->level = 0;
    }
  }
#endif
  
  // work on a cellgroup as a vector of levels. ie only possible race is
  // two children in same warpsize
  
  VVVTN groups(ncell/groupsize);
  for (size_t i = 0; i < groups.size(); ++i) {
    groups[i].resize(maxlevel+1);
  }
  for (size_t i=0; i < nodevec.size(); ++i) {
    TNode* nd = nodevec[i];
    groups[nd->groupindex][nd->level].push_back(nd);
  }

  prgroupsize(groups);

  // deal with each group
  for (size_t i=0; i < groups.size(); ++i) {
    analyze(groups[i]);
  }

  question(groups[0]);
  question2(groups[0]);

  //final nodevec order according to group_index and treenode_order
  std::sort(nodevec.begin() + ncell, nodevec.end(), final_nodevec_cmp);
  set_nodeindex(nodevec);
}

